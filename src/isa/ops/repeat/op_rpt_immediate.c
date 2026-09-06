/*
 * Module contract
 *
 * Owns: validation and value-owned construction of the immediate-RPT
 * REPEAT_START effect. Does not own: encoding, decode, repeat state, PC,
 * CPU/register state, timing, interrupt admission, memory, buses, observers,
 * diagnostics, or allocation.
 */

#include "op_rpt_immediate.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_op_status
ti_c54x_op_rpt_immediate_from_field(
    const ti_c54x_operand_field_value *operand,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (operand == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operand->width_bits != UINT8_C(8) &&
        operand->width_bits != UINT8_C(16)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (operand->width_bits == UINT8_C(8) &&
        operand->raw_value > UINT64_C(0xff)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (operand->width_bits == UINT8_C(16) &&
        operand->raw_value > UINT64_C(0xffff)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    staged.payload.repeat_start.operand = (uint16_t)operand->raw_value;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
