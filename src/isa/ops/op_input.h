#ifndef TI_C54X_ISA_OP_INPUT_H
#define TI_C54X_ISA_OP_INPUT_H

/*
 * Module contract
 *
 * Owns: the private, copied read-only input carrier supplied to every opcode
 * handler. The snapshot is borrowed for one call and no handler may retain
 * its address; raw40 values and status C are canonical and knownness is
 * explicit.
 * Does not own: architectural storage, decode, effects, sequencing, CPU
 * lifecycle, buses, timing, diagnostics, or public persistence formats.
 */

#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"

typedef struct ti_c54x_op_input_snapshot {
    ti_c54x_u40_t accumulator_a_raw40;
    ti_c54x_u40_t accumulator_b_raw40;
    bool accumulator_a_known;
    bool accumulator_b_known;
    uint8_t status_c;
    bool status_c_known;
} ti_c54x_op_input_snapshot;

#endif /* TI_C54X_ISA_OP_INPUT_H */
