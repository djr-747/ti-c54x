#ifndef TI_C54X_BUS_ENDPOINT_H
#define TI_C54X_BUS_ENDPOINT_H

/*
 * Module contract
 *
 * Owns: the vendorable WORD16 bus request, status, endpoint phase,
 * observation, and cancellation vocabulary shared by the C54x core and a
 * host-owned adapter.
 * Does not own: physical memory, address mapping, arbitration, byte order,
 * allocation, timing, CPU state, instruction sequencing, devices, or I/O.
 *
 * Endpoint context and reachable storage are borrowed by value for the CPU
 * lifetime. Requests and observations are borrowed only for the duration of
 * one callback. A zero descriptor is the canonical absent endpoint; a
 * descriptor with exactly one callback is malformed.
 */

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

typedef struct ti_c54x_bus_word16_request {
    ti_c54x_bus_word16_request_space space;
    ti_c54x_bus_word16_request_access access;
    uint32_t address_word;
    uint16_t write_value_word;
} ti_c54x_bus_word16_request;

typedef enum ti_c54x_bus_status {
    TI_C54X_BUS_OK = 0,
    TI_C54X_BUS_UNMAPPED = 1,
    TI_C54X_BUS_FAULT = 2,
    TI_C54X_BUS_NO_CALLBACK = 3
} ti_c54x_bus_status;

typedef enum ti_c54x_bus_word16_advance_phase {
    TI_C54X_BUS_WORD16_ADVANCE_INVALID = 0,
    TI_C54X_BUS_WORD16_ADVANCE_BEGIN = 1,
    TI_C54X_BUS_WORD16_ADVANCE_RESUME = 2
} ti_c54x_bus_word16_advance_phase;

typedef enum ti_c54x_bus_word16_observation_kind {
    TI_C54X_BUS_WORD16_OBSERVATION_INVALID = 0,
    TI_C54X_BUS_WORD16_OBSERVATION_WAIT = 1,
    TI_C54X_BUS_WORD16_OBSERVATION_READY = 2,
    TI_C54X_BUS_WORD16_OBSERVATION_FAULT = 3
} ti_c54x_bus_word16_observation_kind;

typedef struct ti_c54x_bus_word16_observation {
    uint64_t transaction_id;
    ti_c54x_bus_word16_observation_kind kind;
    uint16_t read_value_word;
    ti_c54x_bus_status bus_status;
} ti_c54x_bus_word16_observation;

typedef enum ti_c54x_bus_word16_cancel_result {
    TI_C54X_BUS_WORD16_CANCEL_RESULT_INVALID = 0,
    TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED = 1,
    TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED = 2
} ti_c54x_bus_word16_cancel_result;

typedef void (*ti_c54x_bus_word16_endpoint_advance_fn)(
    void *context,
    ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation);

typedef ti_c54x_bus_word16_cancel_result (*
    ti_c54x_bus_word16_endpoint_cancel_fn)(
    void *context,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request);

typedef struct ti_c54x_bus_word16_endpoint {
    void *context;
    ti_c54x_bus_word16_endpoint_advance_fn advance;
    ti_c54x_bus_word16_endpoint_cancel_fn cancel;
} ti_c54x_bus_word16_endpoint;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_ENDPOINT_H */
