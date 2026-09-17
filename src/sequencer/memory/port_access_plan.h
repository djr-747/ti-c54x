#ifndef TI_C54X_SEQUENCER_MEMORY_PORT_ACCESS_PLAN_H
#define TI_C54X_SEQUENCER_MEMORY_PORT_ACCESS_PLAN_H

/*
 * Module contract
 *
 * Owns: the pure PORT IO-leg plan for one copied IO request: a
 * validated IO-space request or a transactional reject. The DATA-leg
 * access direction selects the inverse IO leg (DATA WRITE plans an
 * IO READ for PORTR; DATA READ plans an IO WRITE template for
 * PORTW); the template value is zero and the PORT reducer replaces
 * it with the observed DATA word before issue. Planning observes no
 * state: the port address is absolute.
 * Does not own: DATA-leg resolution, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, or timing. All inputs
 * are borrowed for one call; no pointer is retained.
 */

#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_port_access_status {
    TI_C54X_PORT_ACCESS_IO_REQUEST = 0,
    TI_C54X_PORT_ACCESS_INVALID_SPEC = 1,
    TI_C54X_PORT_ACCESS_INVALID_ARGUMENT = 2
} ti_c54x_port_access_status;

typedef struct ti_c54x_port_access {
    ti_c54x_port_access_status status;
    ti_c54x_bus_word16_request request;
} ti_c54x_port_access;

/* Compact persistent copy retained by a prepared root transaction:
 * the resolved DATA leg plus the planned IO-leg request and the
 * second-leg flag. The DATA leg lives here rather than in the
 * candidate's generic address resolution because that slot shares
 * union storage with this snapshot; the DATA spec still re-derives
 * deterministically from the frozen arm and input on every shape
 * check, and this snapshot binds both legs. */
typedef struct ti_c54x_port_address_snapshot {
    ti_c54x_memory_address_resolution data_resolution;
    ti_c54x_bus_word16_request io_request;
    uint8_t second_leg_issued;
} ti_c54x_port_address_snapshot;

/* Plan one copied IO-leg request. NULL specs reject; malformed arms
 * and non-READ/WRITE accesses reject with a zeroed request. */
ti_c54x_port_access ti_c54x_sequencer_plan_port_access(
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access data_access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_PORT_ACCESS_PLAN_H */
