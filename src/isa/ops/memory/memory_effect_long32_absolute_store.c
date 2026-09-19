/* Module contract
 *
 * Owns: structural validation of the absolute-Smem LONG32
 * store effect arm; see
 * memory_effect_long32_absolute_store.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#include "memory_effect_long32_absolute_store.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_long32_absolute_store_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16) {
        return false;
    }
    /* Absolute-Smem LONG32 store: the MSW lands at the
     * 16-bit absolute DATA address with the LSW above it
     * (MOD13 LONG32 convention). The halves ride the staged
     * value slots with WRITE access; reserved slots stay
     * zero. No RMW, no T load, no delay, and no leaf
     * completion: the long32 root owns the chaining
     * internally. */
    if (effect->address_spec.long32_absolute.reserved_half !=
            UINT16_C(0) ||
        effect->address_spec.long32_absolute.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.long32_absolute.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.long32_absolute.reserved_word2 !=
            UINT32_C(0)) {
        return false;
    }
    /* The MSW/LSW halves share union storage with the
     * single-word value/rmw slots (MOD13 LONG32 precedent),
     * so only the WRITE access and null completion gate here;
     * T load and Smem-delay write-back never ride a LONG32
     * store because the leaf zeroes the shared slots. */
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           effect->complete_long32 == NULL;
}
