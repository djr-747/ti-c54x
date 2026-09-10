/*
 * Module contract
 *
 * Owns: shared Sind evaluation for the BANZ/BANZD branch family
 * over the image-evidenced I=1/MOD0 (*ARx) and I=1/MOD1 (*ARx-)
 * combinations with always-applied side effects and
 * taken/not-taken BRANCH_AUXILIARY emission gated on known
 * selected AR. Deferred Sinds reject before input consumption.
 * Does not own: catalog rows, operand extraction, identity
 * validation, CPU/state mutation, sequencing, timing, buses,
 * memory, diagnostics, or public ABI declarations.
 */

#include "op_branch_sind_common.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#define SIND_INDIRECT_BIT UINT8_C(0x80)
#define SIND_MOD_SHIFT UINT8_C(3)
#define SIND_MOD_MASK UINT8_C(0x0F)
#define SIND_MOD_OBSERVE UINT8_C(0)
#define SIND_MOD_POSTDECREMENT UINT8_C(1)
#define SIND_ARX_MASK UINT8_C(0x07)

ti_c54x_op_status
ti_c54x_branch_sind_evaluate(uint8_t sind, uint16_t pmad,
                             const ti_c54x_op_input_snapshot *input,
                             bool delayed, ti_c54x_op_effect *effect)
{
    uint8_t mod;
    uint8_t ar_index;
    bool decrement;
    uint16_t value;
    ti_c54x_op_effect staged;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((uint8_t)(sind & SIND_INDIRECT_BIT) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    mod = (uint8_t)((uint8_t)(sind >> SIND_MOD_SHIFT) & SIND_MOD_MASK);
    if (mod != SIND_MOD_OBSERVE && mod != SIND_MOD_POSTDECREMENT) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    ar_index = (uint8_t)(sind & SIND_ARX_MASK);
    decrement = mod == SIND_MOD_POSTDECREMENT;
    if (ar_index >= UINT8_C(8)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (((uint8_t)(input->auxiliary_registers_known_mask >> ar_index) &
         UINT8_C(1)) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    value = input->auxiliary_registers_raw16[ar_index];
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_BRANCH_AUXILIARY,
        .payload.branch_auxiliary = {
            .ar_index = ar_index,
            .value_after = (uint16_t)(decrement ? value - UINT16_C(1)
                                                : value),
            .update = decrement,
            .taken = value != UINT16_C(0),
            .delayed = delayed,
            .pmad = pmad}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
