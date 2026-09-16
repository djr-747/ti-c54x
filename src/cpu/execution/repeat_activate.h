#ifndef TI_C54X_CPU_EXECUTION_REPEAT_ACTIVATE_H
#define TI_C54X_CPU_EXECUTION_REPEAT_ACTIVATE_H

/*
 * Module contract
 *
 * Owns: CPU-side single-repeat run-loop wiring, composed from the
 * sequencer driver: REPEAT_START retirement (RC load with
 * delay-slot and nesting guards plus sequential PC advance) and
 * the ACTIVE predecode admission check (only ELIGIBLE bodies
 * proceed; latched requests stay deferred inside the driver).
 * All future repeat run-loop growth (retirement-tail HOLD sites,
 * count-zero take) extends this module, never run.c.
 * Borrowed CPU, effect, and bridge inputs are used for one call;
 * rejected calls publish no state mutation.
 * Does not own: repeat policy (sequencer driver), PC bridge
 * planning, stop construction, retirement accounting, buses,
 * vectors, take paths, timing, observers, diagnostics,
 * allocation, or logging.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/ops/op_effect.h"
#include "sequencer/program_pc_bridge.h"
#include "ti_c54x/address.h"

typedef struct ti_c54x_cpu ti_c54x_cpu;

typedef enum ti_c54x_cpu_repeat_route_status {
    TI_C54X_CPU_REPEAT_ROUTE_NOT_REPEAT = 0,
    TI_C54X_CPU_REPEAT_ROUTE_RETIRED = 1,
    TI_C54X_CPU_REPEAT_ROUTE_REJECTED = 2
} ti_c54x_cpu_repeat_route_status;

/* Retire one RPT through the driver when the effect starts a
 * repeat; NOT_REPEAT leaves every input untouched. REJECTED
 * publishes nothing: the caller maps it to its stop. */
ti_c54x_cpu_repeat_route_status ti_c54x_cpu_repeat_route(
    ti_c54x_cpu *cpu,
    const ti_c54x_op_effect *effect,
    ti_c54x_program_pc_bridge_status bridge_status,
    ti_c54x_program_word_address candidate_pc);

/* Report whether the ACTIVE predecode check blocks this body
 * fetch. IDLE never blocks; ACTIVE blocks on driver reject or
 * non-ADMIT admission. No bus or state side effects. */
bool ti_c54x_cpu_repeat_predecode_blocks(
    ti_c54x_cpu *cpu,
    ti_c54x_repeatability_class body_class);

#endif /* TI_C54X_CPU_EXECUTION_REPEAT_ACTIVATE_H */
