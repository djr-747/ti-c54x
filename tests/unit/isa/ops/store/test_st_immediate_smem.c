/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the two-word MOD2 WORD16 immediate
 * store effect, including ARF/immediate extraction, checked/trusted/dispatch
 * parity, ignored input state, and transactional rejection. Does not own:
 * catalog rows, operand-layout descriptors, address resolution, buses, CPU or
 * architectural state, sequencing, timing, observers, diagnostics, or the
 * production semantic leaf.
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
#include "../../../../../src/isa/ops/store/op_st_immediate_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t st_immediate_word(uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x7690) | (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction decode(uint8_t arf, uint16_t immediate)
{
    const uint16_t first_word = st_immediate_word(arf);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, immediate}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id ==
                       TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == immediate);
    return instruction;
}

static ti_c54x_op_input_snapshot ignored_input(uint8_t sxm, bool known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 =
        UINT64_C(0x123456789A) ^ (uint64_t)sxm;
    input.accumulator_b_raw40 =
        UINT64_C(0x0FEDCBA987) ^ (known ? UINT64_C(1) : UINT64_C(0));
    input.accumulator_a_known = !known;
    input.accumulator_b_known = known;
    input.status_c = sxm;
    input.status_c_known = !known;
    input.status_tc = known ? UINT8_C(0x55) : UINT8_C(0xAA);
    input.status_tc_known = known;
    input.status_sxm = sxm;
    input.status_sxm_known = !known;
    input.auxiliary_registers_raw16[0] =
        UINT16_C(0xAAAA) ^ (uint16_t)sxm;
    input.auxiliary_registers_known_mask = UINT8_C(0);
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t arf,
                                                        uint16_t immediate)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = arf;
    operands.values[0].width_bits = UINT8_C(3);
    operands.values[1].raw_value = immediate;
    operands.values[1].width_bits = UINT8_C(16);
    operands.values[2].raw_value = TI_C54X_INDIRECT_SINGLE_MOD2;
    operands.values[2].width_bits = UINT8_C(2);
    return operands;
}

static void assert_memory_write(const ti_c54x_op_effect *effect, uint8_t arf,
                                uint16_t immediate)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == immediate);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_metadata_and_all_forms(void)
{
    static const uint16_t immediates[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                          UINT16_C(0x8000), UINT16_C(0xFFFF)};
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x7680));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        for (size_t index = 0U;
             index < sizeof(immediates) / sizeof(immediates[0]); ++index) {
            const uint16_t immediate = immediates[index];
            const ti_c54x_decoded_instruction instruction =
                decode(arf, immediate);
            const ti_c54x_trusted_opcode_binding *binding =
                ti_c54x_trusted_opcode_binding_for_id(
                    TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
            const ti_c54x_instruction_word_bundle bundle = {
                .program_words = {instruction.bundle.program_words[0],
                                  instruction.bundle.program_words[1]},
                .program_word_count = UINT8_C(2)};
            ti_c54x_trusted_execution_packet packet;
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(arf, immediate);
            const ti_c54x_op_input_snapshot input =
                ignored_input((uint8_t)(index + 1U), (index & 1U) != 0U);
            ti_c54x_op_effect direct = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect checked = direct;
            ti_c54x_op_effect trusted_dispatch = direct;
            ti_c54x_op_effect trusted_leaf = direct;

            TI_C54X_TEST_CHECK(binding != NULL);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(
                                   &instruction, &input, &direct) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &input, &trusted_dispatch) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                                   &packet.instruction, &operands, &input,
                                   &trusted_leaf) == TI_C54X_OP_STATUS_OK);
            assert_memory_write(&direct, arf, immediate);
            TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) == 0);
            assert_memory_write(&checked, arf, immediate);
            assert_memory_write(&trusted_dispatch, arf, immediate);
            assert_memory_write(&trusted_leaf, arf, immediate);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_dispatch,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_leaf,
                                      sizeof(direct)) == 0);
        }
    }
}

static void check_transactional_checked_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, UINT16_C(0x55AA));
    const ti_c54x_op_input_snapshot input = ignored_input(UINT8_C(3), false);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
        TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0100);
        TI_C54X_TEST_CHECK(ti_c54x_op_st_immediate_smem(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_operand_rejection(void)
{
    const uint16_t immediate = UINT16_C(0x8000);
    const ti_c54x_op_input_snapshot input = ignored_input(UINT8_C(7), false);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {st_immediate_word(UINT8_C(7)), immediate},
        .program_word_count = UINT8_C(2)};
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(7), immediate);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7), immediate);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7), immediate);
    operands.values[0].raw_value = UINT64_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7), immediate);
    operands.values[1].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7), immediate);
    operands.values[1].raw_value = UINT64_C(0x10000);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_immediate_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata_and_all_forms();
    check_transactional_checked_rejection();
    check_trusted_operand_rejection();
    return 0;
}
