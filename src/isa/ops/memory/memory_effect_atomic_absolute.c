/* Module contract
 *
 * Owns: structural validation of the atomic absolute
 * WORD16 effect arm; see memory_effect_atomic_absolute.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#include "memory_effect_atomic_absolute.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_atomic_absolute_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16) {
        return false;
    }
    /* Atomic absolute WORD16: the RMW engine reads the cell
     * at the 16-bit absolute DATA address, transforms with
     * the staged immediate, and writes back. Reserved slots
     * stay zero; the transform rides `rmw_operation` with
     * the immediate in the value slot, so only the READ
     * access and null completion gate here. */
    if (effect->address_spec.atomic_absolute.reserved_half !=
            UINT16_C(0) ||
        effect->address_spec.atomic_absolute.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.atomic_absolute.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.atomic_absolute.reserved_word2 !=
            UINT32_C(0)) {
        return false;
    }
    if (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return false;
    }
    if (effect->rmw_operation !=
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD) {
        return false;
    }
    return effect->complete == NULL;
}
