#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_FAR_POP_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_FAR_POP_H

/*
 * Module contract
 *
 * Owns: the private, allocation-free far-return second-pop stage
 * (FRET[D]/FRETE[D]) used by the root memory transaction. It
 * recognizes an armed far double-pop WORD16 READ, derives the
 * second DATA TOS target (resolved first TOS plus one with
 * 16-bit wraparound), and issues or resumes the return-address
 * READ through the reused WORD16 child with a fresh transaction
 * ID. The first-leg TOS observation (the return extension page)
 * rides the root far-pop slot the carrier keeps disjoint from
 * RMW staging. The resume and issue orchestrators additionally
 * map each outcome to the root phase and result and retire
 * through the shared READY tail with the second-leg
 * observation. The far architecture commit publishes the
 * stepped SP with the popped PC+XPC redirect. Transaction
 * storage is borrowed for each call.
 * Does not own: carrier validity, address resolution, MMR
 * routing, completions, the READY tail itself, CPU publication,
 * endpoint callbacks, timing, observers, or diagnostics.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_instruction_transaction.h"

typedef enum ti_c54x_memory_instruction_far_pop_status {
    TI_C54X_MEMORY_INSTRUCTION_FAR_POP_READY = 0,
    TI_C54X_MEMORY_INSTRUCTION_FAR_POP_WAIT = 1,
    TI_C54X_MEMORY_INSTRUCTION_FAR_POP_FAULT = 2,
    TI_C54X_MEMORY_INSTRUCTION_FAR_POP_MALFORMED = 3,
    TI_C54X_MEMORY_INSTRUCTION_FAR_POP_ID_EXHAUSTED = 4
} ti_c54x_memory_instruction_far_pop_status;

/* Validate the inte completion contract for a far double-pop
 * kind; a non-far kind is a no-op success. The inte kinds
 * (FRETE[D]) require the staged INTM-clearing completion: a
 * forged candidate without one would silently skip the
 * interrupt enable, so it rejects before any bus issue. Never
 * touches the root phase; the caller maps the outcome. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_op_memory_address_kind kind,
    ti_c54x_op_memory_complete_fn complete);

/* Armed when the WORD16 child carries a far double-pop first
 * READ: a MEMORY effect with a STACK_POP_FAR address kind,
 * READ access, and no RMW. The carrier already pins the effect
 * shape. */
bool ti_c54x_memory_instruction_far_pop_is_armed(
    const ti_c54x_memory_instruction_transaction *transaction);

/* The return-address READ is in flight exactly when an armed
 * root is PENDING while its reused child carries an issued
 * second-leg READ to the second TOS. */
bool ti_c54x_memory_instruction_far_pop_read_in_flight(
    const ti_c54x_memory_instruction_transaction *transaction);

/* The first TOS READ just completed exactly when an armed root
 * carries a completed child READ of the frozen first TOS. The
 * first and second legs are disjoint (their TOS addresses
 * always differ), so this pins the first completion against
 * the second. */
bool ti_c54x_memory_instruction_far_pop_first_read_complete(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Second DATA TOS target from the frozen candidate resolution:
 * the resolved first TOS plus one with 16-bit wraparound. */
uint16_t ti_c54x_memory_instruction_far_pop_target_address(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Issue (begin) or resume the return-address READ at the second
 * TOS. When beginning, the child must carry the completed
 * first READ whose observation arrives in `first_read_word`
 * and is stashed to the root far-pop slot before the child is
 * reset; when resuming, the in-flight child READ is pinned
 * against the frozen candidate target. A READY second leg
 * publishes its observation through `second_read_word_out`.
 * Never commits architecture and never touches the root
 * phase; the caller maps the outcome to phase and result. */
ti_c54x_memory_instruction_far_pop_status
ti_c54x_memory_instruction_far_pop_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, uint16_t first_read_word,
    bool begin, uint16_t *second_read_word_out,
    ti_c54x_bus_status *bus_status);

/* Full second-pop resume orchestration for the root: resume the
 * in-flight return-address READ, and on READY run the shared
 * READY tail with the second-leg observation. Maps WAIT and
 * FAULT to the root phase and result exactly like the endpoint
 * tail. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_resume(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

/* Full second-pop issue orchestration for the root: stash the
 * just-completed first-leg observation, issue the
 * return-address READ, and on READY run the shared READY tail
 * with the second-leg observation. A second-leg FAULT
 * publishes nothing because the tail never runs. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_issue(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result, uint16_t first_read_word,
    bool first_issue);

/* Far double-pop architecture commit: publish the stepped SP
 * (old SP + 2) from the frozen resolution with the far PC+XPC
 * redirect. The return address arrives in `pcl_value` (the
 * second-leg observation); the return extension page rides the
 * root far-pop slot. Immediate forms redirect PC to the popped
 * return address; delayed forms commit the sequential successor
 * while the caller arms the countdown from the same
 * observation. Plans the PC target before mutating anything so
 * a failure stays transactional. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_far_pop_commit_architecture(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, uint16_t pcl_value);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_FAR_POP_H */
