#ifndef TI_C54X_SEQUENCER_MEMORY_DUAL_ADDRESS_RESOLUTION_H
#define TI_C54X_SEQUENCER_MEMORY_DUAL_ADDRESS_RESOLUTION_H

/*
 * Module contract
 *
 * Owns: pure resolution of the W50A dual-indirect MOD0/MOD0 WORD16 data
 * address carrier against one borrowed architectural-state snapshot. It
 * publishes two value-owned address effects, commits, and canonical DATA
 * READ requests only after both operands have passed validation.
 * Does not own: opcode identity or semantics, CMPT/ARP policy, MMR/device
 * behavior, bus callbacks, memory visibility, instruction sequencing,
 * timing, architectural mutation, or diagnostics. State and spec inputs are
 * borrowed for one call; output owns no pointers and is zeroed on rejection.
 */

#include "sequencer/memory/address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_dual_memory_address_resolution {
    ti_c54x_address_result x_address_result;
    ti_c54x_address_result y_address_result;
    ti_c54x_sequencer_address_commit x_address_commit;
    ti_c54x_sequencer_address_commit y_address_commit;
    ti_c54x_bus_word16_request x_request;
    ti_c54x_bus_word16_request y_request;
} ti_c54x_dual_memory_address_resolution;

/*
 * Compact persistent copy retained by a prepared root transaction.  The
 * full resolver output above is deliberately prepare-local: MOD0 has no
 * address commit, so a pending resume needs only the two canonical requests
 * and their frozen addresses.  The dual candidate validator rechecks the
 * request shape and source AR values from this snapshot.
 */
typedef struct ti_c54x_dual_memory_address_snapshot {
    ti_c54x_bus_word16_request x_request;
    ti_c54x_bus_word16_request y_request;
} ti_c54x_dual_memory_address_snapshot;

/*
 * Resolve one frozen MOD0/MOD0 dual-indirect WORD16 READ pair. The access
 * and write-value parameters retain the generic resolver's boundary shape;
 * only READ with a canonical zero write value is admitted for this slice.
 * Both selectors must name actual dual-operation AR2..AR5 registers. The
 * resolver never mutates state and stages no partial result.
 */
ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_dual_memory_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_dual_memory_address_resolution *out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_DUAL_ADDRESS_RESOLUTION_H */
