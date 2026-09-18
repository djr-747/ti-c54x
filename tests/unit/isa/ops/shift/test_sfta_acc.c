/* Module contract
 * Owns: pure row-less SFTA accumulator-handler proofs, including the two
 * TI worked examples, shift-zero identity, SXM fill selection, OVM
 * saturation, unknown/invalid input gating, decode rejection, and
 * checked/trusted parity.
 * Does not own: catalog rows, dispatch binding, accumulator/status
 * mutation, sequencing, CPU lifecycle, timing, or repeat/interrupt policy.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_handler.h"
#include "../../../../../src/isa/ops/shift/op_sfta_acc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t sfta_word(uint8_t source, uint8_t destination,
                          uint8_t shift_raw)
{
    return (uint16_t)(UINT16_C(0xF460) |
                      ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) |
                      (uint16_t)(shift_raw & UINT8_C(0x1F)));
}

static ti_c54x_decoded_instruction forge(uint16_t word, uint8_t count)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_SFTA_ACC;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5;
    instruction.bundle.program_words[0] = word;
    instruction.bundle.program_word_count = count;
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint64_t a, bool a_known,
                                           uint64_t b, bool b_known,
                                           uint8_t sxm, bool sxm_known,
                                           uint8_t ovm, bool ovm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = a;
    input.accumulator_a_known = a_known;
    input.accumulator_b_raw40 = b;
    input.accumulator_b_known = b_known;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    input.status_ovm = ovm;
    input.status_ovm_known = ovm_known;
    return input;
}

static ti_c54x_trusted_operand_values trusted_ops(uint8_t source,
                                                  uint8_t destination,
                                                  uint8_t shift_raw)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = destination;
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = shift_raw;
    operands.values[2].width_bits = UINT8_C(5);
    return operands;
}

static void assert_effect(const ti_c54x_op_effect *effect,
                          uint8_t destination, uint64_t value,
                          uint8_t c_value, bool overflow)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       (c_value == UINT8_C(0)
                            ? TI_C54X_OP_STATUS_BIT_WRITE_0
                            : TI_C54X_OP_STATUS_BIT_WRITE_1));
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta
                           .destination_overflow_action ==
                       (overflow
                            ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                            : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.value_raw40 <=
                       TI_C54X_U40_MASK);
}

/* Runs one vector through both entries and demands identical outcomes. */
static void check_ok(uint8_t source, uint8_t destination, uint8_t shift_raw,
                     const ti_c54x_op_input_snapshot *input,
                     uint64_t expected_value, uint8_t expected_c,
                     bool expected_overflow)
{
    const uint16_t word = sfta_word(source, destination, shift_raw);
    const ti_c54x_decoded_instruction instruction =
        forge(word, UINT8_C(1));
    const ti_c54x_trusted_operand_values operands =
        trusted_ops(source, destination, shift_raw);
    const ti_c54x_op_input_snapshot before_input = *input;
    ti_c54x_op_effect checked = {
        .kind = TI_C54X_OP_EFFECT_NONE,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect trusted = checked;
    ti_c54x_op_status checked_status;
    ti_c54x_op_status trusted_status;
    ti_c54x_op_input_snapshot mutable_input = *input;

    checked_status = ti_c54x_op_sfta_acc(&instruction, input, &checked);
    trusted_status =
        ti_c54x_trusted_op_sfta_acc(&instruction, &operands,
                                    &mutable_input, &trusted);
    TI_C54X_TEST_CHECK(checked_status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted_status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(input, &before_input, sizeof(*input)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&mutable_input, &before_input,
                              sizeof(mutable_input)) == 0);
    assert_effect(&checked, destination, expected_value, expected_c,
                  expected_overflow);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);
}

static void check_reject(const ti_c54x_decoded_instruction *instruction,
                         const ti_c54x_op_input_snapshot *input,
                         ti_c54x_op_status expected_status)
{
    const ti_c54x_op_input_snapshot before_input = *input;
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_sfta_acc(instruction, input, &effect) ==
                       expected_status);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(input, &before_input, sizeof(*input)) == 0);
}

static void check_ti_worked_examples(void)
{
    ti_c54x_op_input_snapshot input;

    /* TI worked example 1: SFTA A,-5,B with SXM set. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0),
                      true, UINT8_C(1), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(27), &input,
             UINT64_C(0xFFFC3B2802), UINT8_C(1), false);

    /* TI worked example 2: SFTA B,+5; overflow by the no-sign-loss rule. */
    input = input_for(UINT64_C(0), true, UINT64_C(0x80AA001234), true,
                      UINT8_C(0), true, UINT8_C(0), true);
    check_ok(UINT8_C(1), UINT8_C(1), UINT8_C(5), &input,
             UINT64_C(0x1540024680), UINT8_C(0), true);
}

static void check_shift_zero_identity(void)
{
    ti_c54x_op_input_snapshot input;

    /* Negative guard keeps its value and reports C equal to the sign. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0),
                      true, UINT8_C(1), true, UINT8_C(1), true);
    check_ok(UINT8_C(0), UINT8_C(0), UINT8_C(0), &input,
             UINT64_C(0xFF87650055), UINT8_C(1), false);

    /* Non-negative value reports C equal to the sign (zero). */
    input = input_for(UINT64_C(0x0001234567), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(0), &input,
             UINT64_C(0x0001234567), UINT8_C(0), false);
}

static void check_sxm_fill_selection(void)
{
    ti_c54x_op_input_snapshot input;

    /* SXM set sign-fills the top COUNT bits on a right shift. */
    input = input_for(UINT64_C(0xFF00000001), true, UINT64_C(0),
                      true, UINT8_C(1), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(31), &input,
             UINT64_C(0xFF80000000), UINT8_C(1), false);

    /* SXM clear zero-fills the same shift. */
    input = input_for(UINT64_C(0xFF00000001), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(31), &input,
             UINT64_C(0x7F80000000), UINT8_C(1), false);

    /* A clear sign bit fills zeros even when SXM is set. */
    input = input_for(UINT64_C(0x0012345678), true, UINT64_C(0),
                      true, UINT8_C(1), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(0), UINT8_C(30), &input,
             UINT64_C(0x00048D159E), UINT8_C(0), false);
}

static void check_ovm_saturation(void)
{
    ti_c54x_op_input_snapshot input;

    /* Positive overflow saturates to 0x007FFFFFFF under OVM. */
    input = input_for(UINT64_C(0x0100000000), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(1), true);
    check_ok(UINT8_C(0), UINT8_C(0), UINT8_C(1), &input,
             UINT64_C(0x007FFFFFFF), UINT8_C(0), true);

    /* The same overflow wraps when OVM is clear. */
    input = input_for(UINT64_C(0x0100000000), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(0), UINT8_C(1), &input,
             UINT64_C(0x0200000000), UINT8_C(0), true);

    /* Negative overflow (wrapped bit 31 set) saturates to 0xFF80000000. */
    input = input_for(UINT64_C(0x0040000000), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(1), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(1), &input,
             UINT64_C(0xFF80000000), UINT8_C(0), true);

    /* The same negative overflow wraps when OVM is clear. */
    input = input_for(UINT64_C(0x0040000000), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(1), &input,
             UINT64_C(0x0080000000), UINT8_C(0), true);

    /* A clean left shift preserves OV. */
    input = input_for(UINT64_C(0x0000000001), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(1), true);
    check_ok(UINT8_C(0), UINT8_C(0), UINT8_C(1), &input,
             UINT64_C(0x0000000002), UINT8_C(0), false);
}

static void check_shift_extremes(void)
{
    ti_c54x_op_input_snapshot input;

    /* SHIFT -16 (raw 16) reads C from bit 15 of the source. */
    input = input_for(UINT64_C(0x0000008000), true, UINT64_C(0),
                      true, UINT8_C(1), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(16), &input,
             UINT64_C(0x0000000000), UINT8_C(1), false);

    /* SHIFT +15 (raw 15) is the largest left shift. */
    input = input_for(UINT64_C(0x0000000001), true, UINT64_C(0),
                      true, UINT8_C(0), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(15), &input,
             UINT64_C(0x0000008000), UINT8_C(0), false);
}

static void check_unknown_and_invalid_inputs(void)
{
    const uint16_t word = sfta_word(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    ti_c54x_decoded_instruction instruction =
        forge(word, UINT8_C(1));
    ti_c54x_op_input_snapshot input;

    /* Unknown selected accumulator rejects. */
    input = input_for(UINT64_C(3), false, UINT64_C(0x12345), true,
                      UINT8_C(1), true, UINT8_C(0), true);
    check_reject(&instruction, &input, TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    /* Unknown unselected accumulator is allowed. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0x12345),
                      false, UINT8_C(1), true, UINT8_C(0), true);
    check_ok(UINT8_C(0), UINT8_C(1), UINT8_C(27), &input,
             UINT64_C(0xFFFC3B2802), UINT8_C(1), false);

    /* Unknown SXM rejects. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0), true,
                      UINT8_C(0), false, UINT8_C(0), true);
    check_reject(&instruction, &input, TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    /* Unknown OVM rejects. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0), true,
                      UINT8_C(1), true, UINT8_C(0), false);
    check_reject(&instruction, &input, TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    /* Raw40 outside the 40-bit mask rejects. */
    input = input_for(TI_C54X_U40_MASK + UINT64_C(1), true,
                      UINT64_C(0), true, UINT8_C(1), true,
                      UINT8_C(0), true);
    check_reject(&instruction, &input, TI_C54X_OP_STATUS_INVALID_OPERAND);

    /* Non-binary SXM rejects. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0), true,
                      UINT8_C(2), true, UINT8_C(0), true);
    check_reject(&instruction, &input, TI_C54X_OP_STATUS_INVALID_OPERAND);

    /* Non-binary OVM rejects. */
    input = input_for(UINT64_C(0xFF87650055), true, UINT64_C(0), true,
                      UINT8_C(1), true, UINT8_C(0xFF), true);
    check_reject(&instruction, &input, TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void check_decode_rejection(void)
{
    const uint16_t word = sfta_word(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0xFF87650055), true, UINT64_C(0), true,
                  UINT8_C(1), true, UINT8_C(0), true);
    ti_c54x_decoded_instruction forged;

    /* A catalog id is not a row-less SFTA. */
    forged = forge(word, UINT8_C(1));
    forged.id = TI_C54X_OPCODE_ID_SFTL_ACC;
    check_reject(&forged, &input, TI_C54X_OP_STATUS_INVALID_DECODE);

    /* A different layout is not SFTA. */
    forged = forge(word, UINT8_C(1));
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE;
    check_reject(&forged, &input, TI_C54X_OP_STATUS_INVALID_DECODE);

    /* SFTA is exactly one program word. */
    forged = forge(word, UINT8_C(2));
    forged.bundle.program_words[1] = UINT16_C(0);
    check_reject(&forged, &input, TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = forge(word, UINT8_C(0));
    check_reject(&forged, &input, TI_C54X_OP_STATUS_INVALID_DECODE);

    /* Masked-out first-word bits must equal 0xF460. */
    forged = forge((uint16_t)(word ^ UINT16_C(0x0040)), UINT8_C(1));
    check_reject(&forged, &input, TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = forge((uint16_t)(UINT16_C(0xF440) | (UINT16_C(0) << 9U) |
                                 (UINT16_C(1) << 8U) | UINT16_C(27)),
                   UINT8_C(1));
    check_reject(&forged, &input, TI_C54X_OP_STATUS_INVALID_DECODE);

    /* Null arguments reject without touching the effect. */
    {
        const ti_c54x_decoded_instruction good =
            forge(word, UINT8_C(1));
        ti_c54x_op_effect effect = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
        const ti_c54x_op_effect before = effect;

        TI_C54X_TEST_CHECK(ti_c54x_op_sfta_acc(NULL, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(ti_c54x_op_sfta_acc(&good, NULL, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(ti_c54x_op_sfta_acc(&good, &input, NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_entry_rejection(void)
{
    const uint16_t word = sfta_word(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    const ti_c54x_decoded_instruction instruction =
        forge(word, UINT8_C(1));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0xFF87650055), true, UINT64_C(0), true,
                  UINT8_C(1), true, UINT8_C(0), true);
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;
    ti_c54x_trusted_operand_values operands;

    /* Fewer than three operand values reject. */
    operands = trusted_ops(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    /* Wrong field widths reject. */
    operands = trusted_ops(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    operands.values[2].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    /* Out-of-range raw values reject. */
    operands = trusted_ops(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    /* Unknown inputs reject through the trusted entry as well. */
    {
        const ti_c54x_op_input_snapshot unknown =
            input_for(UINT64_C(0xFF87650055), true, UINT64_C(0),
                      true, UINT8_C(1), false, UINT8_C(0), true);
        operands = trusted_ops(UINT8_C(0), UINT8_C(1), UINT8_C(27));
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                               &instruction, &operands, &unknown,
                               &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    /* Null trusted arguments reject. */
    operands = trusted_ops(UINT8_C(0), UINT8_C(1), UINT8_C(27));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                           &instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sfta_acc(
                           &instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_ti_worked_examples();
    check_shift_zero_identity();
    check_sxm_fill_selection();
    check_ovm_saturation();
    check_shift_extremes();
    check_unknown_and_invalid_inputs();
    check_decode_rejection();
    check_trusted_entry_rejection();
    return 0;
}
