/*
 * Module contract
 *
 * Owns: structured trace event assembly and compile-time observer gating for
 * the CPU instance.
 * Does not own: lifecycle, execution sequencing, opcode semantics, formatting,
 * I/O, allocation, or diagnostics.
 */

#include "emit.h"

#include "../cpu_internal.h"

#if TI_C54X_ENABLE_OBSERVERS
void
ti_c54x_cpu_emit_trace(ti_c54x_cpu *cpu, ti_c54x_trace_kind kind)
{
    const uint64_t event_mask = ti_c54x_trace_mask_for_kind(kind);
    ti_c54x_trace_event event;

    if (!TI_C54X_OBSERVE_ENABLED(&cpu->observer, event_mask)) {
        return;
    }

    event.kind = kind;
    event.pc_word = cpu->arch_state.pc_word;
    event.opcode_word = cpu->last_fetched_word;
    event.opcode_valid = cpu->last_fetch_valid;
    event.instructions_attempted_total = cpu->instructions_attempted;
    event.instructions_retired_total = cpu->instructions_retired;
    ti_c54x_observer_emit(&cpu->observer, &event);
}
#endif
