#ifndef TI_C54X_BUS_H
#define TI_C54X_BUS_H

/* Module contract
 * Owns: the synchronous program/I/O callback types and the value-type bus
 * topology that carries the resumable DATA endpoint.
 * Does not own: scalar address carriers (owned by address.h), callback
 * context, mapped memory, or any platform bus object.
 */

/*
 * The CPU sees three independent word-addressed spaces.  Program addresses
 * have a 32-bit carrier so extended/XPC addresses are representable; data and
 * I/O addresses are 16-bit word addresses.  This header deliberately contains
 * no storage or platform policy: a consumer supplies a bus object for each CPU
 * instance.
 */

#include <stdint.h>

#include <ti_c54x/address.h>
#include <ti_c54x/bus_endpoint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Addresses are words, never byte offsets. Program and I/O remain synchronous
 * in this contract; their NULL callbacks produce TI_C54X_BUS_NO_CALLBACK.
 * Resumable DATA access is owned by bus_endpoint.h. */
typedef ti_c54x_bus_status (*ti_c54x_program_read_word_fn)(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t *value_word);

typedef ti_c54x_bus_status (*ti_c54x_program_write_word_fn)(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t value_word);

typedef ti_c54x_bus_status (*ti_c54x_io_read_word_fn)(
    void *context,
    ti_c54x_io_word_address address_word,
    uint16_t *value_word);

typedef ti_c54x_bus_status (*ti_c54x_io_write_word_fn)(
    void *context,
    ti_c54x_io_word_address address_word,
    uint16_t value_word);

typedef struct ti_c54x_bus {
    void *context;
    ti_c54x_program_read_word_fn program_read_word;
    ti_c54x_program_write_word_fn program_write_word;
    ti_c54x_bus_word16_endpoint data_endpoint;
    ti_c54x_io_read_word_fn io_read_word;
    ti_c54x_io_write_word_fn io_write_word;
} ti_c54x_bus;

/* A CPU copies this structure during initialization. The contexts and all
 * memory reachable through them remain caller-owned and must stay valid until
 * the CPU is successfully deinitialized. */

#ifdef __cplusplus
}
#endif

#endif
