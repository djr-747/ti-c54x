/* Module contract
 *
 * Owns: structural validation of the absolute DATA WORD16 READ
 * effect arm; see memory_effect_absolute_data.h.
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
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16) {
        return false;
    }
    if (effect->address_spec.absolute_data.reserved_half !=
            UINT16_C(0) ||
        effect->address_spec.absolute_data.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.absolute_data.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.absolute_data.reserved_word2 !=
            UINT32_C(0) ||
        effect->address_spec.absolute_data.reserved_word3 !=
            UINT32_C(0)) {
        return false;
    }
    if (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return false;
    }
    /* The read-phase MMR store sidecar rides existing padding: a
     * set flag requires no RMW transform, and a clear flag
     * requires a zero destination and spare. Completion presence
     * stays with the dispatcher. */
    if (effect->mmr_store_with_read) {
        if (effect->rmw_operation !=
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE) {
            return false;
        }
    } else if (effect->mmr_store_number != UINT8_C(0)) {
        return false;
    }
    return effect->value_word == UINT16_C(0) &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->transfer_reserved == UINT8_C(0);
}
