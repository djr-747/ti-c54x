/* Module contract
 *
 * Owns: structural validation of the single-word Lmem LONG32 READ
 * effect arm; see memory_effect_long32_single.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "memory_effect_long32_single.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_long32_single_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE) {
        return false;
    }
    /* Single-word Lmem LONG32 READ: the direct leg carries the
     * page-0 dma with a reserved-zero DP slot; the indirect leg
     * carries a MOD0--MOD3 selector with its AR (MOD3 reads decline
     * at resolve time). Staged MSW/LSW are zero with a non-null
     * LONG32 completion (the completion slots share one union, so
     * a LONG32 READ carries no single completion or RMW). Arm shape
     * mirrors the LONG32 single-leg planner. */
    if (effect->address_spec.long32_single.indirect > UINT8_C(1) ||
        effect->address_spec.long32_single.dp_page != UINT16_C(0) ||
        effect->address_spec.long32_single.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.long32_single.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.long32_single.reserved_word2 !=
            UINT32_C(0)) {
        return false;
    }
    if (effect->address_spec.long32_single.indirect == UINT8_C(0)) {
        if (effect->address_spec.long32_single.mode != UINT8_C(0) ||
            effect->address_spec.long32_single.arf != UINT8_C(0) ||
            effect->address_spec.long32_single.dma7 > UINT8_C(0x7F)) {
            return false;
        }
    } else if (effect->address_spec.long32_single.mode > UINT8_C(3) ||
               effect->address_spec.long32_single.arf > UINT8_C(7) ||
               effect->address_spec.long32_single.dma7 != UINT8_C(0)) {
        return false;
    }
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           effect->first_value_msw == UINT16_C(0) &&
           effect->second_value_lsw == UINT16_C(0) &&
           effect->complete_long32 != NULL;
}
