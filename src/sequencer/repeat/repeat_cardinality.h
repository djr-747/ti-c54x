#ifndef TI_C54X_SEQUENCER_REPEAT_CARDINALITY_H
#define TI_C54X_SEQUENCER_REPEAT_CARDINALITY_H

/*
 * Module contract
 *
 * Owns: pure, checked derivation of a value-owned uint32_t body-execution
 * cardinality from a copied uint16_t immediate-repeat operand.
 * Does not own: body execution, architectural RC or PC state, opcode effects,
 * timing, interrupts, buses, memory, allocation, observers, or diagnostics.
 * The caller owns the output object; no pointer is retained after the call.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT UINT32_C(65536)

typedef enum ti_c54x_sequencer_repeat_cardinality_status {
    TI_C54X_SEQUENCER_REPEAT_CARDINALITY_OK = 0,
    TI_C54X_SEQUENCER_REPEAT_CARDINALITY_INVALID_ARGUMENT = 1
} ti_c54x_sequencer_repeat_cardinality_status;

typedef struct ti_c54x_sequencer_repeat_cardinality {
    uint32_t body_execution_count;
} ti_c54x_sequencer_repeat_cardinality;

/* Derive operand+1 after widening; a null output is rejected. */
ti_c54x_sequencer_repeat_cardinality_status
ti_c54x_sequencer_repeat_cardinality_from_operand(
    uint16_t operand,
    ti_c54x_sequencer_repeat_cardinality *cardinality);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_REPEAT_CARDINALITY_H */
