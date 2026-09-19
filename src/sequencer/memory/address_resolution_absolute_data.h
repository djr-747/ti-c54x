#ifndef TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_ABSOLUTE_DATA_H
#define TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_ABSOLUTE_DATA_H

/*
 * Module contract
 *
 * Owns: resolution of one copied absolute-DATA WORD16 spec into a
 * validated ABSOLUTE address result with its DATA READ request:
 * the full 16-bit address plans with no live state dependence
 * (no CPL/DP read) and commits with no post-update. Only READ
 * access resolves; WRITE declines as invalid access.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle, or
 * timing. All inputs are borrowed for one call; no pointer is
 * retained.
 */

#include "address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve one absolute-DATA spec. NULL specs, non-absolute kinds,
 * and non-READ accesses decline; planner or request failures
 * surface as ADDRESS_PLAN_REJECTED or REQUEST_REJECTED. */
ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_absolute_data_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    ti_c54x_memory_address_resolution *out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_ABSOLUTE_DATA_H */
