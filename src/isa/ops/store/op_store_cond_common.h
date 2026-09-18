#ifndef TI_C54X_ISA_OPS_STORE_OP_STORE_COND_COMMON_H
#define TI_C54X_ISA_OPS_STORE_OP_STORE_COND_COMMON_H

/*
 * Module contract
 *
 * Owns: shared 4-bit accumulator-condition evaluation for the
 * class-15 conditional stores (SACCD/SRCCD/STRCD) over the TI
 * SPRU172C cond codes: AGEQ=0010, ALT=0011, ANEQ=0100,
 * AEQ=0101, AGT=0110, ALEQ=0111 over accumulator A and
 * BGEQ=1010, BLT=1011, BNEQ=1100, BEQ=1101, BGT=1110,
 * BLEQ=1111 over accumulator B, each a full-40-bit
 * two's-complement signed compare against zero with guard
 * bits included. Consumed-input-only knownness: the
 * unselected accumulator may be unknown; an unknown selected
 * accumulator rejects with UNKNOWN_INPUT and the four
 * unassigned nibbles (0000, 0001, 1000, 1001) reject with
 * INVALID_OPERAND. Pure and read-only over borrowed inputs;
 * the outcome is written only through the taken out-parameter
 * on success.
 * Does not own: catalog rows, operand extraction, effect
 * staging, CPU/state mutation, sequencing, timing, buses,
 * memory, diagnostics, or public ABI declarations.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_store_cond_evaluate(
    uint8_t cond4, const ti_c54x_op_input_snapshot *input, bool *taken);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_STORE_COND_COMMON_H */
