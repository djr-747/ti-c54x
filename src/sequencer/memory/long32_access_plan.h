#ifndef TI_C54X_SEQUENCER_MEMORY_LONG32_ACCESS_PLAN_H
#define TI_C54X_SEQUENCER_MEMORY_LONG32_ACCESS_PLAN_H

/*
 * Module contract
 *
 * Owns: the pure single-word Lmem LONG32 read leg plan for one
 * copied LONG32_SINGLE spec: a validated canonical LONG32 data
 * operand with its source snapshot and AR update, or a
 * transactional reject. Direct legs concatenate the frozen DP
 * page and dma7; indirect legs compose the landed two-word-delta
 * single-indirect LONG32 resolver (MOD3 reads decline there).
 * Planning observes no state beyond the copied AR value.
 * Does not own: request issue, endpoint traffic, the
 * pair-transaction commit root, opcode semantics, CPU lifecycle,
 * or timing. All inputs are borrowed for one call; no pointer is
 * retained.
 */

#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "address/address_effect.h"
#include "isa/ops/memory/memory_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_long32_single_access_status {
    TI_C54X_LONG32_SINGLE_ACCESS_OK = 0,
    TI_C54X_LONG32_SINGLE_ACCESS_INVALID_SPEC = 1,
    TI_C54X_LONG32_SINGLE_ACCESS_INVALID_ARGUMENT = 2,
    TI_C54X_LONG32_SINGLE_ACCESS_ADDRESS_PLAN_REJECTED = 3
} ti_c54x_long32_single_access_status;

typedef struct ti_c54x_long32_single_access {
    ti_c54x_long32_single_access_status status;
    ti_c54x_address_effect effect;
} ti_c54x_long32_single_access;

/* Plan one copied LONG32 leg. NULL specs and non-READ/WRITE
 * accesses reject; malformed arms reject with a zeroed effect;
 * resolver declines (including MOD3 reads) surface as
 * ADDRESS_PLAN_REJECTED with a zeroed effect. */
ti_c54x_long32_single_access ti_c54x_sequencer_plan_long32_single_access(
    const ti_c54x_op_memory_address_spec *spec,
    uint16_t ar_value,
    ti_c54x_bus_word16_request_access access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_LONG32_ACCESS_PLAN_H */
