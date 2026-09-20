#ifndef TI_C54X_SEQUENCER_INTERRUPT_TAKE_COMPLETE_H
#define TI_C54X_SEQUENCER_INTERRUPT_TAKE_COMPLETE_H

/*
 * Module contract
 *
 * Owns: the INTM-setting push completion for the NMI take
 * (P3-S94). The completion ignores the staged push value and
 * stages a STATUS_BIT_WRITE setting ST1 bit 11, mirroring the
 * RETE INTM-clear completion; the root commits it through the
 * generic WRITE-completion seam before the push architecture
 * commit (TI SPRU131G section 6.10.8: INTM is set when the
 * nonmaskable take is acknowledged).
 * Does not own: catalog rows, operand extraction, effect commit,
 * stack writes, program-counter or stack-pointer commit, CPU
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations. All inputs are borrowed read-only; the outcome
 * is written only through the next-effect out-parameter.
 */

#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ST1 bit 11 is INTM per the ST1 register layout; the take sets
 * it to block maskable hardware interrupts (SPRU131G p. 6-35). */
#define TI_C54X_TAKE_INTM_BIT_INDEX UINT8_C(11)

ti_c54x_op_status ti_c54x_take_complete_intm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t staged_value_word,
    ti_c54x_op_effect *next_effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INTERRUPT_TAKE_COMPLETE_H */
