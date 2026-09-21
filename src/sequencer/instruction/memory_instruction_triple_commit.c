/*
 * Module contract
 *
 * Owns: the READY-path commit for one dual-data-plus-program
 * WORD16 read triple. It stages the ordinary complete_triple
 * effect, then publishes both validated post-access AR commits
 * and the sequential PC atomically.
 * Does not own: request preparation, endpoint issue/resume, triple
 * lifecycle, cancellation/reset, opcode semantics, address
 * arithmetic, endpoint storage, timing, CPU lifecycle, observers,
 * diagnostics, allocation, I/O, or global state. No architectural
 * field is written until ordinary-effect staging succeeds.
 */

#include "memory_instruction_triple_commit.h"

#include <stddef.h>
#include <stdint.h>

#include "memory_instruction_dual_commit.h"
#include "sequencer/effect_transaction.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_triple_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t x_word,
    uint16_t y_word, uint16_t program_word)
{
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;
    ti_c54x_op_effect next_effect = {0};
    ti_c54x_effect_transaction ordinary = {0};

    if (memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        memory->complete_triple == NULL ||
        memory->complete_triple(&transaction->candidate.instruction,
                                &transaction->candidate.input, x_word,
                                y_word, program_word,
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
    /* Ordinary-effect staging succeeded after all three reads:
     * publish the address updates and PC together with no
     * further fallible step between them. The X/Y post-access
     * commits ride the shared dual publisher; the PROGRAM leg
     * carries no post-update. */
    ti_c54x_memory_instruction_dual_publish_address_commits(
        &transaction->candidate.dual_address_snapshot, state);
    state->pc_word = transaction->candidate.pc_effect.public_pc_word_after;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}
