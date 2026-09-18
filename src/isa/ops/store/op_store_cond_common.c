/*
 * Module contract
 *
 * Owns: shared 4-bit accumulator-condition evaluation for the
 * class-15 conditional stores (SACCD/SRCCD/STRCD). TI SPRU172C
 * pins the twelve codes and the selected-accumulator-against-
 * zero signed semantics; guard bits participate exactly as in
 * the branch-family evaluator, which this module mirrors in a
 * store-owned, dependency-free shape. Consumed-input-only
 * knownness applies on every path.
 * Does not own: catalog rows, operand extraction, effect
 * staging, CPU/state mutation, sequencing, timing, buses,
 * memory, diagnostics, allocation, logging, or environment.
 */

#include "op_store_cond_common.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#define STORE_COND_AGEQ UINT8_C(0x2)
#define STORE_COND_ALT UINT8_C(0x3)
#define STORE_COND_ANEQ UINT8_C(0x4)
#define STORE_COND_AEQ UINT8_C(0x5)
#define STORE_COND_AGT UINT8_C(0x6)
#define STORE_COND_ALEQ UINT8_C(0x7)
#define STORE_COND_BGEQ UINT8_C(0xA)
#define STORE_COND_BLT UINT8_C(0xB)
#define STORE_COND_BNEQ UINT8_C(0xC)
#define STORE_COND_BEQ UINT8_C(0xD)
#define STORE_COND_BGT UINT8_C(0xE)
#define STORE_COND_BLEQ UINT8_C(0xF)

static bool
store_cond_is_zero(ti_c54x_u40_t value)
{
    return (value & TI_C54X_U40_MASK) == UINT64_C(0);
}

static bool
store_cond_is_negative(ti_c54x_u40_t value)
{
    return ((value >> 39U) & UINT64_C(1)) != UINT64_C(0);
}

ti_c54x_op_status
ti_c54x_store_cond_evaluate(uint8_t cond4,
                            const ti_c54x_op_input_snapshot *input,
                            bool *taken)
{
    ti_c54x_u40_t source;
    bool source_known;

    if (input == NULL || taken == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    /* Consumed-input-only knownness: each condition demands
     * knownness only for the accumulator it consumes. */
    switch (cond4) {
    case STORE_COND_AGEQ:
    case STORE_COND_ALT:
    case STORE_COND_ANEQ:
    case STORE_COND_AEQ:
    case STORE_COND_AGT:
    case STORE_COND_ALEQ:
        source = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
        break;
    case STORE_COND_BGEQ:
    case STORE_COND_BLT:
    case STORE_COND_BNEQ:
    case STORE_COND_BEQ:
    case STORE_COND_BGT:
    case STORE_COND_BLEQ:
        source = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
        break;
    default:
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    /* Full-40-bit two's-complement signed compares against
     * zero, guard bits included. */
    switch (cond4) {
    case STORE_COND_AEQ:
    case STORE_COND_BEQ:
        *taken = store_cond_is_zero(source);
        break;
    case STORE_COND_ANEQ:
    case STORE_COND_BNEQ:
        *taken = !store_cond_is_zero(source);
        break;
    case STORE_COND_AGT:
    case STORE_COND_BGT:
        *taken = !store_cond_is_negative(source) &&
                 !store_cond_is_zero(source);
        break;
    case STORE_COND_AGEQ:
    case STORE_COND_BGEQ:
        *taken = !store_cond_is_negative(source);
        break;
    case STORE_COND_ALT:
    case STORE_COND_BLT:
        *taken = store_cond_is_negative(source);
        break;
    default:
        *taken = store_cond_is_negative(source) ||
                 store_cond_is_zero(source);
        break;
    }
    return TI_C54X_OP_STATUS_OK;
}
