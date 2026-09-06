#ifndef TI_C54X_ISA_OPERAND_LAYOUT_H
#define TI_C54X_ISA_OPERAND_LAYOUT_H

/*
 * Module contract
 *
 * Owns: immutable decoder-owned operand-layout-set descriptors for catalog
 * identities, their width-checked process-lifetime resolution, and copied
 * extraction of one operand through operand_field. The RPT identities are
 * deliberately inert metadata; no production opcode row or semantic
 * effect is registered here. A resolved view borrows only static descriptor
 * storage and retains no caller pointer.
 * Does not own: catalog row syntax, opcode lookup, instruction meaning,
 * repeat state, fetch/bus I/O, CPU or architectural state, timing, memory,
 * observers, diagnostics, environment, allocation, or mutable global state.
 *
 * A caller-supplied descriptor is borrowed for one validation call. A view
 * returned by resolve is value-owned except for its process-lifetime static
 * descriptor pointers. Every extraction failure leaves its output unchanged.
 */

#include <stdint.h>

#include "../catalog/opcode_catalog.h"
#include "operand_field.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_operand_layout_status {
    TI_C54X_OPERAND_LAYOUT_STATUS_OK = 0,
    TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_OPERAND_LAYOUT_STATUS_UNKNOWN_ID = 2,
    TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ID = 3,
    TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_INVALID = 4,
    TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH = 5,
    TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_SET = 6,
    TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_VIEW = 7,
    TI_C54X_OPERAND_LAYOUT_STATUS_OPERAND_INDEX_OUT_OF_RANGE = 8,
    TI_C54X_OPERAND_LAYOUT_STATUS_FIELD_LAYOUT_INVALID = 9,
    TI_C54X_OPERAND_LAYOUT_STATUS_FIELD_SOURCE_WORD_INDEX_OUT_OF_RANGE = 10
} ti_c54x_operand_layout_status;

typedef struct ti_c54x_operand_layout_set {
    const ti_c54x_operand_field_layout *operands;
    uint8_t operand_count;
} ti_c54x_operand_layout_set;

typedef struct ti_c54x_operand_layout_set_view {
    ti_c54x_operand_layout_id id;
    uint8_t program_word_count;
    const ti_c54x_operand_layout_set *descriptor;
} ti_c54x_operand_layout_set_view;

/* Validate a borrowed descriptor without resolving or retaining caller data. */
ti_c54x_operand_layout_status ti_c54x_operand_layout_set_validate_descriptor(
    const ti_c54x_operand_layout_set *descriptor,
    uint8_t program_word_count);

/* Resolve a catalog identity to immutable process-lifetime descriptor data. */
ti_c54x_operand_layout_status ti_c54x_operand_layout_set_resolve(
    ti_c54x_operand_layout_id id,
    uint8_t program_word_count,
    ti_c54x_operand_layout_set_view *output);

/* Extract one copied operand value from a resolved view. */
ti_c54x_operand_layout_status ti_c54x_operand_layout_set_extract_operand(
    const ti_c54x_operand_layout_set_view *view,
    const ti_c54x_program_words_view *word_view,
    uint8_t operand_index,
    ti_c54x_operand_field_value *value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPERAND_LAYOUT_H */
