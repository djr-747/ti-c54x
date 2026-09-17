/*
 * Module contract
 *
 * Owns: pure first-word decode result construction from the catalog's direct
 * lookup plus transactional completion of a copied complete instruction and
 * its scalar layout/repeatability metadata. First words owning a qualified
 * slot re-resolve on the second program word at completion time.
 * Unknown words remain explicit and preserve the raw fetched value.
 * Does not own: bus/memory fetches, CPU state, program-counter sequencing,
 * opcode semantics, timing, observers, or diagnostics.
 */

#include "opcode_decode.h"

#include "operand_layout.h"

#include <stddef.h>

#include "../catalog/opcode_catalog_internal.h"

ti_c54x_decode_result
ti_c54x_decode_first_word(uint16_t first_word)
{
    const ti_c54x_opcode_id id =
        ti_c54x_opcode_catalog_lookup_first_word(first_word);
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    ti_c54x_decode_result result = {
        TI_C54X_DECODE_STATUS_UNKNOWN_WORD,
        TI_C54X_OPCODE_ID_UNKNOWN,
        first_word,
        UINT8_C(0)};

    if (metadata != NULL && id != TI_C54X_OPCODE_ID_UNKNOWN) {
        result.status = TI_C54X_DECODE_STATUS_OK;
        result.id = id;
        result.program_word_count = metadata->program_word_count;
    }
    return result;
}

/* Complete through the slot winner's metadata after the first-word checks
 * above already pinned the borrowed pointers, the first-word identity, and
 * the bundle shape. Output stays untouched on every non-OK status. */
static ti_c54x_instruction_completion_status
ti_c54x_decode_complete_resolved(
    const ti_c54x_decode_result *first_word_result,
    const ti_c54x_instruction_word_bundle *bundle, ti_c54x_opcode_id resolved,
    ti_c54x_decoded_instruction *output)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_decoded_instruction result;

    if (resolved <= TI_C54X_OPCODE_ID_UNKNOWN ||
        resolved >= TI_C54X_OPCODE_ID_COUNT) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_OPCODE_ID;
    }
    metadata = ti_c54x_opcode_metadata_for_id(resolved);
    if (metadata == NULL) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_UNAVAILABLE;
    }
    if (metadata->id != resolved) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_ID_MISMATCH;
    }
    if (metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_COUNT_INVALID;
    }
    if (metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_OPERAND_LAYOUT_INVALID;
    }
    if (metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_REPEATABILITY_INVALID;
    }
    if ((metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_UNAVAILABLE;
    }
    if (first_word_result->program_word_count !=
        metadata->program_word_count) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH;
    }
    if (bundle->program_word_count != metadata->program_word_count) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH;
    }
    if ((uint16_t)(bundle->program_words[0] & metadata->first_word_mask) !=
        metadata->first_word_value) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH;
    }

    result = (ti_c54x_decoded_instruction){
        .id = resolved,
        .operand_layout = metadata->operand_layout,
        .repeatability = metadata->repeatability,
        .input_resources = metadata->input_resources,
        .bundle = *bundle};
    *output = result;
    return TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK;
}

ti_c54x_instruction_completion_status
ti_c54x_decode_complete_instruction(
    const ti_c54x_decode_result *first_word_result,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_decoded_instruction *output)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_decoded_instruction result;

    if (first_word_result == NULL || bundle == NULL || output == NULL) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT;
    }
    if (first_word_result->status != TI_C54X_DECODE_STATUS_OK) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK;
    }
    if (first_word_result->id == TI_C54X_OPCODE_ID_UNKNOWN) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID;
    }
    if (first_word_result->id < TI_C54X_OPCODE_ID_UNKNOWN ||
        first_word_result->id >= TI_C54X_OPCODE_ID_COUNT) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_OPCODE_ID;
    }

    metadata = ti_c54x_opcode_metadata_for_id(first_word_result->id);
    if (metadata == NULL) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_UNAVAILABLE;
    }
    if (metadata->id != first_word_result->id) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_ID_MISMATCH;
    }
    if (metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_COUNT_INVALID;
    }
    if (metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_OPERAND_LAYOUT_INVALID;
    }
    if (metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_REPEATABILITY_INVALID;
    }
    if ((metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_UNAVAILABLE;
    }
    if (first_word_result->program_word_count == UINT8_C(0) ||
        first_word_result->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_INVALID;
    }
    if (first_word_result->program_word_count !=
        metadata->program_word_count) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH;
    }
    if (bundle->program_word_count == UINT8_C(0) ||
        bundle->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID;
    }
    if (bundle->program_word_count != metadata->program_word_count) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH;
    }
    {
        if (ti_c54x_operand_layout_set_resolve(
                metadata->operand_layout, metadata->program_word_count,
                &(ti_c54x_operand_layout_set_view){0}) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_INSTRUCTION_COMPLETION_STATUS_OPERAND_LAYOUT_INVALID;
        }
    }
    if (bundle->program_words[0] != first_word_result->first_word) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH;
    }
    if ((uint16_t)(bundle->program_words[0] & metadata->first_word_mask) !=
        metadata->first_word_value) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH;
    }
    if (ti_c54x_first_word_has_qualified_slot(bundle->program_words[0])) {
        ti_c54x_opcode_id resolved;

        if (bundle->program_word_count < UINT8_C(2)) {
            return TI_C54X_INSTRUCTION_COMPLETION_STATUS_SECOND_WORD_UNMATCHED;
        }
        resolved = ti_c54x_resolve_qualified_slot(
            bundle->program_words[0], bundle->program_words[1]);
        if (resolved == TI_C54X_OPCODE_ID_UNKNOWN) {
            return TI_C54X_INSTRUCTION_COMPLETION_STATUS_SECOND_WORD_UNMATCHED;
        }
        if (resolved != first_word_result->id) {
            return ti_c54x_decode_complete_resolved(first_word_result,
                                                   bundle, resolved, output);
        }
    }

    result = (ti_c54x_decoded_instruction){
        .id = first_word_result->id,
        .operand_layout = metadata->operand_layout,
        .repeatability = metadata->repeatability,
        .input_resources = metadata->input_resources,
        .bundle = *bundle};
    *output = result;
    return TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK;
}
