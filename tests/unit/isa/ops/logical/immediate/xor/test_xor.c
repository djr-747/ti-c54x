/* Module contract
 * Owns: proofs for the pure XOR two-word immediate family, including all
 * first-word encodings, unsigned lk16 construction, source/destination
 * selection, raw40 and knownness guards, transactional failure, and direct
 * versus generated dispatch equivalence.
 * Does not own: catalog rows, generated tables, operand-layout descriptors,
 * CPU fetch/retirement, memory, timing, repeat/interrupt policy, status
 * mutation, diagnostics, or shared parent manifests.
 */

#include "../../../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../../../src/isa/ops/logical/immediate/xor/op_xor.h"
#include "../../../../../../../src/isa/ops/op_effect.h"
#include "../../../../../../../src/isa/ops/op_input.h"
#include "../../../../../../../src/isa/ops/op_status.h"
#include "../../../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t shifted_word(uint8_t source, uint8_t destination,
                             uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF050) |
                      ((uint16_t)source << UINT8_C(9)) |
                      ((uint16_t)destination << UINT8_C(8)) |
                      (uint16_t)shift);
}

static uint16_t fixed_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF065) |
                      ((uint16_t)source << UINT8_C(9)) |
                      ((uint16_t)destination << UINT8_C(8)));
}

static ti_c54x_decoded_instruction decode_xor(uint16_t first_word,
                                              uint16_t extension_word,
                                              bool fixed_shift)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension_word},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id ==
                       (fixed_shift
                            ? TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT16
                            : TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT));
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == first.id);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == extension_word);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(
        instruction.operand_layout ==
        (fixed_shift
             ? TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16
             : TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16));
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    return instruction;
}

static uint64_t expected_xor(uint64_t source, uint16_t immediate,
                             unsigned shift)
{
    return (source ^ ((uint64_t)immediate << shift)) & TI_C54X_U40_MASK;
}

static void assert_effect(const ti_c54x_op_effect *effect, uint8_t destination,
                          uint64_t expected)
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

static ti_c54x_op_status direct_xor(const ti_c54x_decoded_instruction *instruction,
                                    const ti_c54x_op_input_snapshot *input,
                                    ti_c54x_op_effect *effect)
{
    return ti_c54x_op_xor_immediate(instruction, input, effect);
}

static void run_success(uint16_t first_word, uint16_t extension_word,
                        uint8_t source, uint8_t destination, unsigned shift)
{
    const ti_c54x_decoded_instruction instruction =
        decode_xor(first_word, extension_word, shift == 16U);
    const uint64_t a = UINT64_C(0x123456789A);
    const uint64_t b = UINT64_C(0x00ABCDEF01);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(0xA5),
        .status_c_known = false,
        .status_tc = UINT8_C(0x5A),
        .status_tc_known = true};
    const ti_c54x_op_input_snapshot before_input = input;
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect dispatched = direct;
    const uint64_t selected = source == 0U ? a : b;
    const ti_c54x_op_status direct_status =
        direct_xor(&instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(&instruction, &input,
                                                     &dispatched);

    TI_C54X_TEST_CHECK(direct_status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(dispatch_status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&input, &before_input, sizeof(input)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    assert_effect(&direct, destination,
                  expected_xor(selected, extension_word, shift));
}

static void test_all_64_shifted_and_four_fixed_encodings(void)
{
    static const uint16_t immediates[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x7FFF), UINT16_C(0xFFFF)};

    for (uint8_t source = 0U; source < 2U; ++source) {
        for (uint8_t destination = 0U; destination < 2U; ++destination) {
            for (uint8_t shift = 0U; shift < 16U; ++shift) {
                for (size_t i = 0U;
                     i < sizeof(immediates) / sizeof(immediates[0]); ++i) {
                    run_success(shifted_word(source, destination, shift),
                                immediates[i], source, destination, shift);
                }
            }
            for (size_t i = 0U;
                 i < sizeof(immediates) / sizeof(immediates[0]); ++i) {
                run_success(fixed_word(source, destination), immediates[i],
                            source, destination, 16U);
            }
        }
    }
}

static void test_immediate_is_unsigned_and_destination_is_not_an_input(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_xor(shifted_word(0U, 1U, 15U), UINT16_C(0xFFFF), false);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x0000000001),
        .accumulator_b_raw40 = UINT64_C(0x0000000000),
        .accumulator_a_known = true,
        .accumulator_b_known = false,
        .status_c = UINT8_C(0xFF),
        .status_c_known = false,
        .status_tc = UINT8_C(1),
        .status_tc_known = false};
    ti_c54x_op_effect effect = {.kind = TI_C54X_OP_EFFECT_NONE};

    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, 1U, expected_xor(UINT64_C(1), UINT16_C(0xFFFF),
                                             15U));
}

static void assert_rejected(const ti_c54x_decoded_instruction *instruction,
                            const ti_c54x_op_input_snapshot *input,
                            ti_c54x_op_status expected_direct,
                            ti_c54x_op_status expected_dispatch)
{
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;

    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate(instruction, input, &direct) ==
                       expected_direct);
    TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           instruction, input, &dispatched) == expected_dispatch);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) == 0);
}

static void test_raw40_knownness_and_decode_guards(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_xor(fixed_word(0U, 1U), UINT16_C(7), true);
    const ti_c54x_op_input_snapshot good = {
        .accumulator_a_raw40 = UINT64_C(3),
        .accumulator_b_raw40 = UINT64_C(5),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(0),
        .status_c_known = true,
        .status_tc = UINT8_C(0),
        .status_tc_known = true};
    ti_c54x_op_input_snapshot bad = good;
    ti_c54x_decoded_instruction forged = instruction;
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};

    bad.accumulator_a_known = false;
    assert_rejected(&instruction, &bad, TI_C54X_OP_STATUS_UNKNOWN_INPUT,
                    TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    bad = good;
    bad.accumulator_a_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
    assert_rejected(&instruction, &bad, TI_C54X_OP_STATUS_INVALID_OPERAND,
                    TI_C54X_OP_STATUS_INVALID_OPERAND);

    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    assert_rejected(&forged, &good, TI_C54X_OP_STATUS_INVALID_DECODE,
                    TI_C54X_OP_STATUS_UNSUPPORTED_OPCODE);
    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE;
    assert_rejected(&forged, &good, TI_C54X_OP_STATUS_INVALID_DECODE,
                    TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = instruction;
    forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_C;
    assert_rejected(&forged, &good, TI_C54X_OP_STATUS_INVALID_DECODE,
                    TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(1);
    assert_rejected(&forged, &good, TI_C54X_OP_STATUS_INVALID_DECODE,
                    TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = instruction;
    forged.bundle.program_words[0] ^= UINT16_C(0x0001);
    assert_rejected(&forged, &good, TI_C54X_OP_STATUS_INVALID_DECODE,
                    TI_C54X_OP_STATUS_INVALID_DECODE);

    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate(NULL, &good, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate(&instruction, &good, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void test_catalog_bound_aliases(void)
{
    const ti_c54x_decoded_instruction shifted =
        decode_xor(shifted_word(0U, 1U, UINT8_C(3)), UINT16_C(0x1234), false);
    const ti_c54x_decoded_instruction fixed =
        decode_xor(fixed_word(1U, 0U), UINT16_C(0x1234), true);
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
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift(
                           &shifted, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, UINT8_C(1),
                  expected_xor(input.accumulator_a_raw40, UINT16_C(0x1234),
                               UINT8_C(3)));

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift16(
                           &shifted, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift(
                           &fixed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift16(
                           &fixed, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, UINT8_C(0),
                  expected_xor(input.accumulator_b_raw40, UINT16_C(0x1234),
                               UINT8_C(16)));

    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift(&shifted, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift(&shifted, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift16(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift16(&fixed, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xor_immediate_shift16(&fixed, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
    test_all_64_shifted_and_four_fixed_encodings();
    test_immediate_is_unsigned_and_destination_is_not_an_input();
    test_raw40_knownness_and_decode_guards();
    test_catalog_bound_aliases();
    return 0;
}
