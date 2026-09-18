/*
 * Module contract
 *
 * Owns: RED proofs for the ADDS Smem leaf: UNKNOWN-id/first-word/layout
 * validation, direct versus standard-indirect memory-read staging,
 * exotic-mode deferral, Smem always zero-extended regardless of SXM
 * (including SXM-unknown and C-unknown progress), the sibling ADD C
 * rule, OVM saturation with sticky OVdst, checked/trusted parity, and
 * transactional rejection. Vectors follow the TI formula
 * (dst(src) = src + zero_extend16(Smem)), independently recomputed;
 * the TI worked example is E-TI-C54X-SPRU172C-S4-ADDS-P4-10.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/status mutation, CPU sequencing,
 * timing, observers, diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_adds_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(uint16_t first_word)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_ADDS_SMEM;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0000000000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000000003);
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

    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(instruction, input, &staged) ==
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

/* TI worked example (direct dmad 0x34, S=1, word 0x0334): with SXM=1,
 * B = 0x0003 + zero_extend16(0xF006) = 0x00_0000_F009 with C cleared.
 * A sign-extending ADD would instead compute 0xFF_FFFF_F009, so this
 * vector proves sign extension is suppressed. */
static void test_ti_example_direct(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0xF006));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x34));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000F009));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Zero add (S=0 direct, word 0x0234): 0 + 0 = 0 with C cleared and
 * OVdst kept. */
static void test_zero_add(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect next =
        finish_read(&staged, &instruction, &input, UINT16_C(0x0000));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Carry out (word 0x0234): 0x00FFFFFFFF + 1 = 0x0100000000 with C set,
 * per the sibling ADD C rule. */
static void test_carry_sets_c(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x00FFFFFFFF);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0100000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* Smem is unsigned regardless of SXM: with SXM=1, 0 + 0x8000 is still
 * 0x008000 (a sign-extending ADD would give 0xFFFFFF8000). The SXM=0
 * run matches bit-for-bit. */
static void test_unsigned_smem_ignores_sxm(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x008000));
    input.status_sxm = UINT8_C(0);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x008000));
}

/* SXM unknown, SXM=0/1 alike, and C unknown never stop the op: neither
 * SXM nor the C input is consumed. */
static void test_unconsumed_inputs_do_not_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_sxm_known = false;
    input.status_c_known = false;
    input.accumulator_a_raw40 = UINT64_C(0x100);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x101));
}

/* OVM=1 positive overflow: 0x007FFFFFFF + 1 pins at 0x007FFFFFFF and
 * sets OVdst. OVM=0: raw wrap 0x0080000000, OVdst still set. */
static void test_ovm_saturation(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.status_ovm = UINT8_C(1);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(0);
    staged = stage_effect(&instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Indirect MOD2 (word 0x0295: I=1, mode4=2, arf=5) stages the read. */
static void test_indirect_mod2_stages_read(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0295));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = stage_effect(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.arf ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

/* Exotic mode defers (mode4=5, word 0x02AD): INVALID_OPERAND, effect
 * unchanged. */
static void test_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x02AD));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown source accumulator or OVM stops; out-of-range raw40 and OVM
 * reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction select_a =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_decoded_instruction select_b =
        make_instruction(UINT16_C(0x0334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&select_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&select_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=1, I=0, 0x34] matches the checked TI example bit for
 * bit: B = 0x0003 + 0xF006 = 0x0000F009 with C cleared. */
static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0334));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x34);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0xF006));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0000F009));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Malformed trusted operands (bad count, select out of range, bad
 * indirect width) reject as INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0334));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x34);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Transactional decode rejection: a non-UNKNOWN id, a wrong layout, a
 * wrong word count, and a first word outside the 0x0200 field each
 * reject as INVALID_DECODE with the caller-owned effect unchanged. */
static void test_decode_rejects(void)
{
    ti_c54x_decoded_instruction base = make_instruction(UINT16_C(0x0334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect;
    ti_c54x_decoded_instruction bad = base;

    bad.id = (ti_c54x_opcode_id)1;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&bad, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    bad = base;
    bad.operand_layout =
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&bad, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    bad = base;
    bad.bundle.program_word_count = UINT8_C(2);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&bad, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    bad = base;
    bad.bundle.program_words[0] = UINT16_C(0x0034);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&bad, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown OVM at completion time also stops transactionally. */
static void test_completion_unknown_ovm_stops(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = stage_effect(&instruction, &input);
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect next = before;

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0xF006),
                           &next) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof(next)) == 0);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0234));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_adds_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(&instruction, NULL, &input,
                                                   &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(&instruction, &operands,
                                                   NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_adds_smem(&instruction, &operands,
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
    test_ti_example_direct();
    test_zero_add();
    test_carry_sets_c();
    test_unsigned_smem_ignores_sxm();
    test_unconsumed_inputs_do_not_stop();
    test_ovm_saturation();
    test_indirect_mod2_stages_read();
    test_exotic_mode_defers();
    test_checked_input_gates();
    test_trusted_parity();
    test_trusted_shape_rejects();
    test_decode_rejects();
    test_completion_unknown_ovm_stops();
    test_null_rejects();
    return 0;
}
