/*
 * Module contract
 * Owns: pure SSBX/RSBX status-bit effect proofs, including set/clear value
 * ownership, N/SBIT selection, checked/trusted dispatch parity, and
 * transactional rejection of malformed decoded metadata.
 * Does not own: catalog rows, operand extraction, status-register mutation,
 * CPU lifecycle, sequencing, timing, or repeat policy.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/status/op_status_bits.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode(uint16_t word,
                                          ti_c54x_opcode_id expected_id)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == expected_id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static void check_effect(const ti_c54x_op_effect *effect,
                         ti_c54x_op_status_register target, uint8_t bit,
                         uint8_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_STATUS_BIT_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.target_register ==
                       target);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.bit_index == bit);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.raw_value == value);
}

static void check_one(uint16_t base, ti_c54x_opcode_id id,
                      ti_c54x_op_status_register target, uint8_t bit,
                      uint8_t value)
{
    const uint16_t word = (uint16_t)(base | ((target ==
                                             TI_C54X_OP_STATUS_REGISTER_ST1)
                                                ? UINT16_C(0x0200)
                                                : UINT16_C(0)) |
                                      bit);
    const ti_c54x_decoded_instruction instruction = decode(word, id);
    const ti_c54x_op_input_snapshot inputs[] = {
        {.status_c = UINT8_C(0),
         .status_c_known = false,
         .status_tc = UINT8_C(1),
         .status_tc_known = false,
         .status_sxm = UINT8_C(1),
         .status_sxm_known = false},
        {.status_c = UINT8_C(1),
         .status_c_known = true,
         .status_tc = UINT8_C(0),
         .status_tc_known = true,
         .status_sxm = UINT8_C(0),
         .status_sxm_known = true}};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(id);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_trusted_execution_packet packet;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    for (size_t i = 0U; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        ti_c54x_op_effect direct;
        ti_c54x_op_effect checked;
        ti_c54x_op_effect trusted;
        const ti_c54x_op_status direct_status =
            id == TI_C54X_OPCODE_ID_SSBX_STATUS
                ? ti_c54x_op_ssbx(&instruction, &inputs[i], &direct)
                : ti_c54x_op_rsbx(&instruction, &inputs[i], &direct);
        const ti_c54x_op_status checked_status =
            ti_c54x_dispatch_decoded_with_input_snapshot(
                &instruction, &inputs[i], &checked);
        const ti_c54x_op_status trusted_status =
            ti_c54x_dispatch_trusted(&packet, &inputs[i], &trusted);

        TI_C54X_TEST_CHECK(direct_status == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(checked_status == direct_status);
        TI_C54X_TEST_CHECK(trusted_status == direct_status);
        check_effect(&direct, target, bit, value);
        TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
    }
}

static void test_forged_and_null_inputs_are_transactional(void)
{
    const ti_c54x_decoded_instruction valid =
        decode(UINT16_C(0xF5BA), TI_C54X_OPCODE_ID_SSBX_STATUS);
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_ssbx(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ssbx(&valid, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ssbx(&valid, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    {
        ti_c54x_decoded_instruction forged = valid;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ssbx(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = valid;
        forged.bundle.program_words[0] ^= UINT16_C(0x0010);
        TI_C54X_TEST_CHECK(ti_c54x_op_ssbx(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = valid;
        forged.id = TI_C54X_OPCODE_ID_RSBX_STATUS;
        TI_C54X_TEST_CHECK(ti_c54x_op_ssbx(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    for (uint8_t bit = 0U; bit < UINT8_C(16); ++bit) {
        check_one(UINT16_C(0xF5B0), TI_C54X_OPCODE_ID_SSBX_STATUS,
                  TI_C54X_OP_STATUS_REGISTER_ST0, bit, UINT8_C(1));
        check_one(UINT16_C(0xF5B0), TI_C54X_OPCODE_ID_SSBX_STATUS,
                  TI_C54X_OP_STATUS_REGISTER_ST1, bit, UINT8_C(1));
        check_one(UINT16_C(0xF4B0), TI_C54X_OPCODE_ID_RSBX_STATUS,
                  TI_C54X_OP_STATUS_REGISTER_ST0, bit, UINT8_C(0));
        check_one(UINT16_C(0xF4B0), TI_C54X_OPCODE_ID_RSBX_STATUS,
                  TI_C54X_OP_STATUS_REGISTER_ST1, bit, UINT8_C(0));
    }
    test_forged_and_null_inputs_are_transactional();
    return 0;
}
