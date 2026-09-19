/*
 * Module contract
 *
 * Owns: checked mode-independent absolute DATA WORD16 effect
 * planning.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, opcode decoding, instruction sequencing, timing, or diagnostics.
 * Inputs are copied carrier values; the returned result owns all fields.
 */

#include "absolute.h"

#include <stdint.h>

ti_c54x_address_result
ti_c54x_absolute_word(ti_c54x_absolute_address16 address)
{
    ti_c54x_address_result result = {0};

    /* Every 16-bit pattern is a valid DATA word address; the
     * planner still funnels through the status-checked shape so
     * callers observe the same contract as the other planners. */
    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = ti_c54x_data_operand_word16(
        ti_c54x_absolute_address_word(address));
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_ABSOLUTE;
    result.effect.source.value.absolute.address = address.bits;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    return result;
}
