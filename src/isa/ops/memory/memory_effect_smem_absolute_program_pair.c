/* Module contract
 *
 * Owns: structural validation of the Smem-absolute-to-program
 * DATA pair effect arm; see
 * memory_effect_smem_absolute_program_pair.h.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "memory_effect_smem_absolute_program_pair.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"

bool
ti_c54x_op_memory_effect_smem_absolute_program_pair_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
        return false;
    }
    /* Smem-absolute-program pair: the 16-bit absolute DATA leg
     * and the absolute 16-bit PROGRAM leg move one word with
     * the PROGRAM leg inverse to the effect access (MVDP reads
     * Smem); the staged value is zero (the second-leg value is
     * a bus observation the reducer replaces before issue). No
     * RMW, no T load, no delay, and no leaf completion: the
     * pair root owns the chaining internally. Reserved slots
     * stay zero. */
    if (effect->address_spec.smem_absolute_program.reserved_half !=
            UINT16_C(0) ||
        effect->address_spec.smem_absolute_program.reserved_word0 !=
            UINT32_C(0) ||
        effect->address_spec.smem_absolute_program.reserved_word1 !=
            UINT32_C(0) ||
        effect->address_spec.smem_absolute_program.reserved_word2 !=
            UINT32_C(0)) {
        return false;
    }
    if (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
        effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return false;
    }
    return effect->value_word == UINT16_C(0) &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->complete == NULL &&
           effect->transfer_reserved == UINT8_C(0) &&
           !effect->load_t_with_read && !effect->write_smem_delay;
}
