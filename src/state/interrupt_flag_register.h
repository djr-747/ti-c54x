#ifndef TI_C54X_STATE_INTERRUPT_FLAG_REGISTER_H
#define TI_C54X_STATE_INTERRUPT_FLAG_REGISTER_H

/*
 * Module contract
 *
 * Owns: checked access to the private 16-bit IFR register and its scalar
 * knownness bit, plus validated one-bit set/clear helpers. Does not own:
 * interrupt sources, vectors, priority, repeat arbitration, CPU lifecycle,
 * timing, buses, allocation, observers, or public ABI declarations. State is
 * borrowed for one call; unknown/rejected reads and rejected writes preserve
 * caller-visible state. One-bit helpers accept exactly one set bit.
 */

#include <stdint.h>

#include "arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_arch_state_status
ti_c54x_arch_state_read_ifr(const ti_c54x_arch_state *state,
                            uint16_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_ifr(ti_c54x_arch_state *state,
                             uint16_t value);

ti_c54x_arch_state_status
ti_c54x_arch_state_set_ifr_bit(ti_c54x_arch_state *state,
                               uint16_t mask);

ti_c54x_arch_state_status
ti_c54x_arch_state_clear_ifr_bit(ti_c54x_arch_state *state,
                                 uint16_t mask);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_INTERRUPT_FLAG_REGISTER_H */
