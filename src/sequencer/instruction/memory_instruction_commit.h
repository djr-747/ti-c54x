#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_COMMIT_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_COMMIT_H

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
 * paths never reach this owner. The shared READY tail (completion
 * plus deferred-branch report plus retirement bookkeeping) also
 * lives here for the root and the delay stage to reuse.
 * Does not own: request preparation, endpoint issue/resume, pair
 * lifecycle, cancellation/reset, opcode semantics, address arithmetic,
 * endpoint storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. All inputs are borrowed for one
 * call; the transaction's copied snapshot must already be validated
 * against the frozen candidate and live state before this call.
 */

#include <stdint.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_architecture(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_pop_architecture(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, uint16_t tos_value);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t read_value_word);

/* Shared READY tail: run the read-value completion plus the
 * architecture commit, report a deferred branch target for the
 * delayed stack kinds, and retire with the instruction accounting.
 * Delay write-back shapes reach it only after their delay WRITE is
 * READY, passing the stashed READ observation as the commit word.
 * first_issue selects the attempted accounting for a tail that
 * shares its advance call with the bus issue. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_commit_ready_tail(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, uint16_t read_value_word,
    bool first_issue);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_COMMIT_H */
