/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the row-less ST T direct-slice WORD16
 * store effect (first word 0x8C00 under mask 0xFF80, I pinned 0),
 * including the DIRECT_WORD16 dma/value staging, null completion, no
 * delay flag, checked/trusted/dispatch parity, direct/indirect gating
 * with I=1 rejected, and transactional rejection. Does not own: catalog
 * rows, operand-layout descriptors, T state, input collection, address
 * resolution, buses, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
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
#include "../../../../../src/isa/ops/store/op_st_t_smem_direct.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t st_t_direct_word(uint8_t dma)
{
    return (uint16_t)(UINT16_C(0x8C00) | (uint16_t)(dma & UINT8_C(0x7F)));
}

static ti_c54x_decoded_instruction decode(uint8_t dma)
{
    const uint16_t word = st_t_direct_word(dma);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
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

static ti_c54x_trusted_operand_values trusted_operands(uint8_t dma)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = dma;
    operands.values[1].width_bits = UINT8_C(7);
    return operands;
}

static void assert_memory_write(const ti_c54x_op_effect *effect, uint8_t dma,
                                uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.direct_dma7 ==
                       dma);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
    TI_C54X_TEST_CHECK(!effect->payload.memory.write_smem_delay);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation ==
                       (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
}

static void check_metadata_and_all_forms(void)
{
    const ti_c54x_opcode_metadata *metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x8C00));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_T_REGISTER);

    for (uint16_t dma = 0U; dma < UINT16_C(0x80); ++dma) {
        const ti_c54x_decoded_instruction instruction =
            decode((uint8_t)dma);

        TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                           st_t_direct_word((uint8_t)dma));
    }
}

static void check_effects_and_parity(void)
{
    static const uint16_t values[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                      UINT16_C(0x8000), UINT16_C(0xFFFF)};
    static const uint8_t dmas[] = {UINT8_C(0x00), UINT8_C(0x01),
                                   UINT8_C(0x3F), UINT8_C(0x7E),
                                   UINT8_C(0x7F)};

    for (size_t dma_index = 0U;
         dma_index < sizeof(dmas) / sizeof(dmas[0]); ++dma_index) {
        const uint8_t dma = dmas[dma_index];
        const ti_c54x_decoded_instruction instruction = decode(dma);
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(
                TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
        ti_c54x_trusted_execution_packet packet;

        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(binding->handler ==
                           ti_c54x_trusted_op_st_t_smem_direct);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &instruction.bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(packet.instruction.id ==
                           TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
        TI_C54X_TEST_CHECK(packet.operands.count == UINT8_C(2));
        TI_C54X_TEST_CHECK(packet.operands.values[0].raw_value == UINT64_C(0));
        TI_C54X_TEST_CHECK(packet.operands.values[0].width_bits == UINT8_C(1));
        TI_C54X_TEST_CHECK(packet.operands.values[1].raw_value == dma);
        TI_C54X_TEST_CHECK(packet.operands.values[1].width_bits == UINT8_C(7));

        for (size_t index = 0U;
             index < sizeof(values) / sizeof(values[0]); ++index) {
            const ti_c54x_op_input_snapshot input =
                input_for(values[index], true);
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(dma);
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect direct = before;
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted_dispatch = before;
            ti_c54x_op_effect trusted_leaf = before;

            TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(
                                   &instruction, &input, &direct) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &input, &trusted_dispatch) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                                   &packet.instruction, &operands, &input,
                                   &trusted_leaf) == TI_C54X_OP_STATUS_OK);
            assert_memory_write(&direct, dma, values[index]);
            assert_memory_write(&checked, dma, values[index]);
            assert_memory_write(&trusted_dispatch, dma, values[index]);
            assert_memory_write(&trusted_leaf, dma, values[index]);
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
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(0x7F));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT16_C(0xFFFF), false);
    const ti_c54x_op_input_snapshot input_before = input;
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x7F));
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void check_indirect_gating_is_invalid_operand(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(0x03));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT16_C(0x4210), true);
    const ti_c54x_op_input_snapshot input_before = input;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands;

    forged = instruction;
    forged.bundle.program_words[0] |= UINT16_C(0x0080);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x8C90);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands = trusted_operands(UINT8_C(0x03));
    operands.values[0].raw_value = UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void check_checked_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(0x03));
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
        TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(                          \
                               &(candidate), &input, &effect) ==                \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                  \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);      \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(&instruction, NULL,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_t_smem_direct(&instruction, &input,
                                                  NULL) ==
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
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

static void check_trusted_operand_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(0x7F));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT16_C(0x8000), true);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x7F));

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0x7F));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0x7F));
    operands.values[1].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_t_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata_and_all_forms();
    check_effects_and_parity();
    check_unknown_input_is_transactional();
    check_indirect_gating_is_invalid_operand();
    check_checked_rejection_is_transactional();
    check_trusted_operand_rejection_is_transactional();
    return 0;
}
