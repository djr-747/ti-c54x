/*
 * Module contract
 *
 * Owns: checked CPL=0 DP9+dma7 direct WORD16 effect planning.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, opcode decoding, instruction sequencing, timing, or diagnostics.
 * Inputs are copied carrier values; the returned result owns all fields.
 */

#include "direct_dp.h"

#include <stdint.h>

static ti_c54x_address_result
ti_c54x_direct_dp_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

ti_c54x_address_result
ti_c54x_direct_dp_word(ti_c54x_direct_dp9 dp, ti_c54x_direct_dma7 dma)
{
    ti_c54x_address_result result = {0};

    if (dp.bits > UINT16_C(0x01ff) || dma.bits > UINT8_C(0x7f)) {
        return ti_c54x_direct_dp_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = ti_c54x_data_operand_word16(
        ti_c54x_direct_dp_address_word(dp, dma));
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_DIRECT_DP;
    result.effect.source.value.direct_dp.dp9 = dp.bits;
    result.effect.source.value.direct_dp.dma7 = dma.bits;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0;
    return result;
}
