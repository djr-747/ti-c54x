#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_FAR_PUSH_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_FAR_PUSH_H

/*
 * Module contract
 *
 * Owns: the private, allocation-free far-call second-push stage
 * (FCALL[D]/FCALA[D]) used by the root memory transaction. It
 * recognizes an armed far double-push WORD16 WRITE, derives the
 * second DATA TOS target (resolved first TOS minus one with
 * 16-bit wraparound), and issues or resumes the XPC WRITE through
 * the reused WORD16 child with a fresh transaction ID. The frozen
 * live XPC rides the root far-push slot the carrier keeps disjoint
 * from RMW staging. The resume and issue orchestrators
 * additionally map each outcome to the root phase and result and
 * retire through the shared READY tail.
 * Does not own: carrier validity, address resolution, MMR routing,
 * completions, the READY tail itself, CPU publication, endpoint
 * callbacks, timing, observers, or diagnostics. Transaction storage
 * is borrowed for each call.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_instruction_transaction.h"

typedef enum ti_c54x_memory_instruction_far_push_status {
    TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_READY = 0,
    TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_WAIT = 1,
    TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_FAULT = 2,
    TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_MALFORMED = 3,
    TI_C54X_MEMORY_INSTRUCTION_FAR_PUSH_ID_EXHAUSTED = 4
} ti_c54x_memory_instruction_far_push_status;

/* Freeze the live XPC into the root far-push slot for a far
 * double-push kind; a non-far kind is a no-op success. Unknown
 * XPC rejects before any bus issue. Never touches the root
 * phase; the caller maps the outcome. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_push_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_op_memory_address_kind kind,
    const ti_c54x_arch_state *state);

/* Armed when the WORD16 child carries a far double-push first
 * WRITE: a MEMORY effect with a STACK_PUSH_FAR address kind,
 * WRITE access, and no RMW. The carrier already pins the effect
 * shape. */
bool ti_c54x_memory_instruction_far_push_is_armed(
    const ti_c54x_memory_instruction_transaction *transaction);

/* The XPC WRITE is in flight exactly when an armed root is PENDING
 * while its reused child carries an issued XPC WRITE to the second
 * TOS with the frozen XPC value. */
bool ti_c54x_memory_instruction_far_push_write_in_flight(
    const ti_c54x_memory_instruction_transaction *transaction);

/* The first TOS WRITE just completed exactly when an armed root
 * carries a completed child WRITE of the frozen staged return
 * address to the frozen first TOS. The first and second LEGS are
 * disjoint (their TOS addresses always differ), so this pins the
 * first completion against the second. */
bool ti_c54x_memory_instruction_far_push_first_write_complete(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Second DATA TOS target from the frozen candidate resolution: the
 * resolved first TOS minus one with 16-bit wraparound. */
uint16_t ti_c54x_memory_instruction_far_push_target_address(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Issue (begin) or resume the XPC WRITE to the second TOS. When
 * beginning, the child must carry the completed first WRITE; when
 * resuming, the in-flight child WRITE is pinned against the frozen
 * candidate target and the frozen XPC. Never commits architecture
 * and never touches the root phase; the caller maps the outcome to
 * phase and result. */
ti_c54x_memory_instruction_far_push_status
ti_c54x_memory_instruction_far_push_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, bool begin,
    ti_c54x_bus_status *bus_status);

/* Full second-push resume orchestration for the root: resume the
 * in-flight XPC WRITE, and on READY run the shared READY tail.
 * Maps WAIT and FAULT to the root phase and result exactly like
 * the endpoint tail. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_push_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

/* Full second-push issue orchestration for the root: issue the XPC
 * WRITE for a just-completed first WRITE leg, and on READY run the
 * shared READY tail. An XPC FAULT publishes nothing because the
 * tail never runs. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_push_issue(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, bool first_issue);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_FAR_PUSH_H */
