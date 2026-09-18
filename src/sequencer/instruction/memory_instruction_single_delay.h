#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SINGLE_DELAY_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SINGLE_DELAY_H

/*
 * Module contract
 *
 * Owns: the private, allocation-free single-word Smem delay write-back
 * stage (LTD/DELAY) used by the root memory transaction. It recognizes
 * an armed single-word delay READ, derives the DATA delay target
 * (resolved Smem address plus one with 16-bit wraparound), and issues
 * or resumes the delay WRITE through the reused WORD16 child with a
 * fresh transaction ID. The READ observation is stashed in the root
 * slot the carrier keeps disjoint from RMW staging. The resume and
 * issue orchestrators additionally map each outcome to the root
 * phase and result and retire through the shared READY tail.
 * Does not own: carrier validity, address resolution, MMR routing,
 * completions, the READY tail itself, CPU publication, endpoint
 * callbacks, timing, observers, or diagnostics. Transaction storage
 * is borrowed for each call.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "memory_instruction_transaction.h"

typedef enum ti_c54x_memory_instruction_single_delay_status {
    TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_READY = 0,
    TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_WAIT = 1,
    TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_FAULT = 2,
    TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_MALFORMED = 3,
    TI_C54X_MEMORY_INSTRUCTION_SINGLE_DELAY_ID_EXHAUSTED = 4
} ti_c54x_memory_instruction_single_delay_status;

/* Armed when the WORD16 child carries a single-word delay READ: a
 * MEMORY effect with the delay flag on a DIRECT or SINGLE_INDIRECT
 * DATA READ. The carrier already pins access and RMW shape. */
bool ti_c54x_memory_instruction_single_delay_is_armed(
    const ti_c54x_memory_instruction_transaction *transaction);

/* The delay WRITE is in flight exactly when an armed root is PENDING
 * while its reused child carries an issued delay WRITE. */
bool ti_c54x_memory_instruction_single_delay_write_in_flight(
    const ti_c54x_memory_instruction_transaction *transaction);

/* DATA delay target from the frozen candidate resolution: the
 * resolved Smem address plus one with 16-bit wraparound. */
uint16_t ti_c54x_memory_instruction_single_delay_target_address(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Issue (begin) or resume the delay WRITE of the Smem word to the
 * DATA delay target. When beginning, the child must carry the
 * completed READ and read_value_word is stashed before issue; when
 * resuming, the in-flight child WRITE is pinned against the frozen
 * candidate target and the stashed word and read_value_word is
 * ignored. Never commits architecture and never touches the root
 * phase; the caller maps the outcome to phase and result. */
ti_c54x_memory_instruction_single_delay_status
ti_c54x_memory_instruction_single_delay_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, uint16_t read_value_word,
    bool begin, ti_c54x_bus_status *bus_status);

/* Full delay-write resume orchestration for the root: resume the
 * in-flight delay WRITE, and on READY run the shared READY tail
 * with the stashed READ observation. Maps WAIT and FAULT to the
 * root phase and result exactly like the endpoint tail. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_single_delay_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

/* Full delay-issue orchestration for the root: issue the delay WRITE
 * for a just-completed READ leg, and on READY run the shared READY
 * tail with the stashed READ observation. A delay FAULT publishes
 * nothing because the tail never runs. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_single_delay_issue(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, uint16_t read_value_word,
    bool first_issue);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SINGLE_DELAY_H */
