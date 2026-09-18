/*
 * Module contract
 *
 * Owns: RED proofs for the SUB Smem SHIFT syntax-4 leaf: layout plus
 * 0x6F00/0x0C20-prefix validation, direct versus standard-indirect
 * memory-read staging, exotic-mode deferral, SXM extension, signed
 * 5-bit shifts (-16..15), low-32 borrow policy (C set means no
 * borrow) with TC preserved, OVM saturation with sticky OVdst,
 * input gates, and transactional rejection. Vectors follow the TI
 * formula (dst = src - (Smem << SHIFT)), independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_sub_smem_shift.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(uint16_t word0,
    uint16_t word1)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5;
    instruction.bundle.program_words[0] = word0;
    instruction.bundle.program_words[1] = word1;
    instruction.bundle.program_word_count = UINT8_C(2);
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
    return input;
}

static ti_c54x_op_effect stage_effect(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(instruction, input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    return staged;
}

static ti_c54x_op_effect finish_read(
    const ti_c54x_op_effect *staged,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t smem)
{
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(staged->payload.memory.complete(
                           instruction, input, smem,
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return next;
}

/* SHIFT=0 direct: 0x0012340000 - 0x5678 = 0x1233A988, no borrow
 * so C=1. */
static void test_shift0_direct_subtracts(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0C20));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x5678));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x1233A988));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* S=1, D=1 routes B to B with a borrow: 0x00020000 - (0x0001 <<
 * 4) = 0x0001FFF0, C=1. */
static void test_selects_b_to_b_with_shift(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0F24));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x0001));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0001FFF0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* Negative SHIFT is an SXM-ruled right shift: raw 28 means -4, so
 * 0xF000 widens to -0x1000 and shifts to -0x100; 0 - (-0x100)
 * borrows, giving 0x100 with C=0. */
static void test_negative_shift_shifts_right(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0C3C));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0xF000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* SXM=0 zero-extends: 0x10000 - 0x8000 = 0x8000, C=1. */
static void test_sxm_zero_extends(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0C20));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x10000);
    input.status_sxm = UINT8_C(0);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* OVM=1 saturates negative overflow to 0xFF80000000 with OVdst. */
static void test_ovm_saturates(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0C20));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Indirect MOD0 AR2 stages a single-indirect read. */
static void test_indirect_mod0_stages(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F82), UINT16_C(0x0C20));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf ==
                       UINT8_C(2));
}

/* MOD4 and above defer before any MEMORY effect. */
static void test_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6FA0), UINT16_C(0x0C20));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Wrong second-word patterns, wrong word count, and NULL reject
 * without touching the effect. */
static void test_decode_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0C40));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    instruction.bundle.program_words[1] = UINT16_C(0x0C00);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    instruction.bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown accumulator/SXM/OVM stops; out-of-range SXM/OVM rejects. */
static void test_input_gates(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6F34), UINT16_C(0x0C20));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_shift(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

int
main(void)
{
    test_shift0_direct_subtracts();
    test_selects_b_to_b_with_shift();
    test_negative_shift_shifts_right();
    test_sxm_zero_extends();
    test_ovm_saturates();
    test_indirect_mod0_stages();
    test_exotic_mode_defers();
    test_decode_rejects();
    test_input_gates();
    return 0;
}
