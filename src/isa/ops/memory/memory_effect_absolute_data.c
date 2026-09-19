/* Module contract
 *
 * Owns: structural validation of the absolute DATA WORD16
 * READ/WRITE effect arm; see memory_effect_absolute_data.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "memory_effect_absolute_data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_absolute_data_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    uint8_t sidecar_kind;
    uint8_t sidecar_number;

    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16) {
        return false;
    }
    if (effect->address_spec.absolute_data.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.absolute_data.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.absolute_data.reserved_word2 !=
            UINT32_C(0) ||
        effect->address_spec.absolute_data.reserved_word3 !=
            UINT32_C(0)) {
        return false;
    }
    sidecar_kind =
        effect->address_spec.absolute_data.mmr_sidecar_kind;
    sidecar_number =
        effect->address_spec.absolute_data.mmr_sidecar_number;
    /* The read-phase MMR legs ride existing padding: a set leg
     * requires no RMW transform, and a clear leg requires a zero
     * transfer slot. Completion presence stays with the
     * dispatcher. */
    if (effect->rmw_operation !=
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE ||
        effect->transfer_reserved != UINT8_C(0)) {
        return false;
    }
    if (sidecar_kind !=
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE &&
        sidecar_kind !=
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ &&
        sidecar_kind !=
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE) {
        return false;
    }
    if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        /* READ legs carry no staged value: plain absolute reads
         * take no sidecar, while the store leg publishes the
         * read word after the read resolves. */
        if (effect->value_word != UINT16_C(0)) {
            return false;
        }
        if (sidecar_kind ==
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE) {
            return sidecar_number == UINT8_C(0);
        }
        return sidecar_kind ==
               (uint8_t)
                   TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ;
    }
    if (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return false;
    }
    /* WRITE legs exist only with a load leg: the leaf stages zero
     * and the resume path snapshots the MMR into the request, so
     * a nonzero staged value is a forged bypass of the snapshot. */
    return sidecar_kind ==
               (uint8_t)
                   TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE &&
           effect->value_word == UINT16_C(0);
}
