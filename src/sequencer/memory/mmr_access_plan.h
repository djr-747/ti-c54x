#ifndef TI_C54X_SEQUENCER_MEMORY_MMR_ACCESS_PLAN_H
#define TI_C54X_SEQUENCER_MEMORY_MMR_ACCESS_PLAN_H

/*
 * Module contract
 *
 * Owns: the pure MMR access plan for one copied DATA request:
 * endpoint-bound, MMR-routed, or transactionally rejected.
 * READ probes route through a side-effect-free dry run; WRITE
 * routes without probing except for the guard-word range check.
 * Does not own: register storage, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, or timing. All
 * inputs are borrowed for one call; no pointer is retained.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_sequencer_mmr_access {
    TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT = 0,
    TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED = 1,
    TI_C54X_SEQUENCER_MMR_ACCESS_UNKNOWN = 2,
    TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT = 3
} ti_c54x_sequencer_mmr_access;

/* Plan one copied request. ENDPOINT leaves the endpoint path
 * unchanged; ROUTED serves locally with no endpoint traffic;
 * UNKNOWN rejects an MMR read of an unknown value; NULL inputs
 * and invalid spaces or accesses reject. */
ti_c54x_sequencer_mmr_access ti_c54x_sequencer_plan_mmr_access(
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_bus_word16_request_space space, uint32_t address_word,
    ti_c54x_bus_word16_request_access access, uint16_t write_value_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_MMR_ACCESS_PLAN_H */
