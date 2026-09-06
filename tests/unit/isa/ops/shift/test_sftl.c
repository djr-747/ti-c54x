/* Module contract
 * Owns: pure SFTL accumulator-handler and generated-dispatch proofs.
 * Does not own: catalog rows, operand layout extraction, accumulator
 * mutation, status mutation, sequencing, CPU lifecycle, timing, or repeat/
 * interrupt policy.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/shift/op_shift.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t sftl_word(uint8_t source, uint8_t destination,
                          uint8_t shift_raw)
{
    return (uint16_t)(UINT16_C(0xF0E0) |
                      ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) |
                      (uint16_t)(shift_raw & UINT8_C(0x1F)));
}

static int signed_shift(uint8_t shift_raw)
{
    return (shift_raw & UINT8_C(0x10)) != 0U
               ? (int)shift_raw - 32
               : (int)shift_raw;
}

static ti_c54x_decoded_instruction decode(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_SFTL_ACC);
    TI_C54X_TEST_CHECK(
        instruction.operand_layout ==
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static uint64_t expected_value(uint64_t source, int shift)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    if (shift < 0) {
        return low32 >> (unsigned)(-shift);
    }
    if (shift == 0) {
        return low32;
    }
    return (low32 << (unsigned)shift) & UINT64_C(0xFFFFFFFF);
}

static uint8_t expected_c(uint64_t source, int shift)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    if (shift < 0) {
        return (uint8_t)((low32 >> (unsigned)(-shift - 1)) &
                         UINT64_C(1));
    }
    if (shift == 0) {
        return UINT8_C(0);
    }
    return (uint8_t)((low32 >> (unsigned)(32 - shift)) & UINT64_C(1));
}

static void assert_effect(const ti_c54x_op_effect *effect, uint64_t value,
                          uint8_t destination, uint8_t c_value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       (c_value == 0U ? TI_C54X_OP_STATUS_BIT_WRITE_0
                                      : TI_C54X_OP_STATUS_BIT_WRITE_1));
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.value_raw40 <=
                       TI_C54X_U40_MASK);
}

static void check_case(uint16_t word, uint64_t a, uint64_t b, bool a_known,
                       bool b_known, uint8_t pre_c, bool pre_c_known,
                       ti_c54x_op_status expected_status)
{
    const uint8_t source = (uint8_t)((word >> 9U) & UINT16_C(1));
    const uint8_t destination = (uint8_t)((word >> 8U) & UINT16_C(1));
    const int shift = signed_shift((uint8_t)(word & UINT16_C(0x1F)));
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_c = pre_c,
        .status_c_known = pre_c_known};
    const ti_c54x_op_input_snapshot input_before = input;
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_NONE,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        ti_c54x_op_sftl_acc(&instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(
            &instruction, &input, &dispatched);

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
        const uint64_t selected = source == 0U ? a : b;
        assert_effect(&direct, expected_value(selected, shift), destination,
                      expected_c(selected, shift));
        TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    }
}

static void check_all_forms_and_vectors(void)
{
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x000000007FFFFFFF), UINT64_C(0x0000000080000000),
        UINT64_C(0x00000000FFFFFFFF), UINT64_C(0xFF00000000),
        UINT64_C(0xFF80000000), UINT64_C(0x123456789A),
        UINT64_C(0x00ABCDEF01)};
    uint8_t source;
    uint8_t destination;
    uint8_t shift_raw;
    size_t vector_index;

    for (source = UINT8_C(0); source <= UINT8_C(1); ++source) {
        for (destination = UINT8_C(0); destination <= UINT8_C(1);
             ++destination) {
            for (shift_raw = UINT8_C(0); shift_raw < UINT8_C(32);
                 ++shift_raw) {
                const uint16_t word =
                    sftl_word(source, destination, shift_raw);
                for (vector_index = 0U;
                     vector_index < sizeof(vectors) / sizeof(vectors[0]);
                     ++vector_index) {
                    const uint64_t selected = vectors[vector_index];
                    const uint64_t other =
                        vectors[(vector_index + 3U) %
                                (sizeof(vectors) / sizeof(vectors[0]))];
                    check_case(word, source == 0U ? selected : other,
                               source == 1U ? selected : other, true, true,
                               UINT8_C(0xA5), false,
                               TI_C54X_OP_STATUS_OK);
                }
            }
        }
    }
}

static void check_unknown_and_invalid_inputs(void)
{
    /* Unknown unselected state is allowed; selected state is not. */
    check_case(sftl_word(0U, 1U, UINT8_C(0x1F)), UINT64_C(3),
               UINT64_C(0x12345), true, false, UINT8_C(1), false,
               TI_C54X_OP_STATUS_OK);
    check_case(sftl_word(1U, 0U, UINT8_C(0x00)), UINT64_C(0x12345),
               UINT64_C(3), false, true, UINT8_C(0), false,
               TI_C54X_OP_STATUS_OK);
    check_case(sftl_word(0U, 1U, UINT8_C(0x1F)), UINT64_C(3),
               UINT64_C(0x12345), false, true, UINT8_C(1), false,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(sftl_word(1U, 0U, UINT8_C(0x00)), UINT64_C(0x12345),
               UINT64_C(3), true, false, UINT8_C(0), false,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    /* The pre-C value and knownness are not SFTL inputs. */
    check_case(sftl_word(0U, 1U, UINT8_C(0x01)), UINT64_C(0x80000001),
               UINT64_C(0x12), true, true, UINT8_C(0), false,
               TI_C54X_OP_STATUS_OK);
    check_case(sftl_word(0U, 1U, UINT8_C(0x01)), UINT64_C(0x80000001),
               UINT64_C(0x12), true, true, UINT8_C(1), true,
               TI_C54X_OP_STATUS_OK);
    check_case(sftl_word(0U, 1U, UINT8_C(0x01)), UINT64_C(0x80000001),
               UINT64_C(0x12), true, true, UINT8_C(0xFF), false,
               TI_C54X_OP_STATUS_OK);

    check_case(sftl_word(0U, 0U, UINT8_C(0x00)), TI_C54X_U40_MASK + 1U,
               UINT64_C(3), true, true, UINT8_C(0), false,
               TI_C54X_OP_STATUS_INVALID_OPERAND);
    check_case(sftl_word(1U, 1U, UINT8_C(0x00)), UINT64_C(3),
               TI_C54X_U40_MASK + 1U, true, true, UINT8_C(0), false,
               TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void check_forged_decode_rollback(void)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(1),
        .accumulator_b_raw40 = UINT64_C(2),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(0xFF),
        .status_c_known = false};
    const ti_c54x_decoded_instruction instruction =
        decode(sftl_word(0U, 0U, UINT8_C(0)));
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0040);
        TI_C54X_TEST_CHECK(ti_c54x_op_sftl_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE;
        TI_C54X_TEST_CHECK(ti_c54x_op_sftl_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_C;
        TI_C54X_TEST_CHECK(ti_c54x_op_sftl_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_word_count = UINT8_C(2);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_all_forms_and_vectors();
    check_unknown_and_invalid_inputs();
    check_forged_decode_rollback();
    return 0;
}
