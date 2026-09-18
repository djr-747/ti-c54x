/*
 * Module contract
 *
 * Owns: RED proofs for the ABS src[,dst] leaf: layout and
 * fixed-word validation, cross-accumulator absolute value,
 * OVM saturation of a guard-overflowing magnitude with sticky
 * OVdst, C set from a zero stored result, TC preservation,
 * checked/trusted parity, input gates, and transactional
 * rejection. Vectors follow the TI ABS description (SPRU172C
 * section ABS, p. 4-3), independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator/status mutation, CPU sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_abs_acc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_ABS_ACC;
    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFFFFCB);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000000000);
    input.accumulator_b_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect run_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(instruction, input,
                                         &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

/* ABS A,B (word 0xF585): A = 0xFFFFFFFFCB is -53, so B takes
 * 0x0000000035 with OVB preserved and C cleared. */
static void test_cross_accumulator(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000000035));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* ABS A,A (word 0xF485) with OVM=1: A = 0x0312345678 is a
 * positive magnitude with guard bits set, so it saturates to
 * 0x007FFFFFFF with OVA set and C cleared. */
static void test_ovm_saturates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF485),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x0312345678);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=0 with the same guard-overflowing input: the magnitude is
 * held unchanged (no saturation) with OVA sticky-set. */
static void test_ovm_off_holds(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF485),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x0312345678);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0312345678));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Zero input stores zero with C set and OV preserved. */
static void test_zero_sets_carry(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x0000000000);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Most-negative input 0x8000000000 has no 40-bit negation, so it
 * is held with OV set; OVM=1 saturates it to 0x007FFFFFFF. */
static void test_min_negative(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF685),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_b_raw40 = UINT64_C(0x8000000000);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x8000000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Unknown source or unknown OVM stops; out-of-range src raw40 or
 * OVM value rejects. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction src_a_dst_b = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=0,D=1] matches the checked ABS A,B vector byte for
 * byte. */
static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect checked = run_checked(&instruction, &input);
    ti_c54x_op_effect trusted = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);
}

/* Malformed trusted operands (bad count, source out of range, bad
 * width) reject as INVALID_OPERAND without touching the
 * effect. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect effect = before;

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abs_acc(
                           &instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Wrong id, layout, word count, or fixed opcode word rejects as
 * INVALID_DECODE; nulls reject. Every rejection leaves the
 * caller-owned effect unchanged. */
static void test_layout_rejects(void)
{
    ti_c54x_decoded_instruction wrong_id = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = make_instruction(
        UINT16_C(0xF585),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_decoded_instruction bad_word = make_instruction(
        UINT16_C(0xF49F),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

    wrong_id.id = TI_C54X_OPCODE_ID_NOP;
    two_words.bundle.program_word_count = UINT8_C(2);
    forged = wrong_id;
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = wrong_layout;
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = two_words;
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = bad_word;
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&two_words, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_abs_acc(&two_words, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_cross_accumulator();
    test_ovm_saturates();
    test_ovm_off_holds();
    test_zero_sets_carry();
    test_min_negative();
    test_checked_input_gates();
    test_trusted_parity();
    test_trusted_shape_rejects();
    test_layout_rejects();
    return 0;
}
