#ifndef TI_C54X_ISA_OPERAND_LAYOUT_INTERNAL_H
#define TI_C54X_ISA_OPERAND_LAYOUT_INTERNAL_H

/*
 * Module contract
 *
 * Owns: the private trusted hand-off from statically validated operand-layout
 * descriptors to one-pass packet extraction. Does not own: checked decoder
 * APIs, opcode semantics, CPU state, sequencing, buses, timing, diagnostics,
 * allocation, or caller-owned storage. Inputs are borrowed for one call and
 * all extracted values are copied into caller-owned fixed-capacity storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include "instruction_bundle.h"
#include "operand_layout.h"
#include "../catalog/opcode_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TI_C54X_TRUSTED_OPERAND_CAPACITY = 4
};

/* A plan is an immutable, process-lifetime hand-off selected by the binding
 * table.  Its identity and width are already checked when the table is built;
 * trusted extraction therefore consumes the pointer directly and never
 * resolves an operand-layout ID. */
typedef struct ti_c54x_operand_layout_plan {
    ti_c54x_operand_layout_id id;
    uint8_t program_word_count;
    const ti_c54x_operand_layout_set *descriptor;
} ti_c54x_operand_layout_plan;

/* The binding table borrows these immutable plans for the process lifetime. */
extern const ti_c54x_operand_layout_plan ti_c54x_operand_layout_plans[
    TI_C54X_OPERAND_LAYOUT_COUNT];

/* Resolve an ID only at a checked boundary.  Trusted callers must use the
 * plan pointer carried by their immutable binding instead. */
const ti_c54x_operand_layout_plan *ti_c54x_operand_layout_plan_for_id(
    ti_c54x_operand_layout_id id);

/* Extract every field from one prevalidated static plan without revalidating
 * its descriptor or resolving an ID. False means only invalid call storage. */
bool ti_c54x_operand_layout_extract_all_trusted(
    const ti_c54x_operand_layout_plan *plan,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_operand_field_value values[TI_C54X_TRUSTED_OPERAND_CAPACITY],
    uint8_t *value_count);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPERAND_LAYOUT_INTERNAL_H */
