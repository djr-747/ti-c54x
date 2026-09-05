#ifndef TI_C54X_INTERNAL_OBSERVER_H
#define TI_C54X_INTERNAL_OBSERVER_H

/* Module contract
 * Owns: copied per-instance observation configuration, event-mask gating, and
 * structured callback emission.
 * Does not own: CPU architecture, event formatting, output streams, process
 * configuration, or callback context storage.  The context is borrowed by the
 * CPU caller for the lifetime of the initialized instance.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/trace.h>

#ifndef TI_C54X_ENABLE_OBSERVERS
#define TI_C54X_ENABLE_OBSERVERS 0
#endif

#if TI_C54X_ENABLE_OBSERVERS

typedef struct ti_c54x_observer_state {
    uint64_t event_mask;
    void *context;
    ti_c54x_trace_emit_fn emit;
    bool emitting;
} ti_c54x_observer_state;

void ti_c54x_observer_init(ti_c54x_observer_state *state,
                           const ti_c54x_trace_sink *sink);
void ti_c54x_observer_emit(ti_c54x_observer_state *state,
                           const ti_c54x_trace_event *event);

/* This is the only hot-path observation gate.  The non-zero mask check is
 * deliberately first so a zero-mask instance avoids event-specific work. */
#define TI_C54X_OBSERVE_ENABLED(observer_state, event_bit)                       \
    ((observer_state)->event_mask != UINT64_C(0) &&                              \
     (((observer_state)->event_mask & (event_bit)) != UINT64_C(0)))

#define TI_C54X_OBSERVE_EMIT(observer_state, event_bit, event_pointer)           \
    do {                                                                          \
        if (TI_C54X_OBSERVE_ENABLED((observer_state), (event_bit))) {            \
            ti_c54x_observer_emit((observer_state), (event_pointer));             \
        }                                                                         \
    } while (0)

#else

typedef struct ti_c54x_observer_state {
    unsigned char disabled;
} ti_c54x_observer_state;

#define ti_c54x_observer_init(state, sink)                                       \
    do {                                                                          \
        (void)(state);                                                            \
        (void)(sink);                                                             \
    } while (0)

/* A disabled build contains no callback reference or observation branch at a
 * call site.  Keep the expression constant-false for structural audits. */
#define TI_C54X_OBSERVE_ENABLED(observer_state, event_bit)                       \
    (false && ((void)(observer_state), (void)(event_bit), true))

#define TI_C54X_OBSERVE_EMIT(observer_state, event_bit, event_pointer)           \
    do {                                                                          \
        (void)(observer_state);                                                   \
        (void)(event_bit);                                                        \
        (void)(event_pointer);                                                    \
    } while (0)

#endif

#endif
