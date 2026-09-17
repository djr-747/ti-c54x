/*
 * Module contract
 *
 * Owns: RED proofs for the MASA T,src leaf, including checked/trusted
 * decode validation, the register-only SUBTRACT completion of src
 * minus T times A-high with R rounding, unknown T/A/src/FRCT/OVM
 * stops, and transactional rejection.
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
#include "../../../../../src/isa/ops/multiply/op_masa_t.h"
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
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MASA_T);
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

    /* A-high 0x1234 is the multiplicand; B is src and dst. */
    input.accumulator_a_raw40 = UINT64_C(0x0012340000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x00020000);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0444);
    input.t_register_known = true;
    input.status_frct = UINT8_C(1);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static void test_checked_example2_retires(void)
{
    /* TI SPRU179C MASA Example 2 shape: MASA T,B,B (0xF78A) with
     * FRCT=1 stores FF FF66 B460 into B. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0xF78A));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFF66B460));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void test_checked_rounds_masar(void)
{
    /* TI SPRU179C MASA Example 3 shape: R=1 (0xF78B) stores
     * FF FF67 0000 into B. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0xF78B));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFF670000));
}

static void test_checked_src_a_selects(void)
{
    /* 0xF48A: S=0 (src A), D=0 (dst A), R=0: A minus T times A-high
     * back into A. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0xF48A));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    /* 0x0012340000 - 0x9B4BA0. */
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x1198B460));
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0xF78A));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_trusted_example2_completion(void)
{
    /* The trusted entry stages the same Example 2 subtract. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0xF78A));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_masa_t(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFF66B460));
}

static void test_crossed_row_rejects(void)
{
    /* A MACA T word through the MASA leaf is a decode mismatch. */
    ti_c54x_decoded_instruction maca = raw_decode_word(UINT16_C(0xF788));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(maca.id == TI_C54X_OPCODE_ID_MACA_T);
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&maca, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0xF78A));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_masa_t(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_checked_example2_retires();
    test_checked_rounds_masar();
    test_checked_src_a_selects();
    test_checked_unknown_inputs_stop();
    test_trusted_example2_completion();
    test_crossed_row_rejects();
    test_null_rejects();
    return 0;
}
