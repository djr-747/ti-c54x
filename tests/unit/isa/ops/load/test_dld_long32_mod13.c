/*
 * Module contract
 *
 * Owns: pure checked/trusted DLD LONG32 MOD13 semantic proofs.
 * Does not own: catalog tables, address resolution, CPU sequencing, buses,
 * or architectural state.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/load/op_dld_long32.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t dld_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x56E8) |
                      ((uint16_t)(destination & 1U) << 8U) |
                      (uint16_t)(arf & 7U));
}

static ti_c54x_decoded_instruction decode(uint8_t destination, uint8_t arf,
                                          uint16_t extension)
{
    const uint16_t word = dld_word(destination, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, extension}, .program_word_count = 2U};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_DLD_LONG32_MOD13);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

static ti_c54x_trusted_operand_values operands_for(uint8_t destination,
                                                   uint8_t arf, uint16_t lk)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = 4U;
    operands.values[0] = (ti_c54x_operand_field_value){destination, 1U};
    operands.values[1] = (ti_c54x_operand_field_value){arf, 3U};
    operands.values[2] = (ti_c54x_operand_field_value){lk, 16U};
    operands.values[3] = (ti_c54x_operand_field_value){13U, 4U};
    return operands;
}

static void assert_read(const ti_c54x_op_effect *effect, uint8_t arf,
                        uint16_t lk)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long_offset_word == lk);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete_long32 != NULL);
}

static void check_forms_and_completion(void)
{
    const ti_c54x_opcode_metadata *metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_DLD_LONG32_MOD13);
    static const uint16_t reads[][2] = {{0x0000U, 0x0000U},
                                        {0x7FFFU, 0xFFFFU},
                                        {0x8000U, 0x0000U},
                                        {0xFFFFU, 0xFFFFU}};

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->first_word_value == 0x56E8U);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == 0xFEF8U);
    TI_C54X_TEST_CHECK(metadata->program_word_count == 2U);
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    for (uint8_t destination = 0U; destination <= 1U; ++destination) {
        for (uint8_t arf = 0U; arf < 8U; ++arf) {
            const uint16_t lk = (uint16_t)(0x1000U + arf);
            const ti_c54x_decoded_instruction instruction =
                decode(destination, arf, lk);
            const ti_c54x_op_input_snapshot input = input_for(0U, true);
            const ti_c54x_trusted_operand_values operands =
                operands_for(destination, arf, lk);
            ti_c54x_op_effect direct = {0};
            ti_c54x_op_effect trusted = {0};

            TI_C54X_TEST_CHECK(ti_c54x_op_dld_long32_mod13(
                                   &instruction, &input, &direct) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dld_long32_mod13(
                                   &instruction, &operands, &input, &trusted) ==
                               TI_C54X_OP_STATUS_OK);
            assert_read(&direct, arf, lk);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
            for (size_t index = 0U; index < sizeof(reads) / sizeof(reads[0]);
                 ++index) {
                ti_c54x_op_effect completed = {0};
                const uint64_t low32 = ((uint64_t)reads[index][0] << 16U) |
                                       reads[index][1];
                const uint64_t expected =
                    (input.status_sxm != 0U && (reads[index][0] & 0x8000U))
                        ? (UINT64_C(0xFF00000000) | low32)
                        : low32;
                TI_C54X_TEST_CHECK(direct.payload.memory.complete_long32(
                                       &instruction, &input, reads[index][0],
                                       reads[index][1], &completed) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination ==
                                   destination);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                                   expected);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.status_delta.c_action ==
                                   TI_C54X_OP_STATUS_BIT_PRESERVE);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.status_delta.tc_action ==
                                   TI_C54X_OP_STATUS_BIT_PRESERVE);
            }
            {
                const ti_c54x_op_input_snapshot sxm1 = input_for(1U, true);
                ti_c54x_op_effect completed = {0};
                TI_C54X_TEST_CHECK(direct.payload.memory.complete_long32(
                                       &instruction, &sxm1, 0x8000U, 0U,
                                       &completed) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                                   UINT64_C(0xFF80000000));
            }
        }
    }
}

static void check_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U, 0x1234U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = 0xCAFEU}};
    ti_c54x_op_effect effect = before;
    ti_c54x_op_input_snapshot input = input_for(0U, false);
    ti_c54x_trusted_operand_values operands = operands_for(0U, 0U, 0x1234U);

    TI_C54X_TEST_CHECK(ti_c54x_op_dld_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = input_for(2U, true);
    TI_C54X_TEST_CHECK(ti_c54x_op_dld_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands.values[3].raw_value = 12U;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dld_long32_mod13(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = operands_for(0U, 0U, 0x1234U);
    operands.values[2].width_bits = 8U;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dld_long32_mod13(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = operands_for(0U, 0U, 0x1234U);
    operands.count = 5U;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dld_long32_mod13(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    {
        ti_c54x_op_effect read_effect = {0};
        ti_c54x_op_input_snapshot valid = input_for(0U, true);
        TI_C54X_TEST_CHECK(ti_c54x_op_dld_long32_mod13(
                               &instruction, &valid, &read_effect) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete_long32(
                               NULL, &valid, 0U, 0U, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete_long32(
                               &instruction, NULL, 0U, 0U, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete_long32(
                               &instruction, &valid, 0U, 0U, NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        valid.status_sxm_known = false;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete_long32(
                               &instruction, &valid, 0U, 0U, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        valid = input_for(2U, true);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete_long32(
                               &instruction, &valid, 0U, 0U, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.id = TI_C54X_OPCODE_ID_NOP;
        input = input_for(0U, true);
        TI_C54X_TEST_CHECK(ti_c54x_op_dld_long32_mod13(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_forms_and_completion();
    check_rejection_is_transactional();
    return 0;
}
