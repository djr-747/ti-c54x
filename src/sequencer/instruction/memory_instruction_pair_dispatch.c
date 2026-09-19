/*
 * Module contract
 *
 * Owns: cold dispatch from a prepared root memory transaction to one of the
 * pair reducers (LONG32, dual-WORD16, DATA+PROGRAM-WORD16, PORT-WORD16,
 * or stack-pair-WORD16),
 * including
 * the status returned for an unknown child discriminator.
 * Does not own: WORD16 execution, transaction preparation, reducer state,
 * opcode semantics, physical memory, endpoint storage, timing, observers,
 * diagnostics, allocation, I/O, or global state. Callers must have already
 * validated their public arguments and selected a non-WORD16 child.
 */

#include "memory_instruction_pair_dispatch.h"

#include "memory_instruction_data_program.h"
#include "memory_instruction_dual.h"
#include "memory_instruction_long32.h"
#include "memory_instruction_port.h"
#include "memory_instruction_stack_pair.h"
#include "memory_instruction_smem_absolute_pair.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_pair_dispatch_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32) {
        return ti_c54x_memory_instruction_long32_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ) {
        return ti_c54x_memory_instruction_dual_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DATA_PROGRAM_WORD16_READ) {
        return ti_c54x_memory_instruction_data_program_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_PORT_WORD16_PAIR) {
        return ti_c54x_memory_instruction_port_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_STACK_PAIR_WORD16) {
        return ti_c54x_memory_instruction_stack_pair_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_ABSOLUTE_PAIR_WORD16) {
        return ti_c54x_memory_instruction_smem_absolute_pair_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_pair_dispatch_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32) {
        return ti_c54x_memory_instruction_long32_cancel(transaction, endpoint);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ) {
        return ti_c54x_memory_instruction_dual_cancel(transaction, endpoint);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DATA_PROGRAM_WORD16_READ) {
        return ti_c54x_memory_instruction_data_program_cancel(transaction,
                                                                endpoint);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_PORT_WORD16_PAIR) {
        return ti_c54x_memory_instruction_port_cancel(transaction, endpoint);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_STACK_PAIR_WORD16) {
        return ti_c54x_memory_instruction_stack_pair_cancel(transaction,
                                                             endpoint);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_ABSOLUTE_PAIR_WORD16) {
        return ti_c54x_memory_instruction_smem_absolute_pair_cancel(
            transaction, endpoint);
    }
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_pair_dispatch_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32) {
        return ti_c54x_memory_instruction_long32_reset(transaction);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ) {
        return ti_c54x_memory_instruction_dual_reset(transaction);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DATA_PROGRAM_WORD16_READ) {
        return ti_c54x_memory_instruction_data_program_reset(transaction);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_PORT_WORD16_PAIR) {
        return ti_c54x_memory_instruction_port_reset(transaction);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_STACK_PAIR_WORD16) {
        return ti_c54x_memory_instruction_stack_pair_reset(transaction);
    }
    if (transaction->child_kind ==
        TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_ABSOLUTE_PAIR_WORD16) {
        return ti_c54x_memory_instruction_smem_absolute_pair_reset(
            transaction);
    }
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
}
