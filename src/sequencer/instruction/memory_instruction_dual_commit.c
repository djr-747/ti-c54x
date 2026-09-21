/*
 * Module contract
 *
 * Owns: the READY-path commit for one dual-indirect MOD0/MOD1/MOD2 WORD16
 * read pair. It stages the ordinary complete_dual effect, applies the
 * read-phase X-side T load when the staged effect requests it, and then
 * publishes both validated post-access AR commits and the sequential PC
 * atomically.
 * Does not own: request preparation, endpoint issue/resume, pair lifecycle,
 * cancellation/reset, opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics, allocation, I/O,
 * or global state. No architectural field is written until ordinary-effect
 * staging succeeds.
 */

#include "memory_instruction_dual_commit.h"

#include <stddef.h>
#include <stdint.h>

#include "sequencer/effect_transaction.h"
#include "state/temporary/t_register.h"

static ti_c54x_memory_instruction_status commit_architecture(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state)
{
    state->pc_word = transaction->candidate.pc_effect.public_pc_word_after;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}

/* Publish both validated post-access AR updates. The snapshot commits were
 * rechecked against the copied shadows and live state at the commit
 * boundary, so these stores are infallible and land together: distinct
 * selectors publish two updates, a same-selector alias publishes the one
 * X-owned update, and MOD0 sides change nothing. Shared with the
 * dual-move root, whose Y WRITE leg carries the same post-access
 * commit shape. */
void ti_c54x_memory_instruction_dual_publish_address_commits(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    ti_c54x_arch_state *state)
{
    if (snapshot->x_commit.target == TI_C54X_ADDRESS_UPDATE_AR) {
        state->ar[snapshot->x_commit.ar_index] =
            snapshot->x_commit.value_after;
        state->ar_known_mask |=
            (uint8_t)(UINT8_C(1) << snapshot->x_commit.ar_index);
    }
    if (snapshot->y_commit.target == TI_C54X_ADDRESS_UPDATE_AR &&
        !(snapshot->x_commit.target == TI_C54X_ADDRESS_UPDATE_AR &&
          snapshot->y_commit.ar_index == snapshot->x_commit.ar_index)) {
        state->ar[snapshot->y_commit.ar_index] =
            snapshot->y_commit.value_after;
        state->ar_known_mask |=
            (uint8_t)(UINT8_C(1) << snapshot->y_commit.ar_index);
    }
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t x_word, uint16_t y_word)
{
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;
    ti_c54x_op_effect next_effect = {0};
    ti_c54x_effect_transaction ordinary = {0};

    if (memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        memory->complete_dual == NULL ||
        memory->complete_dual(&transaction->candidate.instruction,
                              &transaction->candidate.input, x_word, y_word,
                              &next_effect) != TI_C54X_OP_STATUS_OK ||
        ti_c54x_effect_transaction_stage(
            &next_effect, &state->status, accumulators, t_register,
            &state->mmr_file, &ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
        ti_c54x_effect_transaction_commit(
            &ordinary, &state->status, accumulators, t_register,
            &state->mmr_file) != TI_C54X_EFFECT_TRANSACTION_OK) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (memory->load_t_with_read) {
        /* Read-phase T load (MAC Xmem,Ymem): T takes the X-side word
         * once the pair completion has committed. Staged on a
         * candidate so a rejected write leaves live state
         * untouched. */
        ti_c54x_t_register_state t_candidate;

        if (t_register == NULL) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        t_candidate = *t_register;
        if (ti_c54x_t_register_write(&t_candidate, x_word) !=
            TI_C54X_T_REGISTER_OK) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        *t_register = t_candidate;
    }
    /* Ordinary-effect staging succeeded after both reads: publish the address
     * updates and PC together with no further fallible step between them. */
    ti_c54x_memory_instruction_dual_publish_address_commits(
        &transaction->candidate.dual_address_snapshot, state);
    return commit_architecture(transaction, state);
}
