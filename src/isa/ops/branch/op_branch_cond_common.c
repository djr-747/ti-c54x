/*
 * Module contract
 *
 * Owns: shared single-condition evaluation for the BC/BCD branch
 * family over the sixteen image-evidenced codes (TC/NTC, C/NC,
 * full-40-bit two's-complement signed accumulator compares
 * against zero, guard bits included). Consumed-input-only
 * knownness: each condition demands knownness only for the inputs
 * it consumes and malformed C/TC bound checks apply on every path.
 * Does not own: catalog rows, operand extraction, effect staging,
 * CPU/state mutation, sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations.
 */

#include "op_branch_cond_common.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#define COND_TC UINT8_C(0x30)
#define COND_NTC UINT8_C(0x20)
#define COND_C UINT8_C(0x0C)
#define COND_NC UINT8_C(0x08)
#define COND_AEQ UINT8_C(0x45)
#define COND_ANEQ UINT8_C(0x44)
#define COND_AGT UINT8_C(0x46)
#define COND_AGEQ UINT8_C(0x42)
#define COND_ALT UINT8_C(0x43)
#define COND_ALEQ UINT8_C(0x47)
#define COND_BEQ UINT8_C(0x4D)
#define COND_BNEQ UINT8_C(0x4C)
#define COND_BGT UINT8_C(0x4E)
#define COND_BGEQ UINT8_C(0x4A)
#define COND_BLT UINT8_C(0x4B)
#define COND_BLEQ UINT8_C(0x4F)

static bool
branch_cond_is_zero(uint64_t value)
{
    return (value & TI_C54X_U40_MASK) == UINT64_C(0);
}

static bool
branch_cond_is_negative(uint64_t value)
{
    return ((value >> 39U) & UINT64_C(1)) != UINT64_C(0);
}

ti_c54x_op_status
ti_c54x_branch_cond_evaluate(uint8_t cond,
                             const ti_c54x_op_input_snapshot *input,
                             bool *taken)
{
    uint64_t source;
    bool source_known;

    if (input == NULL || taken == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    /* Consumed-input-only knownness: each condition demands knownness
     * only for the inputs it consumes. */
    switch (cond) {
    case COND_TC:
    case COND_NTC:
        if (!input->status_tc_known || input->status_tc > UINT8_C(1)) {
            return input->status_tc_known
                       ? TI_C54X_OP_STATUS_INVALID_OPERAND
                       : TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        *taken = (cond == COND_TC) == (input->status_tc != UINT8_C(0));
        return TI_C54X_OP_STATUS_OK;
    case COND_C:
    case COND_NC:
        if (!input->status_c_known || input->status_c > UINT8_C(1)) {
            return input->status_c_known
                       ? TI_C54X_OP_STATUS_INVALID_OPERAND
                       : TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        *taken = (cond == COND_C) == (input->status_c != UINT8_C(0));
        return TI_C54X_OP_STATUS_OK;
    case COND_AEQ:
    case COND_ANEQ:
    case COND_AGT:
    case COND_AGEQ:
    case COND_ALT:
    case COND_ALEQ:
        source = input->accumulator_a_raw40 & TI_C54X_U40_MASK;
        source_known = input->accumulator_a_known;
        break;
    case COND_BEQ:
    case COND_BNEQ:
    case COND_BGT:
    case COND_BGEQ:
    case COND_BLT:
    case COND_BLEQ:
        source = input->accumulator_b_raw40 & TI_C54X_U40_MASK;
        source_known = input->accumulator_b_known;
        break;
    default:
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }

    /* Full-40-bit two's-complement signed project-interpretation
     * pins: EQ/NEQ/GT/GEQ/LT/LEQ against zero. */
    switch (cond) {
    case COND_AEQ:
    case COND_BEQ:
        *taken = branch_cond_is_zero(source);
        break;
    case COND_ANEQ:
    case COND_BNEQ:
        *taken = !branch_cond_is_zero(source);
        break;
    case COND_AGT:
    case COND_BGT:
        *taken =
            !branch_cond_is_negative(source) && !branch_cond_is_zero(source);
        break;
    case COND_AGEQ:
    case COND_BGEQ:
        *taken = !branch_cond_is_negative(source);
        break;
    case COND_ALT:
    case COND_BLT:
        *taken = branch_cond_is_negative(source);
        break;
    default:
        *taken =
            branch_cond_is_negative(source) || branch_cond_is_zero(source);
        break;
    }
    return TI_C54X_OP_STATUS_OK;
}
