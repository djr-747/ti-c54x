#ifndef TI_C54X_STATE_ACCUMULATOR_H
#define TI_C54X_STATE_ACCUMULATOR_H

/*
 * Module contract
 *
 * Owns: private canonical uint64_t-backed 40-bit A/B accumulator storage,
 * checked access, and explicit per-lane knownness. Reset physically clears
 * both lanes but leaves them architecturally unknown. No public bootstrap or
 * persistence API is introduced here.
 * Does not own: opcode semantics, status C, CPU lifecycle, sequencing, buses,
 * timing, diagnostics, allocation, or global mutable state.
 */

#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"

typedef enum ti_c54x_accumulator_id {
    TI_C54X_ACCUMULATOR_A = 0,
    TI_C54X_ACCUMULATOR_B = 1,
    TI_C54X_ACCUMULATOR_INVALID = 2
} ti_c54x_accumulator_id;

typedef enum ti_c54x_accumulator_status {
    TI_C54X_ACCUMULATOR_OK = 0,
    TI_C54X_ACCUMULATOR_UNKNOWN = 1,
    TI_C54X_ACCUMULATOR_INVALID_ARGUMENT = 2,
    TI_C54X_ACCUMULATOR_INVALID_VALUE = 3
} ti_c54x_accumulator_status;

typedef struct ti_c54x_accumulator_state {
    ti_c54x_u40_t a_raw40;
    ti_c54x_u40_t b_raw40;
    uint8_t known_mask;
} ti_c54x_accumulator_state;

typedef struct ti_c54x_accumulator_snapshot {
    ti_c54x_u40_t a_raw40;
    ti_c54x_u40_t b_raw40;
    bool a_known;
    bool b_known;
} ti_c54x_accumulator_snapshot;

ti_c54x_accumulator_status ti_c54x_accumulator_reset(
    ti_c54x_accumulator_state *state);
ti_c54x_accumulator_status ti_c54x_accumulator_read(
    const ti_c54x_accumulator_state *state, ti_c54x_accumulator_id id,
    uint64_t *value);
ti_c54x_accumulator_status ti_c54x_accumulator_write(
    ti_c54x_accumulator_state *state, ti_c54x_accumulator_id id,
    uint64_t value);
ti_c54x_accumulator_status ti_c54x_accumulator_read_snapshot(
    const ti_c54x_accumulator_state *state,
    ti_c54x_accumulator_snapshot *snapshot);

#endif /* TI_C54X_STATE_ACCUMULATOR_H */
