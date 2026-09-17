/*
 * Module contract
 *
 * Owns: root orchestration for one DATA+PROGRAM WORD16 read pair. It
 * reserves two consecutive IDs, serializes the Smem DATA then
 * PROGRAM endpoint calls, optionally stages the Smem delay
 * write-back (MACD) through the reused child with a third ID, and
 * delegates the READY commit to the pair-commit owner only after
 * all bus work is ready. The persistent snapshot carries the
 * prevalidated Smem post-access commit alongside the verbatim
 * pmad; both are validated against the copied leg/address shadows
 * before issue and at every resume boundary. The delay stage keeps
 * the pair COMPLETE (staged values intact for the later commit)
 * and reuses only the child transaction, so no new root phase is
 * needed and resume-before-fetch keeps working.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. No architectural field is
 * written until pair completion passes all candidate and
 * live-state checks.
 */

#include "memory_instruction_data_program.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/request/word16_request.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_data_program_candidate_validate.h"
#include "memory_instruction_data_program_commit.h"
#include "sequencer/memory/data_program_address_resolution.h"

static void clear_preserving_sequence(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

/* Pack the Smem leg descriptor (indirect flag plus MOD0/MOD1/MOD2)
 * into the existing byte slot so a pending resume rejects edits
 * that would otherwise remain structurally valid. */
static uint8_t leg_snapshot(uint8_t indirect, uint8_t smem_mode)
{
    return (uint8_t)(((uint8_t)(smem_mode << 1)) |
                     (indirect & UINT8_C(0x01)));
}

bool ti_c54x_memory_instruction_data_program_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL && transaction->child_kind ==
                                      TI_C54X_MEMORY_INSTRUCTION_CHILD_DATA_PROGRAM_WORD16_READ;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_prepare_candidate(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *spec;

    if (transaction == NULL || state == NULL ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    memory = &transaction->candidate.effect.payload.memory;
    spec = &memory->address_spec;
    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 ||
        memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        memory->complete_dual == NULL || spec->pair.indirect > UINT8_C(1) ||
        spec->pair.smem_mode > UINT8_C(2) ||
        spec->pair.smem_arf > UINT8_C(7) ||
        spec->pair.smem_dma7 > UINT8_C(0x7f) ||
        spec->pair.reserved_half != UINT16_C(0) ||
        spec->pair.reserved_word0 != UINT32_C(0) ||
        spec->pair.reserved_word1 != UINT32_C(0) ||
        spec->pair.reserved_word2 != UINT32_C(0) ||
        (spec->pair.indirect == UINT8_C(0) &&
         (spec->pair.smem_mode != UINT8_C(0) ||
          spec->pair.smem_arf != UINT8_C(0))) ||
        (spec->pair.indirect != UINT8_C(0) &&
         spec->pair.smem_dma7 != UINT8_C(0))) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    /* The pair resolver publishes the shared snapshot carrier
     * directly into the candidate; no intermediate copy is staged. */
    if (ti_c54x_sequencer_resolve_data_program_address(
            state, spec, memory->access, UINT16_C(0),
            &transaction->candidate.dual_address_snapshot) !=
        TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (!ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    transaction->child_kind =
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DATA_PROGRAM_WORD16_READ;
    transaction->candidate_rmw_operation =
        leg_snapshot(spec->pair.indirect, spec->pair.smem_mode);
    transaction->data_program_smem_address_word =
        (uint16_t)transaction->candidate.dual_address_snapshot.x_request
            .address_word;
    transaction->data_program_pmad_word =
        (uint16_t)transaction->candidate.dual_address_snapshot.y_request
            .address_word;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

static bool candidate_snapshot_matches(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *spec;

    if (!ti_c54x_memory_instruction_data_program_is_candidate(transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    spec = &memory->address_spec;
    return spec->kind == TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 &&
           spec->pair.indirect <= UINT8_C(1) &&
           spec->pair.smem_mode <= UINT8_C(2) &&
           spec->pair.smem_arf <= UINT8_C(7) &&
           spec->pair.smem_dma7 <= UINT8_C(0x7f) &&
           spec->pair.reserved_half == UINT16_C(0) &&
           spec->pair.reserved_word0 == UINT32_C(0) &&
           spec->pair.reserved_word1 == UINT32_C(0) &&
           spec->pair.reserved_word2 == UINT32_C(0) &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           memory->complete_dual != NULL &&
           transaction->candidate_rmw_operation ==
               leg_snapshot(spec->pair.indirect, spec->pair.smem_mode) &&
           transaction->data_program_smem_address_word ==
               transaction->candidate.dual_address_snapshot.x_request
                   .address_word &&
           transaction->data_program_pmad_word ==
               transaction->candidate.dual_address_snapshot.y_request
                   .address_word &&
           ti_c54x_memory_instruction_data_program_snapshot_commits_match_spec(
               &transaction->candidate.dual_address_snapshot, spec);
}

static bool pair_ids_match_root_watermark(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_data_program_word16_read_request_snapshot *request;

    if (transaction == NULL) {
        return false;
    }
    request = &transaction->data_program_bus_transaction.request;
    return request->x_transaction_id != UINT64_C(0) &&
           request->y_transaction_id == request->x_transaction_id +
                                             UINT64_C(1) &&
           request->y_transaction_id != UINT64_C(0) &&
           request->y_transaction_id == transaction->last_transaction_id;
}

typedef enum data_program_child_result {
    DATA_PROGRAM_CHILD_READY = 0,
    DATA_PROGRAM_CHILD_WAIT = 1,
    DATA_PROGRAM_CHILD_FAULT = 2,
    DATA_PROGRAM_CHILD_MALFORMED = 3
} data_program_child_result;

static data_program_child_result advance_child(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, bool begin,
    ti_c54x_bus_status *bus_status)
{
    ti_c54x_data_program_word16_read_transaction *pair =
        &transaction->data_program_bus_transaction;
    ti_c54x_bus_word16_observation observation = {0};
    ti_c54x_data_program_word16_read_issue_result issue;
    ti_c54x_data_program_word16_read_status pair_status;
    const ti_c54x_bus_word16_transaction *child = &pair->word16_transaction;

    if (begin) {
        issue = ti_c54x_data_program_word16_read_transaction_begin(pair);
        if (issue.status != TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_OK) {
            return DATA_PROGRAM_CHILD_MALFORMED;
        }
    } else if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
               (pair->state != TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_X &&
                pair->state != TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_Y)) {
        return DATA_PROGRAM_CHILD_MALFORMED;
    }

    endpoint->advance(endpoint->context,
                      begin ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                            : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
                      child->transaction_id, &child->request, &observation);
    pair_status =
        ti_c54x_data_program_word16_read_transaction_poll(pair, &observation);
    if (pair_status == TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_WAIT) {
        return DATA_PROGRAM_CHILD_WAIT;
    }
    if (pair_status == TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = pair->staged_bus_status;
        }
        return DATA_PROGRAM_CHILD_FAULT;
    }
    if (pair_status != TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_OK) {
        return DATA_PROGRAM_CHILD_MALFORMED;
    }
    return DATA_PROGRAM_CHILD_READY;
}

static bool prepare_pair(ti_c54x_memory_instruction_transaction *transaction)
{
    ti_c54x_data_program_word16_read_request request = {0};
    const ti_c54x_dual_memory_address_snapshot *snapshot =
        &transaction->candidate.dual_address_snapshot;

    request.x_request = snapshot->x_request;
    request.y_request = snapshot->y_request;
    request.x_transaction_id = transaction->last_transaction_id - UINT64_C(1);
    request.y_transaction_id = transaction->last_transaction_id;
    return ti_c54x_data_program_word16_read_transaction_prepare(
               &transaction->data_program_bus_transaction, &request) ==
           TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_OK;
}

/* The delay write is in flight exactly when the pair holds staged
 * values (COMPLETE) while its reused child carries an issued delay
 * WRITE. The pair wrapper is non-canonical in this shape by
 * construction; only the child state and watermark decide. */
static bool delay_write_in_flight(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_data_program_word16_read_transaction *pair;

    if (transaction == NULL) {
        return false;
    }
    pair = &transaction->data_program_bus_transaction;
    return pair->state == TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_COMPLETE &&
           pair->word16_transaction.state ==
               TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT;
}

static bool delay_write_ids_match(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_bus_word16_transaction *child =
        &transaction->data_program_bus_transaction.word16_transaction;

    return child->transaction_id != UINT64_C(0) &&
           child->transaction_id == transaction->last_transaction_id;
}

static ti_c54x_memory_instruction_status retire_completed_pair(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t x_word, uint16_t y_word,
    ti_c54x_memory_instruction_result *result)
{
    ti_c54x_memory_instruction_status status =
        ti_c54x_memory_instruction_data_program_commit_ready(
            transaction, state, accumulators, t_register, x_word, y_word);

    if (status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
        result->status = status;
        return status;
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

/* Issue (begin) or resume the Smem delay WRITE of the Smem word to
 * DATA Smem-address-plus-one with 16-bit wraparound. The pair must
 * already hold both staged values; the reused child carries the
 * WRITE while the pair wrapper stays COMPLETE. */
static data_program_child_result advance_delay_write(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, uint32_t smem_address,
    uint16_t smem_word, bool begin, ti_c54x_bus_status *bus_status)
{
    ti_c54x_data_program_word16_read_transaction *pair =
        &transaction->data_program_bus_transaction;
    ti_c54x_bus_word16_transaction *child = &pair->word16_transaction;
    ti_c54x_bus_word16_observation observation = {0};
    ti_c54x_bus_word16_request_result built;
    ti_c54x_bus_word16_transaction_issue_result issue;
    ti_c54x_bus_word16_transaction_status child_status;

    if (begin) {
        if (pair->state != TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_COMPLETE ||
            transaction->last_transaction_id == UINT64_MAX) {
            return DATA_PROGRAM_CHILD_MALFORMED;
        }
        if (ti_c54x_bus_word16_transaction_reset(child) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return DATA_PROGRAM_CHILD_MALFORMED;
        }
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            (smem_address + UINT32_C(1)) & UINT32_C(0xffff), smem_word);
        if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return DATA_PROGRAM_CHILD_MALFORMED;
        }
        transaction->last_transaction_id += UINT64_C(1);
        if (ti_c54x_bus_word16_transaction_prepare(
                child, &built.request,
                transaction->last_transaction_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return DATA_PROGRAM_CHILD_MALFORMED;
        }
        issue = ti_c54x_bus_word16_transaction_begin(child);
        if (issue.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return DATA_PROGRAM_CHILD_MALFORMED;
        }
    } else if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
               pair->state !=
                   TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_COMPLETE) {
        return DATA_PROGRAM_CHILD_MALFORMED;
    }

    endpoint->advance(endpoint->context,
                      begin ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                            : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
                      child->transaction_id, &child->request, &observation);
    child_status = ti_c54x_bus_word16_transaction_poll(child, &observation);
    if (child_status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT) {
        return DATA_PROGRAM_CHILD_WAIT;
    }
    if (child_status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT) {
        ti_c54x_bus_word16_transaction_completion_result completion =
            ti_c54x_bus_word16_transaction_complete(child,
                                                    child->transaction_id);
        if (bus_status != NULL) {
            *bus_status = completion.bus_status;
        }
        return DATA_PROGRAM_CHILD_FAULT;
    }
    if (child_status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return DATA_PROGRAM_CHILD_MALFORMED;
    }
    if (ti_c54x_bus_word16_transaction_complete(child,
                                                child->transaction_id)
            .status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return DATA_PROGRAM_CHILD_MALFORMED;
    }
    return DATA_PROGRAM_CHILD_READY;
}

static ti_c54x_memory_instruction_status reject_staged(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_memory_instruction_result *result)
{
    /* The borrowed root watermark may have been edited while the endpoint
     * callback was running. Restore the frozen Y ID before abandoning staged
     * work so a later reset cannot reuse or skip a pair. */
    if (ti_c54x_data_program_word16_read_transaction_is_canonical(
            &transaction->data_program_bus_transaction) &&
        transaction->data_program_bus_transaction.request.y_transaction_id !=
            UINT64_C(0)) {
        transaction->last_transaction_id =
            transaction->data_program_bus_transaction.request.y_transaction_id;
    }
    (void)ti_c54x_data_program_word16_read_transaction_abandon_staged(
        &transaction->data_program_bus_transaction);
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    const bool first_issue = transaction != NULL &&
                             transaction->phase ==
                                 TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    ti_c54x_bus_status bus_status = TI_C54X_BUS_OK;
    data_program_child_result child_result;

    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        accumulators == NULL || t_register == NULL || result == NULL ||
        !ti_c54x_memory_instruction_data_program_is_candidate(transaction) ||
        (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED &&
         transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate) ||
        !candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        if (result != NULL) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
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
    if (first_issue) {
        if (transaction->last_transaction_id > UINT64_MAX - UINT64_C(2) ||
            transaction->data_program_bus_transaction.state !=
                TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_IDLE) {
            result->status =
                transaction->last_transaction_id > UINT64_MAX - UINT64_C(2)
                    ? TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED
                    : TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->last_transaction_id += UINT64_C(2);
        if (!prepare_pair(transaction)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    } else if (delay_write_in_flight(transaction)) {
        /* Delay-write resume: the pair holds staged values while the
         * reused child carries the issued delay WRITE. */
        const ti_c54x_data_program_word16_read_transaction *pair =
            &transaction->data_program_bus_transaction;

        if (!delay_write_ids_match(transaction)) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        child_result = advance_delay_write(
            transaction, endpoint,
            transaction->data_program_smem_address_word,
            pair->staged_x_value_word, false, &bus_status);
        result->transaction_id =
            transaction->data_program_bus_transaction.word16_transaction
                .transaction_id;
        result->instructions_attempted = UINT64_C(0);
        if (child_result == DATA_PROGRAM_CHILD_WAIT) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (child_result == DATA_PROGRAM_CHILD_MALFORMED) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        }
        if (child_result == DATA_PROGRAM_CHILD_FAULT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = bus_status;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (!candidate_snapshot_matches(transaction) ||
            !ti_c54x_memory_instruction_candidate_matches_live(
                &transaction->candidate, state)) {
            clear_preserving_sequence(transaction);
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        return retire_completed_pair(
            transaction, state, accumulators, t_register,
            pair->staged_x_value_word, pair->staged_y_value_word, result);
    } else if (!ti_c54x_data_program_word16_read_transaction_is_canonical(
                   &transaction->data_program_bus_transaction) ||
               !pair_ids_match_root_watermark(transaction)) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }

    child_result = advance_child(transaction, endpoint, first_issue,
                                 &bus_status);
    result->transaction_id = transaction->data_program_bus_transaction
                                 .word16_transaction.transaction_id;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    if (child_result == DATA_PROGRAM_CHILD_WAIT) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (child_result == DATA_PROGRAM_CHILD_MALFORMED) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    if (child_result == DATA_PROGRAM_CHILD_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = bus_status;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }

    /* X READY has only staged the first value and prepared Y. */
    if (transaction->data_program_bus_transaction.state ==
        TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_Y) {
        if (!candidate_snapshot_matches(transaction) ||
            !pair_ids_match_root_watermark(transaction) ||
            !ti_c54x_memory_instruction_candidate_matches_live(
                &transaction->candidate, state)) {
            return reject_staged(transaction, result);
        }
        child_result = advance_child(transaction, endpoint, true, &bus_status);
        result->transaction_id = transaction->data_program_bus_transaction
                                     .word16_transaction.transaction_id;
        if (child_result == DATA_PROGRAM_CHILD_WAIT) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (child_result == DATA_PROGRAM_CHILD_MALFORMED) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        }
        if (child_result == DATA_PROGRAM_CHILD_FAULT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = bus_status;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
    }

    if (!candidate_snapshot_matches(transaction) ||
        !pair_ids_match_root_watermark(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        return reject_staged(transaction, result);
    }
    {
        const ti_c54x_op_memory_effect *memory =
            &transaction->candidate.effect.payload.memory;
        ti_c54x_data_program_word16_read_completion_result completion =
            ti_c54x_data_program_word16_read_transaction_complete(
                &transaction->data_program_bus_transaction);

        if (completion.status != TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_OK) {
            result->status =
                completion.status ==
                        TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_FAULT
                    ? TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT
                    : TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            result->bus_status = completion.bus_status;
            return result->status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT
                       ? TI_C54X_MEMORY_INSTRUCTION_STATUS_OK
                       : result->status;
        }
        /* The delay write-back (MACD) issues after both reads
         * complete and before the architecture commit, so a delay
         * FAULT publishes nothing. */
        if (memory->write_smem_delay) {
            if (transaction->last_transaction_id == UINT64_MAX) {
                result->status =
                    TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
            }
            child_result = advance_delay_write(
                transaction, endpoint,
                transaction->data_program_smem_address_word,
                completion.x_value_word, true, &bus_status);
            result->transaction_id = transaction
                                         ->data_program_bus_transaction
                                         .word16_transaction.transaction_id;
            if (child_result == DATA_PROGRAM_CHILD_WAIT) {
                result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
            }
            if (child_result == DATA_PROGRAM_CHILD_MALFORMED) {
                result->status =
                    TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            }
            if (child_result == DATA_PROGRAM_CHILD_FAULT) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
                result->bus_status = bus_status;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
            }
            if (!candidate_snapshot_matches(transaction) ||
                !ti_c54x_memory_instruction_candidate_matches_live(
                    &transaction->candidate, state)) {
                clear_preserving_sequence(transaction);
                transaction->phase =
                    TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                result->status =
                    TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
        }
        return retire_completed_pair(transaction, state, accumulators,
                                     t_register, completion.x_value_word,
                                     completion.y_value_word, result);
    }
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    ti_c54x_data_program_word16_read_transaction *pair;
    const ti_c54x_bus_word16_transaction *child;

    if (!ti_c54x_memory_instruction_data_program_is_candidate(transaction)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->data_program_bus_transaction.state !=
                TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_IDLE ||
            transaction->data_program_bus_transaction.word16_transaction
                    .state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    pair = &transaction->data_program_bus_transaction;
    child = &pair->word16_transaction;
    /* Delay-write cancel: the pair holds staged values while the
     * reused child carries the issued delay WRITE. */
    if (pair->state == TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_COMPLETE &&
        child->state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        if (!delay_write_ids_match(transaction) ||
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint) !=
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        if (endpoint->cancel(endpoint->context, child->transaction_id,
                             &child->request) !=
                TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED ||
            ti_c54x_bus_word16_transaction_cancel(
                &pair->word16_transaction, child->transaction_id) !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if ((pair->state != TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_X &&
         pair->state != TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_Y) ||
        !ti_c54x_data_program_word16_read_transaction_is_canonical(pair) ||
        child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
        !pair_ids_match_root_watermark(transaction) ||
        ti_c54x_bus_word16_adapter_validate_endpoint(endpoint) !=
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (endpoint->cancel(endpoint->context, child->transaction_id,
                         &child->request) !=
        TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    if (ti_c54x_data_program_word16_read_transaction_cancel(
            pair, child->transaction_id) !=
        TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_data_program_is_candidate(transaction) ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->data_program_bus_transaction.state !=
                TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_IDLE ||
            transaction->data_program_bus_transaction.word16_transaction
                    .state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE &&
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    /* The delay stage reuses only the child while the pair wrapper
     * stays COMPLETE, so the wrapper reset cannot see its canonical
     * shape. Reset the child when the delay stage touched it, then
     * validate the wrapper only when it still owns the child. */
    if (transaction->data_program_bus_transaction.word16_transaction.state ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_data_program_word16_read_transaction_is_canonical(
            &transaction->data_program_bus_transaction)) {
        if (ti_c54x_data_program_word16_read_transaction_reset(
                &transaction->data_program_bus_transaction) !=
            TI_C54X_DATA_PROGRAM_WORD16_READ_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else {
        ti_c54x_data_program_word16_read_transaction *pair =
            &transaction->data_program_bus_transaction;

        if (pair->state != TI_C54X_DATA_PROGRAM_WORD16_READ_STATE_COMPLETE ||
            pair->request.x_transaction_id == UINT64_C(0) ||
            pair->request.y_transaction_id !=
                pair->request.x_transaction_id + UINT64_C(1)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        if (pair->word16_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE &&
            ti_c54x_bus_word16_transaction_reset(
                &pair->word16_transaction) !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
