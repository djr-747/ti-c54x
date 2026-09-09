#ifndef TI_C54X_BUS_MMR_BK_ROUTE_H
#define TI_C54X_BUS_MMR_BK_ROUTE_H

/*
 * Module contract
 *
 * Owns: the private BK memory-mapped route record: the documented
 * DATA-space word address and the pure request router that services BK
 * reads when known, replaces BK on writes, and declines every other
 * address, space, or unknown-BK read without touching state. BK storage
 * and knownness stay owned by src/state; this is a mapping record only.
 * Does not own: BK storage, bus transfers, endpoint callbacks,
 * arbitration, full MMR/device routing, opcode semantics, CPU lifecycle,
 * timing, or diagnostics. The request is borrowed read-only; the state
 * pointer is borrowed for the call and no pointer is retained.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "../../state/arch_state.h"
#include "../../state/block_size_register.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Table 3-3 BK circular-buffer size register data-memory address. */
#define TI_C54X_BUS_MMR_BK_DATA_ADDRESS_WORD UINT16_C(0x0019)

typedef enum ti_c54x_bus_mmr_bk_status {
    TI_C54X_BUS_MMR_BK_STATUS_SERVICED = 0,
    TI_C54X_BUS_MMR_BK_STATUS_DECLINED = 1,
    TI_C54X_BUS_MMR_BK_STATUS_UNKNOWN = 2,
    TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT = 3
} ti_c54x_bus_mmr_bk_status;

typedef struct ti_c54x_bus_mmr_bk_result {
    ti_c54x_bus_mmr_bk_status status;
    uint16_t read_value_word;
} ti_c54x_bus_mmr_bk_result;

/* Route one copied DATA request against BK. Serviced reads carry the BK
 * value; serviced writes replace BK with the request write value.
 * Declined and unknown results carry canonical zero and leave BK
 * unchanged; NULL inputs and invalid requests reject. */
ti_c54x_bus_mmr_bk_result ti_c54x_bus_mmr_bk_route(
    const ti_c54x_bus_word16_request *request,
    ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_MMR_BK_ROUTE_H */
