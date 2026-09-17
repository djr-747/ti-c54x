/*
 * Module contract
 *
 * Owns: RED proofs for the SUBS Smem leaf: layout validation,
 * direct/indirect staging, exotic-mode deferral, unsigned Smem
 * handling regardless of SXM (including SXM-unknown progress), C
 * written from the full 40-bit unsigned comparison with TC
 * preserved, OVM saturation with sticky OVdst, input gates, and
 * transactional rejection. Vectors follow the TI formula
 * (dst = src40 minus unsigned smem16), independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_subs_smem.h"
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
    input.accumulator_b_raw40 = UINT64_C(0x0000000002);
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

    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(instruction, input, &staged) ==
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

/* TI Example (MOD0 *AR2, S=1, word 0x0B82): B = 2 - 0xF006 = -61444,
 * i.e. 0xFFFFFF0FFC in 40 bits, with borrow so C=0. */
static void test_ti_example_indirect_mod0(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0B82), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0xF006));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf == UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF0FFC));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* S=0 direct (word 0x0A34): 0x0012340000 - 0x5678 = 0x001233A988 with
 * no borrow so C=1; TC preserved, OVdst kept. */
static void test_direct_subtracts_no_borrow(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x5678));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x34));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x001233A988));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Exact equality is no borrow: 0x100 - 0x100 = 0 with C=1. */
static void test_exact_equality_sets_c(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x100);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* Smem is unsigned regardless of SXM: with SXM=1, 0 - 0x8000 is still
 * 0xFFFFFF8000 (a sign-extending SUB would give 0x0000008000). The
 * SXM=0 run matches bit-for-bit. */
static void test_unsigned_smem_ignores_sxm(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    input.status_sxm = UINT8_C(0);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF8000));
}

/* SXM unknown (and SXM=0/1 alike) never stops the op: SXM is not
 * consumed. */
static void test_sxm_unknown_does_not_stop(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_sxm_known = false;
    input.accumulator_a_raw40 = UINT64_C(0x100);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF));
}

/* OVM=1 negative overflow: 0xFF80000000 - 1 pins at 0xFF80000000 and
 * sets OVdst. OVM=0: raw wrap 0xFF7FFFFFFF, OVdst still set. */
static void test_ovm_saturation(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(0);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF7FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Exotic mode defers (mode4=5, word 0x0AAD): INVALID_OPERAND, effect
 * unchanged. */
static void test_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0AAD), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown source accumulator or OVM stops; out-of-range raw40 and OVM
 * reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction select_a = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction select_b = make_instruction(
        UINT16_C(0x0B34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&select_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=1, I=1, 0x02]: B = 2 - 0xF006 = 0xFFFFFF0FFC, C=0. */
static void test_trusted_completion(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0B82), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x02);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0xF006));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF0FFC));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Malformed trusted operands (bad count, select out of range, bad
 * indirect width) reject as INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0B82), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x02);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Wrong layout or word count rejects as INVALID_DECODE. */
static void test_layout_rejects(void)
{
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0x0A34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    two_words.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0A34), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_subs_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(&instruction, NULL, &input,
                                                   &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(&instruction, &operands,
                                                   NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subs_smem(&instruction, &operands,
                                                   &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    staged = stage_effect(&instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0), &next) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_ti_example_indirect_mod0();
    test_direct_subtracts_no_borrow();
    test_exact_equality_sets_c();
    test_unsigned_smem_ignores_sxm();
    test_sxm_unknown_does_not_stop();
    test_ovm_saturation();
    test_exotic_mode_defers();
    test_checked_input_gates();
    test_trusted_completion();
    test_trusted_shape_rejects();
    test_layout_rejects();
    test_null_rejects();
    return 0;
}
