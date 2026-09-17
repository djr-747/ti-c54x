#ifndef TI_C54X_ISA_OPCODE_CATALOG_INTERNAL_H
#define TI_C54X_ISA_OPCODE_CATALOG_INTERNAL_H

/*
 * Module contract
 *
 * Owns: the private catalog-to-decoder hand-off for generated exact/masked
 * first-word lookup.
 * Does not own: public decode result policy, opcode semantics, CPU state,
 * buses, timing, or diagnostics. This header is an internal source seam and
 * is not installed as a public library header.
 */

#include <stdbool.h>
#include <stdint.h>

#include "opcode_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Return UNKNOWN for every first word absent from the generated direct table. */
ti_c54x_opcode_id ti_c54x_opcode_catalog_lookup_first_word(uint16_t first_word);

/* One second-word-qualified catalog row in declaration order. */
typedef struct ti_c54x_qualified_slot {
    uint16_t first_word_value;
    uint16_t first_word_mask;
    ti_c54x_opcode_id id;
    uint16_t second_word_value;
    uint16_t second_word_mask;
} ti_c54x_qualified_slot;

/* Whether any qualified row matches the first word. */
bool ti_c54x_first_word_has_qualified_slot(uint16_t first_word);

/* First catalog-order qualified row matching both words, else UNKNOWN. */
ti_c54x_opcode_id ti_c54x_resolve_qualified_slot(uint16_t first_word,
                                                uint16_t second_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPCODE_CATALOG_INTERNAL_H */
