/*
 * Module contract
 *
 * Owns: the private, allocation-free far-call second-push stage
 * (FCALL[D]/FCALA[D]) used by the root memory transaction, as
 * declared in memory_instruction_far_push.h.
 * Does not own: carrier validity, address resolution, MMR routing,
 * completions, the READY tail itself, endpoint storage, timing,
 * observers, or diagnostics.
 */

#include "memory_instruction_far_push.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/request/word16_request.h"
#include "bus/transaction/word16_transaction.h"
#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "memory_instruction_commit.h"
#include "memory_instruction_rmw.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/program_counter_extension.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_push_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_op_memory_address_kind kind,
    const ti_c54x_arch_state *state)
{
    uint16_t frozen_xpc = UINT16_C(0);

    if (transaction == NULL || state == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_op_memory_address_kind_is_far_push(kind)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    /* Far double-pushes stage the live XPC beside the return
     * address: the second TOS WRITE carries it to old SP - 2.
     * Unknown XPC rejects before any bus issue; the frozen word
     * never diverges (only far control modifies XPC and none
     * retires mid-transaction), so no liveness guard reads it. */
    if (ti_c54x_arch_state_read_xpc(state, &frozen_xpc) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    transaction->far_push_xpc_word = frozen_xpc;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

bool
ti_c54x_memory_instruction_far_push_is_armed(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;

    if (transaction == NULL ||
        transaction->child_kind !=
            TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16 ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    return ti_c54x_op_memory_address_kind_is_far_push(
               memory->address_spec.kind) &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           !ti_c54x_memory_instruction_memory_is_rmw(memory);
}

bool
ti_c54x_memory_instruction_far_push_write_in_flight(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_far_push_is_armed(transaction) ||
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return false;
    }
    return transaction->bus_transaction.state ==
               TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT &&
           transaction->bus_transaction.request.access ==
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           transaction->bus_transaction.request.space ==
               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           transaction->bus_transaction.request.address_word ==
               ti_c54x_memory_instruction_far_push_target_address(
                   transaction) &&
           transaction->bus_transaction.request.write_value_word ==
               transaction->far_push_xpc_word;
}

uint16_t
ti_c54x_memory_instruction_far_push_target_address(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    uint32_t base;

    base = (uint32_t)transaction->candidate.address_resolution.request
               .address_word;
    return (uint16_t)((base - UINT32_C(1)) & UINT32_C(0xffff));
}

bool
ti_c54x_memory_instruction_far_push_first_write_complete(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_far_push_is_armed(transaction)) {
        return false;
    }
    return transaction->bus_transaction.state ==
               TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
           transaction->bus_transaction.request.access ==
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           transaction->bus_transaction.request.space ==
               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           transaction->bus_transaction.request.address_word ==
               transaction->candidate.address_resolution.request
                   .address_word &&
           transaction->bus_transaction.request.write_value_word ==
               transaction->candidate.effect.payload.memory.value_word;
}

ti_c54x_memory_instruction_far_push_status
ti_c54x_memory_instruction_far_push_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, bool begin,
    ti_c54x_bus_status *bus_status)
{
    ti_c54x_bus_word16_transaction *child;
    ti_c54x_bus_word16_adapter_result adapter_result;
    uint16_t target;

    if (transaction == NULL || endpoint == NULL ||
        !ti_c54x_memory_instruction_far_push_is_armed(transaction) ||
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
    }
    child = &transaction->bus_transaction;
    target =
        ti_c54x_memory_instruction_far_push_target_address(transaction);
    if (begin) {
        ti_c54x_bus_word16_request_result built;
        uint64_t next_id;

        /* The first TOS WRITE just completed: the reused child
         * still carries it. The frozen XPC rides the root slot
         * (frozen at prepare), so no stash is needed before the
         * child is reset for the second WRITE. */
        if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
        }
        if (transaction->last_transaction_id == UINT64_MAX) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_ID_EXHAUSTED;
        }
        next_id = transaction->last_transaction_id + UINT64_C(1);
        if (next_id == UINT64_C(0)) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_ID_EXHAUSTED;
        }
        if (ti_c54x_bus_word16_transaction_reset(child) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
        }
        /* The second push is DATA-space endpoint traffic by the
         * worked-example precedent (stack at 1110h/110Fh); there
         * is no MMR branch. */
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, (uint32_t)target,
            transaction->far_push_xpc_word);
        if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
        }
        transaction->last_transaction_id = next_id;
        if (ti_c54x_bus_word16_transaction_prepare(child, &built.request,
                                                  next_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
        }
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, child);
    } else {
        /* Second-push resume: pin the in-flight child WRITE against
         * the frozen candidate target and the frozen XPC so an edit
         * across the WAIT boundary cannot redirect the write. */
        if (!ti_c54x_memory_instruction_far_push_write_in_flight(
                transaction) ||
            transaction->last_transaction_id == UINT64_C(0) ||
            child->transaction_id != transaction->last_transaction_id) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
        }
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, child);
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_WAIT;
    }
    if (adapter_result.status ==
        TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = adapter_result.bus_status;
        }
        return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_FAULT;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED;
    }
    return TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_READY;
}

/* Map a far-push outcome to the root phase and result exactly like
 * the endpoint tail: WAIT and FAULT report through OK, ID
 * exhaustion and malformed observations report directly, and READY
 * runs the shared READY tail. The tail receives a zero staged word
 * (both TOS WRITEs carry staged values, never observations); the
 * architecture commit publishes the SP-2 resolution with the far
 * PC+XPC redirect. */
static ti_c54x_memory_instruction_status
report_far_push_outcome(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result,
    ti_c54x_memory_instruction_far_push_status push_status,
    ti_c54x_bus_status push_bus_status, bool first_issue)
{
    if (push_status == TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_WAIT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        result->transaction_id = transaction->last_transaction_id;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (push_status == TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = push_bus_status;
        result->transaction_id = transaction->last_transaction_id;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (push_status ==
        TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_ID_EXHAUSTED) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
    }
    if (push_status != TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_READY) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    result->transaction_id = transaction->last_transaction_id;
    return ti_c54x_memory_instruction_commit_ready_tail(
        transaction, state, accumulators, t_register, result,
        UINT16_C(0), first_issue);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_push_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    ti_c54x_memory_instruction_far_push_status push_status;
    ti_c54x_bus_status push_bus_status = TI_C54X_BUS_OK;

    if (transaction == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    push_status = ti_c54x_memory_instruction_far_push_advance(
        transaction, endpoint, false, &push_bus_status);
    return report_far_push_outcome(transaction, state, accumulators,
                                   t_register, result, push_status,
                                   push_bus_status, false);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_push_issue(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, bool first_issue)
{
    ti_c54x_memory_instruction_far_push_status push_status;
    ti_c54x_bus_status push_bus_status = TI_C54X_BUS_OK;

    if (transaction == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    push_status = ti_c54x_memory_instruction_far_push_advance(
        transaction, endpoint, true, &push_bus_status);
    return report_far_push_outcome(transaction, state, accumulators,
                                   t_register, result, push_status,
                                   push_bus_status, first_issue);
}
