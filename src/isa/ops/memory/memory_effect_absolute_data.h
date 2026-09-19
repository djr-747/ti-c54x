/* Module contract
 *
 * Owns: structural validation of the absolute DATA WORD16 READ
 * effect arm (TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16):
 * the second-word address with reserved-zero slots, the
 * staged-zero value, the optional read-phase MMR store sidecar,
 * and the null single completion with no RMW.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect argument is borrowed for
 * one call; no pointer is retained and no state is mutated.
 */

#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_ABSOLUTE_DATA_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_ABSOLUTE_DATA_H

#include <stdbool.h>

struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_absolute_data_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_ABSOLUTE_DATA_H */
