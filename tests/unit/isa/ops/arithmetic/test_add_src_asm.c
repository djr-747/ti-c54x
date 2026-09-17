/*
 * Module contract
 *
 * Owns: RED proofs for the ADD src,ASM leaf (syntax 10): layout and
 * fixed-word validation, ASM=0/positive/negative shifts with
 * SXM-dependent right shifts, OVM saturation with sticky OVdst,
 * bit-32 carry into C with TC preserved, same- and
 * cross-accumulator forms, checked/trusted parity, input gates, and
 * transactional rejection. Vectors follow the TI formula
 * (dst = dst40 plus (src40 << ASM)), independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator/status mutation, CPU sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_add_src_asm.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0012340000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x00020000);
    input.accumulator_b_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    input.status_asm = UINT8_C(0);
    input.status_asm_known = true;
    return input;
}

static ti_c54x_op_effect run_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(instruction, input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

/* ASM=0, S=0, D=0 (word 0xF480): A = A + A = 0x0024680000, no
 * carry so C=0, TC preserved, OVdst kept. */
static void test_asm_zero_same_accumulator(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0024680000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* ASM=+1, S=1, D=0 (word 0xF680): A = A + (B << 1) =
 * 0x0012340000 + 0x40000 = 0x0012380000, C=0. */
static void test_positive_shift_cross_accumulator(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF680),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.status_asm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0012380000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* ASM=-1 (0x1F), SXM=1, S=0, D=0: A = 0xFFFFFF0000 +
 * (A >> 1 arithmetic = 0xFFFFFF8000) = 0xFFFFFE8000 with carry
 * so C=1, no overflow. */
static void test_negative_shift_sxm_arithmetic(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFF0000);
    input.status_asm = UINT8_C(31);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFE8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* ASM=-1, SXM=0: logical shift gives 0x7FFFFF8000, so
 * A = 0xFFFFFF0000 + 0x7FFFFF8000 = 0x7FFFFE8000 (raw under
 * OVM=0) with positive 32-bit overflow, so OVdst is set and
 * C=1 from the bit-32 carry. */
static void test_negative_shift_sxm_logical(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFF0000);
    input.status_sxm = UINT8_C(0);
    input.status_asm = UINT8_C(31);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x7FFFFE8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=1 positive overflow pins at 0x007FFFFFFF and sets OVdst;
 * OVM=0 keeps the raw wrap 0x00FFFFFFFE with OVdst still set. */
static void test_ovm_positive_saturation(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(0);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00FFFFFFFE));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=1 negative overflow: 0xFF80000000 + 0xFF80000000 pins at
 * 0xFF80000000 with C=1 and OVdst set. */
static void test_ovm_negative_saturation(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* S=1, D=1 (word 0xF780), ASM=2: B = B + (B << 2) =
 * 0x20000 + 0x80000 = 0xA0000 into B, C=0. */
static void test_both_b_accumulators(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF780),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.status_asm = UINT8_C(2);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00000A0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* SXM flips the saturation direction: A = 0xFF80000000 plus
 * (A >> 1). SXM=1 (arithmetic) overflows negative and pins at
 * 0xFF80000000; SXM=0 (logical) overflows positive and pins at
 * 0x007FFFFFFF. */
static void test_sxm_flips_saturation_direction(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    input.status_asm = UINT8_C(31);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    input.status_sxm = UINT8_C(0);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* S=0, D=1 (word 0xF580), ASM=3: B = 0x1000 + (0x100 << 3) =
 * 0x1800 into B, C=0. */
static void test_cross_destination(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF580),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x100);
    input.accumulator_b_raw40 = UINT64_C(0x1000);
    input.status_asm = UINT8_C(3);
    next = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000001800));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Unknown source/destination accumulator or SXM/OVM/ASM stops;
 * out-of-range raw40, SXM, OVM, and ASM reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction same = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_decoded_instruction cross = make_instruction(
        UINT16_C(0xF680),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&cross, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&cross, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_asm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_sxm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_asm = UINT8_C(32);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&same, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=1, D=0]: A = A + (B << 1) = 0x0012380000. */
static void test_trusted_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF680),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    input.status_asm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0012380000));
}

/* Malformed trusted operands (bad count, source out of range, bad
 * destination width) reject as INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0xF680),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Wrong layout, word count, or fixed opcode byte (SUB/LD family
 * words) rejects as INVALID_DECODE; nulls reject. */
static void test_layout_rejects(void)
{
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = make_instruction(
        UINT16_C(0xF480),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_decoded_instruction sub_word = make_instruction(
        UINT16_C(0xF481),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_decoded_instruction ld_word = make_instruction(
        UINT16_C(0xF482),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_trusted_operand_values operands = {0};

    two_words.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&wrong_layout, &input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&sub_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&ld_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&two_words, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_src_asm(&two_words, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &two_words, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &two_words, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_src_asm(
                           &two_words, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_asm_zero_same_accumulator();
    test_positive_shift_cross_accumulator();
    test_negative_shift_sxm_arithmetic();
    test_negative_shift_sxm_logical();
    test_ovm_positive_saturation();
    test_ovm_negative_saturation();
    test_both_b_accumulators();
    test_sxm_flips_saturation_direction();
    test_cross_destination();
    test_checked_input_gates();
    test_trusted_completion();
    test_trusted_shape_rejects();
    test_layout_rejects();
    return 0;
}
