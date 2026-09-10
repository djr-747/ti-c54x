#ifndef TI_C54X_ISA_OPS_BRANCH_OP_BRANCH_SIND_COMMON_H
#define TI_C54X_ISA_OPS_BRANCH_OP_BRANCH_SIND_COMMON_H

/*
 * Module contract
 *
 * Owns: shared Sind evaluation for the BANZ/BANZD branch family
 * over the image-evidenced I=1/MOD0 (*ARx) and I=1/MOD1 (*ARx-)
 * combinations with always-applied side effects (post-decrement
 * value-after on MOD1, observation on MOD0, even when not
 * taken) and taken/not-taken BRANCH_AUXILIARY emission gated on
 * known selected AR. All other Sind combinations reject as
 * deferred before any input is consumed. The caller selects the
 * delay flag the row owns; evaluation is identical either way.
 * Does not own: catalog rows, operand extraction, identity
 * validation, CPU/state mutation, sequencing, timing, buses,
 * memory, diagnostics, or public ABI declarations. All inputs
 * are borrowed read-only; the outcome is written only through
 * the effect out-parameter on success.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_branch_sind_evaluate(
    uint8_t sind, uint16_t pmad,
    const ti_c54x_op_input_snapshot *input, bool delayed,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_BRANCH_SIND_COMMON_H */
