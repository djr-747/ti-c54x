/*
 * Module contract
 *
 * Owns: the READY-path WORD16 commit for one root memory instruction:
 * address-commit publication (AR/SP), stack-push pmad-target commit
 * with atomic RTN staging of the frozen return address, stack-pop
 * TOS-target commit from the READ observation or the frozen RTN value,
 * and static opcode completion plus ordinary effects ahead of the
 * architecture commit. Delayed stack branches commit the sequential
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
#include "sequencer/effect_transaction.h"
#include "sequencer/program_pc_target.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_architecture(ti_c54x_memory_instruction_transaction *transaction,
                    ti_c54x_arch_state *state)
{
    const ti_c54x_sequencer_address_commit *const address_commit =
        &transaction->candidate.address_resolution.address_commit;

    if (address_commit->target == TI_C54X_ADDRESS_UPDATE_AR) {
        state->ar[address_commit->ar_index] = address_commit->value_after;
        state->ar_known_mask |=
            (uint8_t)(UINT8_C(1) << address_commit->ar_index);
    } else if (address_commit->target == TI_C54X_ADDRESS_UPDATE_SP) {
        state->sp = address_commit->value_after;
        state->scalar_known_mask |= (uint8_t)TI_C54X_ARCH_STATE_KNOWN_SP;
    }
    if (transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
        transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16) {
        /* Pushes redirect PC to the frozen pmad instead of the sequential
         * successor. Re-plan deterministically from the frozen bundle word
         * and the stable live PC; prepare already proved this plan, so a
         * failure here is unreachable but still transactional. The frozen
         * staged return address additionally loads RTN atomically with
         * the push (SPRU131G §6: calls load RTN with the address they
         * push). Delayed pushes only prove the deferred pmad target here:
         * the two delay-slot words execute first, so PC advances to the
         * frozen sequential successor and the W67B CPU loop reports pmad
         * from the retirement result. */
        const ti_c54x_instruction_word_bundle *bundle_words =
            &transaction->candidate.instruction.bundle;
        ti_c54x_program_pc_target_effect call_target = {0};
        uint32_t live_pc = state->pc_word;
        uint16_t pmad;

        if (bundle_words->program_word_count != UINT8_C(2)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        pmad = bundle_words->program_words[1];
        if (ti_c54x_program_pc_target_plan(live_pc, pmad, &call_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (transaction->candidate.effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16) {
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
                    t_register,
                    &pop_ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
                ti_c54x_effect_transaction_commit(
                    &pop_ordinary, &state->status, accumulators,
                    t_register) != TI_C54X_EFFECT_TRANSACTION_OK) {
                transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
                return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            }
        }
        return ti_c54x_memory_instruction_commit_pop_architecture(transaction, state, read_value_word);
    }
    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        (ti_c54x_memory_instruction_memory_is_rmw(memory) &&
         transaction->bus_transaction.request.access ==
             TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE)) {
        return ti_c54x_memory_instruction_commit_architecture(transaction, state);
    }
    if (memory->complete == NULL || memory->complete(
                         &transaction->candidate.instruction,
                         &transaction->candidate.input, read_value_word,
                         &next_effect) != TI_C54X_OP_STATUS_OK) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (ti_c54x_effect_transaction_stage(
            &next_effect, &state->status, accumulators, t_register,
            &ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
        ti_c54x_effect_transaction_commit(
            &ordinary, &state->status, accumulators, t_register) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    return ti_c54x_memory_instruction_commit_architecture(transaction, state);
}

