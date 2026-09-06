#ifndef TI_C54X_ISA_OPCODE_DECODE_H
#define TI_C54X_ISA_OPCODE_DECODE_H

/*
 * Module contract
 *
 * Owns: pure by-value classification of one fetched 16-bit first program
 * word, including its raw value and derived program-word width.
 * Does not own: memory/bus fetches, CPU or status state, program-counter
 * sequencing, semantic effects, timing, observers, or diagnostics. Decoder
 * inputs are copied scalars and results retain no caller-owned storage.
 */

#include <stdint.h>

#include "../catalog/opcode_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_decode_status {
    TI_C54X_DECODE_STATUS_OK = 0,
    TI_C54X_DECODE_STATUS_UNKNOWN_WORD = 1
} ti_c54x_decode_status;

typedef struct ti_c54x_decode_result {
    ti_c54x_decode_status status;
    ti_c54x_opcode_id id;
    uint16_t first_word;
    uint8_t program_word_count;
} ti_c54x_decode_result;

/* Classify exactly one copied first program word; no state or I/O is touched. */
ti_c54x_decode_result ti_c54x_decode_first_word(uint16_t first_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPCODE_DECODE_H */
