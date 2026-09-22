/*
 * Module contract
 *
 * Owns: the allocation-free root prepare stage for memory
 * instruction transactions: candidate validation, per-family
 * resolution (WORD16, dual, pair, LONG32, DATA+PROGRAM),
 * return-address and RTN freezing, far-call XPC staging through
 * the far-push owner, the far-return inte completion contract
 * through the far-pop owner, and child arming. Sequent PC/counter
 * candidates and the PREPARED phase commit close preparation.
 * Does not own: bus issue/resume, READY-path commit, CPU
 * lifecycle, physical memory, endpoint context, opcode dispatch,
 * timing, observers, diagnostics, or global state.
 */

#include "memory_instruction_transaction.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_child_route.h"
#include "memory_instruction_data_program.h"
#include "memory_instruction_dual.h"
#include "memory_instruction_dual_move.h"
#include "memory_instruction_far_pop.h"
#include "memory_instruction_far_push.h"
#include "memory_instruction_long32.h"
#include "memory_instruction_pair_dispatch.h"
#include "memory_instruction_rmw.h"
#include "memory_instruction_stack_push.h"
#include "sequencer/memory/dual_address_resolution.h"
#include "sequencer/memory/mmr_access_plan.h"
#include "sequencer/program_pc_bridge.h"
#include "sequencer/program_pc_target.h"
#include "state/program_counter_extension.h"

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
    ti_c54x_memory_instruction_pair_route pair_route;
    bool dual_read;
    bool dual_move;
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
    dual_move = effect->payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16;
    pair_route = ti_c54x_memory_instruction_pair_route_for_kind(
        effect->payload.memory.address_spec.kind);
    stack_push = ti_c54x_op_memory_address_kind_is_stack_push(
        effect->payload.memory.address_spec.kind);
    stack_pop = effect->payload.memory.address_spec.kind ==
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
                effect->payload.memory.address_spec.kind ==
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 ||
                ti_c54x_op_memory_address_kind_is_far_pop(
                    effect->payload.memory.address_spec.kind);
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
    } else if (dual_move) {
        /* Move-pair resolution lives in the move root owner:
         * the X READ and Y WRITE template resolve together
         * from the frozen dual arm. Nothing is resolved here. */
    } else if (pair_route !=
               TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_NONE) {
        /* Pair resolution lives in the routed child (PORT IO/DATA
         * legs, stack TOS/Smem legs, Smem-absolute legs, or
         * Smem-program legs). Nothing is resolved here: the
         * generic resolver knows no pair kinds. */
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
    target->repeat_start_pending = false;
    target->repeat_start_operand = UINT16_C(0);
    /* Paired forms pay their cold helper call; keep the established WORD16
     * preparation path direct. */
    if (dual_read) {
        if (!ti_c54x_memory_instruction_dual_prepare_snapshot(target)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (dual_move) {
        if (!ti_c54x_memory_instruction_dual_move_pair_prepare_snapshot(
                target, state)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (candidate.effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16) {
        return ti_c54x_memory_instruction_data_program_prepare_candidate(
            target, state);
    } else if (candidate.effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32 ||
               candidate.effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE ||
               candidate.effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16 ||
               candidate.effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_LOAD_WORD16) {
        if (!ti_c54x_memory_instruction_long32_prepare_snapshot(target)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (pair_route !=
               TI_C54X_MEMORY_INSTRUCTION_PAIR_ROUTE_NONE) {
        if (!ti_c54x_memory_instruction_pair_route_prepare_snapshot(
                pair_route, target, state)) {
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
    if (ti_c54x_op_memory_address_kind_is_far_push(
            effect->payload.memory.address_spec.kind)) {
        /* Far double-push XPC staging lives in the far-push owner;
         * the root only maps the outcome. Staged after child arming
         * because the WORD16 arm clears the shared staging slot. */
        ti_c54x_memory_instruction_status far_status =
            ti_c54x_memory_instruction_far_push_prepare(
                transaction, effect->payload.memory.address_spec.kind,
                state);
        if (far_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_OK) {
            return far_status;
        }
    }
    if (ti_c54x_op_memory_address_kind_is_far_pop(
            effect->payload.memory.address_spec.kind)) {
        /* The far-pop inte completion contract lives in the
         * far-pop owner; the root only maps the outcome. */
        ti_c54x_memory_instruction_status far_status =
            ti_c54x_memory_instruction_far_pop_prepare(
                transaction, effect->payload.memory.address_spec.kind,
                effect->payload.memory.complete);
        if (far_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_OK) {
            return far_status;
        }
    }
    if (!ti_c54x_memory_instruction_software_interrupt_freeze_vector(
            instruction, effect, state,
            &target->long32_first_value_msw)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    target->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

