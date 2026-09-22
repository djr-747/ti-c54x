/*
 * Module contract
 *
 * Owns: the allocation-free root transaction advance stage: the WORD16
 * issue/resume reducer with its far-call second-push, single-delay,
 * and RMW arms, dual-WORD16, LONG32, and pair dispatch, no-traffic
 * RTN-pop retirement from the frozen RTN value, and the MMR-routed
 * deferred-branch twins that report the deferred target in the
 * retirement result. Preparation (candidate validation, resolution,
 * and per-family freeze) lives in memory_instruction_prepare.c;
 * READY-path architecture commit lives in
 * memory_instruction_commit.c.
 * Does not own: CPU lifecycle, physical memory, endpoint context, opcode
 * dispatch, timing, observers, diagnostics, or global state.
 */

#include "memory_instruction_transaction.h"

#include <stdbool.h>
#include <stddef.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/mmr/mmr_route.h"
#include "sequencer/effect_transaction.h"
#include "sequencer/memory/mmr_access_plan.h"
#include "sequencer/program_pc_target.h"
#include "state/program_counter_extension.h"
#include "memory_instruction_far_push.h"
#include "memory_instruction_rmw.h"
#include "memory_instruction_single_delay.h"
#include "memory_instruction_stack_push.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_child_route.h"
#include "memory_instruction_commit.h"
#include "memory_instruction_data_program.h"
#include "memory_instruction_dual.h"
#include "memory_instruction_long32.h"
#include "memory_instruction_mmr_load.h"
#include "memory_instruction_pair_dispatch.h"

static void
clear_preserving_sequence(ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

static ti_c54x_memory_instruction_status
advance_rmw_write(ti_c54x_memory_instruction_transaction *transaction,
                  const ti_c54x_bus_word16_endpoint *endpoint,
                  ti_c54x_arch_state *state,
                  ti_c54x_memory_instruction_result *result,
                  bool first_issue)
{
    ti_c54x_bus_word16_adapter_result adapter_result;
    ti_c54x_memory_instruction_status operation_status;

    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);

    if (!ti_c54x_memory_instruction_candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_rmw_run_transform(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    {
        uint64_t next_id;

        if (!ti_c54x_memory_instruction_rmw_next_id(transaction, &next_id)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
        }
        if (!ti_c54x_memory_instruction_rmw_prepare_write(
                transaction, next_id, transaction->rmw_write_value_word)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    result->transaction_id = transaction->last_transaction_id;
    adapter_result = ti_c54x_bus_word16_adapter_advance(
        endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN,
        &transaction->bus_transaction);
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = adapter_result.bus_status;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    /* A READY WRITE is externally visible but architectural publication is
     * still local to this reducer. Recheck the frozen candidate and live
     * dependencies after the host callback before committing AR/PC. */
    if (!ti_c54x_memory_instruction_candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    operation_status = ti_c54x_memory_instruction_commit_architecture(transaction, state);
    if (operation_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
        return operation_status;
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

static ti_c54x_memory_instruction_status
advance_rmw_write_mmr(ti_c54x_memory_instruction_transaction *transaction,
                      ti_c54x_arch_state *state,
                      ti_c54x_accumulator_state *accumulators,
                      ti_c54x_t_register_state *t_register,
                      ti_c54x_memory_instruction_result *result)
{
    const ti_c54x_bus_word16_request *planned =
        &transaction->candidate.address_resolution.request;
    ti_c54x_bus_word16_request write_request;
    ti_c54x_sequencer_mmr_access write_plan;
    ti_c54x_bus_mmr_route_result write_result;
    ti_c54x_memory_instruction_status operation_status;

    /* Mirror the ordinary RMW write preflight: the staged MMR read
     * value transforms, then the write plans. No bus ID is
     * allocated and no endpoint issue happens on this path. */
    if (!ti_c54x_memory_instruction_candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_rmw_run_transform(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    write_plan = ti_c54x_sequencer_plan_mmr_access(
        state, accumulators, t_register, planned->space,
        planned->address_word,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
        transaction->rmw_write_value_word);
    if (write_plan != TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    write_request = *planned;
    write_request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    write_request.write_value_word = transaction->rmw_write_value_word;
    write_result = ti_c54x_bus_mmr_route(&write_request, state,
                                         accumulators, t_register);
    if (write_result.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    /* A served MMR WRITE is locally visible but architectural
     * publication is still local to this reducer. Recheck the
     * frozen candidate and live dependencies before committing. */
    if (!ti_c54x_memory_instruction_candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    operation_status = ti_c54x_memory_instruction_commit_architecture(transaction, state);
    if (operation_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
        return operation_status;
    }
    /* The locally served MMR read never issues a bus transaction, so
     * the consumed staged value must clear: retirement leaves the bus
     * transaction IDLE, and IDLE-canonical plus the tail reset require
     * the staged slot empty. */
    transaction->bus_transaction.staged_read_value_word = UINT16_C(0);
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_attempted = UINT64_C(1);
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    ti_c54x_bus_word16_adapter_result adapter_result;
    ti_c54x_memory_instruction_status operation_status;
    bool first_issue;
    bool rmw_read_phase;

    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        accumulators == NULL || t_register == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->child_kind != TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16) {
        return ti_c54x_memory_instruction_pair_dispatch_advance(
            transaction, endpoint, state, accumulators, t_register, result);
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED &&
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    first_issue = transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    if (first_issue) {
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE ||
            !ti_c54x_memory_instruction_candidate_shape_is_valid(
                &transaction->candidate)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        /* Read-phase MMR load (MVMD): snapshot the MMR into
         * the prepared DATA WRITE before first issue; decline
         * or unknown fail closed ahead of endpoint traffic. */
        if (!ti_c54x_memory_instruction_snapshot_mmr_load(
                transaction, state, accumulators, t_register)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (transaction->bus_transaction.state !=
                   TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
               transaction->last_transaction_id == UINT64_C(0) ||
               transaction->bus_transaction.transaction_id !=
                   transaction->last_transaction_id) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    {
        const ti_c54x_bus_word16_adapter_status endpoint_status =
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);
        if (endpoint_status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = TI_C54X_BUS_NO_CALLBACK;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (endpoint_status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    }

    if (!ti_c54x_memory_instruction_candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (ti_c54x_memory_instruction_single_delay_write_in_flight(
            transaction)) {
        /* Delay-write resume: the reused child carries the issued
         * delay WRITE. The delay owner resumes it and retires
         * through the shared READY tail. */
        return ti_c54x_memory_instruction_single_delay_resume(
            transaction, endpoint, state, accumulators, t_register,
            result);
    }
    if (ti_c54x_memory_instruction_far_push_write_in_flight(
            transaction)) {
        /* Far second-push resume: the reused child carries the
         * issued XPC WRITE. The far-push owner resumes it and
         * retires through the shared READY tail. */
        return ti_c54x_memory_instruction_far_push_resume(
            transaction, endpoint, state, accumulators, t_register,
            result);
    }
    if (transaction->candidate.effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        /* Fast interrupt returns retire with no bus traffic: the PC
         * target is the RTN value frozen at prepare, so no transaction
         * issues and no WAIT/FAULT can arise here. The retirement
         * report mirrors the ordinary READY tail with the deferred
         * branch disarmed (this kind is never delayed). */
        operation_status = ti_c54x_memory_instruction_commit_ready(
            transaction, state, accumulators, t_register,
            transaction->candidate.effect.payload.memory.value_word);
        if (operation_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
            return operation_status;
        }
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
        result->bus_status = TI_C54X_BUS_OK;
        result->instructions_attempted =
            first_issue ? UINT64_C(1) : UINT64_C(0);
        result->instructions_retired = UINT64_C(1);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (first_issue) {
        /* MMR-routed DATA traffic retires locally with no endpoint
         * issue: the router serves the register, then the ordinary
         * READY tail commits address and PC. Unknown reads and
         * guard-range writes reject transactionally before any
         * mutation; RMW reads stage locally and retire through the
         * routed RMW write below. The kind-keyed deferred tail is
         * replicated because delayed stack kinds can address the
         * MMR file. */
        const ti_c54x_bus_word16_request *planned =
            &transaction->candidate.address_resolution.request;
        const ti_c54x_sequencer_mmr_access mmr_plan =
            ti_c54x_sequencer_plan_mmr_access(
                state, accumulators, t_register, planned->space,
                planned->address_word, planned->access,
                planned->write_value_word);
        if (mmr_plan == TI_C54X_SEQUENCER_MMR_ACCESS_UNKNOWN ||
            mmr_plan == TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (mmr_plan == TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED &&
            ti_c54x_memory_instruction_single_delay_is_armed(transaction)) {
            /* The single-word delay write-back issues DATA endpoint
             * traffic to Smem+1 after the read: an MMR-routed Smem
             * has no endpoint read and MMR+1 write semantics are
             * unevidenced, so delay on MMR rejects transactionally
             * before any routing. */
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (ti_c54x_memory_instruction_far_push_is_armed(transaction)) {
            /* The far second push issues DATA endpoint traffic to
             * old SP - 2 after the first WRITE. An MMR-routed
             * first TOS has no endpoint first WRITE; an
             * endpoint first TOS with an MMR-routed second TOS
             * would serve the register instead of the endpoint:
             * MMR-1 write semantics are unevidenced either way,
             * so far pushes touching MMR reject transactionally
             * before any routing. */
            if (mmr_plan == TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
                result->status =
                    TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
            {
                const ti_c54x_sequencer_mmr_access second_plan =
                    ti_c54x_sequencer_plan_mmr_access(
                        state, accumulators, t_register,
                        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                        ti_c54x_memory_instruction_far_push_target_address(
                            transaction),
                        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                        transaction->far_push_xpc_word);
                if (second_plan !=
                    TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT) {
                    result->status =
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                }
            }
        }
        if (mmr_plan == TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
            ti_c54x_bus_mmr_route_result mmr_result =
                ti_c54x_bus_mmr_route(planned, state, accumulators,
                                      t_register);
            uint16_t mmr_read_word;

            if (mmr_result.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
                result->status =
                    TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
            mmr_read_word = mmr_result.read_value_word;
            if (ti_c54x_memory_instruction_memory_is_rmw(
                    &transaction->candidate.effect.payload.memory) &&
                planned->access ==
                    TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
                transaction->bus_transaction.staged_read_value_word =
                    mmr_read_word;
                return advance_rmw_write_mmr(transaction, state,
                                             accumulators, t_register,
                                             result);
            }
            operation_status = ti_c54x_memory_instruction_commit_ready(
                transaction, state, accumulators, t_register,
                mmr_read_word);
            if (operation_status !=
                TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
                return operation_status;
            }
            /* Delayed stack branches retire their stack update here
             * but defer the transfer exactly like the endpoint tail:
             * the arming keys off the address kind, which a routed
             * MMR address can carry. */
            if (transaction->candidate.effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16) {
                result->deferred_branch_armed = true;
                result->deferred_branch_pc_word = mmr_read_word;
            } else if (ti_c54x_op_memory_address_kind_is_delayed_push(
                           transaction->candidate.effect.payload.memory
                               .address_spec.kind)) {
                ti_c54x_memory_instruction_status report_status;

                /* Delayed push reporting (bundle or accumulator
                 * target) lives in the push owner. */
                report_status =
                    ti_c54x_memory_instruction_stack_push_deferred_report(
                        &transaction->candidate, result);
                if (report_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_OK) {
                    return report_status;
                }
            }
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
            result->bus_status = TI_C54X_BUS_OK;
            result->instructions_attempted = UINT64_C(1);
            result->instructions_retired = UINT64_C(1);
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
    }
    if (first_issue) {
        uint64_t next_id;

        if (transaction->last_transaction_id == UINT64_MAX) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
        }
        next_id = transaction->last_transaction_id + UINT64_C(1);
        if (next_id == UINT64_C(0)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
        }
        if (ti_c54x_bus_word16_transaction_prepare(
                &transaction->bus_transaction,
                &transaction->candidate.address_resolution.request,
                next_id) != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        transaction->last_transaction_id = next_id;
        result->transaction_id = next_id;
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    } else {
        result->transaction_id = transaction->last_transaction_id;
    }

    rmw_read_phase =
        ti_c54x_memory_instruction_memory_is_rmw(
            &transaction->candidate.effect.payload.memory) &&
        transaction->bus_transaction.request.access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;

    adapter_result = ti_c54x_bus_word16_adapter_advance(
        endpoint,
        first_issue ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                    : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
        &transaction->bus_transaction);
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status ==
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = adapter_result.bus_status;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    /* Recheck the frozen candidate after READY before publication. */
    if (!rmw_read_phase &&
        (!ti_c54x_memory_instruction_candidate_snapshot_matches(transaction) ||
         !ti_c54x_memory_instruction_candidate_matches_live(
             &transaction->candidate, state))) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (rmw_read_phase) {
        return advance_rmw_write(transaction, endpoint, state, result,
                                 first_issue);
    }
    /* Single-word delay write-back: the READ leg just completed, so
     * the delay WRITE issues before any publication. The delay owner
     * issues it and retires through the shared READY tail. */
    if (ti_c54x_memory_instruction_single_delay_is_armed(transaction) &&
        transaction->bus_transaction.request.access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return ti_c54x_memory_instruction_single_delay_issue(
            transaction, endpoint, state, accumulators, t_register,
            result, adapter_result.read_value_word, first_issue);
    }
    /* Far-call second push: the first TOS WRITE just completed, so
     * the XPC WRITE issues before any publication. The first and
     * second legs are disjoint by TOS address, so a completed
     * second WRITE falls through to the shared READY tail below.
     * The far-push owner issues it and retires through the same
     * tail. */
    if (ti_c54x_memory_instruction_far_push_first_write_complete(
            transaction)) {
        return ti_c54x_memory_instruction_far_push_issue(
            transaction, endpoint, state, accumulators, t_register,
            result, first_issue);
    }
    return ti_c54x_memory_instruction_commit_ready_tail(
        transaction, state, accumulators, t_register, result,
        adapter_result.read_value_word, first_issue);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (transaction == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->child_kind != TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16) {
        return ti_c54x_memory_instruction_pair_dispatch_cancel(transaction,
                                                                endpoint);
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING ||
        transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
        transaction->last_transaction_id == UINT64_C(0) ||
        transaction->bus_transaction.transaction_id !=
            transaction->last_transaction_id) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    {
        const ti_c54x_bus_word16_adapter_result adapter_result =
            ti_c54x_bus_word16_adapter_cancel(
                endpoint, &transaction->bus_transaction);
        if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
        }
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (transaction == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    /* A freshly zero-initialized or cleared root has child_kind zero.  It is
     * canonical only while IDLE and must retain the ordinary reset path;
     * prepared/active non-WORD16 values belong to the cold pair dispatcher. */
    if (transaction->child_kind != TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16 &&
        !(transaction->child_kind == UINT8_C(0) &&
          transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE)) {
        return ti_c54x_memory_instruction_pair_dispatch_reset(transaction);
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else if (transaction->phase ==
                   TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE) {
        /* The ordinary READY tail pairs COMPLETE with a completed bus
         * transaction. The no-traffic RTN-pop retirement never issues
         * a request, so its exact postcondition is COMPLETE with the
         * bus transaction still IDLE; both pairings reset here. */
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
            transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT &&
            transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_bus_word16_transaction_reset(&transaction->bus_transaction) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
