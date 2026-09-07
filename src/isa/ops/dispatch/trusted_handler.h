#ifndef TI_C54X_ISA_OPS_DISPATCH_TRUSTED_HANDLER_H
#define TI_C54X_ISA_OPS_DISPATCH_TRUSTED_HANDLER_H

/*
 * Module contract
 *
 * Owns: the private function shape joining a constructor-validated decoded
 * instruction and copied operand values to trusted opcode semantics.
 * Does not own: opcode inventory, packet construction, architectural state, effect
 * commit, sequencing, buses, timing, diagnostics, or caller storage. All
 * pointers are borrowed for one call and are never retained.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/decode/operand_layout_internal.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

typedef struct ti_c54x_trusted_operand_values {
    ti_c54x_operand_field_value
        values[TI_C54X_TRUSTED_OPERAND_CAPACITY];
    uint8_t count;
} ti_c54x_trusted_operand_values;

typedef ti_c54x_op_status (*ti_c54x_trusted_op_handler)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_DISPATCH_TRUSTED_HANDLER_H */
