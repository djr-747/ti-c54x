/*
 * Module contract
 *
 * Owns: defined-width implementation of immediate-repeat cardinality
 * derivation, with publication only after the result is complete.
 * Does not own: body execution, RC or PC mutation, opcode semantics, timing,
 * interrupts, buses, memory, allocation, observers, or diagnostics.
 */

#include "repeat_cardinality.h"

#include <stddef.h>
#include <stdint.h>

_Static_assert(UINT32_MAX >=
                   TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT,
               "uint32_t must represent the maximum repeat cardinality");

ti_c54x_sequencer_repeat_cardinality_status
ti_c54x_sequencer_repeat_cardinality_from_operand(
    uint16_t operand,
    ti_c54x_sequencer_repeat_cardinality *cardinality)
{
    ti_c54x_sequencer_repeat_cardinality staged = {UINT32_C(0)};

    if (cardinality == NULL) {
        return TI_C54X_SEQUENCER_REPEAT_CARDINALITY_INVALID_ARGUMENT;
    }

    staged.body_execution_count = (uint32_t)operand + UINT32_C(1);
    *cardinality = staged;
    return TI_C54X_SEQUENCER_REPEAT_CARDINALITY_OK;
}
