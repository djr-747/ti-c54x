/*
 * Module contract
 *
 * Owns: structural validation of the generic MOD0--MOD3 WORD16 memory
 * effect carrier.
 * Does not own: address state, bus execution, memory visibility, opcode
 * identity, timing, or diagnostics.
 */

#include "memory_effect.h"

#include <stddef.h>

bool
ti_c54x_op_memory_effect_is_valid(const ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16 ||
        effect->address_spec.mode < TI_C54X_INDIRECT_SINGLE_MOD0 ||
        effect->address_spec.mode > TI_C54X_INDIRECT_SINGLE_MOD3 ||
        effect->address_spec.arf > UINT8_C(7)) {
        return false;
    }
    if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        if (effect->address_spec.mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
            return false;
        }
        return effect->write_value_word == UINT16_C(0) &&
               effect->complete != NULL;
    }
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           effect->complete == NULL;
}
