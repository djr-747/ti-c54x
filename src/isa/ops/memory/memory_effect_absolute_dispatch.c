/* Module contract
 *
 * Owns: dispatch of the absolute-addressed memory effect
 * arms; see memory_effect_absolute_dispatch.h.
 * Does not own: other address kinds, the generic effect
 * gate, address resolution, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is
 * borrowed for one call; no pointer is retained and no
 * state is mutated.
 */

#include "memory_effect_absolute_dispatch.h"

#include <stdbool.h>
#include <stddef.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_effect_absolute_data.h"
#include "memory_effect_atomic_absolute.h"
#include "memory_effect_long32_absolute_load.h"
#include "memory_effect_long32_absolute_store.h"
#include "memory_effect_smem_absolute_pair.h"
#include "memory_effect_smem_absolute_program_pair.h"
#include "memory_effect_smem_program_a_pair.h"
#include "memory_effect_smem_program_pair.h"

bool
ti_c54x_op_memory_effect_absolute_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL) {
        return false;
    }
    switch (effect->address_spec.kind) {
    case TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16:
        return ti_c54x_op_memory_effect_absolute_data_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16:
        return ti_c54x_op_memory_effect_smem_absolute_pair_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16:
        return ti_c54x_op_memory_effect_smem_program_pair_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16:
        return ti_c54x_op_memory_effect_smem_program_a_pair_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16:
        return ti_c54x_op_memory_effect_smem_absolute_program_pair_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16:
        return ti_c54x_op_memory_effect_long32_absolute_store_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_LOAD_WORD16:
        return ti_c54x_op_memory_effect_long32_absolute_load_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16:
        return ti_c54x_op_memory_effect_atomic_absolute_shape_is_valid(
            effect);
    default:
        return false;
    }
}
