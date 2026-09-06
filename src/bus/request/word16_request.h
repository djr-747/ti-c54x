#ifndef TI_C54X_BUS_REQUEST_WORD16_REQUEST_H
#define TI_C54X_BUS_REQUEST_WORD16_REQUEST_H

/*
 * Module contract
 *
 * Owns: the private, value-owned request shape for one architectural 16-bit
 * word in one C54x program, data, or I/O space.
 * Does not own: bus callbacks, read results, byte order, transaction
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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_bus_word16_request_space {
    TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID = 0,
    TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM = 1,
    TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA = 2,
    TI_C54X_BUS_WORD16_REQUEST_SPACE_IO = 3
} ti_c54x_bus_word16_request_space;

typedef enum ti_c54x_bus_word16_request_access {
    TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID = 0,
    TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ = 1,
    TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE = 2
} ti_c54x_bus_word16_request_access;

typedef enum ti_c54x_bus_word16_request_status {
    TI_C54X_BUS_WORD16_REQUEST_STATUS_OK = 0,
    TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_SPACE = 1,
    TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_ADDRESS = 2
} ti_c54x_bus_word16_request_status;

typedef struct ti_c54x_bus_word16_request {
    ti_c54x_bus_word16_request_space space;
    ti_c54x_bus_word16_request_access access;
    uint32_t address_word;
    uint16_t write_value_word;
} ti_c54x_bus_word16_request;

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
