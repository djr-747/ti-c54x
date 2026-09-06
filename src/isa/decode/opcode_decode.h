#ifndef TI_C54X_ISA_OPCODE_DECODE_H
#define TI_C54X_ISA_OPCODE_DECODE_H

/*
 * Module contract
 *
 * Owns: pure by-value classification of one fetched 16-bit first program
 * word, including its raw value and derived program-word width, plus checked
 * transactional binding of that result to a copied complete instruction and
 * its catalog-owned scalar layout/repeatability metadata.
 * Does not own: memory/bus fetches, CPU or status state, program-counter
 * sequencing, semantic effects, timing, observers, or diagnostics. Decoder
 * results retain no caller-owned storage. Completion inputs are borrowed and
 * immutable for one call; the caller owns the output and no pointer is retained.
 */

#include <stdint.h>

#include "../catalog/opcode_catalog.h"
#include "instruction_bundle.h"

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

typedef enum ti_c54x_instruction_completion_status {
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK = 0,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK = 2,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID = 3,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_OPCODE_ID = 4,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_UNAVAILABLE = 5,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_ID_MISMATCH = 6,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_METADATA_COUNT_INVALID = 7,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_INVALID = 8,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH = 9,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID = 10,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH = 11,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH = 12,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH = 13,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_OPERAND_LAYOUT_INVALID = 14,
    TI_C54X_INSTRUCTION_COMPLETION_STATUS_REPEATABILITY_INVALID = 15
} ti_c54x_instruction_completion_status;

typedef struct ti_c54x_decoded_instruction {
    ti_c54x_opcode_id id;
    ti_c54x_operand_layout_id operand_layout;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_instruction_word_bundle bundle;
} ti_c54x_decoded_instruction;

/* Classify exactly one copied first program word; no state or I/O is touched. */
ti_c54x_decode_result ti_c54x_decode_first_word(uint16_t first_word);

/*
 * Validate and copy one complete instruction after first-word classification.
 * The first result and bundle are borrowed for this call only; output is
 * caller-owned storage and is unchanged on every non-OK status.
 */
ti_c54x_instruction_completion_status ti_c54x_decode_complete_instruction(
    const ti_c54x_decode_result *first_word_result,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_decoded_instruction *output);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPCODE_DECODE_H */
