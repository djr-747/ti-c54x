#ifndef TI_C54X_ISA_OPS_BRANCH_OP_RETE_COMMON_H
#define TI_C54X_ISA_OPS_BRANCH_OP_RETE_COMMON_H

/*
 * Module contract
 *
 * Owns: the shared INTM-clearing pop completion for the RETE
 * family (RETE, RETED). The completion ignores the TOS
 * observation and stages a STATUS_BIT_WRITE clearing ST1 bit
 * 11; the root commits it through the generic pop-completion
 * seam before the pop architecture commit. Each leaf validates
 * identity and operands and stages no status, accumulator,
 * T-register, or non-stack MEMORY effect itself.
 * Does not own: catalog rows, operand extraction, effect commit,
 * stack reads, program-counter or stack-pointer commit, CPU/state
 * mutation, sequencing, timing, buses, memory, diagnostics, or
 * public ABI declarations. All inputs are borrowed read-only;
 * the outcome is written only through the next-effect
 * out-parameter on success.
 */

#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ST1 bit 11 is INTM per the ST1 register layout; RETE[D] clears
 * it to enable interrupts (SPRU172C p. 4-139). */
#define TI_C54X_RETE_INTM_BIT_INDEX UINT8_C(11)

ti_c54x_op_status ti_c54x_rete_complete_intm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_RETE_COMMON_H */
