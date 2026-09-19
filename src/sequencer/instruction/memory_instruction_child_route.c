/*
 * Module contract
 *
 * Owns: classification of a memory address kind into its pair
 * route plus snapshot-preparation dispatch for the routed pair
 * owners; see memory_instruction_child_route.h.
 * Does not own: transaction orchestration, address resolution,
 * endpoint traffic, architectural mutation, opcode semantics,
 * timing, observers, diagnostics, allocation, I/O, or global
 * state. Transaction storage is borrowed for each call.
 */

#include "memory_instruction_child_route.h"

#include "memory_instruction_port.h"
#include "memory_instruction_smem_absolute_pair.h"
#include "memory_instruction_smem_program_pair.h"
#include "memory_instruction_stack_pair.h"

ti_c54x_memory_instruction_pair_route
ti_c54x_memory_instruction_pair_route_for_kind(
    ti_c54x_op_memory_address_kind kind)
{
    switch (kind) {
    case TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16:
        return TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_PORT_WORD16;
    case TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16:
    case TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16:
        return TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_STACK_PAIR_WORD16;
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16:
        return TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_SMEM_ABSOLUTE_PAIR_WORD16;
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16:
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16:
        return TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_SMEM_PROGRAM_PAIR_WORD16;
    default:
        return TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_NONE;
    }
}

bool
ti_c54x_memory_instruction_pair_route_prepare_snapshot(
    ti_c54x_memory_instruction_pair_route route,
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    switch (route) {
    case TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_PORT_WORD16:
        return ti_c54x_memory_instruction_port_prepare_snapshot(
            transaction, state);
    case TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_STACK_PAIR_WORD16:
        return ti_c54x_memory_instruction_stack_pair_prepare_snapshot(
            transaction, state);
    case TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_SMEM_ABSOLUTE_PAIR_WORD16:
        return ti_c54x_memory_instruction_smem_absolute_pair_prepare_snapshot(
            transaction, state);
    case TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_SMEM_PROGRAM_PAIR_WORD16:
        return ti_c54x_memory_instruction_smem_program_pair_prepare_snapshot(
            transaction, state);
    default:
        return false;
    }
}
