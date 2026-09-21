/*
 * Module contract
 *
 * Owns: root orchestration for one dual-indirect WORD16
 * store-compute pair; see
 * memory_instruction_dual_store_compute.h.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. Transaction storage is
 * borrowed for each call.
 */

#include "memory_instruction_dual_store_compute.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/mmr/mmr_route.h"
#include "bus/request/word16_request.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_commit.h"
#include "memory_instruction_dual_move_candidate_validate.h"
#include "memory_instruction_dual_store_compute_commit.h"
#include "sequencer/memory/address_resolution.h"
#include "sequencer/memory/dual_address_resolution.h"
#include "sequencer/memory/mmr_access_plan.h"
#include "state/arch_state.h"

static void
clear_preserving_sequence(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

bool
ti_c54x_memory_instruction_dual_store_compute_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL && transaction->child_kind ==
                                      TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_STORE_COMPUTE;
}

bool
ti_c54x_memory_instruction_dual_store_compute_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;
    ti_c54x_dual_memory_address_resolution resolution = {0};

    if (transaction == NULL || state == NULL ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    /* The MOVE root owns the same spec kind with a null
     * completion; the store-compute child owns it with the
     * single-read completion set. The predicates are disjoint,
     * so preparation order between the two is irrelevant. */
    if (arm->kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16 ||
        memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        memory->complete == NULL) {
        return false;
    }
    if (ti_c54x_sequencer_resolve_dual_move_pair(
            state, arm, &resolution) !=
        TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return false;
    }
    transaction->candidate.dual_address_snapshot.x_request =
        resolution.x_request;
    transaction->candidate.dual_address_snapshot.y_request =
        resolution.y_request;
    transaction->candidate.dual_address_snapshot.x_commit =
        resolution.x_address_commit;
    transaction->candidate.dual_address_snapshot.y_commit =
        resolution.y_address_commit;
    transaction->candidate.dual_address_snapshot.ar0_shadow =
        resolution.ar0_value;
    transaction->candidate.dual_address_snapshot.bk_shadow =
        resolution.bk_value;
    /* The resolver zero-stages the Y WRITE template; the
     * store-compute child stamps the leaf-frozen ST value over it.
     * The shared tail echoes the stamp against the frozen effect
     * value, so a post-freeze mutation of either side fails the
     * shape rechecks at every issue. */
    transaction->candidate.dual_address_snapshot.y_request.write_value_word =
        memory->value_word;
    /* The full candidate shape (leaf arm, move snapshot legs,
     * counters, PC bridge) is provable only now that the frozen
     * snapshot is populated; the advance path rechecks it plus
     * liveness before each issue. The shared move shape check
     * admits the store-compute effect (non-null completion with
     * any frozen ST value). */
    if (!ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate)) {
        return false;
    }
    /* Second-leg flag rides the free RMW byte; the X-side READ
     * observation stages in the store-compute stash slot once
     * the first leg resolves. */
    transaction->candidate_rmw_operation = UINT8_C(0);
    transaction->store_compute_x_observed_word = UINT16_C(0);
    transaction->child_kind =
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_STORE_COMPUTE;
    return true;
}

static bool
next_id(ti_c54x_memory_instruction_transaction *transaction,
        uint64_t *out_id)
{
    uint64_t id;

    if (transaction->last_transaction_id == UINT64_MAX) {
        return false;
    }
    id = transaction->last_transaction_id + UINT64_C(1);
    if (id == UINT64_C(0)) {
        return false;
    }
    transaction->last_transaction_id = id;
    *out_id = id;
    return true;
}

static bool
prepare_leg(ti_c54x_memory_instruction_transaction *transaction,
            const ti_c54x_bus_word16_request *request, uint64_t id)
{
    if (ti_c54x_bus_word16_transaction_reset(&transaction->bus_transaction) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK ||
        ti_c54x_bus_word16_transaction_prepare(&transaction->bus_transaction,
                                               request, id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return false;
    }
    return true;
}

/* MMR leg service through the S1 seam: ENDPOINT legs return for
 * the endpoint path; ROUTED legs serve locally with the read
 * observation (writes carry none); any other plan or an
 * unserviced route rejects transactionally with no endpoint
 * traffic and no state mutation. Mirrors the dual-move root. */
typedef enum dual_store_compute_leg_path {
    DUAL_STORE_COMPUTE_LEG_ENDPOINT = 0,
    DUAL_STORE_COMPUTE_LEG_ROUTED = 1,
    DUAL_STORE_COMPUTE_LEG_REJECT = 2
} dual_store_compute_leg_path;

static dual_store_compute_leg_path
plan_leg_path(const ti_c54x_bus_word16_request *request,
              ti_c54x_arch_state *state,
              ti_c54x_accumulator_state *accumulators,
              ti_c54x_t_register_state *t_register,
              uint16_t *observed_word)
{
    ti_c54x_sequencer_mmr_access plan;
    ti_c54x_bus_mmr_route_result routed;

    plan = ti_c54x_sequencer_plan_mmr_access(
        state, accumulators, t_register, request->space,
        request->address_word, request->access,
        request->write_value_word);
    if (plan == TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT) {
        return DUAL_STORE_COMPUTE_LEG_ENDPOINT;
    }
    if (plan != TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
        return DUAL_STORE_COMPUTE_LEG_REJECT;
    }
    routed = ti_c54x_bus_mmr_route(request, state, accumulators,
                                   t_register);
    if (routed.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
        return DUAL_STORE_COMPUTE_LEG_REJECT;
    }
    *observed_word = routed.read_value_word;
    return DUAL_STORE_COMPUTE_LEG_ROUTED;
}

static bool
snapshot_matches(const ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_dual_store_compute_is_candidate(
            transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        transaction->candidate_rmw_operation > UINT8_C(1)) {
        return false;
    }
    /* The Y WRITE template carries the leaf-frozen ST value for
     * the whole pair; the shared move shape check admits any
     * staged value because the frozen candidate itself is the
     * tamper witness. */
    return ti_c54x_memory_instruction_dual_move_candidate_shape_is_valid(
        &transaction->candidate.dual_address_snapshot,
        &transaction->candidate.effect.payload.memory);
}

static bool
build_second_leg_request(
    const ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_bus_word16_request *out_request)
{
    ti_c54x_bus_word16_request_result built;
    uint32_t address;

    address = transaction->candidate.dual_address_snapshot.y_request
                  .address_word;
    built = ti_c54x_bus_word16_request_write(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, address,
        transaction->candidate.effect.payload.memory.value_word);
    if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return false;
    }
    *out_request = built.request;
    return true;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    const bool first_issue =
        transaction != NULL &&
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    ti_c54x_bus_word16_adapter_result adapter_result;
    ti_c54x_memory_instruction_status operation_status;
    bool first_leg_routed = false;
    uint16_t first_observed_word = UINT16_C(0);

    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        result == NULL ||
        !ti_c54x_memory_instruction_dual_store_compute_is_candidate(
            transaction) ||
        (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED &&
         transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate) ||
        !snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        if (result != NULL) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    {
        const ti_c54x_bus_word16_adapter_status endpoint_status =
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);

        if (endpoint_status ==
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = TI_C54X_BUS_NO_CALLBACK;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (endpoint_status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    }
    if (first_issue) {
        ti_c54x_bus_word16_request leg =
            transaction->candidate.dual_address_snapshot.x_request;
        uint64_t id;
        dual_store_compute_leg_path path;

        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        /* MMR-routed legs serve synchronously with no bus ID and
         * no endpoint issue; the observation feeds the stash
         * below in this same call. */
        path = plan_leg_path(&leg, state, accumulators, t_register,
                             &first_observed_word);
        if (path == DUAL_STORE_COMPUTE_LEG_REJECT) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        if (path == DUAL_STORE_COMPUTE_LEG_ROUTED) {
            first_leg_routed = true;
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        } else {
            if (!next_id(transaction, &id) ||
                !prepare_leg(transaction, &leg, id)) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                result->status =
                    transaction->last_transaction_id == UINT64_MAX
                        ? TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED
                        : TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                return result->status;
            }
            result->transaction_id = id;
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        }
    } else {
        result->transaction_id = transaction->last_transaction_id;
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
            transaction->last_transaction_id == UINT64_C(0) ||
            transaction->bus_transaction.transaction_id !=
                transaction->last_transaction_id) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
    }
    /* A synchronously routed first leg skips the endpoint
     * entirely; every PENDING resume still finds an in-flight
     * endpoint leg, so the resume invariant below is unchanged. */
    if (!first_leg_routed) {
        adapter_result = ti_c54x_bus_word16_adapter_advance(
            endpoint,
            first_issue ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                        : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
            &transaction->bus_transaction);
        if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
            result->instructions_attempted =
                first_issue ? UINT64_C(1) : UINT64_C(0);
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = adapter_result.bus_status;
            result->instructions_attempted =
                first_issue ? UINT64_C(1) : UINT64_C(0);
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            result->instructions_attempted =
                first_issue ? UINT64_C(1) : UINT64_C(0);
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        first_observed_word =
            transaction->bus_transaction.staged_read_value_word;
    }
    /* Recheck the frozen candidate after READY before stashing
     * the X word for the completion commit. */
    if (!snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (transaction->candidate_rmw_operation == UINT8_C(0)) {
        ti_c54x_bus_word16_request leg;
        uint64_t id;
        dual_store_compute_leg_path path;
        uint16_t routed_word = UINT16_C(0);

        transaction->store_compute_x_observed_word = first_observed_word;
        if (!build_second_leg_request(transaction, &leg)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        /* The built second leg plans the MMR seam against its
         * real store value (first-leg templates plan zero): the
         * AG/BG guard words reject here before any mutation. */
        path = plan_leg_path(&leg, state, accumulators, t_register,
                             &routed_word);
        if (path == DUAL_STORE_COMPUTE_LEG_REJECT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        (void)routed_word;
        if (path == DUAL_STORE_COMPUTE_LEG_ENDPOINT &&
            (!next_id(transaction, &id) ||
             !prepare_leg(transaction, &leg, id))) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        transaction->candidate_rmw_operation = UINT8_C(1);
        if (path == DUAL_STORE_COMPUTE_LEG_ENDPOINT) {
            result->transaction_id = id;
            adapter_result = ti_c54x_bus_word16_adapter_advance(
                endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN,
                &transaction->bus_transaction);
            if (adapter_result.status ==
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
                result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
                result->instructions_attempted = UINT64_C(0);
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
            }
            if (adapter_result.status ==
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
                result->bus_status = adapter_result.bus_status;
                result->instructions_attempted = UINT64_C(0);
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
            }
            if (adapter_result.status !=
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
                result->status =
                    TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
                result->instructions_attempted = UINT64_C(0);
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
            }
        }
        if (!snapshot_matches(transaction) ||
            !ti_c54x_memory_instruction_candidate_shape_is_valid(
                &transaction->candidate) ||
            !ti_c54x_memory_instruction_candidate_matches_live(
                &transaction->candidate, state)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
    }
    operation_status =
        ti_c54x_memory_instruction_dual_store_compute_commit_ready(
            transaction, state, accumulators, t_register,
            transaction->store_compute_x_observed_word);
    if (operation_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
        return operation_status;
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (!ti_c54x_memory_instruction_dual_store_compute_is_candidate(
            transaction)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
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
            ti_c54x_bus_word16_adapter_cancel(endpoint,
                                              &transaction->bus_transaction);
        if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
        }
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_dual_store_compute_is_candidate(
            transaction) ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else if (transaction->phase ==
                   TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE ||
               transaction->phase ==
                   TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
            transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE &&
            transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
