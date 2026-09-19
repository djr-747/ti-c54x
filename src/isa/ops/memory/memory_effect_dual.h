#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_DUAL_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_DUAL_H

/*
 * Module contract
 *
 * Owns: structural validation of the dual-indirect WORD16 READ
 * effect arm (TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16):
 * the X/Y mode and AR2..AR5 selectors with reserved-zero slots,
 * the staged-zero value, and the non-null dual completion.
 * Does not own: other address kinds, address resolution, request
 * issue, endpoint traffic, opcode semantics, CPU lifecycle, timing,
 * observers, or diagnostics. The effect arguments are borrowed
 * for one call; no pointer is retained and no state is mutated.
 */

#include <stdbool.h>

struct ti_c54x_op_memory_address_spec;
struct ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_dual_address_is_valid(
    const struct ti_c54x_op_memory_address_spec *address_spec);

bool ti_c54x_op_memory_effect_dual_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_DUAL_H */
