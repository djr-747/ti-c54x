/* Module contract
 * Owns: the shared pure semantics and metadata validation for the two-word
 * AND/OR/XOR immediate accumulator families. It selects one canonical source,
 * applies an unsigned zero-extended lk16 at the encoded or implicit shift,
 * and stages an accumulator-write effect.
 * Does not own: family wrappers, catalog definitions, generated lookup,
 * operand-layout descriptors, accumulator/status storage, fetch, sequencing,
 * timing, memory, buses, repeat/interrupt policy, observers, diagnostics,
 * environment, allocation, or public ABI.
 */

#include "op_logical_immediate.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"

typedef struct ti_c54x_logical_immediate_family {
    ti_c54x_opcode_id shifted_id;
    ti_c54x_opcode_id shift16_id;
} ti_c54x_logical_immediate_family;

static int ti_c54x_logical_immediate_family_for_kind(
    ti_c54x_logical_immediate_kind kind,
    ti_c54x_logical_immediate_family *family)
{
    if (family == NULL) {
        return 0;
    }
    switch (kind) {
    case TI_C54X_LOGICAL_IMMEDIATE_AND:
        *family = (ti_c54x_logical_immediate_family){
            TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT,
            TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16};
        return 1;
    case TI_C54X_LOGICAL_IMMEDIATE_OR:
        *family = (ti_c54x_logical_immediate_family){
            TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT,
            TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT16};
        return 1;
    case TI_C54X_LOGICAL_IMMEDIATE_XOR:
        *family = (ti_c54x_logical_immediate_family){
            TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT,
            TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT16};
        return 1;
    case TI_C54X_LOGICAL_IMMEDIATE_KIND_COUNT:
    default:
        return 0;
    }
}

static uint64_t ti_c54x_logical_immediate_apply(
    ti_c54x_logical_immediate_kind kind,
    uint64_t source,
    uint64_t shifted_immediate)
{
    switch (kind) {
    case TI_C54X_LOGICAL_IMMEDIATE_AND:
        return source & shifted_immediate;
    case TI_C54X_LOGICAL_IMMEDIATE_OR:
        return source | shifted_immediate;
    case TI_C54X_LOGICAL_IMMEDIATE_XOR:
        return source ^ shifted_immediate;
    case TI_C54X_LOGICAL_IMMEDIATE_KIND_COUNT:
    default:
        return UINT64_C(0);
    }
}

ti_c54x_op_status ti_c54x_op_logical_immediate(
    ti_c54x_logical_immediate_kind kind,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_logical_immediate_family family;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};
    ti_c54x_operand_field_value immediate_field = {0U, 0U};
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t source_raw40;
    ti_c54x_opcode_id expected_id;
    ti_c54x_operand_layout_id expected_layout;
    uint8_t source;
    uint8_t destination;
    uint8_t shift;
    uint64_t shifted_immediate;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_logical_immediate_family_for_kind(kind, &family)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    if (instruction->id == family.shifted_id) {
        expected_id = family.shifted_id;
        expected_layout =
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16;
    } else if (instruction->id == family.shift16_id) {
        expected_id = family.shift16_id;
        expected_layout =
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16;
    } else {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout != expected_layout ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        instruction->bundle.program_word_count != metadata->program_word_count ||
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
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        if (instruction->id == family.shifted_id) {
            if (ti_c54x_operand_layout_set_extract_operand(
                    &layout_view, &words, UINT8_C(2), &shift_field) !=
                    TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
                ti_c54x_operand_layout_set_extract_operand(
                    &layout_view, &words, UINT8_C(3), &immediate_field) !=
                    TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
                return TI_C54X_OP_STATUS_INVALID_DECODE;
            }
        } else if (ti_c54x_operand_layout_set_extract_operand(
                       &layout_view, &words, UINT8_C(2), &immediate_field) !=
                   TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    if (source_field.width_bits != UINT8_C(1) ||
        destination_field.width_bits != UINT8_C(1) ||
        source_field.raw_value > UINT64_C(1) ||
        destination_field.raw_value > UINT64_C(1) ||
        immediate_field.width_bits != UINT8_C(16) ||
        immediate_field.raw_value > UINT64_C(0xffff)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (instruction->id == family.shifted_id) {
        if (shift_field.width_bits != UINT8_C(4) ||
            shift_field.raw_value > UINT64_C(15)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        shift = (uint8_t)shift_field.raw_value;
    } else {
        shift = UINT8_C(16);
    }

    source = (uint8_t)source_field.raw_value;
    destination = (uint8_t)destination_field.raw_value;
    if (source == UINT8_C(0)) {
        source_raw40 = input->accumulator_a_raw40;
        if (!input->accumulator_a_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    } else {
        source_raw40 = input->accumulator_b_raw40;
        if (!input->accumulator_b_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    }
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    shifted_immediate =
        (((uint64_t)immediate_field.raw_value) << shift) & TI_C54X_U40_MASK;
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 = ti_c54x_u40_mask(
        ti_c54x_logical_immediate_apply(kind, source_raw40,
                                        shifted_immediate));
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
