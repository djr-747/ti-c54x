/* Module contract
 * Owns: pure SFTC accumulator-handler and generated-dispatch proofs,
 * including its zero/sign-boundary predicate, guard preservation, and
 * transactional input validation.
 * Does not own: catalog rows, operand layout extraction, accumulator
 * mutation, status mutation, sequencing, CPU lifecycle, timing, or
 * repeat/interrupt policy.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/shift/op_sftc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_SFTC_ACC);
    TI_C54X_TEST_CHECK(
        instruction.operand_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(
        instruction.input_resources == TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static uint64_t expected_value(uint64_t source)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    if (low32 == UINT64_C(0) ||
        (((low32 >> 31U) ^ (low32 >> 30U)) & UINT64_C(1)) != UINT64_C(0)) {
        return source;
    }
    return (source & UINT64_C(0xFF00000000)) |
           ((low32 << 1U) & UINT64_C(0xFFFFFFFF));
}

static ti_c54x_op_status direct_handler(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ti_c54x_op_sftc_acc(instruction, input, effect);
}

static void assert_effect(const ti_c54x_op_effect *effect, uint64_t value,
                          uint8_t destination, uint8_t tc_value)
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
                       (tc_value == UINT8_C(0)
                            ? TI_C54X_OP_STATUS_BIT_WRITE_0
                            : TI_C54X_OP_STATUS_BIT_WRITE_1));
}

static void check_case(uint16_t word, uint64_t a, uint64_t b, bool a_known,
                       bool b_known, uint8_t status_c, bool status_c_known,
                       uint8_t status_tc, bool status_tc_known,
                       ti_c54x_op_status expected_status)
{
    const uint8_t source = (uint8_t)((word >> 8U) & UINT16_C(1));
    const uint8_t destination = source;
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_c = status_c,
        .status_c_known = status_c_known,
        .status_tc = status_tc,
        .status_tc_known = status_tc_known};
    const ti_c54x_op_input_snapshot input_before = input;
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        direct_handler(&instruction, &input, &direct);
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
        const uint64_t selected = source == UINT8_C(0) ? a : b;
        const uint8_t tc_value =
            ((selected & UINT64_C(0xFFFFFFFF)) == UINT64_C(0) ||
             ((((selected & UINT64_C(0xFFFFFFFF)) >> 31U) ^
               ((selected & UINT64_C(0xFFFFFFFF)) >> 30U)) & UINT64_C(1)) !=
                 UINT64_C(0))
                ? UINT8_C(1)
                : UINT8_C(0);

        assert_effect(&direct, expected_value(selected), destination, tc_value);
        TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    }
}

static void check_form_vectors(uint16_t word)
{
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), /* full zero */
        UINT64_C(0x0000000001), UINT64_C(0x00003FFFFFFF),
        UINT64_C(0x0000C0000001), /* sign bits equal, shift */
        UINT64_C(0x000080000000), /* sign bits differ, unchanged */
        UINT64_C(0x000040000000), /* sign bits differ, unchanged */
        UINT64_C(0xFFFFFFF001),   /* TI guard-bearing shift example */
        UINT64_C(0x1200000001)};
    const uint8_t source = (uint8_t)((word >> 8U) & UINT16_C(1));
    size_t index;
    uint8_t status_c;
    uint8_t status_tc;
    unsigned status_c_known_index;
    unsigned status_tc_known_index;

    for (status_c_known_index = 0U; status_c_known_index <= 1U;
         ++status_c_known_index) {
        for (status_tc_known_index = 0U; status_tc_known_index <= 1U;
             ++status_tc_known_index) {
            for (status_c = UINT8_C(0); status_c <= UINT8_C(1); ++status_c) {
                for (status_tc = UINT8_C(0); status_tc <= UINT8_C(1);
                     ++status_tc) {
                    for (index = 0U;
                         index < sizeof(vectors) / sizeof(vectors[0]);
                         ++index) {
                        const uint64_t selected = vectors[index];
                        const uint64_t other =
                            vectors[(index + 3U) %
                                    (sizeof(vectors) / sizeof(vectors[0]))];
                        check_case(word, source == UINT8_C(0) ? selected : other,
                                   source == UINT8_C(1) ? selected : other,
                                   true, true, status_c,
                                   status_c_known_index != 0U, status_tc,
                                   status_tc_known_index != 0U,
                                   TI_C54X_OP_STATUS_OK);
                    }
                }
            }
        }
    }
}

static void check_unknown_and_invalid_inputs(void)
{
    /* An unknown unselected accumulator is not an SFTC input. */
    check_case(UINT16_C(0xF494), UINT64_C(1), UINT64_C(0x12345), true, false,
               UINT8_C(1), false, UINT8_C(1), false,
               TI_C54X_OP_STATUS_OK);
    check_case(UINT16_C(0xF594), UINT64_C(0x12345), UINT64_C(1), false, true,
               UINT8_C(0), true, UINT8_C(1), false,
               TI_C54X_OP_STATUS_OK);

    check_case(UINT16_C(0xF494), UINT64_C(1), UINT64_C(0), false, true,
               UINT8_C(0), true, UINT8_C(0), false,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(UINT16_C(0xF594), UINT64_C(0), UINT64_C(1), true, false,
               UINT8_C(0), true, UINT8_C(1), true,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    /* SFTC does not read either canonical pre-C or pre-TC value. */
    check_case(UINT16_C(0xF494), UINT64_C(0x0000000001), UINT64_C(0), true,
               true, UINT8_C(1), true, UINT8_C(0), true,
               TI_C54X_OP_STATUS_OK);
    check_case(UINT16_C(0xF594), UINT64_C(0), UINT64_C(0x0000000001), true,
               true, UINT8_C(0), false, UINT8_C(1), false,
               TI_C54X_OP_STATUS_OK);

    check_case(UINT16_C(0xF494), TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(0),
               true, true, UINT8_C(0), false, UINT8_C(0), false,
               TI_C54X_OP_STATUS_INVALID_OPERAND);
    check_case(UINT16_C(0xF594), UINT64_C(0), TI_C54X_U40_MASK + UINT64_C(1),
               true, true, UINT8_C(0), false, UINT8_C(0), false,
               TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void check_forged_decode_and_null_rollback(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0xF494));
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(1),
        .accumulator_b_raw40 = UINT64_C(2),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(1),
        .status_c_known = false,
        .status_tc = UINT8_C(1),
        .status_tc_known = false};
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_sftc_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sftc_acc(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sftc_acc(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0010);
        TI_C54X_TEST_CHECK(ti_c54x_op_sftc_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
        TI_C54X_TEST_CHECK(ti_c54x_op_sftc_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC;
        TI_C54X_TEST_CHECK(ti_c54x_op_sftc_acc(&forged, &input, &effect) ==
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
    check_form_vectors(UINT16_C(0xF494));
    check_form_vectors(UINT16_C(0xF594));
    check_unknown_and_invalid_inputs();
    check_forged_decode_and_null_rollback();
    return 0;
}
