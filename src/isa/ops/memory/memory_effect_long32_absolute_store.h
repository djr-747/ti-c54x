/* Module contract
 *
 * Owns: structural validation of the absolute-Smem LONG32
 * store effect arm
 * (TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16):
 * the 16-bit absolute DATA address with reserved-zero
 * slots, WRITE access carrying the staged MSW/LSW halves
 * (which share union storage with the single-word
 * value/rmw slots), and no leaf completion (the long32
 * root owns the chaining internally).
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_LONG32_ABSOLUTE_STORE_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_LONG32_ABSOLUTE_STORE_H

#include <stdbool.h>

struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_long32_absolute_store_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_LONG32_ABSOLUTE_STORE_H */
