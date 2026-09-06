/*
 * Module contract
 *
 * Owns: checked semantic execution of the one-word NOP descriptor.
 * Does not own: opcode inventory, decode, program-counter sequencing, CPU or
 * status state, memory, buses, timing, diagnostics, or observer callbacks.
 * The descriptor is borrowed for the call and the caller-owned effect is
 * unchanged on every failure.
 */

#include "op_nop.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"

ti_c54x_op_status
ti_c54x_op_nop(const ti_c54x_decoded_instruction *instruction,
               ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_op_effect staged;

    if (instruction == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_NOP ||
        metadata->first_word_mask != UINT16_MAX ||
        instruction->id != metadata->id ||
        metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        metadata->program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        instruction->bundle.program_words[0] != metadata->first_word_value ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    staged.kind = TI_C54X_OP_EFFECT_NONE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
