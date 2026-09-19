#ifndef TI_C54X_CPU_EXECUTION_REPEAT_ACTIVATE_H
#define TI_C54X_CPU_EXECUTION_REPEAT_ACTIVATE_H

/*
 * Module contract
 *
 * Owns: CPU-side single/block-repeat run-loop wiring, composed from the
 * sequencer drivers: REPEAT_START retirement (RC load with
 * delay-slot and nesting guards plus sequential PC advance and
 * body-PC latch), REPEAT_BLOCK_START retirement (BRAF plus
 * RSA/REA programming with BRC-knownness guard), the ACTIVE
 * predecode admission check (only ELIGIBLE bodies proceed;
 * latched requests stay deferred inside the driver), and the
 * retirement-tail HOLD re-drive (body retirement records RC
 * progress and rewinds PC to the latched body on HOLD; the
 * block tail loops to RSA on REA fall-through). All future
 * repeat run-loop growth (count-zero take) extends this module,
 * never run.c.
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

/* Retire one RPT/RPTB through the driver when the effect starts a
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

typedef enum ti_c54x_cpu_repeat_retire_step {
    TI_C54X_CPU_REPEAT_RETIRE_ADVANCE = 0,
    TI_C54X_CPU_REPEAT_RETIRE_HOLD = 1,
    TI_C54X_CPU_REPEAT_RETIRE_INVALID = 2
} ti_c54x_cpu_repeat_retire_step;

/* Record one retired body while ACTIVE and re-drive the latched
 * body PC on HOLD. IDLE advances with no side effects; ADVANCE
 * clears the latch. The block tail then maps an REA fall-through
 * to LOOP (PC rewinds to RSA) or END. INVALID publishes nothing:
 * the caller maps it to its stop. */
ti_c54x_cpu_repeat_retire_step ti_c54x_cpu_repeat_body_retired(
    ti_c54x_cpu *cpu);

#endif /* TI_C54X_CPU_EXECUTION_REPEAT_ACTIVATE_H */
