/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the checked, trusted-leaf, and
 * dispatch paths of the two-word ADDM standard-indirect WORD16 semantic
 * effect: ADD-transform staging with OVM/SXM knownness and range gating.
 * Does not own: catalog generation, decode tables, address resolution, bus
 * or CPU state, sequencing, timing, observers, diagnostics, or the
 * transaction that consumes the MEMORY carrier.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/memory/op_atomic_logical_memory.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static ti_c54x_decoded_instruction decode_case(uint8_t arf, uint8_t mode,
                                               uint16_t extension)
{
    const uint16_t first_word = (uint16_t)(UINT16_C(0x6B80) |
                                           (uint16_t)(mode << 3U) | arf);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension}, .program_word_count = 2U};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3);
    TI_C54X_TEST_CHECK(first.program_word_count == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id ==
                       TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(
    const ti_c54x_decoded_instruction *instruction)
{
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words,
        instruction->bundle.program_word_count};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction->operand_layout,
                           instruction->bundle.program_word_count, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    operands.count = view.descriptor->operand_count;
    TI_C54X_TEST_CHECK(operands.count == 3U);
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return operands;
}

static ti_c54x_op_input_snapshot known_modes_input(uint8_t ovm, uint8_t sxm)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_ovm = ovm;
    input.status_ovm_known = true;
    input.status_sxm = sxm;
    input.status_sxm_known = true;
    return input;
}

static void assert_memory_effect(const ti_c54x_op_effect *effect, uint8_t arf,
                                 uint8_t mode, uint16_t extension)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == extension);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation ==
                       TI_C54X_OP_MEMORY_RMW_OPERATION_ADD);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id ==
                       TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x6B80));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == 2U);
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
}

static void check_valid_forms(void)
{
    const uint16_t extensions[] = {UINT16_C(0), UINT16_C(0x55AA),
                                   UINT16_C(0xFFFF)};
    const uint8_t modes[][2] = {{UINT8_C(0), UINT8_C(0)},
                                {UINT8_C(0), UINT8_C(1)},
                                {UINT8_C(1), UINT8_C(0)},
                                {UINT8_C(1), UINT8_C(1)}};

    for (uint8_t arf = 0U; arf < 8U; ++arf) {
        for (uint8_t mode = 0U; mode <= 2U; ++mode) {
            for (size_t extension_index = 0U;
                 extension_index < sizeof(extensions) / sizeof(extensions[0]);
                 ++extension_index) {
                for (size_t mode_index = 0U;
                     mode_index < sizeof(modes) / sizeof(modes[0]);
                     ++mode_index) {
                    const uint16_t extension = extensions[extension_index];
                    const ti_c54x_op_input_snapshot input =
                        known_modes_input(modes[mode_index][0],
                                          modes[mode_index][1]);
                    const ti_c54x_decoded_instruction instruction =
                        decode_case(arf, mode, extension);
                    const ti_c54x_trusted_operand_values operands =
                        trusted_operands(&instruction);
                    const ti_c54x_trusted_opcode_binding *binding =
                        ti_c54x_trusted_opcode_binding_for_id(
                            TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3);
                    const ti_c54x_instruction_word_bundle bundle =
                        instruction.bundle;
                    ti_c54x_trusted_execution_packet packet;
                    const ti_c54x_op_effect sentinel = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand =
                                                     UINT16_C(0xA55A)}};
                    ti_c54x_op_effect checked = sentinel;
                    ti_c54x_op_effect trusted = sentinel;
                    ti_c54x_op_effect trusted_dispatch = sentinel;
                    ti_c54x_op_effect dispatched = sentinel;

                    TI_C54X_TEST_CHECK(binding != NULL);
                    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                           binding, &bundle, &packet) ==
                                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_op_addm_smem_mod0_3(
                                           &instruction, &input, &checked) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addm_smem_mod0_3(
                                           &instruction, &operands, &input,
                                           &trusted) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_dispatch_decoded_with_input_snapshot(
                            &instruction, &input, &dispatched) ==
                        TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                           &packet, &input,
                                           &trusted_dispatch) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_memory_effect(&checked, arf, mode, extension);
                    assert_memory_effect(&trusted, arf, mode, extension);
                    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                              sizeof(checked)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&checked, &dispatched,
                                              sizeof(checked)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted_dispatch,
                                              sizeof(checked)) == 0);
                }
            }
        }
    }
}

static void check_unknown_modes_rejected(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(UINT8_C(2), UINT8_C(1), UINT16_C(0x1234));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = sentinel;
    ti_c54x_op_input_snapshot input = known_modes_input(UINT8_C(0),
                                                        UINT8_C(0));

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_addm_smem_mod0_3(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addm_smem_mod0_3(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    input = known_modes_input(UINT8_C(0), UINT8_C(0));
    input.status_sxm_known = false;
    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_op_addm_smem_mod0_3(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

static void check_out_of_range_modes_rejected(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(UINT8_C(2), UINT8_C(1), UINT16_C(0x1234));
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = sentinel;
    ti_c54x_op_input_snapshot input = known_modes_input(UINT8_C(2),
                                                        UINT8_C(0));

    TI_C54X_TEST_CHECK(ti_c54x_op_addm_smem_mod0_3(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    input = known_modes_input(UINT8_C(0), UINT8_C(3));
    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_op_addm_smem_mod0_3(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

static void check_mod3_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(UINT8_C(3), UINT8_C(3), UINT16_C(0x1234));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction);
    const ti_c54x_op_input_snapshot input =
        known_modes_input(UINT8_C(0), UINT8_C(0));
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect checked = sentinel;
    ti_c54x_op_effect trusted = sentinel;
    ti_c54x_op_effect dispatched = sentinel;

    TI_C54X_TEST_CHECK(ti_c54x_op_addm_smem_mod0_3(&instruction, &input,
                                                  &checked) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_addm_smem_mod0_3(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&checked, &sentinel, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &sentinel, sizeof(trusted)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &sentinel, sizeof(dispatched)) ==
                       0);
}

int main(void)
{
    check_metadata();
    check_valid_forms();
    check_unknown_modes_rejected();
    check_out_of_range_modes_rejected();
    check_mod3_rejection();
    return EXIT_SUCCESS;
}
