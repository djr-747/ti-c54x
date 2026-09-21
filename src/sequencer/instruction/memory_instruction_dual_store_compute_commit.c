/*
 * Module contract
 *
 * Owns: the READY-path commit for one dual-indirect WORD16
 * store-compute pair; see
 * memory_instruction_dual_store_compute_commit.h.
 * Does not own: request preparation, endpoint issue/resume,
 * pair lifecycle, cancellation/reset, opcode semantics,
 * address arithmetic, endpoint storage, timing, CPU lifecycle,
 * observers, diagnostics, allocation, I/O, or global state. No
 * architectural field is written until completion staging
 * succeeds.
 */

#include "memory_instruction_dual_store_compute_commit.h"

#include <stddef.h>
#include <stdint.h>

#include "memory_instruction_dual_commit.h"
#include "sequencer/effect_transaction.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    struct ti_c54x_arch_state *state,
    struct ti_c54x_accumulator_state *accumulators,
    struct ti_c54x_t_register_state *t_register,
    uint16_t x_value_word)
{
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;
    ti_c54x_op_effect next_effect = {0};
    ti_c54x_effect_transaction ordinary = {0};

    if (transaction == NULL || state == NULL || accumulators == NULL ||
        t_register == NULL ||
        memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        memory->complete == NULL ||
        memory->complete(&transaction->candidate.instruction,
                         &transaction->candidate.input, x_value_word,
                         &next_effect) != TI_C54X_OP_STATUS_OK ||
        ti_c54x_effect_transaction_stage(
            &next_effect, &state->status, accumulators, t_register,
            &state->mmr_file, &ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
        ti_c54x_effect_transaction_commit(
            &ordinary, &state->status, accumulators, t_register,
            &state->mmr_file) != TI_C54X_EFFECT_TRANSACTION_OK) {
        if (transaction != NULL) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        }
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    /* Completion staging succeeded after both legs: publish the
     * address updates and PC together with no further fallible
     * step between them. The AR publisher is shared with the
     * dual READ and MOVE roots: an X/Y alias publishes the one
     * X-owned update and MOD0 sides change nothing. */
    ti_c54x_memory_instruction_dual_publish_address_commits(
        &transaction->candidate.dual_address_snapshot, state);
    state->pc_word = transaction->candidate.pc_effect.public_pc_word_after;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}
