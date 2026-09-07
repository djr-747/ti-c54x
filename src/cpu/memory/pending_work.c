/*
 * Module contract
 *
 * Owns: CPU-owned composition of the persistent generic data-memory root
 * transaction's resume and cancellation calls.
 * Does not own: public CPU counters/results, trace emission, opcode
 * dispatch, architectural state, endpoint storage, allocation, timing, or
 * diagnostics.
 */

#include "pending_work.h"

#include "bus/adapter/word16_adapter.h"

#include <stddef.h>

ti_c54x_bus_word16_adapter_status
ti_c54x_cpu_memory_validate_endpoint(
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    return ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);
}

bool
ti_c54x_cpu_memory_is_pending(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL &&
           (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED ||
            transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING);
}

ti_c54x_cpu_memory_resume_result
ti_c54x_cpu_memory_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register)
{
    ti_c54x_cpu_memory_resume_result result = {0};

    result.transaction_status = ti_c54x_memory_instruction_transaction_advance(
        transaction, endpoint, state, accumulators, t_register,
        &result.result);
    return result;
}

ti_c54x_memory_instruction_status
ti_c54x_cpu_memory_reset_transaction(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    ti_c54x_memory_instruction_status status;

    if (transaction == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        status = ti_c54x_memory_instruction_transaction_cancel(transaction,
                                                               endpoint);
        if (status != TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED) {
            return status;
        }
    } else if (transaction->phase ==
               TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        status = ti_c54x_memory_instruction_transaction_reset(transaction);
        if (status != TI_C54X_MEMORY_INSTRUCTION_STATUS_OK) {
            return status;
        }
    }
    return ti_c54x_memory_instruction_transaction_reset(transaction);
}
