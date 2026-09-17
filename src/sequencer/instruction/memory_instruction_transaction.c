/*
 * Module contract
 *
 * Owns: the allocation-free root transaction that composes memory address
 * resolution, the WORD16 issue/resume reducer, dual-WORD16 and LONG32
 * reducers, sequential PC/counter candidates, stack-push return-address
 * staging, no-traffic RTN-pop retirement from the frozen RTN value, and
 * the delayed-branch twins that report the deferred target in the
 * retirement result. READY-path architecture commit lives in
 * memory_instruction_commit.c.
 * Does not own: CPU lifecycle, physical memory, endpoint context, opcode
 * dispatch, timing, observers, diagnostics, or global state.
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
#include "memory_instruction_rmw.h"
#include "memory_instruction_stack_push.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_commit.h"
#include "memory_instruction_data_program.h"
#include "memory_instruction_dual.h"
#include "memory_instruction_long32.h"
#include "memory_instruction_port.h"
#include "memory_instruction_pair_dispatch.h"

static void
clear_preserving_sequence(ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

static bool
candidate_snapshot_matches(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *const memory =
        transaction == NULL || transaction->candidate.effect.kind !=
                                  TI_C54X_OP_EFFECT_MEMORY
            ? NULL
            : &transaction->candidate.effect.payload.memory;

    if (memory == NULL) {
        return false;
    }
    if (transaction->child_kind == TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32) {
        return memory->address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32 &&
               memory->first_value_msw ==
                   transaction->long32_first_value_msw &&
               memory->second_value_lsw ==
                   transaction->long32_second_value_lsw;
    }
    return transaction->child_kind == TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16 &&
           memory->rmw_operation == transaction->candidate_rmw_operation &&
           memory->value_word == transaction->candidate_rmw_value_word;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint64_t instructions_attempted_before,
    uint64_t instructions_retired_before)
{
    ti_c54x_memory_instruction_candidate candidate = {0};
    ti_c54x_memory_address_resolution resolution = {0};
    ti_c54x_dual_memory_address_resolution dual_resolution = {0};
    ti_c54x_program_pc_bridge_effect pc_effect = {0};
    uint16_t push_return_value = UINT16_C(0);
    uint16_t rtn_return_value = UINT16_C(0);
    ti_c54x_memory_instruction_transaction *target = transaction;
    bool dual_read;
    bool port_pair;
    bool stack_push;
    bool stack_pop;
    bool stack_pop_rtn;

    if (target == NULL || instruction == NULL || input == NULL ||
        effect == NULL || state == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (target->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE ||
        target->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY ||
        !ti_c54x_op_memory_effect_is_valid(&effect->payload.memory)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (instructions_attempted_before == UINT64_MAX ||
        instructions_retired_before == UINT64_MAX) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW;
    }
    dual_read = effect->payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16;
    port_pair = effect->payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16;
    stack_push = effect->payload.memory.address_spec.kind ==
                     TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
                 effect->payload.memory.address_spec.kind ==
                     TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
                 effect->payload.memory.address_spec.kind ==
                     TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
                 effect->payload.memory.address_spec.kind ==
                     TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16;
    stack_pop = effect->payload.memory.address_spec.kind ==
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
                effect->payload.memory.address_spec.kind ==
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16;
    stack_pop_rtn = effect->payload.memory.address_spec.kind ==
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16;
    if (stack_pop) {
        ti_c54x_program_pc_target_effect pop_target = {0};

        /* Prove any 16-bit TOS value is plannable against the live PC
         * carrier before any bus issue. Commit re-plans deterministically
         * from the transaction's own READ observation, so the discarded
         * effect is validation only. */
        if (ti_c54x_program_pc_target_plan(state->pc_word, UINT16_C(0),
                                           &pop_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    if (stack_pop_rtn) {
        ti_c54x_program_pc_target_effect rtn_target = {0};
        uint16_t rtn_value = UINT16_C(0);

        /* Fast returns take the PC target from the live RTN register
         * instead of a stack READ observation. Unknown RTN rejects
         * before any bus issue; the value freezes into the candidate
         * below, so the discarded effect is validation only. */
        if (ti_c54x_arch_state_read_rtn(state, &rtn_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (ti_c54x_program_pc_target_plan(state->pc_word, rtn_value,
                                           &rtn_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        rtn_return_value = rtn_value;
    }
    if (stack_push) {
        ti_c54x_memory_instruction_status push_status;

        /* Stack-push target planning, return staging, and TOS
         * resolution live in the push owner; the root only freezes
         * the outputs. */
        push_status = ti_c54x_memory_instruction_stack_push_prepare(
            instruction, input, effect, state, &resolution, &pc_effect,
            &push_return_value);
        if (push_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_OK) {
            return push_status;
        }
        candidate.address_resolution = resolution;
    } else if (dual_read) {
        if (ti_c54x_sequencer_resolve_dual_memory_address(
                state, &effect->payload.memory.address_spec,
                effect->payload.memory.access, UINT16_C(0),
                &dual_resolution) !=
            TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (port_pair) {
        /* PORT resolution lives in the child: the IO-leg planner plus
         * the DATA leg through the single-leg resolver. Nothing is
         * resolved here. */
    } else if (effect->payload.memory.address_spec.kind !=
               TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16) {
        if (ti_c54x_sequencer_resolve_memory_address(
                state, &effect->payload.memory.address_spec,
                effect->payload.memory.access,
                ti_c54x_memory_instruction_memory_is_rmw(
                    &effect->payload.memory)
                    ? UINT16_C(0)
                    : effect->payload.memory.value_word,
                &resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        candidate.address_resolution = resolution;
    }
    if (!stack_push) {
        if (ti_c54x_program_pc_bridge_plan(
                state->pc_word, instruction->bundle.program_word_count,
                &pc_effect) != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    candidate.instruction = *instruction;
    candidate.input = *input;
    candidate.effect = *effect;
    if (stack_push) {
        /* Stage the live return address into the frozen copy. The pure
         * handler cannot know PC, and the shape check requires the frozen
         * effect value to equal the issued TOS WRITE value. Delayed
         * pushes stage PC+4 (bounds-proved in the push arm above);
         * immediate pushes stage the sequential successor. */
        candidate.effect.payload.memory.value_word = push_return_value;
    }
    if (stack_pop_rtn) {
        /* Freeze the live RTN value proved above into the candidate.
         * Commit consumes this frozen value with no bus observation,
         * and the liveness guard pins it against the live register
         * on every resume. */
        candidate.effect.payload.memory.value_word = rtn_return_value;
    }
    if (dual_read) {
        candidate.dual_address_snapshot.x_request = dual_resolution.x_request;
        candidate.dual_address_snapshot.y_request = dual_resolution.y_request;
        candidate.dual_address_snapshot.x_commit =
            dual_resolution.x_address_commit;
        candidate.dual_address_snapshot.y_commit =
            dual_resolution.y_address_commit;
        candidate.dual_address_snapshot.ar0_shadow = dual_resolution.ar0_value;
        candidate.dual_address_snapshot.bk_shadow = dual_resolution.bk_value;
    }
    candidate.pc_effect = pc_effect;
    candidate.instructions_attempted_after =
        instructions_attempted_before + UINT64_C(1);
    candidate.instructions_retired_after = instructions_retired_before +
                                           UINT64_C(1);
    target->candidate = candidate;
    /* Paired forms pay their cold helper call; keep the established WORD16
     * preparation path direct. */
    if (dual_read) {
        if (!ti_c54x_memory_instruction_dual_prepare_snapshot(target)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (candidate.effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16) {
        return ti_c54x_memory_instruction_data_program_prepare_candidate(
            target, state);
    } else if (candidate.effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        if (!ti_c54x_memory_instruction_long32_prepare_snapshot(target)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (port_pair) {
        if (!ti_c54x_memory_instruction_port_prepare_snapshot(target,
                                                              state)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else {
        target->child_kind = TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16;
        target->candidate_rmw_operation =
            candidate.effect.payload.memory.rmw_operation;
        target->candidate_rmw_value_word =
            candidate.effect.payload.memory.value_word;
        target->long32_first_value_msw = UINT16_C(0);
    }
    target->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
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

    if (!candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_rmw_transform(
                       &transaction->candidate.effect.payload.memory,
                       transaction->bus_transaction.staged_read_value_word,
                       &transaction->rmw_write_value_word) ||
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
    if (!candidate_snapshot_matches(transaction) ||
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
    if (!candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_rmw_transform(
                       &transaction->candidate.effect.payload.memory,
                       transaction->bus_transaction.staged_read_value_word,
                       &transaction->rmw_write_value_word) ||
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
    if (!candidate_snapshot_matches(transaction) ||
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

    if (!candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
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
            } else if (transaction->candidate.effect.payload.memory.address_spec
                           .kind ==
                           TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
                       transaction->candidate.effect.payload.memory.address_spec
                               .kind ==
                           TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
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
        (!candidate_snapshot_matches(transaction) ||
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
    {
        const uint16_t ready_read_word = adapter_result.read_value_word;

        operation_status = ti_c54x_memory_instruction_commit_ready(transaction, state, accumulators,
                                        t_register, ready_read_word);
        if (operation_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
            return operation_status;
        }
        /* Delayed stack branches retire their stack update here but
         * defer the PC transfer: report the deferred target so the
         * CPU loop can arm the two-word countdown. Pops defer the
         * TOS READ observation; pushes defer the frozen pmad word.
         * All other retirements leave the report disarmed. */
        if (transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16) {
            result->deferred_branch_armed = true;
            result->deferred_branch_pc_word = ready_read_word;
        } else if (transaction->candidate.effect.payload.memory.address_spec
                           .kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
                   transaction->candidate.effect.payload.memory.address_spec
                           .kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
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
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
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
