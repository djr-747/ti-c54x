/* Module contract
 * Owns: pure ROLTC accumulator-handler and generated-dispatch proofs.
 * Does not own: catalog rows, operand layout extraction, accumulator
 * mutation, status mutation, sequencing, CPU lifecycle, timing, or repeat/
 * interrupt policy.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/rotate/op_roltc.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
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
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_ROLTC_ACC);
    TI_C54X_TEST_CHECK(
        instruction.operand_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC);
    return instruction;
}

static uint64_t expected_value(uint64_t source, uint8_t tc)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    return ((low32 << 1U) & UINT64_C(0xFFFFFFFF)) | (uint64_t)tc;
}

static uint8_t expected_c(uint64_t source)
{
    return (uint8_t)((source >> 31U) & UINT64_C(1));
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
}

static ti_c54x_op_input_snapshot input_for(uint64_t a, uint64_t b,
                                           bool a_known, bool b_known,
                                           uint8_t status_c,
                                           bool status_c_known, uint8_t tc,
                                           bool tc_known)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_c = status_c,
        .status_c_known = status_c_known,
        .status_tc = tc,
        .status_tc_known = tc_known};

    return input;
}

static void check_case(uint16_t word, uint64_t a, uint64_t b, bool a_known,
                       bool b_known, uint8_t status_c, bool status_c_known,
                       uint8_t tc, bool tc_known,
                       ti_c54x_op_status expected_status)
{
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input =
        input_for(a, b, a_known, b_known, status_c, status_c_known, tc,
                  tc_known);
    const ti_c54x_op_input_snapshot input_before = input;
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_NONE,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        ti_c54x_op_roltc_acc(&instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(&instruction, &input,
                                                     &dispatched);
    const uint8_t source = (uint8_t)((word >> 8U) & UINT16_C(1));
    const uint64_t selected = source == 0U ? a : b;

    TI_C54X_TEST_CHECK(direct_status == expected_status);
    TI_C54X_TEST_CHECK(dispatch_status == expected_status);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
    if (expected_status != TI_C54X_OP_STATUS_OK) {
        TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) ==
                           0);
        return;
    }

    assert_effect(&direct, expected_value(selected, tc), source,
                  expected_c(selected));
    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
}

static void check_all_forms_and_vectors(void)
{
    static const uint16_t forms[] = {UINT16_C(0xF492), UINT16_C(0xF592)};
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x0000000007), UINT64_C(0x000000007FFFFFFF),
        UINT64_C(0x0000000080000000), UINT64_C(0x00000000FFFFFFFF),
        UINT64_C(0xFF00000000), UINT64_C(0xFF80000000),
        UINT64_C(0x123456789A), UINT64_C(0x00ABCDEF01)};
    size_t form_index;
    size_t vector_index;
    uint8_t tc;

    for (form_index = 0U; form_index < sizeof(forms) / sizeof(forms[0]);
         ++form_index) {
        const uint8_t source = (uint8_t)((forms[form_index] >> 8U) & 1U);

        for (tc = UINT8_C(0); tc <= UINT8_C(1); ++tc) {
            for (vector_index = 0U;
                 vector_index < sizeof(vectors) / sizeof(vectors[0]);
                 ++vector_index) {
                const uint64_t selected = vectors[vector_index];
                const uint64_t other =
                    vectors[(vector_index + 3U) %
                            (sizeof(vectors) / sizeof(vectors[0]))];

                check_case(forms[form_index], source == 0U ? selected : other,
                           source == 1U ? selected : other, true, true,
                           UINT8_C(1), false, tc, true,
                           TI_C54X_OP_STATUS_OK);
            }
        }
    }
}

static void check_unknown_and_invalid_inputs(void)
{
    /* Unknown unselected state is allowed; selected state is not. */
    check_case(UINT16_C(0xF492), UINT64_C(3), UINT64_C(0x12345), true, false,
               UINT8_C(1), false, UINT8_C(1), true,
               TI_C54X_OP_STATUS_OK);
    check_case(UINT16_C(0xF592), UINT64_C(0x12345), UINT64_C(3), false, true,
               UINT8_C(0), true, UINT8_C(0), true,
               TI_C54X_OP_STATUS_OK);
    check_case(UINT16_C(0xF492), UINT64_C(3), UINT64_C(0), false, true,
               UINT8_C(0), true, UINT8_C(1), true,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(UINT16_C(0xF592), UINT64_C(0), UINT64_C(3), true, false,
               UINT8_C(0), true, UINT8_C(0), true,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(UINT16_C(0xF492), UINT64_C(3), UINT64_C(0), true, true,
               UINT8_C(0), true, UINT8_C(0), false,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    /* C is not an input to ROLTC: either canonical value/knownness is ignored. */
    check_case(UINT16_C(0xF492), UINT64_C(0x80000000), UINT64_C(0), true,
               true, UINT8_C(1), false, UINT8_C(1), true,
               TI_C54X_OP_STATUS_OK);
    check_case(UINT16_C(0xF592), UINT64_C(0), UINT64_C(0x80000000), true,
               true, UINT8_C(1), true, UINT8_C(0), true,
               TI_C54X_OP_STATUS_OK);

    check_case(UINT16_C(0xF492), TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(0),
               true, true, UINT8_C(0), false, UINT8_C(0), true,
               TI_C54X_OP_STATUS_INVALID_OPERAND);
    check_case(UINT16_C(0xF592), UINT64_C(0), UINT64_C(3), true, true,
               UINT8_C(0), false, UINT8_C(2), true,
               TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void check_forged_decode_and_arguments(void)
{
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(1), UINT64_C(2), true, true, UINT8_C(1), false,
                  UINT8_C(1), true);
    const ti_c54x_decoded_instruction instruction = decode(UINT16_C(0xF492));
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_roltc_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_roltc_acc(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_roltc_acc(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0001);
        TI_C54X_TEST_CHECK(ti_c54x_op_roltc_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.operand_layout =
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
        TI_C54X_TEST_CHECK(ti_c54x_op_roltc_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C;
        TI_C54X_TEST_CHECK(ti_c54x_op_roltc_acc(&forged, &input, &effect) ==
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
    check_forged_decode_and_arguments();
    return 0;
}
