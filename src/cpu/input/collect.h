#ifndef TI_C54X_CPU_INPUT_COLLECT_H
#define TI_C54X_CPU_INPUT_COLLECT_H

/* Module contract
 * Owns: transactional collection of the catalog-declared input resources
 * needed by one opcode handler into a copied input snapshot. Status C, TC, SXM,
 * SST, auxiliary registers, and T are independently selectable resources;
 * unrequested sources are not read. Source pointers are borrowed for one call
 * and the copied snapshot is caller-owned.
 * Does not own: opcode semantics, decode, CPU lifecycle, effects, mutation,
 * timing, buses, diagnostics, allocation, or global state.
 */

#include "isa/catalog/opcode_catalog.h"
#include "isa/ops/op_input.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/status/registers.h"
#include "state/temporary/t_register.h"

typedef struct ti_c54x_input_sources {
    const ti_c54x_accumulator_state *accumulators;
    const ti_c54x_status_register_state *status;
    const ti_c54x_arch_state *arch_state;
    const ti_c54x_t_register_state *t_register;
} ti_c54x_input_sources;

typedef enum ti_c54x_input_collect_status {
    TI_C54X_INPUT_COLLECT_OK = 0,
    TI_C54X_INPUT_COLLECT_INVALID_ARGUMENT = 1,
    TI_C54X_INPUT_COLLECT_INVALID_RESOURCE_MASK = 2,
    TI_C54X_INPUT_COLLECT_INVALID_SOURCE = 3
} ti_c54x_input_collect_status;

ti_c54x_input_collect_status ti_c54x_input_collect(
    ti_c54x_input_resource_mask resources,
    const ti_c54x_input_sources *sources,
    ti_c54x_op_input_snapshot *output);

#endif /* TI_C54X_CPU_INPUT_COLLECT_H */
