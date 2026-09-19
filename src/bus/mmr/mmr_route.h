#ifndef TI_C54X_BUS_MMR_ROUTE_H
#define TI_C54X_BUS_MMR_ROUTE_H

/*
 * Module contract
 *
 * Owns: the Table 3-3 MMR address map plus the Table 8-2
 * SWWSR peripheral cell at 0x28, and the pure DATA-space
 * WORD16 request router: owner-backed registers dispatch to
 * checked whole-word access, IMR/TRN/BRC/RSA/REA dispatch to
 * the private MMR file, 0x28 dispatches to the devices-owned
 * SWWSR cell, and reserved 0x02–0x05, XPC 0x1E, 0x1F, and
 * every other space or address decline without touching
 * state. Unknown reads report UNKNOWN with canonical zero.
 * Does not own: register storage, bus transfers, endpoint
 * callbacks, arbitration, opcode semantics, CPU lifecycle,
 * timing, or diagnostics. All inputs are borrowed read-only
 * except the state bundle, which is borrowed for the call;
 * no pointer is retained.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "../../state/accumulator/accumulator.h"
#include "../../state/arch_state.h"
#include "../../state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPRU131G Table 3-3 CPU memory-mapped register DATA addresses. */
#define TI_C54X_BUS_MMR_ADDRESS_IMR UINT16_C(0x0000)
#define TI_C54X_BUS_MMR_ADDRESS_IFR UINT16_C(0x0001)
#define TI_C54X_BUS_MMR_ADDRESS_ST0 UINT16_C(0x0006)
#define TI_C54X_BUS_MMR_ADDRESS_ST1 UINT16_C(0x0007)
#define TI_C54X_BUS_MMR_ADDRESS_AL UINT16_C(0x0008)
#define TI_C54X_BUS_MMR_ADDRESS_AH UINT16_C(0x0009)
#define TI_C54X_BUS_MMR_ADDRESS_AG UINT16_C(0x000A)
#define TI_C54X_BUS_MMR_ADDRESS_BL UINT16_C(0x000B)
#define TI_C54X_BUS_MMR_ADDRESS_BH UINT16_C(0x000C)
#define TI_C54X_BUS_MMR_ADDRESS_BG UINT16_C(0x000D)
#define TI_C54X_BUS_MMR_ADDRESS_T UINT16_C(0x000E)
#define TI_C54X_BUS_MMR_ADDRESS_TRN UINT16_C(0x000F)
#define TI_C54X_BUS_MMR_ADDRESS_AR0 UINT16_C(0x0010)
#define TI_C54X_BUS_MMR_ADDRESS_AR7 UINT16_C(0x0017)
#define TI_C54X_BUS_MMR_ADDRESS_SP UINT16_C(0x0018)
#define TI_C54X_BUS_MMR_ADDRESS_BK UINT16_C(0x0019)
#define TI_C54X_BUS_MMR_ADDRESS_BRC UINT16_C(0x001A)
#define TI_C54X_BUS_MMR_ADDRESS_RSA UINT16_C(0x001B)
#define TI_C54X_BUS_MMR_ADDRESS_REA UINT16_C(0x001C)
#define TI_C54X_BUS_MMR_ADDRESS_PMST UINT16_C(0x001D)
#define TI_C54X_BUS_MMR_ADDRESS_MAX UINT16_C(0x001F)
/* SPRU131G Table 8-2 C542 peripheral MMR: SWWSR lives past the
 * CPU window and routes explicitly, never by window widening. */
#define TI_C54X_BUS_MMR_ADDRESS_SWWSR UINT16_C(0x0028)

typedef enum ti_c54x_bus_mmr_status {
    TI_C54X_BUS_MMR_STATUS_SERVICED = 0,
    TI_C54X_BUS_MMR_STATUS_DECLINED = 1,
    TI_C54X_BUS_MMR_STATUS_UNKNOWN = 2,
    TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT = 3
} ti_c54x_bus_mmr_status;

typedef struct ti_c54x_bus_mmr_route_result {
    ti_c54x_bus_mmr_status status;
    uint16_t read_value_word;
} ti_c54x_bus_mmr_route_result;

/* Pure routability probe: true for DATA-space WORD16 addresses the
 * file claims (0x00–0x1F except reserved 0x02–0x05, XPC 0x1E, and
 * 0x1F). All other spaces and addresses decline to the endpoint. */
bool ti_c54x_bus_mmr_is_routed_address(
    ti_c54x_bus_word16_request_space space, uint32_t address_word);

/* Route one copied DATA request against the MMR file. Serviced
 * reads carry the register value; serviced writes replace it.
 * Declined and unknown results carry canonical zero; declined
 * leaves every owner untouched. NULL inputs and invalid
 * requests reject. */
ti_c54x_bus_mmr_route_result ti_c54x_bus_mmr_route(
    const ti_c54x_bus_word16_request *request,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_MMR_ROUTE_H */
