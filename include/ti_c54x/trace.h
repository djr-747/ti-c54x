#ifndef TI_C54X_TRACE_H
#define TI_C54X_TRACE_H

/* Module contract
 * Owns: trace event value definitions and callback signatures.
 * Does not own: the sink context or any storage observed by the callback.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/address.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_trace_kind {
    TI_C54X_TRACE_RESET = 0,
    TI_C54X_TRACE_FETCH = 1,
    TI_C54X_TRACE_STOP = 2
} ti_c54x_trace_kind;

/* Event values are part of the ABI-major mask contract.  Keep these explicit:
 * callers must never derive a bit by shifting an unchecked enum value. */
#define TI_C54X_TRACE_KIND_COUNT 3U
#define TI_C54X_TRACE_MASK_RESET UINT64_C(0x0000000000000001)
#define TI_C54X_TRACE_MASK_FETCH UINT64_C(0x0000000000000002)
#define TI_C54X_TRACE_MASK_STOP UINT64_C(0x0000000000000004)
#define TI_C54X_TRACE_MASK_ALL                                                  \
    (TI_C54X_TRACE_MASK_RESET | TI_C54X_TRACE_MASK_FETCH |                      \
     TI_C54X_TRACE_MASK_STOP)

#if defined(__cplusplus)
static_assert(TI_C54X_TRACE_KIND_COUNT <= 64U,
              "trace event kinds must fit the uint64_t observer mask");
static_assert((TI_C54X_TRACE_MASK_RESET & TI_C54X_TRACE_MASK_FETCH) == 0U,
              "trace event masks must be unique");
static_assert((TI_C54X_TRACE_MASK_RESET & TI_C54X_TRACE_MASK_STOP) == 0U,
              "trace event masks must be unique");
static_assert((TI_C54X_TRACE_MASK_FETCH & TI_C54X_TRACE_MASK_STOP) == 0U,
              "trace event masks must be unique");
#else
_Static_assert(TI_C54X_TRACE_KIND_COUNT <= 64U,
               "trace event kinds must fit the uint64_t observer mask");
_Static_assert((TI_C54X_TRACE_MASK_RESET & TI_C54X_TRACE_MASK_FETCH) == 0U,
               "trace event masks must be unique");
_Static_assert((TI_C54X_TRACE_MASK_RESET & TI_C54X_TRACE_MASK_STOP) == 0U,
               "trace event masks must be unique");
_Static_assert((TI_C54X_TRACE_MASK_FETCH & TI_C54X_TRACE_MASK_STOP) == 0U,
               "trace event masks must be unique");
#endif

typedef struct ti_c54x_trace_event {
    ti_c54x_trace_kind kind;
    ti_c54x_program_word_address pc_word;
    uint16_t opcode_word;
    bool opcode_valid;
    uint64_t instructions_attempted_total;
    uint64_t instructions_retired_total;
} ti_c54x_trace_event;

typedef void (*ti_c54x_trace_emit_fn)(void *context,
                                      const ti_c54x_trace_event *event);

typedef struct ti_c54x_trace_sink {
    /* Unknown bits are ignored when this descriptor is copied into a CPU. */
    uint64_t event_mask;
    void *context;
    ti_c54x_trace_emit_fn emit;
} ti_c54x_trace_sink;

/* A CPU copies this sink description, including event_mask, during
 * initialization.  Its context is borrowed and must remain valid while events
 * may be emitted.  The callback receives structured events only; formatting,
 * output, and diagnostic policy remain the caller's responsibility.  Callbacks
 * must not re-enter or mutate the running CPU.  They may retain an event only
 * for the duration of the callback; the event storage is borrowed. */

static inline uint64_t
ti_c54x_trace_mask_for_kind(ti_c54x_trace_kind kind)
{
    switch (kind) {
    case TI_C54X_TRACE_RESET:
        return TI_C54X_TRACE_MASK_RESET;
    case TI_C54X_TRACE_FETCH:
        return TI_C54X_TRACE_MASK_FETCH;
    case TI_C54X_TRACE_STOP:
        return TI_C54X_TRACE_MASK_STOP;
    default:
        return UINT64_C(0);
    }
}

#ifdef __cplusplus
}
#endif

#endif
