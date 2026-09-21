#ifndef TI_C54X_ISA_OPS_MEMORY_EFFECT_DUAL_H
#define TI_C54X_ISA_OPS_MEMORY_EFFECT_DUAL_H

/*
 * Module contract
 *
 * Owns: structural validation of the dual-indirect WORD16 READ
 * effect arm (TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16):
 * the X/Y mode and AR2..AR5 selectors with reserved-zero slots,
 * the staged-zero value, and the non-null dual completion; plus
 * the dual-indirect move-pair arm
 * (TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16),
 * which shares the arm and additionally requires WRITE access
 * with null completions. The move root owns the X READ / Y
 * WRITE chaining internally; plus the dual-data-plus-program
 * triple arm
 * (TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16),
 * which carries the X/Y modes and selectors exactly as the dual
 * arm with an absolute PROGRAM pmad, and additionally requires
 * READ access with the non-null triple completion. The triple
 * root owns the X/Y/PROGRAM chaining internally.
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

bool ti_c54x_op_memory_effect_dual_move_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

/* Dispatch one dual-family arm by address kind: the READ kind takes
 * the dual shape, the move-pair kind takes the move shape, and the
 * triple kind takes the triple shape with the triple arm check.
 * Any other kind rejects. */
bool ti_c54x_op_memory_effect_dual_kind_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_op_memory_dual_program_triple_address_is_valid(
    const struct ti_c54x_op_memory_address_spec *address_spec);

bool ti_c54x_op_memory_effect_triple_shape_is_valid(
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_ISA_OPS_MEMORY_EFFECT_DUAL_H */
