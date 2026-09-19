/* Module contract
 *
 * Owns: structural validation of the atomic absolute
 * WORD16 effect arm
 * (TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16):
 * the 16-bit absolute DATA address with reserved-zero
 * slots, a READ access carrying the staged immediate and
 * an RMW transform, and no leaf completion (the RMW
 * engine owns the read/transform/writeback internally).
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_ATOMIC_ABSOLUTE_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_ATOMIC_ABSOLUTE_H

#include <stdbool.h>

struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_atomic_absolute_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_ATOMIC_ABSOLUTE_H */
