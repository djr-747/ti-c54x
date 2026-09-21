#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_CHILD_ROUTE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_CHILD_ROUTE_H

/*
 * Module contract
 *
 * Owns: classification of a memory address kind into its pair
 * route (PORT, stack push/pop pair, Smem-absolute pair,
 * Smem-program pair, or dual-data-plus-coefficient triple) plus
 * snapshot-preparation dispatch for the routed pair owners and
 * for the dual move-pair shared-kind entry (plain move versus
 * store-compute).
 * Does not own: transaction orchestration, address resolution,
 * endpoint traffic, architectural mutation, opcode semantics,
 * timing, observers, diagnostics, allocation, I/O, or global
 * state. Transaction storage is borrowed for each call.
 */

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_memory_instruction_pair_route {
    TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_NONE = 0,
    TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_PORT_WORD16,
    TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_STACK_PAIR_WORD16,
    TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_SMEM_ABSOLUTE_PAIR_WORD16,
    TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_SMEM_PROGRAM_PAIR_WORD16,
    TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_TRIPLE_WORD16
} ti_c54x_memory_instruction_pair_route;

/* Map one address kind to its pair route; all other kinds map
 * to NONE. */
ti_c54x_memory_instruction_pair_route
ti_c54x_memory_instruction_pair_route_for_kind(
    ti_c54x_op_memory_address_kind kind);

/* Prepare the routed pair owner's snapshot. Returns false for
 * NONE and when the owner rejects. */
bool ti_c54x_memory_instruction_pair_route_prepare_snapshot(
    ti_c54x_memory_instruction_pair_route route,
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

/* Prepare the dual move-pair shared-kind entry: the
 * store-compute child owns the MOVE_PAIR spec with the
 * single-read completion set, the plain-move child with null
 * completions. The predicates are disjoint; the completed
 * effect prepares first so it never falls through to the move
 * root. Returns false when both owners reject. */
bool ti_c54x_memory_instruction_dual_move_pair_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_CHILD_ROUTE_H */
