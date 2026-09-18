/*
 * Module contract
 *
 * Owns: checked and trusted operand validation plus value-owned MEMORY-effect
 * staging for the neutral ANDM/ORM/XORM standard-indirect WORD16 forms and
 * the ADDM standard-indirect WORD16 form, which shares their two-word
 * ARF-extension shape but stages an ADD transform descriptor with
 * OVM/SXM knownness gating (the sequencer commits sticky C/OVA from the
 * frozen modes at write time).
 * Does not own: catalog generation, decode, address resolution, buses, CPU or
 * architectural state, sequencing, timing, observers, diagnostics,
 * allocation, callbacks, or status mutation. No raw first-word fields are
 * parsed here; operand extraction is delegated to the decoder-owned layout.
 */

#include "op_atomic_logical_memory.h"

#include <stdbool.h>
#include <stddef.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../dispatch/trusted_semantics.h"

typedef struct atomic_logical_operands {
    uint8_t arf;
    uint16_t extension;
    uint8_t mode;
} atomic_logical_operands;

static ti_c54x_op_status atomic_logical_validate_status_modes(
    const ti_c54x_op_input_snapshot *input)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->status_ovm_known || !input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_ovm > UINT8_C(1) || input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status atomic_logical_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint16_t expected_base,
    ti_c54x_input_resource_mask expected_resources,
    atomic_logical_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value arf = {0U, 0U};
    ti_c54x_operand_field_value extension = {0U, 0U};
    ti_c54x_operand_field_value mode = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_value != expected_base ||
        metadata->first_word_mask != UINT16_C(0xFFE0) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != expected_resources ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &arf) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &extension) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &mode) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            arf.width_bits != UINT8_C(3) || arf.raw_value > UINT64_C(7) ||
            extension.width_bits != UINT8_C(16) ||
            extension.raw_value > UINT64_C(0xFFFF) ||
            mode.width_bits != UINT8_C(2) || mode.raw_value > UINT64_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    operands->arf = (uint8_t)arf.raw_value;
    operands->extension = (uint16_t)extension.raw_value;
    operands->mode = (uint8_t)mode.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status atomic_logical_validate_trusted(
    const ti_c54x_trusted_operand_values *trusted,
    atomic_logical_operands *operands)
{
    if (trusted == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (trusted->count < UINT8_C(3) ||
        trusted->values[0].width_bits != UINT8_C(3) ||
        trusted->values[0].raw_value > UINT64_C(7) ||
        trusted->values[1].width_bits != UINT8_C(16) ||
        trusted->values[1].raw_value > UINT64_C(0xFFFF) ||
        trusted->values[2].width_bits != UINT8_C(2) ||
        trusted->values[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    operands->arf = (uint8_t)trusted->values[0].raw_value;
    operands->extension = (uint16_t)trusted->values[1].raw_value;
    operands->mode = (uint8_t)trusted->values[2].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status atomic_logical_stage(
    const atomic_logical_operands *operands, uint8_t operation,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (operands->mode == UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                .mode = (ti_c54x_indirect_single_mode)operands->mode,
                .arf = operands->arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = operands->extension,
            .rmw_operation = operation,
            .complete = NULL}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status atomic_logical_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_base,
    ti_c54x_input_resource_mask expected_resources, bool needs_status_modes,
    uint8_t operation)
{
    atomic_logical_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = atomic_logical_validate_instruction(
        instruction, expected_id, expected_base, expected_resources,
        &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (needs_status_modes) {
        status = atomic_logical_validate_status_modes(input);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    }
    return atomic_logical_stage(&operands, operation, effect);
}

static ti_c54x_op_status atomic_logical_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    bool needs_status_modes, uint8_t operation)
{
    atomic_logical_operands operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (trusted == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = atomic_logical_validate_trusted(trusted, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (needs_status_modes) {
        status = atomic_logical_validate_status_modes(input);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    }
    return atomic_logical_stage(&operands, operation, effect);
}

#define ATOMIC_LOGICAL_DEFINITIONS(name, id, base, resources, needs_modes,   \
                                   operation)                                 \
    ti_c54x_op_status ti_c54x_op_##name(                                        \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)      \
    {                                                                            \
        return atomic_logical_checked(instruction, input, effect, id, base,     \
                                      resources, needs_modes, operation);        \
    }                                                                            \
    ti_c54x_op_status ti_c54x_trusted_op_##name(                                 \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_trusted_operand_values *trusted,                           \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)      \
    {                                                                            \
        return atomic_logical_trusted(instruction, trusted, input, effect,     \
                                      needs_modes, operation);                   \
    }

ATOMIC_LOGICAL_DEFINITIONS(
    andm_smem_mod0_3, TI_C54X_OPCODE_ID_ANDM_SMEM_MOD0_3, UINT16_C(0x6880),
    TI_C54X_INPUT_RESOURCES_NONE, false,
    TI_C54X_OP_MEMORY_RMW_OPERATION_AND)
ATOMIC_LOGICAL_DEFINITIONS(
    orm_smem_mod0_3, TI_C54X_OPCODE_ID_ORM_SMEM_MOD0_3, UINT16_C(0x6980),
    TI_C54X_INPUT_RESOURCES_NONE, false,
    TI_C54X_OP_MEMORY_RMW_OPERATION_OR)
ATOMIC_LOGICAL_DEFINITIONS(
    xorm_smem_mod0_3, TI_C54X_OPCODE_ID_XORM_SMEM_MOD0_3, UINT16_C(0x6A80),
    TI_C54X_INPUT_RESOURCES_NONE, false,
    TI_C54X_OP_MEMORY_RMW_OPERATION_XOR)
ATOMIC_LOGICAL_DEFINITIONS(
    addm_smem_mod0_3, TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3, UINT16_C(0x6B80),
    TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM, true,
    TI_C54X_OP_MEMORY_RMW_OPERATION_ADD)

#undef ATOMIC_LOGICAL_DEFINITIONS
