#ifndef TI_C54X_CPU_TRACE_EMIT_H
#define TI_C54X_CPU_TRACE_EMIT_H

/*
 * Module contract
 *
 * Owns: one structured observer event assembly/emission from a CPU instance.
 * Does not own: observer policy, CPU lifecycle, execution sequencing, opcode
 * semantics, formatting, I/O, allocation, or diagnostics.
 */

#include <ti_c54x/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#if TI_C54X_ENABLE_OBSERVERS
void ti_c54x_cpu_emit_trace(ti_c54x_cpu *cpu, ti_c54x_trace_kind kind);
#define TI_C54X_CPU_EMIT_TRACE(cpu, kind) \
    ti_c54x_cpu_emit_trace((cpu), (kind))
#else
#define TI_C54X_CPU_EMIT_TRACE(cpu, kind) \
    do {                              \
        (void)(cpu);                  \
        (void)(kind);                 \
    } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif
