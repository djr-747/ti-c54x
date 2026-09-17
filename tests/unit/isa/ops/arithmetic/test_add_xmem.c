/*
 * Module contract
 *
 * Owns: RED proofs for the ADD Xmem syntax-5 leaf: layout plus
 * 0x9000-prefix validation, X-only dual-indirect staging with
 * Y self-alias, SXM-controlled widening with unsigned 4-bit
 * left shifts (SHFT=0 through SHFT=15), low-32 carry policy
 * with TC preserved, OVM saturation with sticky OVsrc, input
 * gates, and transactional rejection. Vectors follow the TI
 * formula (src += SXM-widened Xmem << SHFT), independently
 * recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_add_xmem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
add_word(uint8_t select, uint8_t xmod, uint8_t xar_field, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x9000) |
                      (uint16_t)(((uint16_t)(select & UINT8_C(1))) << 8U) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(shift & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction make_instruction(uint16_t first_word,
    ti_c54x_operand_layout_id layout)
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

    input.accumulator_a_raw40 = UINT64_C(0x0000010000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000000005);
    input.accumulator_b_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect stage_effect(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(instruction, input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    return staged;
}

static ti_c54x_op_effect finish_read(
    const ti_c54x_op_effect *staged,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t xmem)
{
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(staged->payload.memory.complete(
                           instruction, input, xmem,
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return next;
}

/* SHFT=0: 0x10000 + 3 = 0x10003, no low-32 carry so C=0, TC
 * preserved, OVsrc kept. */
static void test_shift0_basic(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x0003));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000010003));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* SHFT=4 (xmod=1, xar field 2 -> AR4): 0x10000 + (0x10 << 4) =
 * 0x10100 with C=0. */
static void test_shift4(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(1), UINT8_C(2), UINT8_C(4)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x0010));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000010100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Max SHFT=15: 0x10000 + (1 << 15) = 0x18000 with C=0. */
static void test_max_shift15(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(15)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x0001));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000018000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* SXM=1 sign-extends 0x8000 to 0xFFFFFF8000: 0x10000 + (-0x8000)
 * = 0x8000 with a low-32 carry so C=1. SXM=0 zero-extends:
 * 0x10000 + 0x8000 = 0x18000 with C=0. */
static void test_sxm_sign_vs_zero_extend(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_sxm = UINT8_C(1);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000008000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    input.status_sxm = UINT8_C(0);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000018000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* S=1 accumulates into B: 5 + (2 << 1) = 9, dst=B, C=0. */
static void test_accumulator_b(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(1), UINT8_C(0), UINT8_C(0), UINT8_C(1)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x0002));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(9));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* OVM=1 positive overflow: 0x7FFFFF00 + 0x100 pins at
 * 0x007FFFFFFF with OVsrc set and C=0. OVM=1 negative
 * overflow: 0xFF80000000 + (SXM-widened 0xFFFF) pins at
 * 0xFF80000000 with OVsrc set and C=1. */
static void test_ovm_saturation(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFF00);
    input.status_ovm = UINT8_C(1);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_sxm = UINT8_C(1);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=0: the same positive overflow wraps to 0x0080000000 raw
 * while OVsrc still latches. */
static void test_ovm_wrap_sticky(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFF00);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Carry policy: 0xFFFFFFFFFF + 1 wraps to 0 with C=1 and no
 * overflow; the no-carry case writes C=0. */
static void test_carry_policy(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFFFFFF);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    input.accumulator_a_raw40 = UINT64_C(0x0000010000);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0003));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Staging shape: X-only dual-indirect WORD16 read, Y
 * self-aliased, no T load, completion present. */
static void test_staging_shape(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(2), UINT8_C(3), UINT8_C(7)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(
        staged.payload.memory.address_spec.dual.reserved_word0 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(
        staged.payload.memory.address_spec.dual.reserved_word1 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(
        staged.payload.memory.address_spec.dual.reserved_word2 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(
        staged.payload.memory.address_spec.dual.reserved_word3 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(staged.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(staged.payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
}

/* A SUB word (0x9300), an LD word (0x9400), a wrong layout, or a
 * two-word bundle rejects as INVALID_DECODE. */
static void test_prefix_layout_rejects(void)
{
    ti_c54x_decoded_instruction sub_word = make_instruction(
        UINT16_C(0x9300),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_decoded_instruction ld_word = make_instruction(
        UINT16_C(0x9400),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    two_words.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&sub_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&ld_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

/* Unknown source accumulator, SXM, or OVM stops; out-of-range
 * raw40 and SXM/OVM reject. Null pointers reject, including at
 * completion. */
static void test_input_gates_and_null(void)
{
    ti_c54x_decoded_instruction select_a = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_decoded_instruction select_b = make_instruction(
        add_word(UINT8_C(1), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_sxm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_xmem(&select_a, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    staged = stage_effect(&select_a, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0), &next) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Trusted [S=0, Xmod=0, Xar=0, SHFT=0]: 0x10000 + 3 = 0x10003,
 * C=0. Malformed trusted operands (bad count, bad width,
 * out-of-range shift) reject as INVALID_OPERAND. */
static void test_trusted_completion_and_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        add_word(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0)),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[3].raw_value = UINT64_C(0);
    operands.values[3].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0003));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000010003));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    operands.count = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(4);
    operands.values[3].raw_value = UINT64_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[3].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(&instruction, NULL, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(&instruction, &operands,
                                                  NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_xmem(&instruction, &operands,
                                                  &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_shift0_basic();
    test_shift4();
    test_max_shift15();
    test_sxm_sign_vs_zero_extend();
    test_accumulator_b();
    test_ovm_saturation();
    test_ovm_wrap_sticky();
    test_carry_policy();
    test_staging_shape();
    test_prefix_layout_rejects();
    test_input_gates_and_null();
    test_trusted_completion_and_rejects();
    return 0;
}
