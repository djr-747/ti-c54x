#ifndef TI_C54X_ISA_OPS_MULTIPLY_H
#define TI_C54X_ISA_OPS_MULTIPLY_H

/* Module contract
 * Owns: the pure multiply-accumulate datapath core shared by the MAC/MPY
 * family (signed 16x16 product, unsigned 16x16 product when the
 * unsigned-operands select is set, or mixed unsigned-T/signed-multiplicand
 * product when the unsigned-t-only select is set, FRCT shift, R rounding,
 * add/sub/store select, OVM saturation with overflow disposition) over
 * copied inputs.
 * Inputs are borrowed for one call; the result is written only after
 * complete validation, and nothing is staged on unknown/invalid stops.
 * SMUL product pre-saturation is out of scope: the common target models
 * the SMUL=0 baseline (only the accumulate/store result saturates).
 * Does not own: catalog lookup generation, operand addressing, T or
 * accumulator or status mutation, CPU sequencing, timing, buses, or
 * diagnostics.
 */

#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_multiply_select {
    TI_C54X_MULTIPLY_STORE = 0,
    TI_C54X_MULTIPLY_ADD = 1,
    TI_C54X_MULTIPLY_SUBTRACT = 2,
    TI_C54X_MULTIPLY_COUNT = 3
} ti_c54x_multiply_select;

typedef enum ti_c54x_multiply_status {
    TI_C54X_MULTIPLY_OK = 0,
    TI_C54X_MULTIPLY_UNKNOWN = 1,
    TI_C54X_MULTIPLY_INVALID_ARGUMENT = 2
} ti_c54x_multiply_status;

typedef struct ti_c54x_multiply_operands {
    uint16_t t_raw16;
    bool t_known;
    uint16_t multiplicand_raw16;
    bool multiplicand_known;
    ti_c54x_u40_t accumulator_raw40;
    bool accumulator_known;
    uint8_t frct;
    bool frct_known;
    uint8_t ovm;
    bool ovm_known;
    bool rounding;
    bool unsigned_operands;
    /* Mixed-sign select (MACSU): T is read unsigned while the
     * multiplicand stays signed. The both-unsigned select keeps
     * precedence when both selects are set. */
    bool unsigned_t_only;
    ti_c54x_multiply_select select;
} ti_c54x_multiply_operands;

typedef struct ti_c54x_multiply_result {
    ti_c54x_u40_t accumulator_raw40;
    bool overflow;
} ti_c54x_multiply_result;

ti_c54x_multiply_status ti_c54x_multiply_accumulate(
    const ti_c54x_multiply_operands *operands,
    ti_c54x_multiply_result *result);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MULTIPLY_H */
