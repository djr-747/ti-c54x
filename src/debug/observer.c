/* Module contract
 * Owns: the enabled observer's copied mask, callback lifetime guard, and
 * structured event handoff.
 * Does not own: CPU state, output formatting, environment reads, streams, or
 * callback context allocation.  The disabled translation unit intentionally
 * emits no observer symbols.
 */

#include "observer.h"

#include <stddef.h>

#if TI_C54X_ENABLE_OBSERVERS

void
ti_c54x_observer_init(ti_c54x_observer_state *state,
                      const ti_c54x_trace_sink *sink)
{
    if (state == NULL) {
        return;
    }

    state->event_mask = sink != NULL ? sink->event_mask & TI_C54X_TRACE_MASK_ALL
                                     : UINT64_C(0);
    state->context = sink != NULL ? sink->context : NULL;
    state->emit = sink != NULL ? sink->emit : NULL;
    state->emitting = false;
}

void
ti_c54x_observer_emit(ti_c54x_observer_state *state,
                      const ti_c54x_trace_event *event)
{
    if (state == NULL || event == NULL || state->emit == NULL ||
        state->emitting) {
        return;
    }

    state->emitting = true;
    state->emit(state->context, event);
    state->emitting = false;
}

#else

typedef int ti_c54x_observer_disabled_translation_unit;

#endif
