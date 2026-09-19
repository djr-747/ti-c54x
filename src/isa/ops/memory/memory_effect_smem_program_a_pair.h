/* Module contract
 *
 * Owns: structural validation of the Smem-to-accumulator-
 * addressed-program DATA pair effect arm
 * (TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16): the
 * Smem selectors with the reserved immediate-pmad slot, the
 * staged-zero value, no RMW, no T load, no delay, and no leaf
 * completion (the pair root owns the chaining internally).
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_SMEM_PROGRAM_A_PAIR_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_SMEM_PROGRAM_A_PAIR_H

#include <stdbool.h>

struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_smem_program_a_pair_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_SMEM_PROGRAM_A_PAIR_H */
