/* Module contract
 *
 * Owns: dispatch of the absolute-addressed memory effect
 * arms (absolute DATA WORD16, Smem absolute/pair arms,
 * LONG32 absolute store/load, atomic absolute) to their
 * owning shape validators.
 * Does not own: other address kinds, the generic effect
 * gate, address resolution, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is
 * borrowed for one call; no pointer is retained and no
 * state is mutated.
 */

#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_ABSOLUTE_DISPATCH_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_ABSOLUTE_DISPATCH_H

#include <stdbool.h>

struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_absolute_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_ABSOLUTE_DISPATCH_H */
