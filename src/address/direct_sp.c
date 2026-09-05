/*
 * Module contract
 *
 * Owns: checked CPL=1 SP16+dma7 direct WORD16 effect planning with explicit
 * unresolved-boundary rejection.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, DP addressing, opcode decoding, instruction sequencing, timing, or
 * diagnostics. Inputs are copied carrier values; all result fields are
 * value-owned by the returned object.
 */

#include "direct_sp.h"

#include <stdint.h>

static ti_c54x_address_result
ti_c54x_direct_sp_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

ti_c54x_address_result
ti_c54x_direct_sp_word(ti_c54x_direct_sp16 sp, ti_c54x_direct_dma7 dma)
{
    const uint32_t sum = (uint32_t)sp.bits + (uint32_t)dma.bits;
    ti_c54x_address_result result = {0};

    if (dma.bits > UINT8_C(0x7f)) {
        return ti_c54x_direct_sp_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    if (sum > UINT32_C(0xffff)) {
        return ti_c54x_direct_sp_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = ti_c54x_data_operand_word16((uint16_t)sum);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_DIRECT_SP;
    result.effect.source.value.direct_sp.sp16 = sp.bits;
    result.effect.source.value.direct_sp.dma7 = dma.bits;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1;
    return result;
}
