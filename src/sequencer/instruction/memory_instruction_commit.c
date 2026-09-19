/*
 * Module contract
 *
 * Owns: the READY-path WORD16 commit for one root memory instruction:
 * address-commit publication (AR/SP), stack-push pmad-target commit
 * with atomic RTN staging of the frozen return address, stack-pop
 * TOS-target commit from the READ observation or the frozen RTN value,
 * and static opcode completion plus ordinary effects ahead of the
 * architecture commit. Single-word delay shapes without a completion
 * skip the completion and stage nothing: the delay WRITE is the only
 * publication. Delayed stack branches commit the sequential
 * successor here; WAIT, FAULT, cancellation, and other nonterminal
 * paths never reach this owner.
 * Does not own: request preparation, endpoint issue/resume, pair
 * lifecycle, cancellation/reset, opcode semantics, address arithmetic,
 * endpoint storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. All inputs are borrowed for one
 * call; the transaction's copied snapshot must already be validated
 * against the frozen candidate and live state before this call.
 */

#include "memory_instruction_commit.h"

#include <stddef.h>

#include "memory_instruction_rmw.h"
#include "memory_instruction_stack_push.h"
#include "bus/mmr/mmr_route.h"
#include "bus/request/word16_request.h"
#include "sequencer/effect_transaction.h"
#include "sequencer/memory/mmr_access_plan.h"
#include "sequencer/program_pc_target.h"
#include "state/status/registers.h"
#include "state/temporary/t_register.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_architecture(ti_c54x_memory_instruction_transaction *transaction,
                    ti_c54x_arch_state *state)
{
    /* The PORT DATA-leg commit rides the pair snapshot: the generic
     * slot shares union storage with that snapshot, so it holds no
     * DATA commit for PORT candidates. The stack-pair SP commit
     * rides the snapshot's TOS stack-leg resolution for the same
     * reason. */
    const ti_c54x_sequencer_address_commit *const address_commit =
        transaction->candidate.effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16
            ? &transaction->candidate.port_address_snapshot.data_resolution
                   .address_commit
            : (transaction->candidate.effect.payload.memory.address_spec
                       .kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 ||
               transaction->candidate.effect.payload.memory.address_spec
                       .kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16)
            ? &transaction->candidate.stack_pair_address_snapshot
                   .stack_resolution.address_commit
            : &transaction->candidate.address_resolution.address_commit;

    if (address_commit->target == TI_C54X_ADDRESS_UPDATE_AR) {
        state->ar[address_commit->ar_index] = address_commit->value_after;
        state->ar_known_mask |=
            (uint8_t)(UINT8_C(1) << address_commit->ar_index);
    } else if (address_commit->target == TI_C54X_ADDRESS_UPDATE_SP) {
        state->sp = address_commit->value_after;
        state->scalar_known_mask |= (uint8_t)TI_C54X_ARCH_STATE_KNOWN_SP;
    }
    if (transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 ||
        transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) {
        /* The stack leg above publishes SP; the Smem DATA leg
         * publishes its own update beside it (dual-pair
         * precedent: both stores are infallible and land
         * together). Direct and MMR DATA legs carry no update;
         * indirect legs publish the AR update. */
        const ti_c54x_sequencer_address_commit *const data_commit =
            &transaction->candidate.stack_pair_address_snapshot
                 .data_resolution.address_commit;

        if (data_commit->target == TI_C54X_ADDRESS_UPDATE_AR) {
            state->ar[data_commit->ar_index] = data_commit->value_after;
            state->ar_known_mask |=
                (uint8_t)(UINT8_C(1) << data_commit->ar_index);
        }
    }
    if (transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
        transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
        transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
        transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
        /* Pushes redirect PC to the frozen target instead of the
         * sequential successor. Re-plan deterministically from the
         * frozen target and the stable live PC; prepare already proved
         * this plan, so a failure here is unreachable but still
         * transactional. Two-word pushes read the target from the
         * frozen bundle word; one-word accumulator pushes re-derive
         * it from the frozen input snapshot gated on accumulator
         * knownness. The frozen staged return address additionally
         * loads RTN atomically with the push (SPRU131G §6: calls load
         * RTN with the address they push). Delayed pushes only prove
         * the deferred target here: the two delay-slot words execute
         * first, so PC advances to the frozen sequential successor
         * and the W67B CPU loop reports the target from the
         * retirement result. */
        const ti_c54x_instruction_word_bundle *bundle_words =
            &transaction->candidate.instruction.bundle;
        const ti_c54x_op_input_snapshot *frozen_input =
            &transaction->candidate.input;
        uint8_t push_select =
            transaction->candidate.effect.payload.memory.address_spec.arf;
        ti_c54x_program_pc_target_effect call_target = {0};
        uint32_t live_pc = state->pc_word;
        uint16_t pmad;

        if (transaction->candidate.effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
            transaction->candidate.effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
            if (bundle_words->program_word_count != UINT8_C(1)) {
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
            if (push_select == UINT8_C(0)) {
                if (!frozen_input->accumulator_a_known) {
                    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                }
                pmad = (uint16_t)(frozen_input->accumulator_a_raw40 &
                                  UINT64_C(0xFFFF));
            } else if (push_select == UINT8_C(1)) {
                if (!frozen_input->accumulator_b_known) {
                    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
                }
                pmad = (uint16_t)(frozen_input->accumulator_b_raw40 &
                                  UINT64_C(0xFFFF));
            } else {
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
        } else {
            if (bundle_words->program_word_count != UINT8_C(2)) {
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
            pmad = bundle_words->program_words[1];
        }
        if (ti_c54x_program_pc_target_plan(live_pc, pmad, &call_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (transaction->candidate.effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
            transaction->candidate.effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
            state->pc_word =
                transaction->candidate.pc_effect.public_pc_word_after;
        } else {
            if (ti_c54x_program_pc_target_apply(&live_pc, &call_target) !=
                TI_C54X_PROGRAM_PC_TARGET_OK) {
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
            state->pc_word = live_pc;
        }
        state->rtn = transaction->candidate.effect.payload.memory.value_word;
        state->scalar_known_mask |= (uint8_t)TI_C54X_ARCH_STATE_KNOWN_RTN;
    } else {
        state->pc_word = transaction->candidate.pc_effect.public_pc_word_after;
    }
    if (transaction->candidate.effect.payload.memory.rmw_operation ==
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD) {
        /* ADDM (SPRU172C §ADDM) affects C and OVA with sticky-set
         * policy matching the accumulator ADD precedent: C takes 1 on
         * the frozen carry and OVA takes 1 on the frozen overflow,
         * both otherwise preserved. Staged on a candidate so a
         * rejected write leaves live status untouched. */
        ti_c54x_status_register_state candidate_status = state->status;

        if (transaction->rmw_add_carry &&
            ti_c54x_status_register_write_c(&candidate_status,
                                            UINT8_C(1)) !=
                TI_C54X_STATUS_REGISTER_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (transaction->rmw_add_overflow &&
            ti_c54x_status_register_write_ova(&candidate_status,
                                              UINT8_C(1)) !=
                TI_C54X_STATUS_REGISTER_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        state->status = candidate_status;
    }
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_pop_architecture(ti_c54x_memory_instruction_transaction *transaction,
                        ti_c54x_arch_state *state, uint16_t tos_value)
{
    const ti_c54x_sequencer_address_commit *const address_commit =
        &transaction->candidate.address_resolution.address_commit;
    ti_c54x_program_pc_target_effect pop_target = {0};
    uint32_t live_pc = state->pc_word;

    /* Plan the TOS target before mutating anything: a failure here is
     * unreachable (prepare proved plannability with the same live PC)
     * but stays transactional. Delayed pops only prove the deferred
     * TOS target here: the two delay-slot words execute first, so PC
     * advances to the frozen sequential successor and the W67B CPU
     * loop reports TOS from the retirement result. */
    if (address_commit->target != TI_C54X_ADDRESS_UPDATE_SP ||
        ti_c54x_program_pc_target_plan(live_pc, tos_value, &pop_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    state->sp = address_commit->value_after;
    state->scalar_known_mask |= (uint8_t)TI_C54X_ARCH_STATE_KNOWN_SP;
    if (transaction->candidate.effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16) {
        state->pc_word =
            transaction->candidate.pc_effect.public_pc_word_after;
    } else {
        if (ti_c54x_program_pc_target_apply(&live_pc, &pop_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        state->pc_word = live_pc;
    }
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_ready(ti_c54x_memory_instruction_transaction *transaction,
             ti_c54x_arch_state *state,
             ti_c54x_accumulator_state *accumulators,
             ti_c54x_t_register_state *t_register,
             uint16_t read_value_word)
{
    ti_c54x_effect_transaction ordinary = {0};
    ti_c54x_op_effect next_effect = {0};
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;

    if (memory->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
        memory->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 ||
        memory->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        /* Pops consume the READ observation in the pop commit (RTN pops
         * consume the frozen RTN value the no-traffic advance path
         * passes here instead). A non-null completion additionally
         * stages one follow-on effect (for example a status clear)
         * committed here before the pop architecture commit; failures
         * stay transactional exactly like the load completion path
         * below. */
        if (memory->complete != NULL) {
            ti_c54x_effect_transaction pop_ordinary = {0};
            ti_c54x_op_effect pop_next_effect = {0};

            if (memory->complete(&transaction->candidate.instruction,
                                 &transaction->candidate.input,
                                 read_value_word,
                                 &pop_next_effect) != TI_C54X_OP_STATUS_OK) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
            if (ti_c54x_effect_transaction_stage(
                    &pop_next_effect, &state->status, accumulators,
                    t_register, &state->mmr_file,
                    &pop_ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
                ti_c54x_effect_transaction_commit(
                    &pop_ordinary, &state->status, accumulators,
                    t_register, &state->mmr_file) !=
                TI_C54X_EFFECT_TRANSACTION_OK) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
        }
        return ti_c54x_memory_instruction_commit_pop_architecture(transaction, state, read_value_word);
    }
    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        /* A WRITE carrying a completion (CMPS TRN/TC precedent)
         * runs it after the write resolves with the staged value
         * word, committing at most one ordinary follow-on before
         * the architecture commit; failures fault transactionally
         * exactly like the read-value completion path below. The
         * paired RMW write leg never carries a completion. */
        if (memory->complete != NULL) {
            if (memory->complete(&transaction->candidate.instruction,
                                 &transaction->candidate.input,
                                 memory->value_word,
                                 &next_effect) != TI_C54X_OP_STATUS_OK ||
                ti_c54x_effect_transaction_stage(
                    &next_effect, &state->status, accumulators,
                    t_register, &state->mmr_file,
                    &ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
                ti_c54x_effect_transaction_commit(
                    &ordinary, &state->status, accumulators,
                    t_register, &state->mmr_file) !=
                    TI_C54X_EFFECT_TRANSACTION_OK) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
        }
        return ti_c54x_memory_instruction_commit_architecture(transaction, state);
    }
    if (ti_c54x_memory_instruction_memory_is_rmw(memory) &&
        transaction->bus_transaction.request.access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return ti_c54x_memory_instruction_commit_architecture(transaction, state);
    }
    if (memory->complete == NULL) {
        /* Single-word delay shapes without a completion (DELAY)
         * publish only the delay WRITE: skip the read-value
         * completion and the follow-on stage below, falling through
         * to the address/PC tail. Reads without delay still
         * reject. Reads with the MMR store sidecar (MVDM) publish
         * the MMR write in the sidecar block below. */
        if (!memory->write_smem_delay && !memory->mmr_store_with_read) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else {
        if (memory->complete(
                &transaction->candidate.instruction,
                &transaction->candidate.input, read_value_word,
                &next_effect) != TI_C54X_OP_STATUS_OK) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (ti_c54x_effect_transaction_stage(
                &next_effect, &state->status, accumulators, t_register,
                &state->mmr_file, &ordinary) !=
                TI_C54X_EFFECT_TRANSACTION_OK ||
            ti_c54x_effect_transaction_commit(
                &ordinary, &state->status, accumulators, t_register,
                &state->mmr_file) != TI_C54X_EFFECT_TRANSACTION_OK) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    if (memory->load_t_with_read) {
        /* Read-phase T load (MPYA): T takes the read value word once
         * the read-value completion has committed. Staged on a
         * candidate so a rejected write leaves live state
         * untouched. */
        ti_c54x_t_register_state t_candidate;

        if (t_register == NULL) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        t_candidate = *t_register;
        if (ti_c54x_t_register_write(&t_candidate, read_value_word) !=
            TI_C54X_T_REGISTER_OK) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        *t_register = t_candidate;
    }
    if (memory->mmr_store_with_read) {
        /* Read-phase MMR store (MVDM): the observed DATA word
         * publishes to the MMR destination through the router
         * after the read resolves. Mirror the RMW-MMR write
         * preflight: plan, route, and fault when the router
         * declines (the shape gate keeps routable destinations,
         * so a decline here is a forged candidate). */
        ti_c54x_sequencer_mmr_access store_plan;
        ti_c54x_bus_word16_request_result store_built;
        ti_c54x_bus_mmr_route_result store_result;

        store_plan = ti_c54x_sequencer_plan_mmr_access(
            state, accumulators, t_register,
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            (uint32_t)memory->mmr_store_number,
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, read_value_word);
        if (store_plan != TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        store_built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            (uint32_t)memory->mmr_store_number, read_value_word);
        if (store_built.status !=
            TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        store_result = ti_c54x_bus_mmr_route(
            &store_built.request, state, accumulators, t_register);
        if (store_result.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    return ti_c54x_memory_instruction_commit_architecture(transaction, state);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_ready_tail(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, uint16_t read_value_word,
    bool first_issue)
{
    ti_c54x_memory_instruction_status operation_status;

    operation_status = ti_c54x_memory_instruction_commit_ready(
        transaction, state, accumulators, t_register, read_value_word);
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
        result->deferred_branch_pc_word = read_value_word;
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
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

