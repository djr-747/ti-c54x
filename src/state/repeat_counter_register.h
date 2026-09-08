#ifndef TI_C54X_STATE_REPEAT_COUNTER_REGISTER_H
#define TI_C54X_STATE_REPEAT_COUNTER_REGISTER_H

/*
 * Module contract
 *
 * Owns: checked reads and writes of the private 16-bit RC repeat-counter
 * register and its dedicated knownness bit. Does not own: repeat sequencing,
 * effect validation, CPU lifecycle, program flow, timing, buses, allocation,
 * observers, or public ABI declarations. State is borrowed for one call and
 * no pointer is retained. Unknown or rejected reads preserve caller output;
 * writes mutate only RC and its knownness bit.
 */

#include <stdint.h>

#include "arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read RC when its dedicated knownness bit is set. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_rc(const ti_c54x_arch_state *state,
                           uint16_t *value);

/* Copy any 16-bit value into RC and mark only RC known. */
ti_c54x_arch_state_status
ti_c54x_arch_state_write_rc(ti_c54x_arch_state *state,
                            uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_REPEAT_COUNTER_REGISTER_H */
