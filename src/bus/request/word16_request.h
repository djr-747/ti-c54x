#ifndef TI_C54X_BUS_REQUEST_WORD16_REQUEST_H
#define TI_C54X_BUS_REQUEST_WORD16_REQUEST_H

/*
 * Module contract
 *
 * Owns: private value-owned constructors and validation for the public request
 * shape representing one architectural 16-bit word in one C54x program, data,
 * or I/O space.
 * Does not own: the public request type (owned by bus_endpoint.h), bus
 * callbacks, read results, byte order, transaction
 * lifecycle, sequencer phases, memory, CPU state, opcode effects, timing,
 * arbitration, diagnostics, or public ABI.
 *
 * Constructors consume copied scalar values and retain no pointer. A DATA or
 * IO address is represented by the common uint32_t constructor input but is
 * admitted only through UINT16_MAX before any narrowing. A READ request
 * carries a canonical zero write value. Invalid constructor results carry an
 * all-zero request in their named fields.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_bus_word16_request_status {
    TI_C54X_BUS_WORD16_REQUEST_STATUS_OK = 0,
    TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_SPACE = 1,
    TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_ADDRESS = 2
} ti_c54x_bus_word16_request_status;

typedef struct ti_c54x_bus_word16_request_result {
    ti_c54x_bus_word16_request_status status;
    ti_c54x_bus_word16_request request;
} ti_c54x_bus_word16_request_result;

/* Construct one copied READ request. The returned value borrows no storage. */
ti_c54x_bus_word16_request_result ti_c54x_bus_word16_request_read(
    ti_c54x_bus_word16_request_space space,
    uint32_t address_word);

/* Construct one copied WRITE request. The returned value borrows no storage. */
ti_c54x_bus_word16_request_result ti_c54x_bus_word16_request_write(
    ti_c54x_bus_word16_request_space space,
    uint32_t address_word,
    uint16_t write_value_word);

/* The request is borrowed read-only until return and is never retained. */
bool ti_c54x_bus_word16_request_is_valid(
    const ti_c54x_bus_word16_request *request);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_REQUEST_WORD16_REQUEST_H */
