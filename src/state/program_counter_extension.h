#ifndef TI_C54X_STATE_PROGRAM_COUNTER_EXTENSION_H
#define TI_C54X_STATE_PROGRAM_COUNTER_EXTENSION_H

/*
 * Module contract
 *
 * Owns: checked access to the private 16-bit XPC program-counter
 * extension register and its scalar knownness bit. XPC carries
 * the upper 7 bits of the extended program address; only the
 * six far control instructions modify it, and it initializes to
 * 0 at hardware reset. Does not own: interrupt sources,
 * vectors, priority, far branch/call/return semantics, MMR
 * routing (XPC 0x1E stays outside the MMR file), CPU
 * lifecycle, timing, buses, allocation, observers, or public
 * ABI declarations. State is borrowed for one call;
 * unknown/rejected reads and rejected writes preserve
 * caller-visible state.
 */

#include <stdint.h>

#include "arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_arch_state_status
ti_c54x_arch_state_read_xpc(const ti_c54x_arch_state *state,
                            uint16_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_xpc(ti_c54x_arch_state *state,
                             uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_PROGRAM_COUNTER_EXTENSION_H */
