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

static bool valid_word(ti_c54x_accumulator_word_id word)
{
    return (int)word >= (int)TI_C54X_ACCUMULATOR_WORD_AL &&
           word < TI_C54X_ACCUMULATOR_WORD_INVALID;
}

static ti_c54x_accumulator_id word_accumulator(
    ti_c54x_accumulator_word_id word)
{
    return word <= TI_C54X_ACCUMULATOR_WORD_AG ? TI_C54X_ACCUMULATOR_A
                                              : TI_C54X_ACCUMULATOR_B;
}

static uint8_t word_shift(ti_c54x_accumulator_word_id word)
{
    switch (word) {
    case TI_C54X_ACCUMULATOR_WORD_AL:
    case TI_C54X_ACCUMULATOR_WORD_BL:
        return UINT8_C(0);
    case TI_C54X_ACCUMULATOR_WORD_AH:
    case TI_C54X_ACCUMULATOR_WORD_BH:
        return UINT8_C(16);
    case TI_C54X_ACCUMULATOR_WORD_AG:
    case TI_C54X_ACCUMULATOR_WORD_BG:
    default:
        return UINT8_C(32);
    }
}

static uint16_t word_mask(ti_c54x_accumulator_word_id word)
{
    switch (word) {
    case TI_C54X_ACCUMULATOR_WORD_AG:
    case TI_C54X_ACCUMULATOR_WORD_BG:
        return UINT16_C(0x00FF);
    default:
        return UINT16_C(0xFFFF);
    }
}

ti_c54x_accumulator_status ti_c54x_accumulator_read_word(
    const ti_c54x_accumulator_state *state,
    ti_c54x_accumulator_word_id word, uint16_t *value)
{
    uint64_t canonical;

    if (state == NULL || value == NULL || !valid_word(word)) {
        return TI_C54X_ACCUMULATOR_INVALID_ARGUMENT;
    }
    if (ti_c54x_accumulator_read(state, word_accumulator(word),
                                 &canonical) != TI_C54X_ACCUMULATOR_OK) {
        return TI_C54X_ACCUMULATOR_UNKNOWN;
    }
    *value = (uint16_t)((canonical >> word_shift(word)) & word_mask(word));
    return TI_C54X_ACCUMULATOR_OK;
}

ti_c54x_accumulator_status ti_c54x_accumulator_write_word(
    ti_c54x_accumulator_state *state, ti_c54x_accumulator_word_id word,
    uint16_t value)
{
    ti_c54x_accumulator_id id;
    uint64_t base = UINT64_C(0);
    uint64_t cleared;
    uint64_t composed;

    if (state == NULL || !valid_word(word)) {
        return TI_C54X_ACCUMULATOR_INVALID_ARGUMENT;
    }
    if (value > word_mask(word)) {
        return TI_C54X_ACCUMULATOR_INVALID_VALUE;
    }
    id = word_accumulator(word);
    if (ti_c54x_accumulator_read(state, id, &base) !=
        TI_C54X_ACCUMULATOR_OK) {
        base = UINT64_C(0);
    }
    cleared = base & ~(uint64_t)((uint64_t)word_mask(word)
                                 << word_shift(word));
    composed = cleared | ((uint64_t)value << word_shift(word));
    return ti_c54x_accumulator_write(state, id, composed);
}
