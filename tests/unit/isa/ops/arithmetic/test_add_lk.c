/*
 * Module contract
 *
 * Owns: RED proofs for the ADD #lk leaf: layout plus first-word
 * gating for syn7 (lk<<SHFT) and syn8 (lk<<16), SXM widening in
 * both directions, C written from the low-32 carry with TC
 * preserved, OVM saturation with sticky OVdst, input gates, and
 * transactional rejection. Vectors follow the TI formula
 * (dst = src40 plus SXM-widened lk shifted left), independently
 * recomputed. The syn7 TI vector uses extension word 0x4568, which
 * is the value matching the brief's expected B=0x00457A00.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator/status mutation, CPU sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_add_lk.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LAYOUT_SYN7 \
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16
#define LAYOUT_SYN8 \
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, uint16_t second_word,
    ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = second_word;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0000001200);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000001800);
    input.accumulator_b_known = true;
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect run_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(instruction, input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

/* TI vector (syn7, SXM=1): ADD #0x4568,8,A,B with A=0x1200, B=0x1800,
 * C=1 -> A unchanged (effect writes B only), B = 0x1200 + 0x456800
 * = 0x00457A00, C=0 (no low-32 carry), OVdst kept. */
static void test_ti_vector_syn7(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00457A00));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* dst==src (syn7, word 0xF001: S=0,D=0,SHFT=1): A = 0x1200 + (1<<1)
 * = 0x1202 with C=0. */
static void test_syn7_dst_eq_src(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF001), UINT16_C(0x0001), LAYOUT_SYN7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x1202));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* SXM=1 sign-extends a negative lk (word 0xF100: S=0,D=1,SHFT=0):
 * A = 0x1200 + (-1) = 0x11FF with a low-32 carry so C=1. A
 * zero-extending ADD would give 0x111FF instead. */
static void test_syn7_sxm_sign_extends(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF100), UINT16_C(0xFFFF), LAYOUT_SYN7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x11FF));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* SXM=0 zero-extends the same word: A = 0x1200 + 0xFFFF = 0x111FF
 * with no carry so C=0. */
static void test_syn7_sxm_zero_extends(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF100), UINT16_C(0xFFFF), LAYOUT_SYN7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.status_sxm = UINT8_C(0);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x111FF));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Derived syn8 vector: ADD #0x1234,16,A,B (word 0xF160, SXM=1):
 * B = 0x1200 + 0x12340000 = 0x12341200, C=0. */
static void test_syn8_basic(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF160), UINT16_C(0x1234), LAYOUT_SYN8);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x12341200));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Syn8 SXM split with A=0 and lk=0x8000: SXM=1 sign-extends then
 * shifts to addend 0xFF80000000 (true sum -134217728, in int32
 * range, so OVdst kept); SXM=0 zero-extends to addend 0x0080000000
 * (true sum +2^31, positive overflow, so OVdst set with the raw
 * value kept under OVM=0). Both write C=0. */
static void test_syn8_sxm_sign_vs_zero(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF160), UINT16_C(0x8000), LAYOUT_SYN8);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    input.status_sxm = UINT8_C(0);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM positive saturation (syn7, A=0x007FFFFFFF + 1 = 0x0080000000,
 * positive overflow): OVM=0 keeps the raw wrap with OVdst set;
 * OVM=1 pins at 0x007FFFFFFF with OVdst set. */
static void test_ovm_positive_saturation(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF100), UINT16_C(0x0001), LAYOUT_SYN7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Wrong layout, wrong word count, sibling-op bases (SUB syn7 0xF118
 * and SUB syn8 0xF161), and a bad fixed-bits word all reject as
 * INVALID_DECODE with the effect untouched. */
static void test_decode_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction wrong_layout =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568),
                         TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction one_word =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_decoded_instruction sub_syn7 =
        make_instruction(UINT16_C(0xF118), UINT16_C(0x3039), LAYOUT_SYN7);
    ti_c54x_decoded_instruction sub_syn8 =
        make_instruction(UINT16_C(0xF161), UINT16_C(0x1234), LAYOUT_SYN8);
    ti_c54x_decoded_instruction bad_base =
        make_instruction(UINT16_C(0xE108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    one_word.bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&one_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&sub_syn7, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&sub_syn8, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&bad_base, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown source/destination/SXM/OVM stops; out-of-range raw40 and
 * SXM/OVM values reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction src_a_dst_b =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_sxm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&src_a_dst_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted syn7 [S=1,D=1,SHFT=2,lk=0x0100]: B = 0x1800 + 0x400 =
 * 0x1C00, C=0. Trusted syn8 [S=0,D=0,lk=0x0002]: A = 0x1200 +
 * 0x20000 = 0x21200, C=0. */
static void test_trusted_completions(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(2);
    operands.values[2].width_bits = UINT8_C(4);
    operands.values[3].raw_value = UINT64_C(0x0100);
    operands.values[3].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x1C00));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x0002);
    operands.values[2].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x21200));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Malformed trusted operands (bad count, select out of range, shift
 * width or range, lk width) reject as INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(5);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(2);
    operands.values[2].width_bits = UINT8_C(4);
    operands.values[3].raw_value = UINT64_C(0x0100);
    operands.values[3].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[2].width_bits = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[2].raw_value = UINT64_C(16);
    operands.values[2].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[2].raw_value = UINT64_C(2);
    operands.values[3].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x0002);
    operands.values[2].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, NULL, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk(&instruction, &operands,
                                                &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Per-syntax checked entries: the shift entry serves the syn7 TI
 * vector and rejects the syn8 layout; the _16 entry serves syn8
 * and rejects the syn7 layout. */
static void test_plain_syntax_entries_dispatch(void)
{
    ti_c54x_decoded_instruction syn7 =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_decoded_instruction syn8 =
        make_instruction(UINT16_C(0xF060), UINT16_C(0x0002), LAYOUT_SYN8);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk_shift(&syn7, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00457A00));
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk_shift(&syn8, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk_16(&syn8, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x21200));
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk_16(&syn7, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk_shift(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_lk_16(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Per-syntax trusted entries gate on operand count: four values
 * for syn7, three for syn8. */
static void test_trusted_syntax_entries_gate(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF108), UINT16_C(0x4568), LAYOUT_SYN7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(2);
    operands.values[2].width_bits = UINT8_C(4);
    operands.values[3].raw_value = UINT64_C(0x0100);
    operands.values[3].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk_shift(&instruction, &operands,
                                                      &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x1C00));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk_16(&instruction, &operands,
                                                   &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x0002);
    operands.values[2].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk_16(&instruction, &operands,
                                                   &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x21200));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_lk_shift(&instruction, &operands,
                                                      &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

int
main(void)
{
    test_ti_vector_syn7();
    test_syn7_dst_eq_src();
    test_syn7_sxm_sign_extends();
    test_syn7_sxm_zero_extends();
    test_syn8_basic();
    test_syn8_sxm_sign_vs_zero();
    test_ovm_positive_saturation();
    test_decode_rejects();
    test_checked_input_gates();
    test_trusted_completions();
    test_trusted_shape_rejects();
    test_null_rejects();
    test_plain_syntax_entries_dispatch();
    test_trusted_syntax_entries_gate();
    return 0;
}
