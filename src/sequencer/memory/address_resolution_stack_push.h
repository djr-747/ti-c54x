#ifndef TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_STACK_PUSH_H
#define TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_STACK_PUSH_H

/*
 * Module contract
 *
 * Owns: resolution of one copied stack-push WORD16 spec into a
 * validated TOS address result with its DATA WRITE request: the
 * first TOS plans at old SP - 1 with a PRE_ACCESS SP update
 * while far-call double-push kinds publish old SP - 2 (the
 * second TOS carries the frozen live XPC, planned by the
 * far-push owner from this frozen resolution). Only WRITE
 * access resolves; only SP is observed.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle, or
 * timing. All inputs are borrowed for one call; no pointer is
 * retained.
 */

#include "address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve one stack-push spec (bundle, accumulator, take, trap,
 * intr, or far double-push). NULL specs, non-push kinds, and
 * non-WRITE access decline; unknown SP, planner, validation, or
 * request failures surface as UNKNOWN_SOURCE,
 * ADDRESS_PLAN_REJECTED, or REQUEST_REJECTED. WRITE requests
 * carry the staged return-address word. */
ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_stack_push_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_memory_address_resolution *out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_STACK_PUSH_H */
