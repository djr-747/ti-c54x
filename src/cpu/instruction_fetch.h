#ifndef TI_C54X_CPU_INSTRUCTION_FETCH_H
#define TI_C54X_CPU_INSTRUCTION_FETCH_H

/*
 * Module contract
 *
 * Owns: the CPU-owned, allocation-free transaction that extends a copied
 * first program word by zero or one bus-read word. It publishes a value-owned
 * fixed-capacity bundle only after a requested extension read succeeds.
 * Does not own: the first-word read, CPU/state or program-counter mutation,
 * instruction decoding, timing, memory, observers, diagnostics, or opcode
 * semantics. The fixed two-word bundle is helper capacity, not an ISA limit.
 *
 * The bus descriptor and its callback context are borrowed for one call only;
 * no pointer is retained. The first program word and its exact uint32_t word
 * address are copied values. A failed call returns zero-valued named bundle
 * fields and never partially publishes a callback output.
 */

#include <stdint.h>

#include <ti_c54x/bus.h>

#include "isa/decode/instruction_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_instruction_extension_fetch_status {
    TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK = 0,
    TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_ADDRESS_OVERFLOW = 2,
    TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT = 3
} ti_c54x_instruction_extension_fetch_status;

typedef struct ti_c54x_instruction_extension_fetch_result {
    ti_c54x_instruction_extension_fetch_status status;
    ti_c54x_bus_status bus_status;
    ti_c54x_instruction_word_bundle bundle;
} ti_c54x_instruction_extension_fetch_result;

/* Collect one supplied first word and, for count two, its exact successor. */
ti_c54x_instruction_extension_fetch_result
ti_c54x_instruction_extension_fetch(const ti_c54x_bus *bus,
                                    uint32_t first_program_word_address,
                                    uint16_t first_program_word,
                                    uint8_t program_word_count);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_CPU_INSTRUCTION_FETCH_H */
