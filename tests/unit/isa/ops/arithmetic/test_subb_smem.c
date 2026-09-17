/*
 * Module contract
 *
 * Owns: RED proofs for the SUBB Smem leaf: layout validation, direct/
 * indirect staging, exotic-mode deferral, borrow/no-borrow C edges with
 * the full-width no-borrow rule, zero-extended Smem (SXM not consumed),
 * OVM saturation with sticky OVdst, input gates, and transactional
 * rejection. Vectors follow the TI formula dst = src - Smem - !C,
 * independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/C/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_subb_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*subb_smem_leaf)(
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

    input.accumulator_a_raw40 = UINT64_C(0x0000000006);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0xFF80000006);
    input.accumulator_b_known = true;
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect stage_effect(subb_smem_leaf leaf,
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

/* TI Example 1 (direct): 6 - 6 - !0 = -1 = 0xFFFFFFFFFF, C=0, OVdst kept,
 * TC preserved. S=0 writes A. */
static void test_ti_example1_direct(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x05));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0006));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* TI Example 2 (indirect MOD0, arf=1): 0xFF80000006 - 6 - 0 =
 * 0xFF80000000, C=1. The signed-40 difference lands exactly on
 * INT32_MIN, so there is no overflow and OVdst is preserved. S=1
 * selects B; no AR update is staged by the handler (single read
 * effect only). */
static void test_ti_example2_indirect(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0181), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_ovm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD0);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf == UINT8_C(1));
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0006));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Borrow edges: exact-zero differences set C; any borrow clears it.
 * 0x100 - 0x100 - 0 = 0 (C=1); 0x100 - 0x100 - 1 = 0xFFFFFFFFFF (C=0);
 * 0 - 0xFFFF - 0 = 0xFFFFFF0001 (C=0); 0 - 0 - 1 = 0xFFFFFFFFFF (C=0). */
static void test_borrow_edges(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x100);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_c = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Full-width no-borrow rule: guard bits count, so 0x0100000000 - 1 - 0 =
 * 0x00FFFFFFFF with C=1 even though the low 32 bits borrow. */
static void test_borrow_uses_full_width(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x0100000000);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00FFFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* OVM negative: 0xFF80000000 - 1 - 0 underflows the signed range and pins
 * at 0xFF80000000. OVM positive: 0x0100000000 - 0 - 1 exceeds the signed
 * range and pins at 0x007FFFFFFF. Both set OVdst; OVM=0 leaves the raw
 * wrap (0xFF7FFFFFFF) while still setting OVdst. */
static void test_ovm_saturates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0x0100000000);
    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_c = UINT8_C(1);
    input.status_ovm = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF7FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* SXM is not consumed: unknown SXM still stages, and SXM=1 does not sign
 * extend Smem (0 - 0x8000 - 0 = 0xFFFFFF8000, not 0x0000008000). */
static void test_sxm_not_consumed(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_sxm_known = false;
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF8000));
    input.status_sxm_known = true;
    input.status_sxm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF8000));
}

/* Exotic mode defers (mode4=5): INVALID_OPERAND, effect unchanged. */
static void test_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x00AD), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown accumulator/C/OVM stop; out-of-range raw40 and status bits
 * reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction select_b = make_instruction(
        UINT16_C(0x0105), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&select_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_c_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_c = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=0, I=0, 0x05]: 6 - 6 - 1 = 0xFFFFFFFFFF with C=0. */
static void test_trusted_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.status_c = UINT8_C(0);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x05);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0006));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Wrong layout, wrong word count, and malformed trusted operands reject. */
static void test_layout_and_trusted_shape_reject(void)
{
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0x3C34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction plain = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = plain;
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    two_words.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_subb_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(&instruction, NULL, &input,
                                                   &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(&instruction, &operands,
                                                   NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subb_smem(&instruction, &operands,
                                                   &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    staged = stage_effect(ti_c54x_op_subb_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0), &next) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_ti_example1_direct();
    test_ti_example2_indirect();
    test_borrow_edges();
    test_borrow_uses_full_width();
    test_ovm_saturates();
    test_sxm_not_consumed();
    test_exotic_mode_defers();
    test_checked_input_gates();
    test_trusted_completion();
    test_layout_and_trusted_shape_reject();
    test_null_rejects();
    return 0;
}
