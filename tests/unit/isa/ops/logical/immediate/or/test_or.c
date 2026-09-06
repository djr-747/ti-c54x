/* Module contract
 * Owns: proofs for the pure OR-immediate family, including every first
 * word encoding, unsigned lk16 construction, selected-source semantics,
 * direct/dispatch agreement, status preservation, and transactional reject.
 * Does not own: catalog rows, generated lookup, operand-layout extraction,
 * CPU fetch/retirement, accumulator/status mutation, timing, repeat/IRQ,
 * memory, diagnostics, or parent test registration.
 */

#include "../../../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../../../src/isa/ops/logical/immediate/or/op_or.h"
#include "../../../../../../../src/isa/ops/op_effect.h"
#include "../../../../../../../src/isa/ops/op_input.h"
#include "../../../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t encode_shift(uint8_t source, uint8_t destination,
                             uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF040) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) | shift);
}

static uint16_t encode_shift16(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF064) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U));
}

static ti_c54x_decoded_instruction decode(uint16_t first_word,
                                          uint16_t extension_word,
                                          ti_c54x_operand_layout_id layout,
                                          ti_c54x_opcode_id expected_id)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension_word},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == expected_id);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == expected_id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == layout);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    return instruction;
}

static uint64_t expected_value(uint8_t source, uint8_t shift,
                               uint16_t immediate,
                               uint64_t accumulator_a,
                               uint64_t accumulator_b)
{
    const uint64_t selected = source == 0U ? accumulator_a : accumulator_b;
    return (selected | ((uint64_t)immediate << shift)) & TI_C54X_U40_MASK;
}

static void assert_effect(const ti_c54x_op_effect *effect, uint64_t expected,
                          uint8_t destination)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           expected);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.value_raw40 <=
                       TI_C54X_U40_MASK);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

static void check_success(uint16_t first_word, uint16_t immediate,
                          uint8_t source, uint8_t destination, uint8_t shift,
                          ti_c54x_operand_layout_id layout,
                          ti_c54x_opcode_id opcode_id)
{
    const ti_c54x_decoded_instruction instruction =
        decode(first_word, immediate, layout, opcode_id);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x0123456789),
        .accumulator_b_raw40 = UINT64_C(0x00ABCDEF01),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(0xA5),
        .status_c_known = false,
        .status_tc = UINT8_C(0x5A),
        .status_tc_known = true};
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_input_snapshot input_before = input;
    const uint64_t expected = expected_value(
        source, shift, immediate, input.accumulator_a_raw40,
        input.accumulator_b_raw40);

    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate(&instruction, &input, &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&direct, expected, destination);
    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void test_every_shifted_encoding_and_immediate_boundary(void)
{
    const uint16_t immediates[] = {UINT16_C(0), UINT16_C(1),
                                   UINT16_C(0x7FFF), UINT16_C(0xFFFF)};
    const uint8_t shifts[] = {UINT8_C(0), UINT8_C(1), UINT8_C(15)};

    for (uint8_t source = 0U; source < 2U; ++source) {
        for (uint8_t destination = 0U; destination < 2U; ++destination) {
            for (uint8_t shift = 0U; shift < 16U; ++shift) {
                const uint16_t word = encode_shift(source, destination, shift);
                for (size_t index = 0U;
                     index < sizeof(immediates) / sizeof(immediates[0]);
                     ++index) {
                    check_success(
                        word, immediates[index], source, destination, shift,
                        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
                        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
                }
            }
        }
    }

    for (uint8_t source = 0U; source < 2U; ++source) {
        for (uint8_t destination = 0U; destination < 2U; ++destination) {
            for (size_t index = 0U;
                 index < sizeof(immediates) / sizeof(immediates[0]); ++index) {
                check_success(
                    encode_shift16(source, destination), immediates[index],
                    source, destination, UINT8_C(16),
                    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16,
                    TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT16);
            }
        }
    }

    /* Keep the named shift boundaries explicit in the test contract. */
    for (size_t index = 0U; index < sizeof(shifts) / sizeof(shifts[0]);
         ++index) {
        check_success(
            encode_shift(UINT8_C(1), UINT8_C(0), shifts[index]),
            UINT16_C(0xFFFF), UINT8_C(1), UINT8_C(0), shifts[index],
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
            TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
    }
}

static void test_unknown_selected_source_and_raw40_are_rejected(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(1)), UINT16_C(7),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
    const ti_c54x_op_input_snapshot unknown = {
        .accumulator_a_raw40 = UINT64_C(1),
        .accumulator_b_raw40 = UINT64_C(2),
        .accumulator_a_known = false,
        .accumulator_b_known = true};
    const ti_c54x_op_input_snapshot invalid = {
        .accumulator_a_raw40 = TI_C54X_U40_MASK + UINT64_C(1),
        .accumulator_b_raw40 = UINT64_C(2),
        .accumulator_a_known = true,
        .accumulator_b_known = true};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate(&instruction, &unknown,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate(&instruction, &invalid,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void test_unknown_distinct_destination_is_overwritten(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(0)), UINT16_C(0x8000),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x1234),
        .accumulator_b_raw40 = UINT64_C(0xFFFF),
        .accumulator_a_known = true,
        .accumulator_b_known = false};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, UINT64_C(0x9234), UINT8_C(1));
}

static void test_forged_decode_is_transactional(void)
{
    ti_c54x_decoded_instruction forged = decode(
        encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(0)), UINT16_C(1),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(1),
        .accumulator_b_raw40 = UINT64_C(2),
        .accumulator_a_known = true,
        .accumulator_b_known = true};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect;

    forged.bundle.program_words[0] ^= UINT16_C(0x0010);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = decode(
        encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(0)), UINT16_C(1),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void test_catalog_bound_aliases(void)
{
    const ti_c54x_decoded_instruction shifted = decode(
        encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(3)), UINT16_C(0x1234),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT);
    const ti_c54x_decoded_instruction fixed = decode(
        encode_shift16(UINT8_C(1), UINT8_C(0)), UINT16_C(0x1234),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16,
        TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT16);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x0FEDCBA987),
        .accumulator_b_raw40 = UINT64_C(0x0012345678),
        .accumulator_a_known = true,
        .accumulator_b_known = true};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect effect;

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift(
                           &shifted, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect,
                  expected_value(UINT8_C(0), UINT8_C(3), UINT16_C(0x1234),
                                 input.accumulator_a_raw40,
                                 input.accumulator_b_raw40),
                  UINT8_C(1));

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift16(
                           &shifted, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift(
                           &fixed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift16(
                           &fixed, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect,
                  expected_value(UINT8_C(1), UINT8_C(16), UINT16_C(0x1234),
                                 input.accumulator_a_raw40,
                                 input.accumulator_b_raw40),
                  UINT8_C(0));

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift(&shifted, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift(&shifted, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift16(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift16(&fixed, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_immediate_shift16(&fixed, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
    test_every_shifted_encoding_and_immediate_boundary();
    test_unknown_selected_source_and_raw40_are_rejected();
    test_unknown_distinct_destination_is_overwritten();
    test_forged_decode_is_transactional();
    test_catalog_bound_aliases();
    return 0;
}
