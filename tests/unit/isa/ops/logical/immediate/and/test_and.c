/* Module contract
 * Owns: proofs for the two-word immediate AND family, including every
 * shifted/fixed encoding, direct and generated dispatch, selected-source
 * knownness, zero-extended immediates, raw-40 guards, and transactional
 * rejection of malformed decoded carriers.
 * Does not own: catalog rows, generated lookup, operand-layout extraction,
 * CPU fetch/retirement, accumulator/status mutation, timing, repeat/IRQ
 * policy, diagnostics, or any parent/shared test manifest.
 */

#include "../../../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../../../src/isa/ops/logical/immediate/and/op_and.h"
#include "../../../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t shifted_word(uint8_t source, uint8_t destination,
                             uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF030) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) | shift);
}

static uint16_t fixed_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF063) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U));
}

static ti_c54x_opcode_id expected_id(uint8_t fixed)
{
    return fixed != 0U ? TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16
                       : TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT;
}

static ti_c54x_operand_layout_id expected_layout(uint8_t fixed)
{
    return fixed != 0U
               ? TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16
               : TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16;
}

static ti_c54x_decoded_instruction decode(uint16_t first_word, uint16_t lk,
                                          uint8_t fixed)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, lk}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == expected_id(fixed));
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == expected_id(fixed));
    TI_C54X_TEST_CHECK(instruction.operand_layout == expected_layout(fixed));
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static uint64_t expected_value(uint64_t source, uint64_t immediate,
                               uint8_t shift)
{
    return source & ((immediate << shift) & TI_C54X_U40_MASK);
}

static void assert_effect(const ti_c54x_op_effect *effect, uint64_t value,
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

static void check_success(uint16_t first_word, uint16_t lk, uint8_t source,
                          uint8_t destination, uint8_t shift, uint8_t fixed,
                          uint64_t a, uint64_t b)
{
    const ti_c54x_decoded_instruction instruction =
        decode(first_word, lk, fixed);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(0xA5),
        .status_c_known = false,
        .status_tc = UINT8_C(0x5A),
        .status_tc_known = false};
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_input_snapshot input_before = input;
    const uint64_t selected = source == 0U ? a : b;
    const uint8_t actual_shift = fixed != 0U ? UINT8_C(16) : shift;
    const ti_c54x_op_status direct_status = ti_c54x_op_and_immediate(
        &instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(&instruction, &input,
                                                     &dispatched);

    TI_C54X_TEST_CHECK(direct_status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(dispatch_status == TI_C54X_OP_STATUS_OK);
    assert_effect(&direct, expected_value(selected, lk, actual_shift),
                  destination);
    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void check_all_encodings_and_boundaries(void)
{
    static const uint16_t immediates[] = {UINT16_C(0x0000), UINT16_C(0x0001),
                                          UINT16_C(0x7FFF), UINT16_C(0xFFFF)};
    static const uint64_t vectors[] = {UINT64_C(0x0000000000),
                                       UINT64_C(0xFFFFFFFFFF),
                                       UINT64_C(0x8000000001),
                                       UINT64_C(0x123456789A)};
    uint8_t source;
    uint8_t destination;
    uint8_t shift;
    size_t immediate_index;
    size_t vector_index;

    for (source = 0U; source <= 1U; ++source) {
        for (destination = 0U; destination <= 1U; ++destination) {
            for (shift = 0U; shift < UINT8_C(16); ++shift) {
                for (immediate_index = 0U;
                     immediate_index < sizeof(immediates) /
                                           sizeof(immediates[0]);
                     ++immediate_index) {
                    for (vector_index = 0U;
                         vector_index < sizeof(vectors) / sizeof(vectors[0]);
                         ++vector_index) {
                        check_success(
                            shifted_word(source, destination, shift),
                            immediates[immediate_index], source, destination,
                            shift, 0U, vectors[vector_index],
                            vectors[(vector_index + 1U) %
                                    (sizeof(vectors) / sizeof(vectors[0]))]);
                    }
                }
            }
            for (immediate_index = 0U;
                 immediate_index < sizeof(immediates) /
                                       sizeof(immediates[0]);
                 ++immediate_index) {
                check_success(fixed_word(source, destination),
                              immediates[immediate_index], source, destination,
                              UINT8_C(16), 1U, UINT64_C(0xFFFFFFFFFF),
                              UINT64_C(0x123456789A));
            }
        }
    }
}

static void check_destination_is_write_only(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(shifted_word(0U, 1U, UINT8_C(15)), UINT16_C(0xFFFF), 0U);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x123456789A),
        .accumulator_b_raw40 = UINT64_C(0xDEADBEEF01),
        .accumulator_a_known = true,
        .accumulator_b_known = false,
        .status_c = UINT8_C(1),
        .status_c_known = true,
        .status_tc = UINT8_C(0),
        .status_tc_known = false};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, expected_value(input.accumulator_a_raw40,
                                          UINT16_C(0xFFFF), UINT8_C(15)),
                  UINT8_C(1));
}

static void check_unknown_and_noncanonical_inputs(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(shifted_word(0U, 1U, UINT8_C(1)), UINT16_C(7), 0U);
    const ti_c54x_op_input_snapshot good = {
        UINT64_C(3), UINT64_C(5), true, true, UINT8_C(1), false, UINT8_C(0),
        false};
    const ti_c54x_op_input_snapshot unknown_source = {
        UINT64_C(3), UINT64_C(5), false, true, UINT8_C(1), true, UINT8_C(0),
        true};
    const ti_c54x_op_input_snapshot noncanonical_source = {
        TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(5), true, true, UINT8_C(1),
        true, UINT8_C(0), true};
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&instruction, &unknown_source,
                                                &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(
                           &instruction, &noncanonical_source, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(NULL, &good, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&instruction, &good, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void check_malformed_carriers_are_transactional(void)
{
    const ti_c54x_decoded_instruction valid =
        decode(shifted_word(0U, 1U, UINT8_C(1)), UINT16_C(7), 0U);
    const ti_c54x_op_input_snapshot input = {
        UINT64_C(3), UINT64_C(5), true, true, UINT8_C(0), true, UINT8_C(1),
        true};
    ti_c54x_decoded_instruction malformed = valid;
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    const ti_c54x_op_effect before = effect;

    malformed.bundle.program_words[0] ^= UINT16_C(0x0020);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&malformed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    malformed = valid;
    malformed.bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&malformed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    malformed = valid;
    malformed.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&malformed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    malformed = valid;
    malformed.id = TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate(&malformed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_catalog_bound_aliases(void)
{
    const ti_c54x_decoded_instruction shifted =
        decode(shifted_word(0U, 1U, UINT8_C(3)), UINT16_C(0x1234), 0U);
    const ti_c54x_decoded_instruction fixed =
        decode(fixed_word(1U, 0U), UINT16_C(0x1234), 1U);
    const ti_c54x_op_input_snapshot input = {
        UINT64_C(0x0FEDCBA987), UINT64_C(0x0012345678), true, true,
        UINT8_C(0), true, UINT8_C(0), true};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect effect;

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift(
                           &shifted, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect,
                  expected_value(input.accumulator_a_raw40, UINT16_C(0x1234),
                                 UINT8_C(3)),
                  UINT8_C(1));

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift16(
                           &shifted, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift(
                           &fixed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift16(
                           &fixed, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect,
                  expected_value(input.accumulator_b_raw40, UINT16_C(0x1234),
                                 UINT8_C(16)),
                  UINT8_C(0));

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift(&shifted, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift(&shifted, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift16(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift16(&fixed, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_and_immediate_shift16(&fixed, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
    check_all_encodings_and_boundaries();
    check_destination_is_write_only();
    check_unknown_and_noncanonical_inputs();
    check_malformed_carriers_are_transactional();
    check_catalog_bound_aliases();
    return 0;
}
