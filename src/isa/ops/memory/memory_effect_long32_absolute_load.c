/* Module contract
 *
 * Owns: structural validation of the absolute-Smem LONG32
 * load effect arm; see
 * memory_effect_long32_absolute_load.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#include "memory_effect_long32_absolute_load.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_long32_absolute_load_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_LOAD_WORD16) {
        return false;
    }
    /* Absolute-Smem LONG32 load: the MSW reads at the
     * 16-bit absolute DATA address with the LSW above it
     * (MOD13 LONG32 convention). The staged halves stay
     * zero with READ access and a non-null leaf long32
     * completion (the halves share union storage with the
     * single-word value/rmw slots, so a LONG32 READ
     * carries no single completion or RMW); T load and
     * Smem-delay write-back never ride a LONG32 load
     * because the leaf zeroes the shared slots. */
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
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           effect->first_value_msw == UINT16_C(0) &&
           effect->second_value_lsw == UINT16_C(0) &&
           effect->complete_long32 != NULL;
}
