#ifndef TI_C54X_CPU_MEMORY_PENDING_WORK_H
#define TI_C54X_CPU_MEMORY_PENDING_WORK_H

/*
 * Module contract
 *
 * Owns: the CPU-owned integration seam for validating, resuming, and
 * cancelling one persistent generic data-memory root transaction. It returns
 * value-owned root status/deltas to the CPU orchestrator.
 * Does not own: public CPU counters/results, trace emission, opcode dispatch,
 * architectural state, endpoint storage, allocation, timing, or diagnostics.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "bus/adapter/word16_adapter.h"

#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_cpu_memory_resume_result {
    ti_c54x_memory_instruction_status transaction_status;
    ti_c54x_memory_instruction_result result;
} ti_c54x_cpu_memory_resume_result;

ti_c54x_bus_word16_adapter_status ti_c54x_cpu_memory_validate_endpoint(
    const ti_c54x_bus_word16_endpoint *endpoint);

bool ti_c54x_cpu_memory_is_pending(
    const ti_c54x_memory_instruction_transaction *transaction);

ti_c54x_cpu_memory_resume_result ti_c54x_cpu_memory_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register);

ti_c54x_memory_instruction_status ti_c54x_cpu_memory_reset_transaction(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

#ifdef __cplusplus
}
#endif

#endif
