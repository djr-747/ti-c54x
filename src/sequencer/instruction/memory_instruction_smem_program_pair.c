/*
 * Module contract
 *
 * Owns: root orchestration for one Smem-to-program DATA pair
 * (MVPD/MVDP): leg issue, completion, cancel, and reset; Smem-leg
 * spec synthesis and snapshot preparation live in
 * memory_instruction_smem_program_pair_prepare.c. See
 * memory_instruction_smem_program_pair.h.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. Transaction storage is
 * borrowed for each call.
 */

#include "memory_instruction_smem_program_pair.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/mmr/mmr_route.h"
#include "bus/request/word16_request.h"
#include "bus/transaction/word16_transaction.h"
#include "foundation/arithmetic40.h"
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
ti_c54x_memory_instruction_smem_program_pair_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL &&
           transaction->child_kind ==
               TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_PROGRAM_PAIR_WORD16;
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

/* Drive one leg through the endpoint. DATA/IO legs ride the
 * WORD16 endpoint adapter; PROGRAM legs run the same
 * begin/poll/complete sequence around a direct endpoint call
 * because the adapter serves DATA/IO only (DATA_PROGRAM
 * precedent: its Y PROGRAM leg bypasses the adapter the same
 * way). */
static ti_c54x_bus_word16_adapter_result
advance_leg(const ti_c54x_bus_word16_endpoint *endpoint,
            ti_c54x_bus_word16_advance_phase phase,
            ti_c54x_bus_word16_transaction *bus_transaction)
{
    ti_c54x_bus_word16_adapter_result result = {0};

    if (bus_transaction == NULL ||
        bus_transaction->request.space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM) {
        return ti_c54x_bus_word16_adapter_advance(endpoint, phase,
                                                  bus_transaction);
    }
    if (endpoint == NULL ||
        (phase != TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
         phase != TI_C54X_BUS_WORD16_ADVANCE_RESUME)) {
        result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT;
        return result;
    }
    {
        const ti_c54x_bus_word16_adapter_status endpoint_status =
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);

        if (endpoint_status ==
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK) {
            result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT;
            result.bus_status = TI_C54X_BUS_NO_CALLBACK;
            return result;
        }
        if (endpoint_status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            result.status =
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT;
            return result;
        }
    }
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        const ti_c54x_bus_word16_transaction_issue_result issue =
            ti_c54x_bus_word16_transaction_begin(bus_transaction);

        if (issue.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            result.status =
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT;
            return result;
        }
    }
    {
        ti_c54x_bus_word16_observation observation = {0};
        ti_c54x_bus_word16_transaction_status poll_status;

        endpoint->advance(endpoint->context, phase,
                          bus_transaction->transaction_id,
                          &bus_transaction->request, &observation);
        poll_status = ti_c54x_bus_word16_transaction_poll(bus_transaction,
                                                          &observation);
        if (poll_status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT) {
            result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT;
            return result;
        }
        {
            const ti_c54x_bus_word16_transaction_completion_result
                completion = ti_c54x_bus_word16_transaction_complete(
                    bus_transaction, bus_transaction->transaction_id);

            result.read_value_word = completion.read_value_word;
            result.bus_status = completion.bus_status;
            if (poll_status ==
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT) {
                result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT;
                return result;
            }
            if (poll_status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK ||
                completion.status !=
                    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
                result.status =
                    TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION;
                result.read_value_word = UINT16_C(0);
                result.bus_status = TI_C54X_BUS_OK;
                return result;
            }
            result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK;
            return result;
        }
    }
}

static ti_c54x_bus_word16_request
first_leg_request(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    /* MVPD (access WRITE) reads PROGRAM first; MVDP (access
     * READ) reads Smem first. Both legs live in the pair
     * snapshot. */
    if (transaction->candidate.effect.payload.memory.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
                   &transaction->candidate)
            ->program_request;
    }
    return ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
                   &transaction->candidate)
            ->smem_resolution.request;
}

static bool
build_second_leg_request(
    const ti_c54x_memory_instruction_transaction *transaction,
    uint16_t observed_word, ti_c54x_bus_word16_request *out_request)
{
    ti_c54x_bus_word16_request_result built;

    if (transaction->candidate.effect.payload.memory.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        /* MVPD writes the PROGRAM observation to Smem. */
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
                &transaction->candidate)
                ->smem_resolution.request.address_word,
            observed_word);
    } else {
        /* MVDP writes the Smem observation to PROGRAM. */
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
            ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
                &transaction->candidate)
                ->program_request.address_word,
            observed_word);
    }
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
 * traffic and no state mutation. Mirrors the stack-pair MMR
 * branch, including the probe-then-serve READ double access
 * the plan itself performs. PROGRAM legs plan ENDPOINT (only
 * DATA AG/BG route); the generic plan still guards them. */
typedef enum smem_program_pair_leg_path {
    SMEM_PROGRAM_PAIR_LEG_ENDPOINT = 0,
    SMEM_PROGRAM_PAIR_LEG_ROUTED = 1,
    SMEM_PROGRAM_PAIR_LEG_REJECT = 2
} smem_program_pair_leg_path;

static smem_program_pair_leg_path
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
        return SMEM_PROGRAM_PAIR_LEG_ENDPOINT;
    }
    if (plan != TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
        return SMEM_PROGRAM_PAIR_LEG_REJECT;
    }
    routed = ti_c54x_bus_mmr_route(request, state, accumulators,
                                   t_register);
    if (routed.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
        return SMEM_PROGRAM_PAIR_LEG_REJECT;
    }
    *observed_word = routed.read_value_word;
    return SMEM_PROGRAM_PAIR_LEG_ROUTED;
}

static bool
snapshot_matches(const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;

    if (!ti_c54x_memory_instruction_smem_program_pair_is_candidate(
            transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    if ((arm->kind !=
             TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16 &&
         arm->kind !=
             TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16 &&
         arm->kind !=
             TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) ||
        ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
            &transaction->candidate)
                ->second_leg_issued > UINT8_C(1)) {
        return false;
    }
    /* The PROGRAM template stays zero until the observed word
     * replaces it at second-leg issue; re-derive the Smem spec
     * and echo the PROGRAM request from the frozen arm on every
     * check. */
    {
        ti_c54x_op_memory_address_spec smem_spec = {0};
        const ti_c54x_bus_word16_request *program_request =
            &ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
                 &transaction->candidate)
                 ->program_request;

        if (!ti_c54x_memory_instruction_smem_program_pair_synthesize_smem_spec(
                arm, &transaction->candidate.input, &smem_spec)) {
            return false;
        }
        if (program_request->space !=
                TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM) {
            return false;
        }
        if (arm->kind == TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16) {
            if (program_request->address_word !=
                (uint32_t)(uint16_t)transaction->candidate.input.accumulator_a_raw40) {
                return false;
            }
        } else if (arm->kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
            if (program_request->address_word !=
                (uint32_t)arm->smem_absolute_program.pmad) {
                return false;
            }
        } else if (program_request->address_word !=
                   (uint32_t)arm->smem_program.pmad) {
            return false;
        }
        if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            if (program_request->access !=
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
                return false;
            }
        } else if (program_request->access !=
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            return false;
        }
        (void)smem_spec;
    }
    return true;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_smem_program_pair_advance(
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
        !ti_c54x_memory_instruction_smem_program_pair_is_candidate(
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
        ti_c54x_bus_word16_request leg = first_leg_request(transaction);
        uint64_t id;
        smem_program_pair_leg_path path;

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
        if (path == SMEM_PROGRAM_PAIR_LEG_REJECT) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        if (path == SMEM_PROGRAM_PAIR_LEG_ROUTED) {
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
        adapter_result = advance_leg(
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
    if (ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
            &transaction->candidate)
            ->second_leg_issued == UINT8_C(0)) {
        ti_c54x_bus_word16_request leg;
        uint64_t id;
        smem_program_pair_leg_path path;
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
        if (path == SMEM_PROGRAM_PAIR_LEG_REJECT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        (void)routed_word;
        if (path == SMEM_PROGRAM_PAIR_LEG_ENDPOINT &&
            (!next_id(transaction, &id) ||
             !prepare_leg(transaction, &leg, id))) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        ti_c54x_memory_instruction_smem_program_pair_mutable_snapshot_for_kind(
            &transaction->candidate)
            ->second_leg_issued = UINT8_C(1);
        if (path == SMEM_PROGRAM_PAIR_LEG_ENDPOINT) {
            result->transaction_id = id;
            adapter_result = advance_leg(
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
ti_c54x_memory_instruction_smem_program_pair_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (!ti_c54x_memory_instruction_smem_program_pair_is_candidate(
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
ti_c54x_memory_instruction_smem_program_pair_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_smem_program_pair_is_candidate(
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
    if (ti_c54x_bus_word16_transaction_reset(&transaction->bus_transaction) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
