/*
 * Module contract
 *
 * Owns: root orchestration for one dual-data-plus-program
 * MOD0/MOD1/MOD2/MOD3 WORD16 read triple. It reserves three
 * consecutive IDs, serializes the X then Y then PROGRAM
 * endpoint calls, and delegates the READY commit to the
 * triple-commit owner only after all three values are ready.
 * The dual X/Y address snapshot carries both prevalidated
 * post-access commits; the PROGRAM leg is absolute with no
 * post-update. Shadows are validated against the copied
 * mode/selector/address/pmад values before issue and at every
 * resume boundary.
 * Does not own: opcode semantics, address arithmetic, endpoint storage,
 * timing, CPU lifecycle, observers, diagnostics, allocation, I/O, or global
 * state. No architectural field is written until triple completion passes all
 * candidate and live-state checks.
 */

#include "memory_instruction_triple.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_triple_candidate_validate.h"
#include "memory_instruction_triple_commit.h"
#include "sequencer/memory/dual_address_resolution.h"

static void clear_preserving_sequence(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

static uint8_t selector_snapshot(uint8_t x_ar, uint8_t y_ar)
{
    /* AR2..AR5 each fit in four bits; keep both in the existing byte slot. */
    return (uint8_t)((x_ar << 4) | (y_ar & UINT8_C(0x0f)));
}

bool ti_c54x_memory_instruction_triple_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL && transaction->child_kind ==
                                      TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_PROGRAM_TRIPLE_WORD16;
}

bool ti_c54x_memory_instruction_triple_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *spec;
    ti_c54x_op_memory_address_spec dual_spec = {0};
    ti_c54x_dual_memory_address_resolution dual_resolution = {0};

    if (transaction == NULL || state == NULL ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    spec = &memory->address_spec;
    /* RMW_NONE rides the effect-validity gate proved at prepare, as in
     * the dual root; no effect-RMW spelling here keeps the bounded
     * transform-dispatch count. */
    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16 ||
        memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        memory->complete_triple == NULL ||
        spec->dual_program.x_mode > UINT8_C(3) ||
        spec->dual_program.y_mode > UINT8_C(3) ||
        spec->dual_program.x_ar < UINT8_C(2) ||
        spec->dual_program.x_ar > UINT8_C(5) ||
        spec->dual_program.y_ar < UINT8_C(2) ||
        spec->dual_program.y_ar > UINT8_C(5) ||
        spec->dual_program.reserved_half != UINT16_C(0) ||
        spec->dual_program.reserved_word1 != UINT32_C(0) ||
        spec->dual_program.reserved_word2 != UINT32_C(0) ||
        spec->dual_program.reserved_word3 != UINT32_C(0)) {
        return false;
    }
    /* The triple arm carries the X/Y modes and selectors at the same
     * offsets as the dual arm; re-freeze them under the dual kind so
     * the shared resolver plans the DATA legs. The PROGRAM pmad never
     * enters the dual plan. */
    dual_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16;
    dual_spec.dual.x_mode = spec->dual_program.x_mode;
    dual_spec.dual.x_ar = spec->dual_program.x_ar;
    dual_spec.dual.y_mode = spec->dual_program.y_mode;
    dual_spec.dual.y_ar = spec->dual_program.y_ar;
    if (ti_c54x_sequencer_resolve_dual_memory_address(
            state, &dual_spec, memory->access, UINT16_C(0),
            &dual_resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return false;
    }
    transaction->candidate.dual_address_snapshot.x_request =
        dual_resolution.x_request;
    transaction->candidate.dual_address_snapshot.y_request =
        dual_resolution.y_request;
    transaction->candidate.dual_address_snapshot.x_commit =
        dual_resolution.x_address_commit;
    transaction->candidate.dual_address_snapshot.y_commit =
        dual_resolution.y_address_commit;
    transaction->candidate.dual_address_snapshot.ar0_shadow =
        dual_resolution.ar0_value;
    transaction->candidate.dual_address_snapshot.bk_shadow =
        dual_resolution.bk_value;
    if (!ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate)) {
        return false;
    }
    transaction->child_kind =
        TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_PROGRAM_TRIPLE_WORD16;
    transaction->candidate_rmw_operation = selector_snapshot(
        spec->dual_program.x_ar, spec->dual_program.y_ar);
    transaction->dual_x_address_word =
        (uint16_t)transaction->candidate.dual_address_snapshot.x_request
            .address_word;
    /* The Y shadow shares its slot with the DATA+PROGRAM pmad shadow;
     * the triple keeps the Y address here and cross-checks the pmad
     * against the triple reducer's own frozen program-address copy
     * once the first issue prepares it. */
    transaction->dual_y_address_word =
        (uint16_t)transaction->candidate.dual_address_snapshot.y_request
            .address_word;
    return true;
}

static bool candidate_snapshot_matches(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *spec;

    if (!ti_c54x_memory_instruction_triple_is_candidate(transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    spec = &memory->address_spec;
    return spec->kind ==
               TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16 &&
           spec->dual_program.x_mode <= UINT8_C(3) &&
           spec->dual_program.y_mode <= UINT8_C(3) &&
           spec->dual_program.x_ar >= UINT8_C(2) &&
           spec->dual_program.x_ar <= UINT8_C(5) &&
           spec->dual_program.y_ar >= UINT8_C(2) &&
           spec->dual_program.y_ar <= UINT8_C(5) &&
           spec->dual_program.reserved_half == UINT16_C(0) &&
           spec->dual_program.reserved_word1 == UINT32_C(0) &&
           spec->dual_program.reserved_word2 == UINT32_C(0) &&
           spec->dual_program.reserved_word3 == UINT32_C(0) &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           memory->complete_triple != NULL &&
           transaction->candidate_rmw_operation == selector_snapshot(
               spec->dual_program.x_ar, spec->dual_program.y_ar) &&
           transaction->dual_x_address_word ==
               transaction->candidate.dual_address_snapshot.x_request
                   .address_word &&
           transaction->dual_y_address_word ==
               transaction->candidate.dual_address_snapshot.y_request
                   .address_word &&
           ti_c54x_memory_instruction_triple_snapshot_commits_match_spec(
               &transaction->candidate.dual_address_snapshot, spec);
}

static bool triple_ids_match_root_watermark(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_triple_word16_read_request_snapshot *request;
    uint16_t frozen_pmad;

    if (transaction == NULL) {
        return false;
    }
    /* The pmad rides no dedicated root slot (that slot shadows the Y
     * address); the triple reducer's own frozen program-address copy
     * is the second pmad witness. This check runs only after the
     * first issue prepares that copy. */
    frozen_pmad = transaction->candidate.effect.payload.memory.address_spec
                      .dual_program.program_pmad;
    request = &transaction->triple_bus_transaction.request;
    return request->x_transaction_id != UINT64_C(0) &&
           request->y_transaction_id ==
               request->x_transaction_id + UINT64_C(1) &&
           request->program_transaction_id ==
               request->y_transaction_id + UINT64_C(1) &&
           request->program_transaction_id != UINT64_C(0) &&
           request->program_transaction_id ==
               transaction->last_transaction_id &&
           request->program_request.address_word == frozen_pmad;
}

typedef enum triple_child_result {
    TRIPLE_CHILD_READY = 0,
    TRIPLE_CHILD_WAIT = 1,
    TRIPLE_CHILD_FAULT = 2,
    TRIPLE_CHILD_MALFORMED = 3
} triple_child_result;

/* The active leg needs a fresh issue while its child is
 * ISSUE_READY; an IN_FLIGHT child resumes instead. */
static bool triple_needs_issue(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_triple_word16_read_transaction *triple =
        &transaction->triple_bus_transaction;
    const ti_c54x_bus_word16_transaction *child = NULL;

    if (triple->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
        child = &triple->dual.word16_transaction;
    } else if (triple->state == TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) {
        child = &triple->program;
    } else {
        return false;
    }
    return child->state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY;
}

static uint64_t triple_active_id(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_triple_word16_read_transaction *triple =
        &transaction->triple_bus_transaction;

    if (triple->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
        return triple->dual.word16_transaction.transaction_id;
    }
    if (triple->state == TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) {
        return triple->program.transaction_id;
    }
    return triple->request.program_transaction_id;
}

static triple_child_result advance_child(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, bool begin,
    ti_c54x_bus_status *bus_status)
{
    ti_c54x_triple_word16_read_transaction *triple =
        &transaction->triple_bus_transaction;
    ti_c54x_bus_word16_observation observation = {0};
    ti_c54x_triple_word16_read_issue_result issue = {
        TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE, UINT64_C(0), {0}};
    ti_c54x_triple_word16_read_status triple_status;
    const ti_c54x_bus_word16_transaction *child = NULL;

    if (begin) {
        issue = ti_c54x_triple_word16_read_transaction_begin(triple);
        if (issue.status != TI_C54X_TRIPLE_WORD16_READ_STATUS_OK) {
            return TRIPLE_CHILD_MALFORMED;
        }
    } else {
        if ((triple->state != TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL &&
             triple->state != TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) ||
            !ti_c54x_triple_word16_read_transaction_is_canonical(triple)) {
            return TRIPLE_CHILD_MALFORMED;
        }
        /* Resume drives the currently active leg: the dual X/Y
         * child while the pair runs, the program child after. */
        if (triple->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
            child = &triple->dual.word16_transaction;
        } else {
            child = &triple->program;
        }
        if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
            return TRIPLE_CHILD_MALFORMED;
        }
        issue.transaction_id = child->transaction_id;
        issue.request = child->request;
    }

    endpoint->advance(endpoint->context,
                      begin ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                            : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
                      issue.transaction_id, &issue.request, &observation);
    triple_status = ti_c54x_triple_word16_read_transaction_poll(triple,
                                                                &observation);
    if (triple_status == TI_C54X_TRIPLE_WORD16_READ_STATUS_WAIT) {
        return TRIPLE_CHILD_WAIT;
    }
    if (triple_status == TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = triple->staged_bus_status;
        }
        return TRIPLE_CHILD_FAULT;
    }
    if (triple_status != TI_C54X_TRIPLE_WORD16_READ_STATUS_OK) {
        return TRIPLE_CHILD_MALFORMED;
    }
    return TRIPLE_CHILD_READY;
}

static bool prepare_triple(ti_c54x_memory_instruction_transaction *transaction)
{
    ti_c54x_triple_word16_read_request request = {0};
    const ti_c54x_dual_memory_address_snapshot *snapshot =
        &transaction->candidate.dual_address_snapshot;
    const ti_c54x_op_memory_address_spec *spec =
        &transaction->candidate.effect.payload.memory.address_spec;

    request.x_request = snapshot->x_request;
    request.y_request = snapshot->y_request;
    request.program_request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    request.program_request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    request.program_request.address_word = spec->dual_program.program_pmad;
    request.program_request.write_value_word = UINT16_C(0);
    request.x_transaction_id = transaction->last_transaction_id - UINT64_C(2);
    request.y_transaction_id = transaction->last_transaction_id - UINT64_C(1);
    request.program_transaction_id = transaction->last_transaction_id;
    return ti_c54x_triple_word16_read_transaction_prepare(
               &transaction->triple_bus_transaction, &request) ==
           TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
}

static ti_c54x_memory_instruction_status reject_staged(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_memory_instruction_result *result)
{
    /* The borrowed root watermark may have been edited while the endpoint
     * callback was running. Restore the frozen program ID before abandoning
     * staged work so a later reset cannot reuse or skip an ID. */
    if (ti_c54x_triple_word16_read_transaction_is_canonical(
            &transaction->triple_bus_transaction) &&
        transaction->triple_bus_transaction.request.program_transaction_id !=
            UINT64_C(0)) {
        transaction->last_transaction_id = transaction->triple_bus_transaction
                                               .request.program_transaction_id;
    }
    (void)ti_c54x_triple_word16_read_transaction_abandon_staged(
        &transaction->triple_bus_transaction);
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_triple_advance(
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
    triple_child_result child_result;

    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        accumulators == NULL || t_register == NULL || result == NULL ||
        !ti_c54x_memory_instruction_triple_is_candidate(transaction) ||
        (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED &&
         transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate) ||
        !candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_triple_candidate_matches_live(
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
        if (transaction->last_transaction_id > UINT64_MAX - UINT64_C(3) ||
            transaction->triple_bus_transaction.state !=
                TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE) {
            result->status =
                transaction->last_transaction_id > UINT64_MAX - UINT64_C(3)
                    ? TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED
                    : TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->last_transaction_id += UINT64_C(3);
        if (!prepare_triple(transaction)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    } else if (!ti_c54x_triple_word16_read_transaction_is_canonical(
                   &transaction->triple_bus_transaction) ||
               !triple_ids_match_root_watermark(transaction)) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }

    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    /* Drive legs until one waits, faults, or all three are ready.
     * Each leg issue rechecks the frozen shadows and live ARs. */
    for (;;) {
        result->transaction_id = triple_active_id(transaction);
        child_result = advance_child(transaction, endpoint,
                                     triple_needs_issue(transaction),
                                     &bus_status);
        if (child_result == TRIPLE_CHILD_WAIT) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
            result->transaction_id =
                triple_active_id(transaction);
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (child_result == TRIPLE_CHILD_MALFORMED) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        }
        if (child_result == TRIPLE_CHILD_FAULT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = bus_status;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (transaction->triple_bus_transaction.state ==
            TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY) {
            break;
        }
        if (!candidate_snapshot_matches(transaction) ||
            !triple_ids_match_root_watermark(transaction) ||
            !ti_c54x_memory_instruction_triple_candidate_matches_live(
                &transaction->candidate, state)) {
            return reject_staged(transaction, result);
        }
    }

    if (!candidate_snapshot_matches(transaction) ||
        !triple_ids_match_root_watermark(transaction) ||
        !ti_c54x_memory_instruction_triple_candidate_matches_live(
            &transaction->candidate, state)) {
        return reject_staged(transaction, result);
    }
    {
        ti_c54x_triple_word16_read_completion_result completion =
            ti_c54x_triple_word16_read_transaction_complete(
                &transaction->triple_bus_transaction);
        ti_c54x_memory_instruction_status status;

        if (completion.status != TI_C54X_TRIPLE_WORD16_READ_STATUS_OK) {
            result->status =
                completion.status == TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT
                    ? TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT
                    : TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            result->bus_status = completion.bus_status;
            return result->status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT
                       ? TI_C54X_MEMORY_INSTRUCTION_STATUS_OK
                       : result->status;
        }
        status = ti_c54x_memory_instruction_triple_commit_ready(
            transaction, state, accumulators, t_register,
            completion.x_value_word, completion.y_value_word,
            completion.program_value_word);
        if (status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
            result->status = status;
            return status;
        }
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_triple_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    ti_c54x_triple_word16_read_transaction *triple;
    uint64_t active_id;

    if (!ti_c54x_memory_instruction_triple_is_candidate(transaction)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->triple_bus_transaction.state !=
            TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    triple = &transaction->triple_bus_transaction;
    if ((triple->state != TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL &&
         triple->state != TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) ||
        !ti_c54x_triple_word16_read_transaction_is_canonical(triple) ||
        !triple_ids_match_root_watermark(transaction) ||
        ti_c54x_bus_word16_adapter_validate_endpoint(endpoint) !=
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    active_id = triple_active_id(transaction);
    if (ti_c54x_triple_word16_read_transaction_cancel(triple, active_id) !=
        TI_C54X_TRIPLE_WORD16_READ_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_triple_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_triple_is_candidate(transaction) ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->triple_bus_transaction.state !=
            TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE &&
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_triple_word16_read_transaction_reset(
            &transaction->triple_bus_transaction) !=
        TI_C54X_TRIPLE_WORD16_READ_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
