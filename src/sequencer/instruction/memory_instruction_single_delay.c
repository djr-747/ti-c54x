/*
 * Module contract
 *
 * Owns: the single-word Smem delay write-back stage (LTD/DELAY): armed
 * recognition, DATA delay-target derivation from the frozen candidate
 * resolution, issue/resume of the delay WRITE through the reused
 * WORD16 child with a fresh transaction ID, and the root-level
 * resume/issue orchestration that maps each outcome to the root
 * phase and result and retires through the shared READY tail. A
 * delay FAULT leaves the architecture untouched because the tail
 * only runs after READY.
 * Does not own: carrier validity, address resolution, MMR routing,
 * completions, the READY tail itself, endpoint storage, timing,
 * observers, or diagnostics. All inputs are borrowed for one call;
 * the transaction retains only the stashed read word, the child
 * WRITE, and the ID watermark.
 */

#include "memory_instruction_single_delay.h"

#include <stddef.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/request/word16_request.h"
#include "bus/transaction/word16_transaction.h"
#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "memory_instruction_commit.h"
#include "sequencer/instruction/memory_instruction_transaction.h"

bool
ti_c54x_memory_instruction_single_delay_is_armed(
    const struct ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;

    if (transaction == NULL ||
        transaction->child_kind !=
            TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16 ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    return memory->write_smem_delay &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           memory->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           (memory->address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16 ||
            memory->address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
}

bool
ti_c54x_memory_instruction_single_delay_write_in_flight(
    const struct ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_single_delay_is_armed(transaction) ||
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return false;
    }
    return transaction->bus_transaction.state ==
               TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT &&
           transaction->bus_transaction.request.access ==
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
}

uint16_t
ti_c54x_memory_instruction_single_delay_target_address(
    const struct ti_c54x_memory_instruction_transaction *transaction)
{
    uint32_t base;

    base = (uint32_t)transaction->candidate.address_resolution.request
               .address_word;
    return (uint16_t)((base + UINT32_C(1)) & UINT32_C(0xffff));
}

ti_c54x_memory_instruction_single_delay_status
ti_c54x_memory_instruction_single_delay_advance(
    struct ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, uint16_t read_value_word,
    bool begin, ti_c54x_bus_status *bus_status)
{
    ti_c54x_bus_word16_transaction *child;
    ti_c54x_bus_word16_adapter_result adapter_result;
    uint16_t target;

    if (transaction == NULL || endpoint == NULL ||
        !ti_c54x_memory_instruction_single_delay_is_armed(transaction) ||
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
    }
    child = &transaction->bus_transaction;
    target =
        ti_c54x_memory_instruction_single_delay_target_address(transaction);
    if (begin) {
        ti_c54x_bus_word16_request_result built;
        uint64_t next_id;

        /* The READ leg just completed: the reused child still
         * carries it. Stash the observation before the child is
         * reset for the WRITE so a WAIT keeps it for the later
         * commit. */
        if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
        }
        if (transaction->last_transaction_id == UINT64_MAX) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_ID_EXHAUSTED;
        }
        next_id = transaction->last_transaction_id + UINT64_C(1);
        if (next_id == UINT64_C(0)) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_ID_EXHAUSTED;
        }
        transaction->single_delay_read_value_word = read_value_word;
        if (ti_c54x_bus_word16_transaction_reset(child) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
        }
        /* The delay write-back is DATA-space endpoint traffic by
         * the MACD pair precedent; there is no MMR branch. */
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, (uint32_t)target,
            read_value_word);
        if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
        }
        transaction->last_transaction_id = next_id;
        if (ti_c54x_bus_word16_transaction_prepare(child, &built.request,
                                                  next_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
        }
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, child);
    } else {
        /* Delay-write resume: pin the in-flight child WRITE against
         * the frozen candidate target and the stashed word so an
         * edit across the WAIT boundary cannot redirect the write. */
        if (!ti_c54x_memory_instruction_single_delay_write_in_flight(
                transaction) ||
            transaction->last_transaction_id == UINT64_C(0) ||
            child->transaction_id != transaction->last_transaction_id ||
            child->request.space !=
                TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
            child->request.address_word != target ||
            child->request.write_value_word !=
                transaction->single_delay_read_value_word) {
            return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
        }
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, child);
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_WAIT;
    }
    if (adapter_result.status ==
        TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION) {
        return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = adapter_result.bus_status;
        }
        return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_FAULT;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED;
    }
    return TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_READY;
}

/* Map a delay outcome to the root phase and result exactly like the
 * endpoint tail: WAIT and FAULT report through OK, ID exhaustion and
 * malformed observations report directly, and READY runs the shared
 * READY tail with the stashed READ observation. */
static ti_c54x_memory_instruction_status
report_delay_outcome(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result,
    ti_c54x_memory_instruction_single_delay_status delay_status,
    ti_c54x_bus_status delay_bus_status, bool first_issue)
{
    if (delay_status == TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_WAIT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        result->transaction_id = transaction->last_transaction_id;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (delay_status == TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = delay_bus_status;
        result->transaction_id = transaction->last_transaction_id;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (delay_status ==
        TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_ID_EXHAUSTED) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
    }
    if (delay_status != TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_READY) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    result->transaction_id = transaction->last_transaction_id;
    return ti_c54x_memory_instruction_commit_ready_tail(
        transaction, state, accumulators, t_register, result,
        transaction->single_delay_read_value_word, first_issue);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_single_delay_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    ti_c54x_memory_instruction_single_delay_status delay_status;
    ti_c54x_bus_status delay_bus_status = TI_C54X_BUS_OK;

    if (transaction == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    delay_status = ti_c54x_memory_instruction_single_delay_advance(
        transaction, endpoint, UINT16_C(0), false, &delay_bus_status);
    return report_delay_outcome(transaction, state, accumulators,
                                t_register, result, delay_status,
                                delay_bus_status, false);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_single_delay_issue(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, uint16_t read_value_word,
    bool first_issue)
{
    ti_c54x_memory_instruction_single_delay_status delay_status;
    ti_c54x_bus_status delay_bus_status = TI_C54X_BUS_OK;

    if (transaction == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    delay_status = ti_c54x_memory_instruction_single_delay_advance(
        transaction, endpoint, read_value_word, true, &delay_bus_status);
    return report_delay_outcome(transaction, state, accumulators,
                                t_register, result, delay_status,
                                delay_bus_status, first_issue);
}
