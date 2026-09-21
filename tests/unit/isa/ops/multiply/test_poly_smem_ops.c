/*
 * Module contract
 *
 * Owns: RED proofs for the POLY Smem leaf, including checked/trusted
 * decode validation, direct versus standard-indirect memory-read
 * staging with no T load, exotic-mode deferral, the read-value
 * pair completion (A rounds A-high times live T plus B-old, B
 * loads the SXM-widened Smem word shifted left 16), unknown
 * A/B/T/SXM/FRCT/OVM stops, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, T/accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/multiply/op_poly_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode_word(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_POLY_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_decoded_instruction raw_decode_word(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    /* TI SPRU172C POLY example: A=00 1234 0000, B=00 0001 0000,
     * T=0x5678, SXM=0, FRCT=0, OVM=0. */
    input.accumulator_a_raw40 = UINT64_C(0x0012340000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x00010000);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x5678);
    input.t_register_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sxm_known = true;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static void test_checked_direct_stages_read(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3620));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* 0x3620: I=0, addr=0x20: direct page-0 read with no T load. */
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_checked_indirect_mod2_stages_read(void)
{
    /* addr7 0x13: mode4=2 (MOD2 post-increment), arf=3 (*AR3+). */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3693));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_checked_exotic_mode_defers(void)
{
    /* addr7 0x20: mode4=4 sits above MOD3. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x36A0));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3620));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_input_snapshot unknown;
    ti_c54x_op_effect effect = {0};

    unknown = input;
    unknown.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &unknown,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    unknown = input;
    unknown.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &unknown,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    unknown = input;
    unknown.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &unknown,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    unknown = input;
    unknown.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &unknown,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    unknown = input;
    unknown.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &unknown,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_checked_unknown_t_stops(void)
{
    /* Unlike the MACA family, POLY reads live T: the Smem read
     * never loads T, so unknown T rejects at stage time. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3620));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3693));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    operands.count = UINT8_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[1].raw_value = UINT64_C(0x13);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_poly_smem(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_poly_smem(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_completion_ti_example_pair(void)
{
    /* TI SPRU172C POLY example: A=00 1234 0000, B=00 0001 0000,
     * T=0x5678, Smem=0x2000, FRCT=0, SXM=0: A rounds
     * 0x1234*0x5678+0x10000 = 0x06270BC0 to 00 0627 0000 and B
     * loads 00 2000 0000. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3693));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x2000),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind ==
                       TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first.value_raw40 ==
                       UINT64_C(0x6270000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write_pair.first.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second.value_raw40 ==
                       UINT64_C(0x20000000));
}

static void test_completion_sxm_sign_extends_b(void)
{
    /* A negative Smem word with SXM=1 sets the B guard bits;
     * with SXM=0 the load zero-fills. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3693));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8001),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second.value_raw40 ==
                       UINT64_C(0xFF80010000));

    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8001),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second.value_raw40 ==
                       UINT64_C(0x80010000));
}

static void test_crossed_row_rejects(void)
{
    /* A MACA Smem word through the POLY leaf is a decode mismatch. */
    ti_c54x_decoded_instruction maca = raw_decode_word(UINT16_C(0x3534));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(maca.id == TI_C54X_OPCODE_ID_MACA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&maca, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3620));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_poly_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_checked_direct_stages_read();
    test_checked_indirect_mod2_stages_read();
    test_checked_exotic_mode_defers();
    test_checked_unknown_inputs_stop();
    test_checked_unknown_t_stops();
    test_trusted_parity();
    test_completion_ti_example_pair();
    test_completion_sxm_sign_extends_b();
    test_crossed_row_rejects();
    test_null_rejects();
    return 0;
}
