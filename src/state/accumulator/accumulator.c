/* Module contract
 * Owns: private checked canonical A/B raw40 storage and knownness.
 * Does not own: opcode semantics, status C, CPU lifecycle, or sequencing.
 */

#include "accumulator.h"

#include <stddef.h>
#include <string.h>

static uint8_t known_bit(ti_c54x_accumulator_id id)
{
    return (uint8_t)(UINT8_C(1) << (uint8_t)id);
}

static bool valid_id(ti_c54x_accumulator_id id)
{
    return (int)id >= (int)TI_C54X_ACCUMULATOR_A &&
           id < TI_C54X_ACCUMULATOR_INVALID;
}

ti_c54x_accumulator_status ti_c54x_accumulator_reset(
    ti_c54x_accumulator_state *state)
{
    if (state == NULL) {
        return TI_C54X_ACCUMULATOR_INVALID_ARGUMENT;
    }
    memset(state, 0, sizeof(*state));
    return TI_C54X_ACCUMULATOR_OK;
}

ti_c54x_accumulator_status ti_c54x_accumulator_read(
    const ti_c54x_accumulator_state *state, ti_c54x_accumulator_id id,
    uint64_t *value)
{
    if (state == NULL || value == NULL || !valid_id(id)) {
        return TI_C54X_ACCUMULATOR_INVALID_ARGUMENT;
    }
    if ((state->known_mask & known_bit(id)) == 0U) {
        return TI_C54X_ACCUMULATOR_UNKNOWN;
    }
    *value = id == TI_C54X_ACCUMULATOR_A ? state->a_raw40 : state->b_raw40;
    return TI_C54X_ACCUMULATOR_OK;
}

ti_c54x_accumulator_status ti_c54x_accumulator_write(
    ti_c54x_accumulator_state *state, ti_c54x_accumulator_id id,
    uint64_t value)
{
    if (state == NULL || !valid_id(id)) {
        return TI_C54X_ACCUMULATOR_INVALID_ARGUMENT;
    }
    if (value > TI_C54X_U40_MASK) {
        return TI_C54X_ACCUMULATOR_INVALID_VALUE;
    }
    if (id == TI_C54X_ACCUMULATOR_A) {
        state->a_raw40 = ti_c54x_u40_mask(value);
    } else {
        state->b_raw40 = ti_c54x_u40_mask(value);
    }
    state->known_mask = (uint8_t)(state->known_mask | known_bit(id));
    return TI_C54X_ACCUMULATOR_OK;
}

ti_c54x_accumulator_status ti_c54x_accumulator_read_snapshot(
    const ti_c54x_accumulator_state *state,
    ti_c54x_accumulator_snapshot *snapshot)
{
    if (state == NULL || snapshot == NULL) {
        return TI_C54X_ACCUMULATOR_INVALID_ARGUMENT;
    }
    snapshot->a_raw40 = state->a_raw40;
    snapshot->b_raw40 = state->b_raw40;
    snapshot->a_known = (state->known_mask & UINT8_C(1)) != 0U;
    snapshot->b_known = (state->known_mask & UINT8_C(2)) != 0U;
    return snapshot->a_known && snapshot->b_known
               ? TI_C54X_ACCUMULATOR_OK
               : TI_C54X_ACCUMULATOR_UNKNOWN;
}
