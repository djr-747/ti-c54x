/*
 * Module contract
 *
 * Owns: RED proofs for the SAT src leaf: layout and fixed-word
 * validation, 40-to-32-bit saturation regardless of OVM with
 * OVsrc recomputed (set on clamp, cleared otherwise), C/TC
 * preservation, write-back into the source accumulator,
 * checked/trusted parity, input gates, and transactional
 * rejection. Vectors follow the TI SAT
 * examples (B = 71 2345 6789h pins positive, A = F8 1234 5678h
 * pins negative), independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator/status mutation, CPU sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_sat_acc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_SAT_ACC;
    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0xF812345678);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x7123456789);
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

    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(instruction, input,
                                         &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

/* SAT B (word 0xF583): B = 0x7123456789 is positive beyond 32
 * bits, so it pins at 0x007FFFFFFF with OVB set and C/TC
 * preserved. */
static void test_positive_clamp(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* SAT A (word 0xF483): A = 0xF812345678 is negative beyond 32
 * bits, so it pins at 0xFF80000000 with OVA set. */
static void test_negative_clamp(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF483), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* SAT B with B = 0x0000123456 already fitting in 32 bits:
 * unchanged value with OVB recomputed to 0 (TI Example 3). */
static void test_no_clamp_preserves(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_b_raw40 = UINT64_C(0x0000123456);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000123456));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_0);
}

/* A negative value already fitting in 32 bits (0xFFFFEDCBA9,
 * guard 0xFF with bit 31 set) passes through unchanged. */
static void test_negative_no_clamp_preserves(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_b_raw40 = UINT64_C(0xFFFFEDCBA9);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFEDCBA9));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_0);
}

/* Saturation ignores OVM: the clamping vector pins identically
 * under OVM=1 and under unknown OVM. */
static void test_ovm_independent(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm_known = false;
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Unknown source accumulator stops; out-of-range src raw40
 * rejects. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction src_b = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_decoded_instruction src_a = make_instruction(
        UINT16_C(0xF483), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    input = known_input();
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=1] matches the checked SAT B vector. */
static void test_trusted_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Malformed trusted operands (bad count, source out of range, bad
 * width) reject as INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Wrong id, layout, word count, or fixed opcode word rejects as
 * INVALID_DECODE; nulls reject. */
static void test_layout_rejects(void)
{
    ti_c54x_decoded_instruction wrong_id = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0xF583),
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = make_instruction(
        UINT16_C(0xF583), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_decoded_instruction rnd_word = make_instruction(
        UINT16_C(0xF59F), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_trusted_operand_values operands = {0};

    wrong_id.id = TI_C54X_OPCODE_ID_NOP;
    two_words.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&wrong_id, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&wrong_layout, &input,
                                         &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&rnd_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&two_words, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_sat_acc(&two_words, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &two_words, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &two_words, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sat_acc(
                           &two_words, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_positive_clamp();
    test_negative_clamp();
    test_no_clamp_preserves();
    test_negative_no_clamp_preserves();
    test_ovm_independent();
    test_checked_input_gates();
    test_trusted_completion();
    test_trusted_shape_rejects();
    test_layout_rejects();
    return 0;
}
