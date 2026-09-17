/*
 * Module contract
 *
 * Owns: RED proofs for the SUB Smem leaves (syntaxes 1-3): layout
 * validation, direct/indirect staging, exotic-mode deferral, SXM,
 * T-register and fixed-16 shifts, per-syntax C rules, OVM saturation
 * with sticky OVdst, input gates, and transactional rejection. Vectors
 * follow the TI formulas (src minus shifted Smem), independently
 * recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/T/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaves.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_sub_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*sub_smem_leaf)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

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
    input.t_register_raw16 = UINT16_C(0x0400);
    input.t_register_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect stage_effect(sub_smem_leaf leaf,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(leaf(instruction, input, &staged) ==
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

/* S=0 direct: 0x0012340000 - 0x5678 = 0x001233A988, C=1, OVdst kept.
 * S=1 selects B: 0x00020000 - 0x5678 = 0x0001A988. */
static void test_syntax1_direct_subtracts(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction select_b = make_instruction(
        UINT16_C(0x0134), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged =
        stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x5678));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x001233A988));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    staged = stage_effect(ti_c54x_op_sub_smem, &select_b, &input);
    next = finish_read(&staged, &select_b, &input, UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0001A988));
}

/* SXM=1 sign-extends: 0 - 0xFFFFFF8000 = 0x8000; SXM=0 zero-extends:
 * 0 - 0x8000 = 0xFFFFFF8000 (mod 2^40). */
static void test_syntax1_sxm_extension(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    staged = stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000008000));
    input.status_sxm = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF8000));
}

/* Borrow: 0x100 - 0x200 = 0xFFFFFFFF00 with C=0. */
static void test_syntax1_borrow_clears_c(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x100);
    staged = stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFF00));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* OVM negative: 0xFF80000000 - 1 pins at 0xFF80000000. OVM positive:
 * 0x007FFFFFFF - (-1) pins at 0x007FFFFFFF. Both set OVdst. */
static void test_syntax1_ovm_saturates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    staged = stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Indirect MOD2 (mode4=2, arf=5) stages the read; A - 1 = 0x001233FFFF. */
static void test_syntax1_indirect_mod2_stages_read(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0095), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
    next = finish_read(&effect, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x001233FFFF));
}

/* Exotic mode defers (mode4=5): INVALID_OPERAND, effect unchanged. */
static void test_syntax1_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x00AD), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* TS left shift T=4: 0x100 - (0x3 << 4) = 0xD0. TS right band T=0x33:
 * -32768 >> 13 = -4, so 0 - (-4) = 4. */
static void test_syntax2_ts_shifts(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0434), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x100);
    input.t_register_raw16 = UINT16_C(0x0004);
    staged = stage_effect(ti_c54x_op_sub_smem_ts, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0003));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xD0));
    input.accumulator_a_raw40 = UINT64_C(0);
    input.t_register_raw16 = UINT16_C(0x0033);
    staged = stage_effect(ti_c54x_op_sub_smem_ts, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x4));
}

/* Unknown T stops; out-of-band TS (0x20) rejects. */
static void test_syntax2_t_gates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0434), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_ts(&instruction, &input,
                                              &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.t_register_raw16 = UINT16_C(0x0020);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_ts(&instruction, &input,
                                              &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* S=0/D=0: 0x30000 - (0x2 << 16) = 0x10000, C set on no-borrow.
 * Borrow: 0x100 - (0x2 << 16) = 0xFFFFFE0100 with C preserved. */
static void test_syntax3_c_rules(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x3C34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x30000);
    staged = stage_effect(ti_c54x_op_sub_smem_sh16, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0002));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x10000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0x100);
    staged = stage_effect(ti_c54x_op_sub_smem_sh16, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0002));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFE0100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

/* S=1/D=1: B = 0x12360000 - (0x1234 << 16) = 0x20000. */
static void test_syntax3_cross_accumulators(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x3F34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_b_raw40 = UINT64_C(0x12360000);
    staged = stage_effect(ti_c54x_op_sub_smem_sh16, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x20000));
}

/* Unknown accumulator/SXM/OVM stop; out-of-range raw40 and SXM reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_sxm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=0, I=0, 0x34]: 0x0012340000 - 0x5678 = 0x001233A988. */
static void test_trusted_syntax1_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x34);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_smem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x001233A988));
}

/* Trusted [S=0, I=0, 0x34] T=4: 0x100 - (0x3 << 4) = 0xD0. */
static void test_trusted_syntax2_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0434), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x100);
    input.t_register_raw16 = UINT16_C(0x0004);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x34);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_smem_ts(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0003));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xD0));
}

/* Trusted [S=1, D=1, I=0, 0x34]: B = 0x12360000 - (0x1234 << 16). */
static void test_trusted_syntax3_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x3F34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_b_raw40 = UINT64_C(0x12360000);
    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(1);
    operands.values[3].raw_value = UINT64_C(0x34);
    operands.values[3].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_smem_sh16(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x20000));
}

/* 4-operand word through the unshifted leaf (and vice versa) mismatch;
 * malformed trusted operands (bad count, select out of range) reject. */
static void test_layout_and_trusted_shape_reject(void)
{
    ti_c54x_decoded_instruction sh16 = make_instruction(
        UINT16_C(0x3C34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction plain = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&sh16, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_sh16(&plain, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0034), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    staged = stage_effect(ti_c54x_op_sub_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0), &next) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_syntax1_direct_subtracts();
    test_syntax1_sxm_extension();
    test_syntax1_borrow_clears_c();
    test_syntax1_ovm_saturates();
    test_syntax1_indirect_mod2_stages_read();
    test_syntax1_exotic_mode_defers();
    test_syntax2_ts_shifts();
    test_syntax2_t_gates();
    test_syntax3_c_rules();
    test_syntax3_cross_accumulators();
    test_checked_input_gates();
    test_trusted_syntax1_completion();
    test_trusted_syntax2_completion();
    test_trusted_syntax3_completion();
    test_layout_and_trusted_shape_reject();
    test_null_rejects();
    return 0;
}
