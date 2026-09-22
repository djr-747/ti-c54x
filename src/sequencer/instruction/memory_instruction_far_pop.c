/*
 * Module contract
 *
 * Owns: the private, allocation-free far-return second-pop stage
 * (FRET[D]/FRETE[D]) used by the root memory transaction, as
 * declared in memory_instruction_far_pop.h.
 * Does not own: carrier validity, address resolution, MMR routing,
 * completions, the READY tail itself, endpoint storage, timing,
 * observers, or diagnostics.
 */

#include "memory_instruction_far_pop.h"

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
#include "sequencer/program_pc_target.h"
#include "state/program_counter_extension.h"
#include "state/register_layout.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_op_memory_address_kind kind,
    ti_c54x_op_memory_complete_fn complete)
{
    if (transaction == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_op_memory_address_kind_is_far_pop(kind)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    /* The inte kinds (FRETE[D]) clear INTM through the staged
     * completion: a forged candidate without one would retire
     * the far return with interrupts still disabled, so it
     * rejects before any bus issue. Plain kinds carry no
     * completion. */
    if ((kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_INTE_WORD16 ||
         kind ==
             TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_INTE_DELAYED_WORD16) &&
        complete == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

bool
ti_c54x_memory_instruction_far_pop_is_armed(
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
    return ti_c54x_op_memory_address_kind_is_far_pop(
               memory->address_spec.kind) &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           !ti_c54x_memory_instruction_memory_is_rmw(memory);
}

bool
ti_c54x_memory_instruction_far_pop_read_in_flight(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_far_pop_is_armed(transaction) ||
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return false;
    }
    return transaction->bus_transaction.state ==
               TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT &&
           transaction->bus_transaction.request.access ==
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           transaction->bus_transaction.request.space ==
               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           transaction->bus_transaction.request.address_word ==
               ti_c54x_memory_instruction_far_pop_target_address(
                   transaction);
}

uint16_t
ti_c54x_memory_instruction_far_pop_target_address(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    uint32_t base;

    base = (uint32_t)transaction->candidate.address_resolution.request
               .address_word;
    return (uint16_t)((base + UINT32_C(1)) & UINT32_C(0xffff));
}

bool
ti_c54x_memory_instruction_far_pop_first_read_complete(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_far_pop_is_armed(transaction)) {
        return false;
    }
    return transaction->bus_transaction.state ==
               TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
           transaction->bus_transaction.request.access ==
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           transaction->bus_transaction.request.space ==
               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           transaction->bus_transaction.request.address_word ==
               transaction->candidate.address_resolution.request
                   .address_word;
}

ti_c54x_memory_instruction_far_pop_status
ti_c54x_memory_instruction_far_pop_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, uint16_t first_read_word,
    bool begin, uint16_t *second_read_word_out,
    ti_c54x_bus_status *bus_status)
{
    ti_c54x_bus_word16_transaction *child;
    ti_c54x_bus_word16_adapter_result adapter_result;
    uint16_t target;

    if (transaction == NULL || endpoint == NULL ||
        second_read_word_out == NULL ||
        !ti_c54x_memory_instruction_far_pop_is_armed(transaction) ||
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
    }
    child = &transaction->bus_transaction;
    target =
        ti_c54x_memory_instruction_far_pop_target_address(transaction);
    if (begin) {
        ti_c54x_bus_word16_request_result built;
        uint64_t next_id;

        /* The first TOS READ just completed: the reused child
         * still carries it. The return extension page rides the
         * root slot, so it is stashed before the child is reset
         * for the second READ. */
        if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
        }
        if (transaction->last_transaction_id == UINT64_MAX) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_ID_EXHAUSTED;
        }
        next_id = transaction->last_transaction_id + UINT64_C(1);
        if (next_id == UINT64_C(0)) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_ID_EXHAUSTED;
        }
        transaction->far_pop_xpc_word = first_read_word;
        if (ti_c54x_bus_word16_transaction_reset(child) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
        }
        /* The second pop is DATA-space endpoint traffic by the
         * RET TOS precedent; there is no MMR branch. */
        built = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, (uint32_t)target);
        if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
        }
        transaction->last_transaction_id = next_id;
        if (ti_c54x_bus_word16_transaction_prepare(child, &built.request,
                                                  next_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
        }
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, child);
    } else {
        /* Second-pop resume: pin the in-flight child READ against
         * the frozen candidate target so an edit across the WAIT
         * boundary cannot redirect the read. */
        if (!ti_c54x_memory_instruction_far_pop_read_in_flight(
                transaction) ||
            transaction->last_transaction_id == UINT64_C(0) ||
            child->transaction_id != transaction->last_transaction_id) {
            return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
        }
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, child);
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_WAIT;
    }
    if (adapter_result.status ==
        TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = adapter_result.bus_status;
        }
        return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_FAULT;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED;
    }
    *second_read_word_out = adapter_result.read_value_word;
    return TI_C54X_MEMORY_INSTRUCTION_FAR_POP_READY;
}

/* Map a far-pop outcome to the root phase and result exactly like
 * the endpoint tail: WAIT and FAULT report through OK, ID
 * exhaustion and malformed observations report directly, and
 * READY runs the shared READY tail. The tail receives the
 * second-leg observation (the return address); the first-leg
 * observation rides the root far-pop slot into the far
 * architecture commit. */
static ti_c54x_memory_instruction_status
report_far_pop_outcome(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result,
    ti_c54x_memory_instruction_far_pop_status pop_status,
    ti_c54x_bus_status pop_bus_status, uint16_t second_read_word,
    bool first_issue)
{
    if (pop_status == TI_C54X_MEMORY_INSTRUCTION_FAR_POP_WAIT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        result->transaction_id = transaction->last_transaction_id;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (pop_status == TI_C54X_MEMORY_INSTRUCTION_FAR_POP_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = pop_bus_status;
        result->transaction_id = transaction->last_transaction_id;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (pop_status ==
        TI_C54X_MEMORY_INSTRUCTION_FAR_POP_ID_EXHAUSTED) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
    }
    if (pop_status != TI_C54X_MEMORY_INSTRUCTION_FAR_POP_READY) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    result->transaction_id = transaction->last_transaction_id;
    return ti_c54x_memory_instruction_commit_ready_tail(
        transaction, state, accumulators, t_register, result,
        second_read_word, first_issue);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    ti_c54x_memory_instruction_far_pop_status pop_status;
    ti_c54x_bus_status pop_bus_status = TI_C54X_BUS_OK;
    uint16_t second_read_word = UINT16_C(0);

    if (transaction == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    pop_status = ti_c54x_memory_instruction_far_pop_advance(
        transaction, endpoint, UINT16_C(0), false, &second_read_word,
        &pop_bus_status);
    return report_far_pop_outcome(transaction, state, accumulators,
                                  t_register, result, pop_status,
                                  pop_bus_status, second_read_word,
                                  false);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_issue(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, uint16_t first_read_word,
    bool first_issue)
{
    ti_c54x_memory_instruction_far_pop_status pop_status;
    ti_c54x_bus_status pop_bus_status = TI_C54X_BUS_OK;
    uint16_t second_read_word = UINT16_C(0);

    if (transaction == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    pop_status = ti_c54x_memory_instruction_far_pop_advance(
        transaction, endpoint, first_read_word, true, &second_read_word,
        &pop_bus_status);
    return report_far_pop_outcome(transaction, state, accumulators,
                                  t_register, result, pop_status,
                                  pop_bus_status, second_read_word,
                                  first_issue);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_commit_architecture(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, uint16_t pcl_value)
{
    const ti_c54x_sequencer_address_commit *const address_commit =
        &transaction->candidate.address_resolution.address_commit;
    ti_c54x_program_pc_target_effect pop_target = {0};
    uint32_t live_pc = state->pc_word;

    /* Plan the popped return-address target before mutating
     * anything: a failure here is unreachable (prepare proved
     * plannability with the same live PC) but stays
     * transactional. Delayed pops only prove the deferred
     * target here: the two delay-slot words execute first, so
     * PC advances to the frozen sequential successor and the
     * CPU loop reports the popped address from the retirement
     * result. The return extension page commits to XPC on both
     * timings, mirroring the far-call retirement that commits
     * the staged page while arming the countdown. */
    if (address_commit->target != TI_C54X_ADDRESS_UPDATE_SP ||
        ti_c54x_program_pc_target_plan(live_pc, pcl_value, &pop_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    state->sp = address_commit->value_after;
    state->scalar_known_mask |= (uint8_t)TI_C54X_ARCH_STATE_KNOWN_SP;
    if (transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_DELAYED_WORD16 ||
        transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_INTE_DELAYED_WORD16) {
        state->pc_word =
            transaction->candidate.pc_effect.public_pc_word_after;
    } else {
        if (ti_c54x_program_pc_target_apply(&live_pc, &pop_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        state->pc_word = live_pc;
    }
    if (ti_c54x_arch_state_write_xpc(
            state, transaction->far_pop_xpc_word) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}
