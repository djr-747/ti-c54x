/*
 * Module contract
 *
 * Owns: RED proofs for the MACD Smem,Pmad leaf, including
 * checked/trusted decode validation, the DATA+PROGRAM pair staging
 * with Smem read-phase T load, pair completion of dst plus Smem
 * times the PROGRAM coefficient into the S-selected accumulator
 * without rounding, unknown dst/FRCT/OVM stops with unknown T and
 * unselected-accumulator tolerance, exotic-mode deferral, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, T/accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/multiply/op_macd_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode_pair(uint16_t first_word,
                                              uint16_t pmad)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, pmad},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MACD_SMEM);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0400);
    input.t_register_known = false;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static void test_checked_direct_stages_pair(void)
{
    /* 0x7A34: S=0, I=0, addr=0x34: direct page-0 Smem leg with the
     * verbatim pmad PROGRAM leg. */
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7A34), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.indirect ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.smem_dma7 ==
                       UINT8_C(0x34));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.program_pmad ==
                       UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.write_smem_delay == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete_dual != NULL);
}

static void test_checked_indirect_mod2_stages_pair(void)
{
    /* 0x7B95: S=1, addr7 0x15: mode4=2 (MOD2 post-increment), arf=5
     * (*AR5+) with dst B. */
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.indirect ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.smem_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.smem_arf ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.program_pmad ==
                       UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.write_smem_delay == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete_dual != NULL);
}

static void test_checked_exotic_mode_defers(void)
{
    /* addr7 0x2D: mode4=5 (AR0-indexed), deferred like MAC indirect. */
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7BAD), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* S=1 selects B: unknown B stops. */
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_checked_unselected_accumulator_tolerated(void)
{
    /* S=1 selects B: unknown A and unknown T never stop staging. */
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
}

static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x15);
    operands.values[2].width_bits = UINT8_C(7);
    operands.values[3].raw_value = UINT64_C(0x0100);
    operands.values[3].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_macd_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.smem_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.smem_arf ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.pair.program_pmad ==
                       UINT16_C(0x0100));
}

static void test_completion_accumulates_coefficient_product(void)
{
    /* Oracle shape: B=0 plus Smem 0x4000 times coefficient 0x4000
     * equals 0x10000000 with FRCT=0. */
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x4000),
                           UINT16_C(0x4000),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x10000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void test_completion_frct_doubles(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.status_frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x4000),
                           UINT16_C(0x4000),
                           &next) == TI_C54X_OP_STATUS_OK);
    /* 0x10000000 doubled by FRCT. */
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x20000000));
}

static void test_completion_selects_a(void)
{
    /* 0x7A95: S=0 selects A: A=0x1000 plus 0x1234 * 0x0002. */
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7A95), UINT16_C(0x0200));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x1000);
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x1234),
                           UINT16_C(0x0002),
                           &next) == TI_C54X_OP_STATUS_OK);
    /* 0x1234 * 0x0002 = 0x2468, plus A=0x1000. */
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x3468));
}

static void test_crossed_row_rejects(void)
{
    /* A MACA Smem word through the MACD leaf is a decode mismatch. */
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0x3534));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0x3534), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction maca;
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &maca) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(maca.id == TI_C54X_OPCODE_ID_MACA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&maca, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_pair(UINT16_C(0x7B95), UINT16_C(0x0100));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_macd_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_checked_direct_stages_pair();
    test_checked_indirect_mod2_stages_pair();
    test_checked_exotic_mode_defers();
    test_checked_unknown_inputs_stop();
    test_checked_unselected_accumulator_tolerated();
    test_trusted_parity();
    test_completion_accumulates_coefficient_product();
    test_completion_frct_doubles();
    test_completion_selects_a();
    test_crossed_row_rejects();
    test_null_rejects();
    return 0;
}
