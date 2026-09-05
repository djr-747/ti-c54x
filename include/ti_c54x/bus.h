#ifndef TI_C54X_BUS_H
#define TI_C54X_BUS_H

/* Module contract
 * Owns: the callback type and the value-type bus description.
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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_bus_status {
    TI_C54X_BUS_OK = 0,
    TI_C54X_BUS_UNMAPPED = 1,
    TI_C54X_BUS_FAULT = 2,
    TI_C54X_BUS_NO_CALLBACK = 3
} ti_c54x_bus_status;

/* Addresses are words, never byte offsets.  No timing or wait-state result is
 * part of this phase-one contract.  A NULL callback means that space is not
 * mapped by this bus and produces TI_C54X_BUS_NO_CALLBACK when accessed. */
typedef ti_c54x_bus_status (*ti_c54x_program_read_word_fn)(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t *value_word);

typedef ti_c54x_bus_status (*ti_c54x_program_write_word_fn)(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t value_word);

typedef ti_c54x_bus_status (*ti_c54x_data_read_word_fn)(
    void *context,
    ti_c54x_data_word_address address_word,
    uint16_t *value_word);

typedef ti_c54x_bus_status (*ti_c54x_data_write_word_fn)(
    void *context,
    ti_c54x_data_word_address address_word,
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
    ti_c54x_data_read_word_fn data_read_word;
    ti_c54x_data_write_word_fn data_write_word;
    ti_c54x_io_read_word_fn io_read_word;
    ti_c54x_io_write_word_fn io_write_word;
} ti_c54x_bus;

/* A CPU copies this structure during initialization.  The context and all
 * memory reachable through it are borrowed and must remain valid until the
 * CPU is reinitialized or deinitialized. */

#ifdef __cplusplus
}
#endif

#endif
