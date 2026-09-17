/*
 * Module contract
 *
 * Owns: root orchestration for one PORT I/O pair: IO-leg planning,
 * DATA-leg synthesis through the existing single-leg resolver,
 * deferred two-request issue with observation handoff, atomic
 * address-commit publication, and terminal cancellation/reset. The
 * DATA-leg direction rides the effect access with the IO leg always
 * inverse; WRITE templates carry zero until the observed word
 * replaces them at second-leg issue. No architectural field is
 * written until both legs and all candidate and live-state checks
 * pass.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. Transaction storage is borrowed
 * for each call.
 */

#include "memory_instruction_port.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "bus/request/word16_request.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_commit.h"
#include "sequencer/memory/port_access_plan.h"
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
ti_c54x_memory_instruction_port_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL &&
           transaction->child_kind ==
               TI_C54X_MEMORY_INSTRUCTION_CHILD_PORT_WORD16_PAIR;
}

bool
ti_c54x_memory_instruction_port_synthesize_data_spec(
    const ti_c54x_op_memory_address_spec *arm,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_memory_address_spec *data_spec)
{
    ti_c54x_op_memory_address_spec staged = {0};

    if (arm->port.indirect == UINT8_C(0)) {
        if (!input->status_dp_known) {
            return false;
        }
        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16;
        staged.long_offset_word = input->status_dp;
        staged.direct_dma7 = arm->port.smem_dma7;
        *data_spec = staged;
        return true;
    }
    switch (arm->port.smem_mode) {
    case UINT8_C(0):
    case UINT8_C(1):
    case UINT8_C(2):
    case UINT8_C(3):
        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
        staged.mode = arm->port.smem_mode;
        staged.arf = arm->port.smem_arf;
        break;
    case UINT8_C(4):
    case UINT8_C(7):
        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16;
        staged.reverse_carry_mode = arm->port.smem_mode;
        staged.arf = arm->port.smem_arf;
        break;
    case UINT8_C(5):
    case UINT8_C(6):
        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16;
        staged.indexed_mode = arm->port.smem_mode;
        staged.arf = arm->port.smem_arf;
        break;
    case UINT8_C(8):
    case UINT8_C(10):
        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16;
        staged.circular_mode = arm->port.smem_mode;
        staged.arf = arm->port.smem_arf;
        break;
    default:
        return false;
    }
    *data_spec = staged;
    return true;
}

bool
ti_c54x_memory_instruction_port_io_request_matches_arm(
    const ti_c54x_bus_word16_request *io_request,
    const ti_c54x_op_memory_effect *memory)
{
    const ti_c54x_op_memory_address_spec *arm = &memory->address_spec;

    if (io_request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_IO ||
        io_request->address_word != arm->port.port_address) {
        return false;
    }
    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return io_request->access ==
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
               io_request->write_value_word == UINT16_C(0);
    }
    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return io_request->access ==
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
               io_request->write_value_word == UINT16_C(0);
    }
    return false;
}

static bool
snapshot_matches(const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;
    const ti_c54x_port_address_snapshot *snapshot;

    if (!ti_c54x_memory_instruction_port_is_candidate(transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    snapshot = &transaction->candidate.port_address_snapshot;
    if (arm->kind != TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16 ||
        snapshot->second_leg_issued > UINT8_C(1)) {
        return false;
    }
    return ti_c54x_memory_instruction_port_io_request_matches_arm(
        &snapshot->io_request, memory);
}

bool
ti_c54x_memory_instruction_port_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;
    ti_c54x_port_access io_plan;
    ti_c54x_op_memory_address_spec data_spec = {0};
    ti_c54x_memory_address_resolution data_resolution = {0};

    if (transaction == NULL || state == NULL ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    if (arm->kind != TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        return false;
    }
    io_plan = ti_c54x_sequencer_plan_port_access(arm, memory->access);
    if (io_plan.status != TI_C54X_PORT_ACCESS_IO_REQUEST) {
        return false;
    }
    if (!ti_c54x_memory_instruction_port_synthesize_data_spec(
            arm, &transaction->candidate.input, &data_spec)) {
        return false;
    }
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &data_spec, memory->access, UINT16_C(0),
            &data_resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return false;
    }
    transaction->candidate.port_address_snapshot.data_resolution =
        data_resolution;
    transaction->candidate.port_address_snapshot.io_request =
        io_plan.request;
    transaction->candidate.port_address_snapshot.second_leg_issued =
        UINT8_C(0);
    transaction->child_kind = TI_C54X_MEMORY_INSTRUCTION_CHILD_PORT_WORD16_PAIR;
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
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;

    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return transaction->candidate.port_address_snapshot.io_request;
    }
    return transaction->candidate.port_address_snapshot.data_resolution
        .request;
}

static bool
build_second_leg_request(
    const ti_c54x_memory_instruction_transaction *transaction,
    uint16_t observed_word, ti_c54x_bus_word16_request *out_request)
{
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;
    ti_c54x_bus_word16_request_result built;

    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            transaction->candidate.port_address_snapshot.data_resolution
                .request.address_word,
            observed_word);
    } else {
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_IO,
            transaction->candidate.port_address_snapshot.io_request
                .address_word,
            observed_word);
    }
    if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return false;
    }
    *out_request = built.request;
    return true;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_port_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    const bool first_issue =
        transaction != NULL &&
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    ti_c54x_bus_word16_adapter_result adapter_result;
    ti_c54x_memory_instruction_status operation_status;

    (void)accumulators;
    (void)t_register;
    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        result == NULL ||
        !ti_c54x_memory_instruction_port_is_candidate(transaction) ||
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

        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        if (!next_id(transaction, &id) || !prepare_leg(transaction, &leg, id)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = transaction->last_transaction_id == UINT64_MAX
                                 ? TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED
                                 : TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        result->transaction_id = id;
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
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
    adapter_result = ti_c54x_bus_word16_adapter_advance(
        endpoint,
        first_issue ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                    : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
        &transaction->bus_transaction);
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = adapter_result.bus_status;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return result->status;
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
    if (transaction->candidate.port_address_snapshot.second_leg_issued ==
        UINT8_C(0)) {
        ti_c54x_bus_word16_request leg;
        uint64_t id;

        if (!build_second_leg_request(
                transaction,
                transaction->bus_transaction.staged_read_value_word, &leg) ||
            !next_id(transaction, &id) ||
            !prepare_leg(transaction, &leg, id)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->candidate.port_address_snapshot.second_leg_issued =
            UINT8_C(1);
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
        if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            result->instructions_attempted = UINT64_C(0);
            return result->status;
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
ti_c54x_memory_instruction_port_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (!ti_c54x_memory_instruction_port_is_candidate(transaction)) {
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
ti_c54x_memory_instruction_port_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_port_is_candidate(transaction) ||
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
               transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
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
