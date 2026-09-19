/*
 * Module contract
 *
 * Owns: root orchestration for one stack push/pop pair: Smem
 * DATA-leg synthesis through the existing single-leg resolver,
 * TOS stack-leg resolution through the generic stack push/pop
 * resolver (both legs frozen in the pair snapshot because the
 * candidate's generic slot aliases that storage; the SP commit
 * rides the routed architecture commit), deferred two-request
 * issue with observation handoff, and terminal
 * cancellation/reset. PSHD reads Smem first and writes the
 * observed word to TOS; POPD reads TOS first and writes the
 * observed word to Smem. No architectural field is written
 * until both legs and all candidate and live-state checks pass.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, or global state. Transaction storage is borrowed
 * for each call.
 */

#include "memory_instruction_stack_pair.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/mmr/mmr_route.h"
#include "bus/request/word16_request.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_commit.h"
#include "sequencer/memory/address_resolution.h"
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
ti_c54x_memory_instruction_stack_pair_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL &&
           transaction->child_kind ==
               TI_C54X_MEMORY_INSTRUCTION_CHILD_STACK_PAIR_WORD16;
}

bool
ti_c54x_memory_instruction_stack_pair_synthesize_data_spec(
    const ti_c54x_op_memory_address_spec *arm,
    ti_c54x_op_memory_address_spec *data_spec)
{
    ti_c54x_op_memory_address_spec staged = {0};

    if (arm == NULL || data_spec == NULL) {
        return false;
    }
    /* Direct-only slice: indirect arms reject here; the leaf
     * already rejects them, so this is defense in depth. */
    if (arm->stack_pair.indirect != UINT8_C(0) ||
        arm->stack_pair.smem_mode != UINT8_C(0) ||
        arm->stack_pair.smem_arf != UINT8_C(0) ||
        arm->stack_pair.smem_dma7 > UINT8_C(0x7F)) {
        return false;
    }
    staged.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
    staged.direct_dma7 = arm->stack_pair.smem_dma7;
    *data_spec = staged;
    return true;
}

bool
ti_c54x_memory_instruction_stack_pair_synthesize_stack_spec(
    const ti_c54x_op_memory_address_spec *arm,
    ti_c54x_op_memory_address_kind *stack_kind)
{
    if (arm == NULL || stack_kind == NULL) {
        return false;
    }
    if (arm->kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16) {
        *stack_kind = TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16;
        return true;
    }
    if (arm->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) {
        *stack_kind = TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16;
        return true;
    }
    return false;
}

static bool
is_push_pair(const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction->candidate.effect.payload.memory.access ==
           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
}

bool
ti_c54x_memory_instruction_stack_pair_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;
    ti_c54x_op_memory_address_spec data_spec = {0};
    ti_c54x_op_memory_address_kind stack_kind =
        TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    ti_c54x_op_memory_address_spec stack_spec = {0};
    ti_c54x_memory_address_resolution data_resolution = {0};
    ti_c54x_memory_address_resolution stack_resolution = {0};
    ti_c54x_bus_word16_request_access data_access;

    if (transaction == NULL || state == NULL ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    if (arm->kind !=
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 &&
        arm->kind != TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) {
        return false;
    }
    if (!ti_c54x_memory_instruction_stack_pair_synthesize_data_spec(
            arm, &data_spec)) {
        return false;
    }
    if (!ti_c54x_memory_instruction_stack_pair_synthesize_stack_spec(
            arm, &stack_kind)) {
        return false;
    }
    stack_spec.kind = stack_kind;
    /* The Smem DATA leg runs inverse to the effect access (the
     * access names the stack-leg direction). */
    data_access =
        (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE)
            ? TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            : TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &data_spec, data_access, UINT16_C(0),
            &data_resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return false;
    }
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &stack_spec, memory->access, UINT16_C(0),
            &stack_resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return false;
    }
    /* Both legs land in the pair snapshot: the generic slot
     * shares union storage with it, so it holds no stack-pair
     * commit. The SP commit rides the routed architecture commit
     * from the snapshot's stack-leg resolution. */
    transaction->candidate.stack_pair_address_snapshot.stack_resolution =
        stack_resolution;
    transaction->candidate.stack_pair_address_snapshot.data_resolution =
        data_resolution;
    transaction->candidate.stack_pair_address_snapshot.second_leg_issued =
        UINT8_C(0);
    transaction->child_kind =
        TI_C54X_MEMORY_INSTRUCTION_CHILD_STACK_PAIR_WORD16;
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

static ti_c54x_bus_word16_request
first_leg_request(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    /* PSHD (access WRITE) reads Smem first; POPD (access READ)
     * reads TOS first. Both legs live in the pair snapshot. */
    if (is_push_pair(transaction)) {
        return transaction->candidate.stack_pair_address_snapshot
            .data_resolution.request;
    }
    return transaction->candidate.stack_pair_address_snapshot
        .stack_resolution.request;
}

static bool
build_second_leg_request(
    const ti_c54x_memory_instruction_transaction *transaction,
    uint16_t observed_word, ti_c54x_bus_word16_request *out_request)
{
    ti_c54x_bus_word16_request_result built;
    uint32_t address;

    if (is_push_pair(transaction)) {
        /* PSHD writes the Smem observation to TOS. */
        address = transaction->candidate.stack_pair_address_snapshot
                      .stack_resolution.request.address_word;
    } else {
        /* POPD writes the TOS observation to Smem. */
        address = transaction->candidate.stack_pair_address_snapshot
                      .data_resolution.request.address_word;
    }
    built = ti_c54x_bus_word16_request_write(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, address, observed_word);
    if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return false;
    }
    *out_request = built.request;
    return true;
}

/* MMR leg service through the S1 seam: ENDPOINT legs return for
 * the endpoint path; ROUTED legs serve locally with the read
 * observation (writes carry none); any other plan or an
 * unserviced route rejects transactionally with no endpoint
 * traffic and no state mutation. Mirrors the generic first-issue
 * MMR branch, including the probe-then-serve READ double access
 * the plan itself performs. */
typedef enum stack_pair_leg_path {
    STACK_PAIR_LEG_ENDPOINT = 0,
    STACK_PAIR_LEG_ROUTED = 1,
    STACK_PAIR_LEG_REJECT = 2
} stack_pair_leg_path;

static stack_pair_leg_path
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
        return STACK_PAIR_LEG_ENDPOINT;
    }
    if (plan != TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
        return STACK_PAIR_LEG_REJECT;
    }
    routed = ti_c54x_bus_mmr_route(request, state, accumulators,
                                   t_register);
    if (routed.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
        return STACK_PAIR_LEG_REJECT;
    }
    *observed_word = routed.read_value_word;
    return STACK_PAIR_LEG_ROUTED;
}

static bool
snapshot_matches(const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;

    if (!ti_c54x_memory_instruction_stack_pair_is_candidate(transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    if ((arm->kind !=
             TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 &&
         arm->kind !=
             TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) ||
        transaction->candidate.stack_pair_address_snapshot
                .second_leg_issued > UINT8_C(1)) {
        return false;
    }
    /* The WRITE template stays zero until the observed word
     * replaces it at second-leg issue; re-derive both leg specs
     * from the frozen arm on every check. */
    {
        ti_c54x_op_memory_address_spec data_spec = {0};
        ti_c54x_op_memory_address_kind stack_kind =
            TI_C54X_OP_MEMORY_ADDRESS_INVALID;

        if (!ti_c54x_memory_instruction_stack_pair_synthesize_data_spec(
                arm, &data_spec)) {
            return false;
        }
        if (!ti_c54x_memory_instruction_stack_pair_synthesize_stack_spec(
                arm, &stack_kind)) {
            return false;
        }
        if (data_spec.direct_dma7 != arm->stack_pair.smem_dma7) {
            return false;
        }
        (void)stack_kind;
    }
    return true;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_pair_advance(
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
        !ti_c54x_memory_instruction_stack_pair_is_candidate(transaction) ||
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
        ti_c54x_bus_word16_request leg = first_leg_request(transaction);
        uint64_t id;
        stack_pair_leg_path path;

        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        /* MMR-routed legs serve synchronously with no bus ID and
         * no endpoint issue; the observation feeds the second leg
         * below in this same call. */
        path = plan_leg_path(&leg, state, accumulators, t_register,
                             &first_observed_word);
        if (path == STACK_PAIR_LEG_REJECT) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        if (path == STACK_PAIR_LEG_ROUTED) {
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
    /* Recheck the frozen candidate after READY before publication. */
    if (!snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return result->status;
    }
    if (transaction->candidate.stack_pair_address_snapshot.second_leg_issued ==
        UINT8_C(0)) {
        ti_c54x_bus_word16_request leg;
        uint64_t id;
        stack_pair_leg_path path;
        uint16_t routed_word = UINT16_C(0);

        if (!build_second_leg_request(transaction, first_observed_word,
                                      &leg)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        /* The built second leg plans the MMR seam against its
         * real store value (first-leg templates plan zero): the
         * AG/BG guard words reject here before any mutation. */
        path = plan_leg_path(&leg, state, accumulators, t_register,
                             &routed_word);
        if (path == STACK_PAIR_LEG_REJECT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        (void)routed_word;
        if (path == STACK_PAIR_LEG_ENDPOINT &&
            (!next_id(transaction, &id) ||
             !prepare_leg(transaction, &leg, id))) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->candidate.stack_pair_address_snapshot.second_leg_issued =
            UINT8_C(1);
        if (path == STACK_PAIR_LEG_ENDPOINT) {
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
            return result->status;
        }
    }
    operation_status = ti_c54x_memory_instruction_commit_architecture(
        transaction, state);
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
ti_c54x_memory_instruction_stack_pair_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (!ti_c54x_memory_instruction_stack_pair_is_candidate(transaction)) {
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
ti_c54x_memory_instruction_stack_pair_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_stack_pair_is_candidate(transaction) ||
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
    if (ti_c54x_bus_word16_transaction_reset(&transaction->bus_transaction) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
