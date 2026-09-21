#ifndef TI_C54X_ISA_OPS_BRANCH_OP_FBACC_H
#define TI_C54X_ISA_OPS_BRANCH_OP_FBACC_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word FBACC
 * far accumulator branch with the S-selected accumulator low 16
 * bits plus page bits 22:16 carried verbatim into a FAR_PC_WRITE
 * effect. Each leaf validates identity and operands and stages
 * exactly one far program-counter target with no status, T-register,
 * or memory effect; an unknown selected accumulator rejects.
 * Does not own: catalog lookup generation, program-counter or XPC
 * commit, sequencing, timing, buses, memory, diagnostics, or public
 * ABI declarations. All inputs are borrowed read-only; outputs are
 * written only through the effect out-parameter on success.
 */

#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_fbacc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_fbacc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_FBACC_H */
