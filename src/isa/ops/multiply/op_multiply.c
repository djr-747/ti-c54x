/* Module contract
 * Owns: the pure multiply-accumulate datapath core shared by the MAC/MPY
 * family (signed 16x16 product, FRCT shift, R rounding, add/sub/store
 * select, OVM saturation with overflow disposition) over copied inputs.
 * Does not own: catalog lookup generation, operand addressing, T or
 * accumulator or status mutation, CPU sequencing, timing, buses, or
 * diagnostics.
 */

#include "op_multiply.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"

#define MULTIPLY_ROUND_CONSTANT INT64_C(0x8000)
#define MULTIPLY_ROUND_CLEAR_MASK UINT64_C(0xFFFFFFFFFFFF0000)
#define MULTIPLY_SATURATE_POSITIVE UINT64_C(0x007FFFFFFF)
#define MULTIPLY_SATURATE_NEGATIVE UINT64_C(0xFF80000000)

static int64_t sign_extend16(uint16_t value)
{
    return (int64_t)(int16_t)value;
}

static int64_t sign_extend40(ti_c54x_u40_t value)
{
    const uint64_t masked = (uint64_t)value & (uint64_t)TI_C54X_U40_MASK;
    if ((masked & UINT64_C(0x8000000000)) != UINT64_C(0)) {
        return (int64_t)(masked | UINT64_C(0xFFFFFF0000000000));
    }
    return (int64_t)masked;
}

ti_c54x_multiply_status ti_c54x_multiply_accumulate(
    const ti_c54x_multiply_operands *operands,
    ti_c54x_multiply_result *result)
{
    int64_t product;
    int64_t sum;
    uint64_t staged;
    bool overflow;

    if (operands == NULL || result == NULL) {
        return TI_C54X_MULTIPLY_INVALID_ARGUMENT;
    }
    if (operands->select >= TI_C54X_MULTIPLY_COUNT) {
        return TI_C54X_MULTIPLY_INVALID_ARGUMENT;
    }
    if (operands->frct > UINT8_C(1) || operands->ovm > UINT8_C(1)) {
        return TI_C54X_MULTIPLY_INVALID_ARGUMENT;
    }
    if (!operands->t_known || !operands->multiplicand_known ||
        !operands->frct_known || !operands->ovm_known) {
        return TI_C54X_MULTIPLY_UNKNOWN;
    }
    if (operands->select != TI_C54X_MULTIPLY_STORE) {
        if (!operands->accumulator_known) {
            return TI_C54X_MULTIPLY_UNKNOWN;
        }
        if (operands->accumulator_raw40 > (ti_c54x_u40_t)TI_C54X_U40_MASK) {
            return TI_C54X_MULTIPLY_INVALID_ARGUMENT;
        }
    }

    product = sign_extend16(operands->t_raw16) *
              sign_extend16(operands->multiplicand_raw16);
    if (operands->frct != UINT8_C(0)) {
        product *= 2;
    }

    if (operands->select == TI_C54X_MULTIPLY_STORE) {
        sum = product;
    } else if (operands->select == TI_C54X_MULTIPLY_ADD) {
        sum = sign_extend40(operands->accumulator_raw40) + product;
    } else {
        sum = sign_extend40(operands->accumulator_raw40) - product;
    }

    if (operands->rounding) {
        const uint64_t rounded =
            ((uint64_t)(sum + MULTIPLY_ROUND_CONSTANT)) &
            MULTIPLY_ROUND_CLEAR_MASK;
        sum = (int64_t)rounded;
        if (sum >= INT64_C(0x800000000000)) {
            sum -= INT64_C(0x1000000000000);
        }
    }

    overflow = sum > INT64_C(0x7FFFFFFF) || sum < INT64_C(-0x80000000);
    if (overflow && operands->ovm != UINT8_C(0)) {
        staged = sum > INT64_C(0) ? MULTIPLY_SATURATE_POSITIVE
                                  : MULTIPLY_SATURATE_NEGATIVE;
    } else {
        staged = (uint64_t)sum & (uint64_t)TI_C54X_U40_MASK;
    }

    result->accumulator_raw40 = (ti_c54x_u40_t)staged;
    result->overflow = overflow;
    return TI_C54X_MULTIPLY_OK;
}
