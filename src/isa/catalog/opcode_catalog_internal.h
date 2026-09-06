#ifndef TI_C54X_ISA_OPCODE_CATALOG_INTERNAL_H
#define TI_C54X_ISA_OPCODE_CATALOG_INTERNAL_H

/*
 * Module contract
 *
 * Owns: the private catalog-to-decoder hand-off for exact first-word lookup.
 * Does not own: public decode result policy, opcode semantics, CPU state,
 * buses, timing, or diagnostics. This header is an internal source seam and
 * is not installed as a public library header.
 */

#include <stdint.h>

#include "opcode_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Return UNKNOWN for every first word absent from the generated direct table. */
ti_c54x_opcode_id ti_c54x_opcode_catalog_lookup_first_word(uint16_t first_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPCODE_CATALOG_INTERNAL_H */
