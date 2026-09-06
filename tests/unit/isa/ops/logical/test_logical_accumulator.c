/* Module contract
 * Owns: RED proofs for the pure AND/OR/XOR accumulator handlers and their
 * generated-dispatch seam over decoded one-word instructions.
 * Does not own: catalog rows, generated tables, operand-layout extraction,
 * accumulator mutation, status mutation, sequencing, CPU lifecycle, timing,
 * repeat/interrupt policy, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/logical/op_logical.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum logical_kind {
    LOGICAL_AND = 0,
    LOGICAL_OR = 1,
    LOGICAL_XOR = 2,
    LOGICAL_KIND_COUNT = 3
} logical_kind;

static uint16_t logical_word(logical_kind kind,
                             uint8_t source,
                             uint8_t destination,
                             uint8_t shift_raw)
{
    static const uint16_t bases[LOGICAL_KIND_COUNT] = {
        UINT16_C(0xF080), UINT16_C(0xF0A0), UINT16_C(0xF0C0)};

    return (uint16_t)(bases[kind] | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) |
                      (uint16_t)(shift_raw & UINT8_C(0x1F)));
}

static ti_c54x_opcode_id logical_id(logical_kind kind)
{
    static const ti_c54x_opcode_id ids[LOGICAL_KIND_COUNT] = {
        TI_C54X_OPCODE_ID_AND_ACC_SHIFT,
        TI_C54X_OPCODE_ID_OR_ACC_SHIFT,
        TI_C54X_OPCODE_ID_XOR_ACC_SHIFT};

    return ids[kind];
}

static ti_c54x_decoded_instruction decode(logical_kind kind,
                                          uint8_t source,
                                          uint8_t destination,
                                          uint8_t shift_raw)
{
    const uint16_t word = logical_word(kind, source, destination, shift_raw);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == logical_id(kind));
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == logical_id(kind));
    TI_C54X_TEST_CHECK(
        instruction.operand_layout ==
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static int signed_shift(uint8_t shift_raw)
{
    return (shift_raw & UINT8_C(0x10)) != UINT8_C(0)
               ? (int)shift_raw - 32
               : (int)shift_raw;
}

static uint64_t shifted_full40(uint64_t source, int shift)
{
    if (shift < 0) {
        return source >> (unsigned)(-shift);
    }
    if (shift == 0) {
        return source;
    }
    return (source << (unsigned)shift) & TI_C54X_U40_MASK;
}

static uint64_t combined(logical_kind kind, uint64_t destination,
                         uint64_t shifted_source)
{
    switch (kind) {
    case LOGICAL_AND:
        return destination & shifted_source;
    case LOGICAL_OR:
        return destination | shifted_source;
    case LOGICAL_XOR:
        return destination ^ shifted_source;
    default:
        return UINT64_C(0);
    }
}

static ti_c54x_op_status direct_handler(
    logical_kind kind,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    switch (kind) {
    case LOGICAL_AND:
        return ti_c54x_op_and_acc_shift(instruction, input, effect);
    case LOGICAL_OR:
        return ti_c54x_op_or_acc_shift(instruction, input, effect);
    case LOGICAL_XOR:
        return ti_c54x_op_xor_acc_shift(instruction, input, effect);
    default:
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
}

static void assert_effect(const ti_c54x_op_effect *effect,
                          uint64_t value,
                          uint8_t destination)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.value_raw40 <=
                       TI_C54X_U40_MASK);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

static void check_case(logical_kind kind,
                       uint8_t source,
                       uint8_t destination,
                       uint8_t shift_raw,
                       uint64_t a,
                       uint64_t b,
                       bool a_known,
                       bool b_known,
                       uint8_t status_c,
                       bool status_c_known,
                       ti_c54x_op_status expected_status)
{
    const ti_c54x_decoded_instruction instruction =
        decode(kind, source, destination, shift_raw);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_c = status_c,
        .status_c_known = status_c_known};
    const ti_c54x_op_input_snapshot input_before = input;
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        direct_handler(kind, &instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(&instruction, &input,
                                                     &dispatched);

    TI_C54X_TEST_CHECK(direct_status == expected_status);
    TI_C54X_TEST_CHECK(dispatch_status == expected_status);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
    if (expected_status != TI_C54X_OP_STATUS_OK) {
        TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) ==
                           0);
        return;
    }

    {
        const uint64_t selected = source == UINT8_C(0) ? a : b;
        const uint64_t destination_value =
            destination == UINT8_C(0) ? a : b;
        const uint64_t shifted =
            shifted_full40(selected, signed_shift(shift_raw));
        assert_effect(&direct,
                      combined(kind, destination_value, shifted), destination);
        TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    }
}

static void check_all_384_operand_encodings(void)
{
    uint8_t source;
    uint8_t destination;
    uint8_t shift_raw;
    logical_kind kind;

    /* Every family opcode encoding must decode and execute exactly once. */
    for (kind = LOGICAL_AND; kind < LOGICAL_KIND_COUNT; ++kind) {
        for (source = UINT8_C(0); source <= UINT8_C(1); ++source) {
            for (destination = UINT8_C(0); destination <= UINT8_C(1);
                 ++destination) {
                for (shift_raw = UINT8_C(0); shift_raw < UINT8_C(32);
                     ++shift_raw) {
                    check_case(kind, source, destination, shift_raw,
                               UINT64_C(0x123456789A),
                               UINT64_C(0x00ABCDEF01), true, true,
                               UINT8_C(0xA5), false,
                               TI_C54X_OP_STATUS_OK);
                }
            }
        }
    }
}

static void check_shift_boundaries_and_full40_vectors(void)
{
    static const uint8_t shifts[] = {
        UINT8_C(0x10), UINT8_C(0x1F), UINT8_C(0x00), UINT8_C(0x01),
        UINT8_C(0x0F)};
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x000000007F), UINT64_C(0x0000000080),
        UINT64_C(0x00000000FFFFFFFF), UINT64_C(0x8000000000),
        UINT64_C(0xFF00000000), UINT64_C(0xFF80000000),
        UINT64_C(0x123456789A), UINT64_C(0x00ABCDEF01),
        UINT64_C(0xFFFFFFFFFF)};
    size_t vector_index;
    size_t shift_index;
    logical_kind kind;
    uint8_t source;
    uint8_t destination;

    for (kind = LOGICAL_AND; kind < LOGICAL_KIND_COUNT; ++kind) {
        for (source = UINT8_C(0); source <= UINT8_C(1); ++source) {
            for (destination = UINT8_C(0); destination <= UINT8_C(1);
                 ++destination) {
                for (shift_index = 0U;
                     shift_index < sizeof(shifts) / sizeof(shifts[0]);
                     ++shift_index) {
                    for (vector_index = 0U;
                         vector_index < sizeof(vectors) / sizeof(vectors[0]);
                         ++vector_index) {
                        const uint64_t selected = vectors[vector_index];
                        const uint64_t other =
                            vectors[(vector_index + 3U) %
                                    (sizeof(vectors) / sizeof(vectors[0]))];
                        check_case(kind, source, destination,
                                   shifts[shift_index],
                                   source == UINT8_C(0) ? selected : other,
                                   source == UINT8_C(1) ? selected : other,
                                   true, true, UINT8_C(0xFF), true,
                                   TI_C54X_OP_STATUS_OK);
                    }
                }
            }
        }
    }
}

static void check_knownness_and_canonicality(void)
{
    logical_kind kind;

    for (kind = LOGICAL_AND; kind < LOGICAL_KIND_COUNT; ++kind) {
        /* S != D: both participating values are required and canonical. */
        check_case(kind, 0U, 1U, UINT8_C(0x01), UINT64_C(3), UINT64_C(5),
                   false, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        check_case(kind, 0U, 1U, UINT8_C(0x01), UINT64_C(3), UINT64_C(5),
                   true, false, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        check_case(kind, 1U, 0U, UINT8_C(0x01), UINT64_C(3), UINT64_C(5),
                   false, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        check_case(kind, 1U, 0U, UINT8_C(0x01), UINT64_C(3), UINT64_C(5),
                   true, false, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        check_case(kind, 0U, 1U, UINT8_C(0x01), TI_C54X_U40_MASK + 1U,
                   UINT64_C(5), true, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_INVALID_OPERAND);
        check_case(kind, 1U, 0U, UINT8_C(0x01), UINT64_C(3),
                   TI_C54X_U40_MASK + 1U, true, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_INVALID_OPERAND);
        check_case(kind, 0U, 1U, UINT8_C(0x01), UINT64_C(3),
                   TI_C54X_U40_MASK + 1U, true, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_INVALID_OPERAND);
        check_case(kind, 1U, 0U, UINT8_C(0x01), TI_C54X_U40_MASK + 1U,
                   UINT64_C(5), true, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_INVALID_OPERAND);

        /* S == D: the selected value is required; unselected unknownness is
         * permitted because it cannot affect this pure handler. */
        check_case(kind, 0U, 0U, UINT8_C(0x1F), UINT64_C(3),
                   UINT64_C(7), true, false, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_OK);
        check_case(kind, 1U, 1U, UINT8_C(0x01), UINT64_C(3),
                   UINT64_C(7), false, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_OK);
        check_case(kind, 0U, 0U, UINT8_C(0x1F), UINT64_C(3),
                   UINT64_C(7), false, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        check_case(kind, 1U, 1U, UINT8_C(0x01), UINT64_C(3),
                   UINT64_C(7), true, false, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        check_case(kind, 0U, 0U, UINT8_C(0x00), TI_C54X_U40_MASK + 1U,
                   UINT64_C(7), true, false, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_INVALID_OPERAND);
        check_case(kind, 1U, 1U, UINT8_C(0x00), UINT64_C(7),
                   TI_C54X_U40_MASK + 1U, false, true, UINT8_C(1), false,
                   TI_C54X_OP_STATUS_INVALID_OPERAND);

        /* Status C is not an input resource: unknown and arbitrary values are
         * accepted, and both status bits are preserved. */
        check_case(kind, 0U, 1U, UINT8_C(0x00), UINT64_C(0x12),
                   UINT64_C(0x34), true, true, UINT8_C(0x00), false,
                   TI_C54X_OP_STATUS_OK);
        check_case(kind, 0U, 1U, UINT8_C(0x00), UINT64_C(0x12),
                   UINT64_C(0x34), true, true, UINT8_C(0xFF), true,
                   TI_C54X_OP_STATUS_OK);
    }
}

static void assert_rejected_transactionally(logical_kind kind,
                                             ti_c54x_decoded_instruction forged)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(1),
        .accumulator_b_raw40 = UINT64_C(2),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(0xFF),
        .status_c_known = false};
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;

    TI_C54X_TEST_CHECK(direct_handler(kind, &forged, &input, &direct) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &forged, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) == 0);
}

static void check_forged_decode_layout_resource_and_bundle(void)
{
    logical_kind kind;

    for (kind = LOGICAL_AND; kind < LOGICAL_KIND_COUNT; ++kind) {
        const ti_c54x_decoded_instruction instruction =
            decode(kind, UINT8_C(0), UINT8_C(1), UINT8_C(0));
        ti_c54x_decoded_instruction forged = instruction;

        /* Fixed encoding bits must be checked by the family handler. */
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        assert_rejected_transactionally(kind, forged);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE;
        assert_rejected_transactionally(kind, forged);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_C;
        assert_rejected_transactionally(kind, forged);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(2);
        assert_rejected_transactionally(kind, forged);
    }
}

int main(void)
{
    check_all_384_operand_encodings();
    check_shift_boundaries_and_full40_vectors();
    check_knownness_and_canonicality();
    check_forged_decode_layout_resource_and_bundle();
    return 0;
}
