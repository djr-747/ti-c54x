#ifndef TI_C54X_SEQUENCER_MEMORY_DUAL_ADDRESS_RESOLUTION_H
#define TI_C54X_SEQUENCER_MEMORY_DUAL_ADDRESS_RESOLUTION_H

/*
 * Module contract
 *
 * Owns: pure resolution of the W51A dual-indirect MOD0/MOD1/MOD2 WORD16 data
 * address carrier against one borrowed architectural-state snapshot. It
 * publishes two value-owned address effects, commits, and canonical DATA
 * READ requests only after both operands have passed validation. The
 * persistent snapshot carries both prevalidated post-access commits so the
 * W51B root transaction can validate and publish them atomically.
 * Does not own: opcode identity or semantics, CMPT/ARP policy, MMR/device
 * behavior, bus callbacks, memory visibility, instruction sequencing,
 * timing, architectural mutation, or diagnostics. State and spec inputs are
 * borrowed for one call; output owns no pointers and is zeroed on rejection.
 */

#include "sequencer/memory/address_resolution.h"
#include "sequencer/address_effect_validate.h"

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
    /* Copied AR0/BK snapshots consumed by a MOD3 side; zero when neither
     * side selects MOD3. The root transaction shadows these verbatim. */
    uint16_t ar0_value;
    uint16_t bk_value;
} ti_c54x_dual_memory_address_resolution;

/*
 * Compact persistent copy retained by a prepared root transaction. The full
 * resolver output above is deliberately prepare-local. W51B carries exactly
 * two value-owned commit descriptors alongside the copied requests: MOD0
 * sides hold a zeroed no-update commit, MOD1/MOD2 sides hold the prevalidated
 * post-access AR decrement/increment, and a same-selector alias holds the
 * X-owned update with a zeroed Y commit under Xmod precedence. The dual
 * candidate validator rechecks request shape, commit consistency against the
 * copied mode/selector/address shadows, and source AR values from this
 * snapshot; the root publishes the commits atomically only after both reads
 * and ordinary-effect staging succeed.
 */
typedef struct ti_c54x_dual_memory_address_snapshot {
    ti_c54x_bus_word16_request x_request;
    ti_c54x_bus_word16_request y_request;
    ti_c54x_sequencer_address_commit x_commit;
    ti_c54x_sequencer_address_commit y_commit;
    /* Copied AR0/BK shadows consumed only by MOD3 commit revalidation;
     * zero when the resolved pair selects no MOD3 side. */
    uint16_t ar0_shadow;
    uint16_t bk_shadow;
} ti_c54x_dual_memory_address_snapshot;

/*
 * Resolve one frozen MOD0/MOD1/MOD2/MOD3 dual-indirect WORD16 READ pair. The
 * access and write-value parameters retain the generic resolver's boundary
 * shape; only READ with a canonical zero write value is admitted for this
 * slice. Both selectors must name actual dual-operation AR2..AR5 registers.
 * A MOD3 side additionally reads known AR0 and BK snapshots; unknown
 * sources reject, while pairs without a MOD3 side never read BK. The
 * resolver never mutates state, observes no CMPT/ARP state, and stages no
 * partial result.
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
