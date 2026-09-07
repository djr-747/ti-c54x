#ifndef TI_C54X_ISA_OPS_DISPATCH_TRUSTED_OPCODE_BINDING_H
#define TI_C54X_ISA_OPS_DISPATCH_TRUSTED_OPCODE_BINDING_H

/*
 * Module contract
 *
 * Owns: immutable catalog-derived per-ID bindings between copied scalar
 * metadata and trusted semantic code, including the canonical UNKNOWN slot.
 * Does not own: first-word table generation, packet construction, operand
 * extraction, CPU state, sequencing, buses, timing, diagnostics, allocation,
 * or mutable process state. Returned pointers borrow process-lifetime static
 * storage and must not be modified or freed.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout_internal.h"
#include "trusted_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_trusted_opcode_binding {
    ti_c54x_opcode_metadata metadata;
    const ti_c54x_operand_layout_plan *operand_plan;
    ti_c54x_trusted_op_handler handler;
} ti_c54x_trusted_opcode_binding;

/* Select one canonical per-ID binding; invalid enum carriers return NULL. */
const ti_c54x_trusted_opcode_binding *
ti_c54x_trusted_opcode_binding_for_id(ti_c54x_opcode_id id);

/* Perform the one generated first-word lookup and select its binding. */
const ti_c54x_trusted_opcode_binding *
ti_c54x_trusted_opcode_binding_for_first_word(uint16_t first_word);

/* Prove that a borrowed binding points into this immutable canonical table. */
bool ti_c54x_trusted_opcode_binding_is_canonical(
    const ti_c54x_trusted_opcode_binding *binding);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_DISPATCH_TRUSTED_OPCODE_BINDING_H */
