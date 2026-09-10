#ifndef TI_C54X_ISA_OPS_BRANCH_OP_BRANCH_COND_COMMON_H
#define TI_C54X_ISA_OPS_BRANCH_OP_BRANCH_COND_COMMON_H

/*
 * Module contract
 *
 * Owns: shared single-condition evaluation for the BC/BCD branch
 * family over the sixteen image-evidenced codes (TC/NTC, C/NC,
 * full-40-bit two's-complement signed accumulator compares
 * against zero). Consumed-input-only knownness: status-only
 * conditions ignore unknown accumulators and vice versa. Deferred
 * codes (BIO/NBIO, UNC, overflow, multi-condition combinations)
 * are not evaluated here; callers reject them.
 * Does not own: catalog rows, operand extraction, effect staging,
 * CPU/state mutation, sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations. All inputs are
 * borrowed read-only; the outcome is written only through the
 * taken out-parameter on success.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_branch_cond_evaluate(
    uint8_t cond, const ti_c54x_op_input_snapshot *input, bool *taken);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_BRANCH_COND_COMMON_H */
