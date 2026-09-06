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
ti_c54x_op_nop(const ti_c54x_decode_result *instruction,
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
        instruction->status != TI_C54X_DECODE_STATUS_OK ||
        instruction->id != metadata->id ||
        instruction->first_word != metadata->first_word_value ||
        instruction->program_word_count != metadata->program_word_count) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    staged.kind = TI_C54X_OP_EFFECT_NONE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
