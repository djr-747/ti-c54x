/*
 * Module contract
 *
 * Owns: checked construction of one value-owned trusted execution packet from
 * a canonical binding and complete instruction bundle, including one-pass
 * extraction through the decode-owned static operand plan. Does not own:
 * fetch, semantics, architectural state, sequencing, buses, timing,
 * diagnostics, allocation, or mutable global state.
 */

#include "trusted_execution_packet.h"

#include <stddef.h>

#include "operand_layout_internal.h"

static bool ti_c54x_trusted_execution_packet_stage(
    const ti_c54x_trusted_opcode_binding *binding,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_trusted_execution_packet *staged)
{
    const ti_c54x_opcode_metadata *metadata = &binding->metadata;

    staged->instruction.id = metadata->id;
    staged->instruction.operand_layout = metadata->operand_layout;
    staged->instruction.repeatability = metadata->repeatability;
    staged->instruction.input_resources = metadata->input_resources;
    staged->instruction.bundle = *bundle;
    if (!ti_c54x_operand_layout_extract_all_trusted(
            binding->operand_plan, bundle, staged->operands.values,
            &staged->operands.count)) {
        return false;
    }
    staged->handler = binding->handler;
    return true;
}

ti_c54x_trusted_execution_packet_status
ti_c54x_trusted_execution_packet_prepare(
    const ti_c54x_trusted_opcode_binding *binding,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_trusted_execution_packet *output)
{
    ti_c54x_trusted_execution_packet staged = {0};
    const ti_c54x_opcode_metadata *metadata;

    if (binding == NULL || bundle == NULL || output == NULL) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_ARGUMENT;
    }
    if (!ti_c54x_trusted_opcode_binding_is_canonical(binding)) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_BINDING;
    }
    metadata = &binding->metadata;
    if (metadata->id == TI_C54X_OPCODE_ID_UNKNOWN ||
        binding->handler == NULL) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_UNKNOWN_OPCODE;
    }
    if (bundle->program_word_count != metadata->program_word_count ||
        bundle->program_word_count == UINT8_C(0) ||
        bundle->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_BUNDLE_COUNT_MISMATCH;
    }
    if ((uint16_t)(bundle->program_words[0] & metadata->first_word_mask) !=
        metadata->first_word_value) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_FIRST_WORD_MISMATCH;
    }

    if (!ti_c54x_trusted_execution_packet_stage(binding, bundle, &staged)) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_OPERAND_PLAN_UNAVAILABLE;
    }
    *output = staged;
    return TI_C54X_TRUSTED_EXECUTION_PACKET_OK;
}

ti_c54x_trusted_execution_packet_status
ti_c54x_trusted_execution_packet_prepare_fast(
    const ti_c54x_trusted_opcode_binding *binding,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_trusted_execution_packet *output)
{
    ti_c54x_trusted_execution_packet staged = {0};

    if (binding == NULL || bundle == NULL || output == NULL) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_ARGUMENT;
    }
    /* Private precondition: binding came from the one first-word lookup and
     * names a supported catalog row.  Do not canonical-check or re-resolve
     * anything on this hot path. */
    if (!ti_c54x_trusted_execution_packet_stage(binding, bundle, &staged)) {
        return TI_C54X_TRUSTED_EXECUTION_PACKET_OPERAND_PLAN_UNAVAILABLE;
    }
    *output = staged;
    return TI_C54X_TRUSTED_EXECUTION_PACKET_OK;
}
