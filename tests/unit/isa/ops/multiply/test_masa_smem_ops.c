/*
 * Module contract
 *
 * Owns: RED proofs for the MASA Smem leaf, including checked/trusted
 * decode validation, direct versus standard-indirect memory-read
 * staging with read-phase T load, exotic-mode deferral, the
 * read-value SUBTRACT completion of B minus Smem times A-high
 * through the multiply core, unknown A/B/FRCT/OVM stops with
 * T-unknown tolerance, and transactional rejection.
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
#include "../../../../../src/isa/ops/multiply/op_masa_smem.h"
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
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MASA_SMEM);
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

    /* A-high 0x1234 is the multiplicand; B holds the subtract base. */
    input.accumulator_a_raw40 = UINT64_C(0x0012340000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x00020000);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0400);
    input.t_register_known = false;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static void test_checked_direct_stages_read(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* 0x3334: I=0, addr=0x34: direct page-0 read. */
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_checked_indirect_mod2_stages_read(void)
{
    /* addr7 0x15: mode4=2 (MOD2 post-increment), arf=5 (*AR5+). */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3395));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_checked_exotic_mode_defers(void)
{
    /* addr7 0x2D: mode4=5 (AR0-indexed), deferred like MACA indirect. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x33AD));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_checked_unknown_t_tolerated(void)
{
    /* T arrives via the read-phase load: no prior knownness needed. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
}

static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0x34);
    operands.values[1].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_masa_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
}

static void test_completion_example1_subtracts(void)
{
    /* TI SPRU179C MASA Example 1 shape: B=00 0002 0000 minus
     * 0x1234 * 0x5678 stores FF F9DB FFA0 into B. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x5678),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFF9DBFFA0));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void test_trusted_example1_completion(void)
{
    /* The trusted entry stages the same Example 1 subtract. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0x34);
    operands.values[1].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_masa_smem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x5678),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFF9DBFFA0));
}

static void test_crossed_row_rejects(void)
{
    /* A MACA Smem word through the MASA leaf is a decode mismatch. */
    ti_c54x_decoded_instruction maca = raw_decode_word(UINT16_C(0x3534));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(maca.id == TI_C54X_OPCODE_ID_MACA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&maca, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3334));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_checked_direct_stages_read();
    test_checked_indirect_mod2_stages_read();
    test_checked_exotic_mode_defers();
    test_checked_unknown_inputs_stop();
    test_checked_unknown_t_tolerated();
    test_trusted_parity();
    test_completion_example1_subtracts();
    test_trusted_example1_completion();
    test_crossed_row_rejects();
    test_null_rejects();
    return 0;
}
