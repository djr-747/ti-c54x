/*
 * Module contract
 *
 * Owns: structural validation of the generic SINGLE_INDIRECT MOD0--MOD3,
 * INDEXED_AR0 MOD5/MOD6, and CIRCULAR MOD8/MOD10 WORD16 memory effect
 * carriers.
 * Does not own: address state, bus execution, memory visibility, opcode
 * identity, timing, or diagnostics.
 */

#include "memory_effect.h"

#include <stddef.h>

static bool
ti_c54x_op_memory_effect_access_shape_is_valid(
    const ti_c54x_op_memory_effect *effect)
{
    if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return effect->write_value_word == UINT16_C(0) &&
               effect->complete != NULL;
    }
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           effect->complete == NULL;
}

bool
ti_c54x_op_memory_effect_is_valid(const ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL) {
        return false;
    }

    switch (effect->address_spec.kind) {
    case TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16:
        if (effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.mode < TI_C54X_INDIRECT_SINGLE_MOD0 ||
            effect->address_spec.mode > TI_C54X_INDIRECT_SINGLE_MOD3 ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
            effect->address_spec.mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.indexed_mode <
                TI_C54X_INDIRECT_INDEXED_MOD5 ||
            effect->address_spec.indexed_mode >
                TI_C54X_INDIRECT_INDEXED_MOD6 ||
            effect->address_spec.arf == UINT8_C(0) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            (effect->address_spec.circular_mode !=
                 TI_C54X_INDIRECT_CIRCULAR_MOD8 &&
             effect->address_spec.circular_mode !=
                 TI_C54X_INDIRECT_CIRCULAR_MOD10) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_INVALID:
    default:
        return false;
    }
}
