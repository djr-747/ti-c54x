/*
 * Module contract
 *
 * Owns: RED proofs for the SQURA/SQURS Smem leaves, including
 * checked/trusted decode validation per row, direct versus
 * standard-indirect memory-read staging with the read-phase T
 * load, exotic-mode deferral, the read-value ADD/SUBTRACT
 * completion of Smem squared into the S-selected accumulator
 * through the multiply core, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/T/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/multiply/op_squr_acc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode_word(uint16_t first_word,
                                               ti_c54x_opcode_id id)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(uint64_t acc_a, uint64_t acc_b)
{
    ti_c54x_op_input_snapshot input = {0};

    /* T and src are written, never read; only FRCT/OVM gate staging. */
    input.accumulator_a_raw40 = acc_a;
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = acc_b;
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0003);
    input.t_register_known = false;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static void test_squra_direct_example1(void)
{
    /* 0x391E: S=1 (B), I=0, addr=0x1E. TI SQURA Example 1. */
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x391E), TI_C54X_OPCODE_ID_SQURA_SMEM);
    ti_c54x_op_input_snapshot input =
        known_input(UINT64_C(0), UINT64_C(0x0032000000));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x000F),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00320000E1));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void test_squrs_direct_example1(void)
{
    /* 0x3A09: S=0 (A), I=0, addr=0x09. TI SQURS Example 1. */
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x3A09), TI_C54X_OPCODE_ID_SQURS_SMEM);
    ti_c54x_op_input_snapshot input =
        known_input(UINT64_C(0x00014B5DB0), UINT64_C(0));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_squrs_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x1234),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x320));
}

static void test_indirect_mod0_stages_read(void)
{
    /* 0x3B82: S=1, addr7 0x02: mode4=0 (MOD0, no update), arf=2. */
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x3B82), TI_C54X_OPCODE_ID_SQURS_SMEM);
    ti_c54x_op_input_snapshot input = known_input(UINT64_C(0), UINT64_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_squrs_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_exotic_mode_defers(void)
{
    /* addr7 0x2D: mode4=5 (AR0-indexed), deferred like SQUR Smem. */
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x39AD), TI_C54X_OPCODE_ID_SQURA_SMEM);
    ti_c54x_op_input_snapshot input = known_input(UINT64_C(0), UINT64_C(0));
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void test_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x391E), TI_C54X_OPCODE_ID_SQURA_SMEM);
    ti_c54x_op_input_snapshot input = known_input(UINT64_C(0), UINT64_C(0));
    ti_c54x_op_effect effect = {0};

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input.status_frct_known = true;

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_crossed_row_rejects(void)
{
    /* A SQURS word through the SQURA leaf (and back) is a decode. */
    ti_c54x_decoded_instruction squrs =
        decode_word(UINT16_C(0x3A09), TI_C54X_OPCODE_ID_SQURS_SMEM);
    ti_c54x_decoded_instruction squra =
        decode_word(UINT16_C(0x391E), TI_C54X_OPCODE_ID_SQURA_SMEM);
    ti_c54x_op_input_snapshot input = known_input(UINT64_C(0), UINT64_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(&squrs, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_squrs_smem(&squra, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static bool staged_complete_example(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *staged)
{
    ti_c54x_op_effect next = {0};

    if (staged->payload.memory.complete == NULL) {
        return false;
    }
    if (staged->payload.memory.complete(instruction, input, UINT16_C(0x000F),
                                         &next) != TI_C54X_OP_STATUS_OK) {
        return false;
    }
    return next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE &&
           next.payload.accumulator_write.destination == UINT8_C(1) &&
           next.payload.accumulator_write.value_raw40 ==
               UINT64_C(0x00320000E1);
}

static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x391E), TI_C54X_OPCODE_ID_SQURA_SMEM);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input =
        known_input(UINT64_C(0), UINT64_C(0x0032000000));
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x1E);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_squra_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged_complete_example(&instruction, &input, &effect));
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(UINT16_C(0x391E), TI_C54X_OPCODE_ID_SQURA_SMEM);
    ti_c54x_op_input_snapshot input = known_input(UINT64_C(0), UINT64_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_squrs_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_squra_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_squra_direct_example1();
    test_squrs_direct_example1();
    test_indirect_mod0_stages_read();
    test_exotic_mode_defers();
    test_unknown_inputs_stop();
    test_crossed_row_rejects();
    test_trusted_parity();
    test_null_rejects();
    return 0;
}
