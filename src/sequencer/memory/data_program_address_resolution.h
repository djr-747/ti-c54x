#ifndef TI_C54X_SEQUENCER_MEMORY_DATA_PROGRAM_ADDRESS_RESOLUTION_H
#define TI_C54X_SEQUENCER_MEMORY_DATA_PROGRAM_ADDRESS_RESOLUTION_H

/*
 * Module contract
 *
 * Owns: pure resolution of the DATA+PROGRAM WORD16 READ pair carrier
 * against one borrowed architectural-state snapshot. It stages the
 * Smem DATA leg through the generic single-leg resolver (direct
 * page-0 or standard-indirect MOD0/MOD1/MOD2) and pairs it with an
 * absolute PROGRAM-leg coefficient read; the persistent snapshot
 * carries the single Smem post-access commit with a zeroed
 * no-update PROGRAM commit. The snapshot reuses the dual pair
 * carrier shape so the pair root and reducer share one layout.
 * Does not own: opcode identity or semantics, CMPT/ARP policy,
 * MMR/device behavior, bus callbacks, memory visibility,
 * instruction sequencing, timing, architectural mutation, or
 * diagnostics. State and spec inputs are borrowed for one call;
 * output owns no pointers and is zeroed on rejection.
 */

#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "sequencer/memory/dual_address_resolution.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Resolve one frozen DATA+PROGRAM WORD16 READ pair. The Smem leg
 * admits direct page-0 or standard-indirect MOD0/MOD1/MOD2 through
 * the generic resolver (MOD3 reads stay rejected by the carrier
 * rule); the PROGRAM leg is an absolute 16-bit program-word read
 * with no state dependence and no post-update. Only READ with a
 * canonical zero write value is admitted. The resolver never
 * mutates state and stages no partial result.
 */
ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_data_program_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_dual_memory_address_snapshot *out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_DATA_PROGRAM_ADDRESS_RESOLUTION_H */
