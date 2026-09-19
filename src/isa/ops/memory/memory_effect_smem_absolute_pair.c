/* Module contract
 *
 * Owns: structural validation of the Smem-to-absolute DATA pair
 * effect arm; see memory_effect_smem_absolute_pair.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "memory_effect_smem_absolute_pair.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_smem_absolute_pair_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
        return false;
    }
    /* Smem-to-absolute pair: the Smem DATA leg is read first and
     * the observed word is written to `dmad`; the staged value is
     * zero (the second-leg value is a bus observation the reducer
     * replaces before issue). No RMW, no T load, no delay, and no
     * leaf completion: the pair root owns the chaining
     * internally. Direct legs carry the DP-relative offset;
     * indirect legs carry the MOD0--MOD3/ARF selector with no
     * direct address. MVDK completes WRITE; MVKD reuses this
     * carrier with READ. */
    if (effect->address_spec.smem_absolute.indirect > UINT8_C(1) ||
        effect->address_spec.smem_absolute.reserved_half !=
            UINT16_C(0) ||
        effect->address_spec.smem_absolute.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.smem_absolute.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.smem_absolute.reserved_word2 !=
            UINT32_C(0)) {
        return false;
    }
    if (effect->address_spec.smem_absolute.indirect == UINT8_C(0)) {
        if (effect->address_spec.smem_absolute.smem_mode !=
                UINT8_C(0) ||
            effect->address_spec.smem_absolute.smem_arf !=
                UINT8_C(0) ||
            effect->address_spec.smem_absolute.smem_dma7 >
                UINT8_C(0x7F)) {
            return false;
        }
    } else if (effect->address_spec.smem_absolute.smem_dma7 !=
                   UINT8_C(0) ||
               effect->address_spec.smem_absolute.smem_arf >
                   UINT8_C(7) ||
               effect->address_spec.smem_absolute.smem_mode >
                   UINT8_C(3)) {
        return false;
    }
    if (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return false;
    }
    return effect->value_word == UINT16_C(0) &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->complete == NULL &&
           effect->transfer_reserved == UINT8_C(0) &&
           !effect->load_t_with_read && !effect->write_smem_delay;
}
