#ifndef TI_C54X_STATE_BLOCK_SIZE_REGISTER_H
#define TI_C54X_STATE_BLOCK_SIZE_REGISTER_H

/*
 * Module contract
 *
 * Owns: checked reads and writes of the private 16-bit BK circular-buffer
 * size register and its dedicated knownness bit.
 * Does not own: architectural-state construction or reset, MMR address
 * routing, circular address arithmetic, CPU lifecycle, buses, diagnostics,
 * or public ABI declarations. The state pointer is borrowed for the duration
 * of each call and no pointer is retained. Read outputs belong to the caller;
 * rejected and unknown reads leave them unchanged. Writes mutate only BK and
 * its knownness bit, and accept every uint16_t value.
 */

#include <stdint.h>

#include "arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read BK when its dedicated knownness bit is set. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_bk(const ti_c54x_arch_state *state,
                           uint16_t *value);

/* Copy any 16-bit value into BK and mark only BK known. */
ti_c54x_arch_state_status
ti_c54x_arch_state_write_bk(ti_c54x_arch_state *state,
                            uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_BLOCK_SIZE_REGISTER_H */
