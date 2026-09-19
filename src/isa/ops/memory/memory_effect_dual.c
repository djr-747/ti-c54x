/*
 * Module contract
 *
 * Owns: structural validation of the dual-indirect WORD16 READ
 * effect arm; see memory_effect_dual.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "memory_effect_dual.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_dual_address_is_valid(
    const struct ti_c54x_op_memory_address_spec *address_spec)
{
    if (address_spec == NULL) {
        return false;
    }
    return address_spec->dual.x_mode <= UINT8_C(3) &&
           address_spec->dual.y_mode <= UINT8_C(3) &&
           address_spec->dual.x_ar >= UINT8_C(2) &&
           address_spec->dual.x_ar <= UINT8_C(5) &&
           address_spec->dual.y_ar >= UINT8_C(2) &&
           address_spec->dual.y_ar <= UINT8_C(5) &&
           address_spec->dual.reserved_word0 == UINT32_C(0) &&
           address_spec->dual.reserved_word1 == UINT32_C(0) &&
           address_spec->dual.reserved_word2 == UINT32_C(0) &&
           address_spec->dual.reserved_word3 == UINT32_C(0);
}

bool
ti_c54x_op_memory_effect_dual_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL) {
        return false;
    }
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           effect->value_word == UINT16_C(0) &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->transfer_reserved == UINT8_C(0) &&
           effect->complete_dual != NULL;
}
