#ifndef TI_C54X_BUS_ADAPTER_WORD16_ADAPTER_H
#define TI_C54X_BUS_ADAPTER_WORD16_ADAPTER_H

/*
 * Module contract
 *
 * Owns: the private bridge from one copied public DATA/IO WORD16 endpoint to
 * the callback-free transaction reducer, including endpoint-shape validation
 * and status mapping. DATA and PORT IO legs share the bridge; PROGRAM space
 * stays rejected at advance.
 * Does not own: physical storage, address mapping, arbitration, byte order,
 * CPU lifecycle, instruction effects, timing, allocation, diagnostics, or
 * environment state.
 */

#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "../transaction/word16_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_bus_word16_adapter_status {
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK = 0,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT = 1,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT = 2,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK = 3,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT = 4,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT = 5,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION = 6,
    TI_C54X_BUS_WORD16_ADAPTER_STATUS_CANCEL_FAILED = 7
} ti_c54x_bus_word16_adapter_status;

typedef struct ti_c54x_bus_word16_adapter_result {
    ti_c54x_bus_word16_adapter_status status;
    uint16_t read_value_word;
    ti_c54x_bus_status bus_status;
} ti_c54x_bus_word16_adapter_result;

/* Classify endpoint shape before a caller allocates a transaction ID. */
ti_c54x_bus_word16_adapter_status
ti_c54x_bus_word16_adapter_validate_endpoint(
    const ti_c54x_bus_word16_endpoint *endpoint);

/* Advance BEGIN or RESUME exactly once for the supplied reducer token. */
ti_c54x_bus_word16_adapter_result ti_c54x_bus_word16_adapter_advance(
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_bus_word16_advance_phase phase,
    ti_c54x_bus_word16_transaction *transaction);

/* Cancel an active token; failed endpoint cancellation preserves it. */
ti_c54x_bus_word16_adapter_result ti_c54x_bus_word16_adapter_cancel(
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_bus_word16_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_ADAPTER_WORD16_ADAPTER_H */
