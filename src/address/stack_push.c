/*
 * Module contract
 *
 * Owns: checked stack-push WORD16 effect planning with explicit mod-65536
 * SP wrap. The TOS slot is the post-decrement address, so the update is
 * PRE_ACCESS by the established MOD3 shape vocabulary.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, mode policy, opcode decoding, instruction sequencing, timing, or
 * diagnostics. Inputs are copied carrier values; all result fields are
 * value-owned by the returned object.
 */

#include "stack_push.h"

#include <stdint.h>

#include "data_operand.h"

ti_c54x_address_result
ti_c54x_stack_push_word16(uint16_t sp_before)
{
    const uint16_t tos_address = (uint16_t)(sp_before - UINT16_C(1));
    ti_c54x_address_result result = {0};

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = ti_c54x_data_operand_word16(tos_address);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_STACK_PUSH;
    result.effect.source.value.stack_push.sp_before = sp_before;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_SP;
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
    result.effect.update.ar_index = UINT8_C(0);
    result.effect.update.value_before = sp_before;
    result.effect.update.value_after = tos_address;
    return result;
}
