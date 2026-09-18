/*
 * Module contract
 *
 * Owns: RED proofs for the ADDC Smem leaf: UNKNOWN-id plus
 * 0x0600/0xFE00 first-word gating through the shared
 * SELECT_INDIRECT_ADDR7 layout, direct/indirect memory-read
 * staging, exotic-mode deferral, C-in addend edges, SXM-suppressed
 * (zero-extended) Smem widening, bit-32 carry-out, OVM saturation
 * with sticky OVdst, input gates, checked/trusted parity, and
 * transactional rejection. Vectors follow the TI formula
 * dst(src) = src + Smem + C (SPRU172C section ADDC, p. 4-8),
 * independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/C/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_addc_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*addc_smem_leaf)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_ADDC_SMEM;
    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x13);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x10);
    input.accumulator_b_known = true;
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect stage_effect(addc_smem_leaf leaf,
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

/* TI shape, direct dma 0x04 holding 4: 0x13 + 4 + 1 = 0x18 into A
 * with C written 0, TC preserved, and OVdst preserved. */
static void test_direct_carry_in_set(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x04));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x18));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* C=0 variant: 0x13 + 4 + 0 = 0x17. */
static void test_direct_carry_in_clear(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x17));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* S=1 selects B: 0x10 + 4 + 1 = 0x15 into B. */
static void test_selects_b(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0704), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x15));
}

/* Sign extension is suppressed regardless of SXM: with SXM=1,
 * 0 + 0x8000 + 0 = 0x008000, not 0xFFFFFF8000. Unknown SXM still
 * stages and SXM=0 agrees. */
static void test_sxm_suppressed(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x008000));
    input.status_sxm_known = false;
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x008000));
    input.status_sxm_known = true;
    input.status_sxm = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x008000));
}

/* Carry-out follows the ADD rule on the full addition:
 * 0x00FFFFFFFF + 1 + 0 = 0x0100000000 with C=1. The carry bit
 * alone also produces the carry: 0x00FFFFFFFF + 0 + 1. */
static void test_carry_out(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x00FFFFFFFF);
    input.status_c = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0100000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
    input.status_c = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0100000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* OVM saturation mirrors the ADD leaf. Positive:
 * 0x007FFFFFFF + 1 + 0 pins at 0x007FFFFFFF. Negative:
 * 0xFF00000000 + 0 + 0 pins at 0xFF80000000. Both set OVdst and
 * write C=0. OVM=0 leaves the raw wrap 0x0080000000 while still
 * setting OVdst. */
static void test_ovm_saturates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.status_c = UINT8_C(0);
    input.status_ovm = UINT8_C(1);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0xFF00000000);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.status_ovm = UINT8_C(0);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Indirect MOD2 arf=5 (0x0695) stages the single read with no T
 * traffic and completes 0x13 + 4 + 1 = 0x18. */
static void test_indirect_mod2_stages_read(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0695), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf == UINT8_C(5));
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    next = finish_read(&staged, &instruction, &input, UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x18));
}

/* Exotic mode defers (mode4=5, 0x06AD): INVALID_OPERAND with the
 * caller-owned effect unchanged. */
static void test_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x06AD), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown accumulator/C/OVM stop transactionally; out-of-range
 * raw40 and status bits reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction select_b = make_instruction(
        UINT16_C(0x0704), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = known_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&select_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = known_input();
    input.status_c_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_c = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Trusted [S=0, I=0, 0x04] matches the checked entry bit for bit:
 * 0x13 + 4 + 1 = 0x18 with C written 0. */
static void test_trusted_checked_parity(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
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
    operands.values[2].raw_value = UINT64_C(0x04);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input,
                                           &checked_staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(
                           &instruction, &operands, &input,
                           &trusted_staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted_staged, &checked_staged,
                              sizeof(checked_staged)) == 0);
    checked_next = finish_read(&checked_staged, &instruction, &input,
                               UINT16_C(0x0004));
    trusted_next = finish_read(&trusted_staged, &instruction, &input,
                               UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(memcmp(&trusted_next, &checked_next,
                              sizeof(checked_next)) == 0);
    TI_C54X_TEST_CHECK(trusted_next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(trusted_next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x18));
    TI_C54X_TEST_CHECK(
        trusted_next.payload.accumulator_write.status_delta.c_action ==
        TI_C54X_OP_STATUS_BIT_WRITE_0);
}

/* Transactional decode rejection: bad id, bad layout, bad word
 * count, and bad first word each return INVALID_DECODE with the
 * caller-owned effect unchanged; malformed trusted operands
 * return INVALID_OPERAND. */
static void test_decode_rejects(void)
{
    ti_c54x_decoded_instruction plain = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction bad_id = plain;
    ti_c54x_decoded_instruction bad_layout = make_instruction(
        UINT16_C(0x0604),
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    ti_c54x_decoded_instruction bad_count = plain;
    ti_c54x_decoded_instruction bad_word = make_instruction(
        UINT16_C(0x0004), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    bad_id.id = TI_C54X_OPCODE_ID_ADD_SMEM;
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&bad_id, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&bad_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    bad_count.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&bad_count, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&bad_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(
                           &plain, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(
        UINT16_C(0x0604), TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_addc_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(&instruction, NULL, &input,
                                                   &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(&instruction, &operands,
                                                   NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addc_smem(&instruction, &operands,
                                                   &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    staged = stage_effect(ti_c54x_op_addc_smem, &instruction, &input);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0), &next) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_direct_carry_in_set();
    test_direct_carry_in_clear();
    test_selects_b();
    test_sxm_suppressed();
    test_carry_out();
    test_ovm_saturates();
    test_indirect_mod2_stages_read();
    test_exotic_mode_defers();
    test_checked_input_gates();
    test_trusted_checked_parity();
    test_decode_rejects();
    test_null_rejects();
    return 0;
}
