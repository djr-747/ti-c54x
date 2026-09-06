#ifndef TI_C54X_ISA_OPERAND_FIELD_H
#define TI_C54X_ISA_OPERAND_FIELD_H

/*
 * Module contract
 *
 * Owns: pure, allocation-free validation and assembly of raw operand fields
 * from borrowed 16-bit program words. It publishes a copied value and width
 * only after the complete layout is valid. Does not own: opcode meanings,
 * fetch or bus I/O, CPU/state/PC sequencing, timing, memory, diagnostics,
 * or semantic interpretation of a field.
 *
 * Word views and layouts are borrowed for the duration of one call; callers
 * retain ownership. The program_words array must contain at least
 * program_word_count uint16_t elements, and the pieces array must contain at
 * least piece_count elements; both remain alive and unchanged through that
 * call. The output is caller-owned storage, and no input pointer is retained
 * after the function returns. If descriptors violate multiple classes,
 * callers must rely only on a non-OK status and unchanged output, not on
 * precedence among the diagnostic statuses. A failed call leaves the output
 * bytes unchanged.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_operand_field_status {
    TI_C54X_OPERAND_FIELD_STATUS_OK = 0,
    TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_OPERAND_FIELD_STATUS_INVALID_WORD_VIEW = 2,
    TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT = 3,
    TI_C54X_OPERAND_FIELD_STATUS_SOURCE_WORD_INDEX_OUT_OF_RANGE = 4
} ti_c54x_operand_field_status;

typedef struct ti_c54x_program_words_view {
    const uint16_t *program_words;
    uint8_t program_word_count;
} ti_c54x_program_words_view;

typedef struct ti_c54x_operand_field_piece {
    uint8_t program_word_index;
    uint8_t source_lsb;
    uint8_t width_bits;
    uint8_t value_lsb;
} ti_c54x_operand_field_piece;

typedef struct ti_c54x_operand_field_layout {
    const ti_c54x_operand_field_piece *pieces;
    uint8_t piece_count;
    uint8_t value_width_bits;
} ti_c54x_operand_field_layout;

typedef struct ti_c54x_operand_field_value {
    uint64_t raw_value;
    uint8_t width_bits;
} ti_c54x_operand_field_value;

/* Validate one borrowed descriptor against a program-word width. */
ti_c54x_operand_field_status ti_c54x_operand_field_validate_layout(
    const ti_c54x_operand_field_layout *layout,
    uint8_t program_word_count);

/* Validate and assemble one borrowed field; no sign extension is performed. */
ti_c54x_operand_field_status ti_c54x_operand_field_extract(
    const ti_c54x_program_words_view *word_view,
    const ti_c54x_operand_field_layout *layout,
    ti_c54x_operand_field_value *value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPERAND_FIELD_H */
