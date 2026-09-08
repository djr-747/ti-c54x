/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the one-word MOD2 WORD16 ST T
 * memory-write effect, including all ARF/T boundary values, checked/trusted/
 * dispatch parity, and transactional rejection. Does not own: catalog rows,
 * operand-layout descriptors, T state, input collection, address resolution,
 * buses, CPU/state mutation, sequencing, timing, observers, diagnostics, or
 * the production semantic leaf.
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
#include "../../../../../src/isa/ops/store/op_st_t_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t st_t_word(uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x8C90) | (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction decode(uint8_t arf)
{
    const uint16_t word = st_t_word(arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ST_T_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_ST_T_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_T_REGISTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint16_t value, bool known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.t_register_raw16 = value;
    input.t_register_known = known;
    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_tc = UINT8_C(0);
    input.status_tc_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        input.auxiliary_registers_raw16[index] =
            (uint16_t)(UINT16_C(0xA500) | index);
    }
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t arf)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = arf;
    operands.values[0].width_bits = UINT8_C(3);
    operands.values[1].raw_value = TI_C54X_INDIRECT_SINGLE_MOD2;
    operands.values[1].width_bits = UINT8_C(2);
    return operands;
}

static void assert_memory_write(const ti_c54x_op_effect *effect, uint8_t arf,
                                uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_metadata_and_all_forms(void)
{
    const ti_c54x_opcode_metadata *metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_ST_T_SMEM);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ST_T_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x8C80));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_T_REGISTER);

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        const ti_c54x_decoded_instruction instruction = decode(arf);

        TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                           st_t_word(arf));
    }
}

static void check_effects_and_parity(void)
{
    static const uint16_t values[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                      UINT16_C(0x8000), UINT16_C(0xFFFF)};

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        const ti_c54x_decoded_instruction instruction = decode(arf);
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(
                TI_C54X_OPCODE_ID_ST_T_SMEM);
        ti_c54x_trusted_execution_packet packet;

        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_st_t_smem);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &instruction.bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(packet.instruction.id ==
                           TI_C54X_OPCODE_ID_ST_T_SMEM);
        TI_C54X_TEST_CHECK(packet.operands.count == UINT8_C(2));
        TI_C54X_TEST_CHECK(packet.operands.values[0].raw_value == arf);
        TI_C54X_TEST_CHECK(packet.operands.values[0].width_bits == UINT8_C(3));
        TI_C54X_TEST_CHECK(packet.operands.values[1].raw_value ==
                           TI_C54X_INDIRECT_SINGLE_MOD2);
        TI_C54X_TEST_CHECK(packet.operands.values[1].width_bits == UINT8_C(2));

        for (size_t index = 0U;
             index < sizeof(values) / sizeof(values[0]); ++index) {
            const ti_c54x_op_input_snapshot input =
                input_for(values[index], true);
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(arf);
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect direct = before;
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted_dispatch = before;
            ti_c54x_op_effect trusted_leaf = before;

            TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem(
                                   &instruction, &input, &direct) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &input, &trusted_dispatch) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                                   &packet.instruction, &operands, &input,
                                   &trusted_leaf) == TI_C54X_OP_STATUS_OK);
            assert_memory_write(&direct, arf, values[index]);
            assert_memory_write(&checked, arf, values[index]);
            assert_memory_write(&trusted_dispatch, arf, values[index]);
            assert_memory_write(&trusted_leaf, arf, values[index]);
            TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_dispatch,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_leaf,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) ==
                               0);
        }
    }
}

static void check_unknown_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(7));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT16_C(0xFFFF), false);
    const ti_c54x_op_input_snapshot input_before = input;
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_ST_T_SMEM);
    const ti_c54x_trusted_operand_values operands = trusted_operands(7U);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void check_checked_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(3));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT16_C(0x4210), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                         \
    do {                                                                        \
        effect = before;                                                        \
        TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem(                                \
                               &(candidate), &input, &effect) ==                \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                  \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);      \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(2);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[0] ^= UINT16_C(0x0020);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

static void check_trusted_operand_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(7));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT16_C(0x8000), true);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_ST_T_SMEM);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands = trusted_operands(UINT8_C(7));

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7));
    operands.values[0].raw_value = UINT64_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata_and_all_forms();
    check_effects_and_parity();
    check_unknown_input_is_transactional();
    check_checked_rejection_is_transactional();
    check_trusted_operand_rejection_is_transactional();
    return 0;
}
