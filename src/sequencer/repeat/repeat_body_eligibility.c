/*
 * Module contract
 *
 * Owns: transactional mapping from the catalog's tri-state repeatability
 * scalar to a repeat-body admission decision.
 * Does not own: catalog lookup, opcode or addressing inference, decode, body
 * execution, RC, PC, interrupts, timing, buses, memory, or diagnostics.
 */

#include "repeat_body_eligibility.h"

#include <stddef.h>

ti_c54x_sequencer_repeat_body_eligibility_status
ti_c54x_sequencer_repeat_body_admission_from_class(
    ti_c54x_repeatability_class repeatability,
    ti_c54x_sequencer_repeat_body_admission *admission)
{
    ti_c54x_sequencer_repeat_body_admission staged;

    if (admission == NULL) {
        return TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_ARGUMENT;
    }

    switch (repeatability) {
    case TI_C54X_REPEATABILITY_UNRESOLVED:
        staged = TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED;
        break;
    case TI_C54X_REPEATABILITY_ELIGIBLE:
        staged = TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;
        break;
    case TI_C54X_REPEATABILITY_INELIGIBLE:
        staged = TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE;
        break;
    case TI_C54X_REPEATABILITY_COUNT:
    default:
        return
            TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_REPEATABILITY;
    }

    *admission = staged;
    return TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK;
}
