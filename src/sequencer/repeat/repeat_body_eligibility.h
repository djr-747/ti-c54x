#ifndef TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_H
#define TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_H

/*
 * Module contract
 *
 * Owns: pure admission policy for an already classified single-repeat body.
 * Does not own: opcode identity, catalog lookup, classification inference,
 * operand/addressing interpretation, decode, body execution, RC or PC state,
 * interrupts, timing, buses, memory, allocation, or diagnostics. The caller
 * owns the output and no pointer is retained after the call.
 */

#include "isa/catalog/opcode_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_sequencer_repeat_body_eligibility_status {
    TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK = 0,
    TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_ARGUMENT = 1,
    TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_REPEATABILITY = 2
} ti_c54x_sequencer_repeat_body_eligibility_status;

typedef enum ti_c54x_sequencer_repeat_body_admission {
    TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED = 0,
    TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT = 1,
    TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE = 2
} ti_c54x_sequencer_repeat_body_admission;

/* Map an existing catalog scalar without inspecting any catalog row. */
ti_c54x_sequencer_repeat_body_eligibility_status
ti_c54x_sequencer_repeat_body_admission_from_class(
    ti_c54x_repeatability_class repeatability,
    ti_c54x_sequencer_repeat_body_admission *admission);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_H */
