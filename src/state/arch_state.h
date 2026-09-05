#ifndef TI_C54X_STATE_ARCH_STATE_H
#define TI_C54X_STATE_ARCH_STATE_H

/* Module contract
 * Owns: private, fixed-size architectural register storage, common TI reset
 * seeding, and checked knownness-aware auxiliary-register/stack accessors.
 * Does not own: CPU lifecycle or configured bootstrap reset, address planning,
 * effect sequencing, bus transfers, opcode semantics, diagnostics, or public
 * ABI declarations.
 *
 * All inputs are copied into the supplied state.  No pointer or callback is
 * retained after a call; the caller owns the state object for its lifetime.
 */

#include <stdint.h>

#include "register_layout.h"

typedef enum ti_c54x_arch_state_status {
    TI_C54X_ARCH_STATE_OK = 0,
    TI_C54X_ARCH_STATE_UNKNOWN = 1,
    TI_C54X_ARCH_STATE_INVALID_ARGUMENT = 2
} ti_c54x_arch_state_status;

typedef struct ti_c54x_arch_state {
    uint16_t pc_word;
    uint16_t ifr;
    uint16_t rc;
    uint16_t sp;
    uint16_t ar[8];
    uint16_t st0;
    uint16_t st1;
    uint16_t pmst;

    uint16_t st0_known_mask;
    uint16_t st1_known_mask;
    uint16_t pmst_known_mask;
    uint8_t scalar_known_mask;
    uint8_t ar_known_mask;
} ti_c54x_arch_state;

/* Seed the documented common TI reset subset.  The reset clears stale storage
 * and knownness first.  AR0-AR7 and SP remain physically zero but unknown,
 * because SPRU131G §6.9 does not initialize them.  A NULL state returns
 * INVALID_ARGUMENT. */
ti_c54x_arch_state_status
ti_c54x_arch_state_reset_common(ti_c54x_arch_state *state);

/* Checked accessors never write *value on UNKNOWN or INVALID_ARGUMENT.  NULL
 * pointers and AR indices outside 0..7 return INVALID_ARGUMENT; rejected
 * writes leave the state unchanged. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_ar(const ti_c54x_arch_state *state,
                           uint8_t index,
                           uint16_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_ar(ti_c54x_arch_state *state,
                            uint8_t index,
                            uint16_t value);

ti_c54x_arch_state_status
ti_c54x_arch_state_read_sp(const ti_c54x_arch_state *state,
                           uint16_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_sp(ti_c54x_arch_state *state,
                            uint16_t value);

#endif /* TI_C54X_STATE_ARCH_STATE_H */
