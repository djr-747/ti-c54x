/*
 * Module contract
 *
 * Owns: checked DP-independent page-0 direct WORD16 effect planning.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, opcode decoding, instruction sequencing, timing, or diagnostics.
 * Inputs are copied carrier values; the returned result owns all fields.
 */

#include "direct_page0.h"

#include <stdint.h>

static ti_c54x_address_result
ti_c54x_direct_page0_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

ti_c54x_address_result
ti_c54x_direct_page0_word(ti_c54x_direct_page0_dma7 dma)
{
    ti_c54x_address_result result = {0};

    if (dma.bits > UINT8_C(0x7f)) {
        return ti_c54x_direct_page0_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = ti_c54x_data_operand_word16(
        ti_c54x_direct_page0_address_word(dma));
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0;
    result.effect.source.value.direct_page0.dma7 = dma.bits;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    return result;
}
