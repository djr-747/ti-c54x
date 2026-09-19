/* Module contract
 *
 * Owns: structural validation of the single-word Lmem LONG32 READ
 * effect arm (TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE): the
 * page-0/indirect leg selectors with reserved-zero slots, the
 * staged-zero MSW/LSW, and the non-null LONG32 completion. The arm
 * shape mirrors the LONG32 single-leg planner; MOD3 reads decline
 * at resolve time.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_LONG32_SINGLE_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_LONG32_SINGLE_H

#include <stdbool.h>

struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_long32_single_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_LONG32_SINGLE_H */
