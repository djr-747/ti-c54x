#ifndef TI_C54X_ISA_OP_RPT_IMMEDIATE_H
#define TI_C54X_ISA_OP_RPT_IMMEDIATE_H

/*
 * Module contract
 *
 * Owns: pure construction of a value-owned immediate-RPT repeat-start
 * effect from an already extracted operand field.
 * Does not own: instruction encoding or lookup, layout resolution, field
 * extraction, repeat state, PC sequencing, CPU/register state, timing,
 * interrupts, memory, buses, observers, diagnostics, or allocation. The
 * field is borrowed for the call; the effect is caller-owned and unchanged
 * on every failure.
 */

#include "../../decode/operand_field.h"

#include "../op_effect.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_rpt_immediate_from_field(
    const ti_c54x_operand_field_value *operand,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_RPT_IMMEDIATE_H */
