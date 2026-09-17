/*
 * Module contract
 *
 * Owns: the READY-path commit for one DATA+PROGRAM WORD16 read
 * pair. It stages the ordinary complete_dual effect, applies the
 * read-phase Smem-side T load when the staged effect requests it,
 * and then publishes the single validated Smem post-access AR
 * commit and the sequential PC atomically.
 * Does not own: request preparation, endpoint issue/resume, pair
 * lifecycle, cancellation/reset, opcode semantics, address
 * arithmetic, endpoint storage, timing, CPU lifecycle, observers,
 * diagnostics, allocation, I/O, or global state. No architectural
 * field is written until ordinary-effect staging succeeds.
 */

#include "memory_instruction_data_program_commit.h"

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

/* Publish the single validated Smem post-access AR update. The
 * snapshot commit was rechecked against the copied shadows and
 * live state at the commit boundary, so this store is infallible;
 * the PROGRAM leg carries a zeroed no-update commit and publishes
 * nothing. Direct and MOD0 Smem legs likewise change nothing. */
static void publish_smem_address_commit(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    ti_c54x_arch_state *state)
{
    if (snapshot->x_commit.target == TI_C54X_ADDRESS_UPDATE_AR) {
        state->ar[snapshot->x_commit.ar_index] =
            snapshot->x_commit.value_after;
        state->ar_known_mask |=
            (uint8_t)(UINT8_C(1) << snapshot->x_commit.ar_index);
    }
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_commit_ready(
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
            &ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
        ti_c54x_effect_transaction_commit(
            &ordinary, &state->status, accumulators, t_register) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (memory->load_t_with_read) {
        /* Read-phase T load (MACP Smem): T takes the Smem-side word
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
    /* Ordinary-effect staging succeeded after both reads: publish the
     * Smem address update and PC together with no further fallible
     * step between them. */
    publish_smem_address_commit(
        &transaction->candidate.dual_address_snapshot, state);
    return commit_architecture(transaction, state);
}
