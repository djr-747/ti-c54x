/*
 * Module contract
 *
 * Owns: RED proofs for the SUBC Smem leaf: UNKNOWN-id plus first-word
 * mask validation, direct/indirect staging, exotic-mode deferral, the
 * t > 0 / t <= 0 divide-step edges with C written from the test, SXM
 * gating with the SXM==1/MSB-set reject, no OVM saturation with
 * sticky OVdst on signed-32 misfit, input gates, checked/trusted
 * parity, and transactional rejection. Vectors follow the TI formula
 * t = sign_extend40(src) - (Smem << 15): t > 0 stores wrap((t<<1)+1)
 * with C=1, else stores wrap(src<<1) with C=0.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/C/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_subc_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*subc_smem_leaf)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_SUBC_SMEM;
    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0000000004);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000000005);
    input.accumulator_b_known = true;
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect stage_effect(subc_smem_leaf leaf,
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

/* TI worked example 1 (direct dma 0x05): t = 4 - (1 << 15) < 0, so A
 * stores wrap(4 << 1) = 8 with C cleared, OVdst preserved (8 fits
 * signed-32), TC preserved. S=0 writes A. */
static void test_ti_example1_direct(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x05));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000000008));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* C-set vector (indirect MOD0, arf=1): t = 5 - (0 << 15) = 5 > 0, so
 * B stores (5 << 1) + 1 = 11 with C set and OVdst preserved. S=1
 * selects B; no AR update is staged by the handler (single read
 * effect only). */
static void test_c_set_indirect(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1F81), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD0);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf == UINT8_C(1));
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x000000000B));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Positive-branch overflow: src = 2^30, Smem = 0 gives
 * t = 2^30 > 0, stored = 2^31 + 1 = 0x0080000001. Bits 39:31 are
 * neither all clear nor all set, so OVdst is sticky-set even though
 * OVM is clear (SUBC never saturates). C is set. */
static void test_positive_branch_sets_ov(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x0040000000);
    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Non-positive branch with a negative source: src = -2^32
 * (0xFF00000000), Smem = 0 gives t < 0, stored = wrap(src << 1) =
 * 0xFE00000000 with C cleared. The stored value misfits signed-32,
 * so OVdst is sticky-set with no saturation. */
static void test_nonpositive_branch_sets_ov(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF00000000);
    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFE00000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* SXM==0 zero-extends: 0 - (0x8000 << 15) < 0, so A stores 0 with C
 * cleared. A sign-extended Smem would have given a different t. */
static void test_sxm_zero_extends(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* SXM==1 with the Smem MSB set rejects with INVALID_OPERAND and
 * leaves the completion output unchanged. */
static void test_sxm_msb_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect next = before;

    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8000),
                           &next) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof(next)) == 0);
}

/* SXM==1 with the Smem MSB clear is accepted: t = 4 - (1 << 15) < 0,
 * so A stores 8 with C cleared, exactly like the SXM==0 step. */
static void test_sxm_one_msb_clear_ok(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_sxm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000000008));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Exotic mode defers (mode4=5): INVALID_OPERAND, effect unchanged. */
static void test_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1EAD), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown selected accumulator or unknown SXM stops; out-of-range
 * raw40 and SXM reject. C and OVM are not consumed, so unknown C or
 * OVM still stages. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction select_b = make_instruction(
        UINT16_C(0x1F05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&select_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_sxm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_c_known = false;
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
}

/* Checked/trusted parity: trusted [S=0, I=0, 0x05] matches the
 * checked 0x1E05 step value, destination, and status deltas. */
static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect checked_staged = {0};
    ti_c54x_op_effect trusted_staged = {0};
    ti_c54x_op_effect checked_next = {0};
    ti_c54x_op_effect trusted_next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x05);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(
                           &instruction, &operands, &input,
                           &trusted_staged) == TI_C54X_OP_STATUS_OK);
    trusted_next = finish_read(&trusted_staged, &instruction, &input,
                               UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input,
                                           &checked_staged) ==
                       TI_C54X_OP_STATUS_OK);
    checked_next = finish_read(&checked_staged, &instruction, &input,
                               UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(trusted_next.payload.accumulator_write.destination ==
                       checked_next.payload.accumulator_write.destination);
    TI_C54X_TEST_CHECK(trusted_next.payload.accumulator_write.value_raw40 ==
                       checked_next.payload.accumulator_write.value_raw40);
    TI_C54X_TEST_CHECK(
        trusted_next.payload.accumulator_write.status_delta.c_action ==
        checked_next.payload.accumulator_write.status_delta.c_action);
    TI_C54X_TEST_CHECK(
        trusted_next.payload.accumulator_write.status_delta.tc_action ==
        checked_next.payload.accumulator_write.status_delta.tc_action);
    TI_C54X_TEST_CHECK(
        trusted_next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        checked_next.payload.accumulator_write.status_delta
            .destination_overflow_action);
    TI_C54X_TEST_CHECK(checked_next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000000008));
}

/* Bad id, wrong layout, wrong word count, and a first word outside
 * the 0x1E00 pattern reject as INVALID_DECODE with the effect
 * unchanged. */
static void test_decode_rejects_transactional(void)
{
    ti_c54x_decoded_instruction bad_id = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0x3C34),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction bad_word = make_instruction(
        UINT16_C(0x0005), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction two_words = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    bad_id.id = TI_C54X_OPCODE_ID_COUNT;
    two_words.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&bad_id, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&bad_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Malformed trusted operands reject. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction plain = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x1E05), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_subc_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(&instruction, NULL, &input,
                                                   &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(&instruction, &operands,
                                                   NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_subc_smem(&instruction, &operands,
                                                   &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    staged = stage_effect(ti_c54x_op_subc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0), &next) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_ti_example1_direct();
    test_c_set_indirect();
    test_positive_branch_sets_ov();
    test_nonpositive_branch_sets_ov();
    test_sxm_zero_extends();
    test_sxm_msb_rejects();
    test_sxm_one_msb_clear_ok();
    test_exotic_mode_defers();
    test_checked_input_gates();
    test_trusted_parity();
    test_decode_rejects_transactional();
    test_trusted_shape_rejects();
    test_null_rejects();
    return 0;
}
