/* Module contract
 * Owns: pure CMPL accumulator effect and generated-dispatch proofs.
 * Does not own: accumulator mutation, CPU lifecycle, sequencing, or timing.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/accumulator/op_cmpl.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
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
    return instruction;
}

static void check(uint16_t word, uint64_t a, uint64_t b, bool a_known,
                  bool b_known, ti_c54x_op_status expected_status,
                  uint64_t expected_value, uint8_t expected_destination)
{
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known};
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_NONE,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        ti_c54x_op_cmpl_acc(&instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(
            &instruction, &input, &dispatched);

    TI_C54X_TEST_CHECK(direct_status == expected_status);
    TI_C54X_TEST_CHECK(dispatch_status == expected_status);
    if (expected_status != TI_C54X_OP_STATUS_OK) {
        TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) == 0);
        return;
    }
    TI_C54X_TEST_CHECK(direct.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(direct.payload.accumulator_write.destination ==
                       expected_destination);
    TI_C54X_TEST_CHECK_U64(direct.payload.accumulator_write.value_raw40,
                           expected_value);
    TI_C54X_TEST_CHECK(direct.payload.accumulator_write.c_action ==
                       TI_C54X_OP_ACCUMULATOR_C_PRESERVE);
    TI_C54X_TEST_CHECK(direct.payload.accumulator_write.c_value == 0U);
    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
}

int main(void)
{
    check(UINT16_C(0xF493), UINT64_C(0), UINT64_C(0xFFFF), true, true,
          TI_C54X_OP_STATUS_OK, UINT64_C(0xFFFFFFFFFF), 0U);
    check(UINT16_C(0xF693), UINT64_C(0), UINT64_C(0x8000000000), true, true,
          TI_C54X_OP_STATUS_OK, UINT64_C(0x7FFFFFFFFF), 0U);
    check(UINT16_C(0xF593), UINT64_C(0x12345), UINT64_C(0xAAAAA), true,
          true, TI_C54X_OP_STATUS_OK, UINT64_C(0xFFFFFEDCBA), 1U);
    check(UINT16_C(0xF793), UINT64_C(0x8000000000),
          UINT64_C(0x0000000000), true, true, TI_C54X_OP_STATUS_OK,
          UINT64_C(0xFFFFFFFFFF), 1U);
    check(UINT16_C(0xF493), UINT64_C(0xFFFFFFFFFF), UINT64_C(0), true,
          true, TI_C54X_OP_STATUS_OK, UINT64_C(0), 0U);
    check(UINT16_C(0xF793), UINT64_C(0), UINT64_C(0x7FFFFFFFFF), true,
          true, TI_C54X_OP_STATUS_OK, UINT64_C(0x8000000000), 1U);
    check(UINT16_C(0xF493), 1U, 2U, false, true,
          TI_C54X_OP_STATUS_UNKNOWN_INPUT, 0U, 0U);
    check(UINT16_C(0xF693), 1U, 2U, true, false,
          TI_C54X_OP_STATUS_UNKNOWN_INPUT, 0U, 0U);
    check(UINT16_C(0xF493), 1U, 2U, true, false,
          TI_C54X_OP_STATUS_OK, UINT64_C(0xFFFFFFFFFE), 0U);
    check(UINT16_C(0xF693), 1U, 2U, false, true,
          TI_C54X_OP_STATUS_OK, UINT64_C(0xFFFFFFFFFD), 0U);
    check(UINT16_C(0xF493), TI_C54X_U40_MASK + 1U, 2U, true, true,
          TI_C54X_OP_STATUS_INVALID_OPERAND, 0U, 0U);
    check(UINT16_C(0xF693), 1U, TI_C54X_U40_MASK + 1U, true, true,
          TI_C54X_OP_STATUS_INVALID_OPERAND, 0U, 0U);
    check(UINT16_C(0xF493), 1U, TI_C54X_U40_MASK + 1U, true, true,
          TI_C54X_OP_STATUS_OK, UINT64_C(0xFFFFFFFFFE), 0U);
    check(UINT16_C(0xF693), TI_C54X_U40_MASK + 1U, 2U, true, true,
          TI_C54X_OP_STATUS_OK, UINT64_C(0xFFFFFFFFFD), 0U);

    {
        ti_c54x_decoded_instruction instruction = decode(UINT16_C(0xF493));
        const ti_c54x_op_input_snapshot input = {
            .accumulator_a_raw40 = 1U,
            .accumulator_b_raw40 = 2U,
            .accumulator_a_known = true,
            .accumulator_b_known = true};
        ti_c54x_op_effect effect = {0};
        ti_c54x_op_effect before;

        instruction.bundle.program_words[0] ^= UINT16_C(0x0010);
        before.kind = TI_C54X_OP_EFFECT_REPEAT_START;
        before.payload.repeat_start.operand = UINT16_C(0xCAFE);
        effect = before;
        TI_C54X_TEST_CHECK(ti_c54x_op_cmpl_acc(&instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        instruction = decode(UINT16_C(0xF493));
        instruction.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        effect = before;
        TI_C54X_TEST_CHECK(ti_c54x_op_cmpl_acc(&instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    return 0;
}
