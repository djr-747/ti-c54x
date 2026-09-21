/*
 * Module contract
 *
 * Owns: RED proof of the pure RESET software-reset effect
 * (P3-S103, TI SPRU172C section RESET, pp. 4-137-4-138, 1 word):
 * decode identity, checked/trusted equivalence, input
 * independence, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors,
 * address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, or diagnostics.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/isa/ops/op_reset.h"
#include "../../../../src/isa/ops/op_status.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction
decode_reset(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RESET);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static void
assert_reset_effect(const ti_c54x_op_effect *effect)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_RESET);
}

static void
test_effect_and_parity(void)
{
    /* RESET takes no operands: the trusted path carries none. */
    const ti_c54x_decoded_instruction instruction =
        decode_reset(UINT16_C(0xF7E0));
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_trusted_operand_values operands = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect checked = before;
    ti_c54x_op_effect trusted = before;

    TI_C54X_TEST_CHECK(operands.count == UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_op_reset(&instruction, &input,
                                       &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reset(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    assert_reset_effect(&checked);
    assert_reset_effect(&trusted);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                              sizeof(checked)) == 0);
}

static void
test_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_reset(UINT16_C(0xF7E0));
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_reset(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_reset(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_reset(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_reset(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(1);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reset(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reset(
                           &instruction, NULL, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reset(
                           &instruction, &operands, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_effect_and_parity();
    test_rejection_is_transactional();
    return 0;
}
