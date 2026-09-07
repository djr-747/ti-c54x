#ifndef TI_C54X_STATE_ARCH_STATE_H
#define TI_C54X_STATE_ARCH_STATE_H

/* Module contract
 * Owns: private architectural register storage, including the wide public
 * program-word PC carrier, common TI reset seeding, and checked
 * knownness-aware AR/SP and ARP/CMPT status-field access, knownness, and
 * field-limited mutation for ARP, DP, CPL, and CMPT. Status register storage
 * is composed from the status owner; block_size_register.h owns BK access.
 * Does not own: CPU lifecycle or configured bootstrap reset, raw ST0/ST1/PMST
 * status-register APIs, address planning or sequencer policy, effect sequencing,
 * bus transfers, opcode semantics, diagnostics, or public ABI declarations.
 *
 * All inputs are copied into the supplied state.  No pointer or callback is
 * retained after a call; the caller owns the state object for its lifetime.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

#include "register_layout.h"
#include "status/registers.h"

typedef enum ti_c54x_arch_state_status {
    TI_C54X_ARCH_STATE_OK = 0,
    TI_C54X_ARCH_STATE_UNKNOWN = 1,
    TI_C54X_ARCH_STATE_INVALID_ARGUMENT = 2
} ti_c54x_arch_state_status;

typedef struct ti_c54x_arch_state {
    ti_c54x_program_word_address pc_word;
    uint16_t ifr;
    uint16_t rc;
    uint16_t bk;
    uint16_t sp;
    uint16_t ar[8];
    ti_c54x_status_register_state status;
    uint8_t scalar_known_mask;
    uint8_t ar_known_mask;
} ti_c54x_arch_state;

/* Seed the documented common TI reset subset.  The reset clears stale storage
 * and knownness first. AR0-AR7, SP, and BK remain physically zero but
 * architecturally unknown: no reset value is adopted for them because
 * SPRU131G §6.9 does not specify one. This zero-plus-unknown representation
 * is project policy. A NULL state returns INVALID_ARGUMENT. */
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

/* Checked accessors for the three-bit ST0 ARP field.  Reads require every
 * ARP knownness bit and preserve *value on UNKNOWN or INVALID_ARGUMENT.
 * Writes accept only 0..7, replace ARP bits in ST0, preserve all other ST0
 * bits and knownness, and mark ARP known.  NULL inputs and rejected writes
 * leave the state unchanged. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_arp(const ti_c54x_arch_state *state,
                            uint8_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_arp(ti_c54x_arch_state *state,
                             uint8_t value);

/* Checked accessors for the nine-bit ST0 DP field.  Reads require every DP
 * knownness bit and preserve *value on UNKNOWN or INVALID_ARGUMENT.  Writes
 * accept only 0..0x1ff, replace DP bits in ST0, preserve all other ST0 bits
 * and knownness, and mark DP known.  NULL inputs and rejected writes leave
 * the state unchanged. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_dp(const ti_c54x_arch_state *state,
                           uint16_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_dp(ti_c54x_arch_state *state,
                            uint16_t value);

/* Checked accessors for the one-bit ST1 CPL field.  Reads require the CPL
 * knownness bit and preserve *value on UNKNOWN or INVALID_ARGUMENT.  Writes
 * accept only 0 or 1, replace CPL in ST1, preserve all other ST1 bits and
 * knownness, and mark CPL known.  NULL inputs and rejected writes leave the
 * state unchanged. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_cpl(const ti_c54x_arch_state *state,
                            uint8_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_cpl(ti_c54x_arch_state *state,
                             uint8_t value);

/* Checked accessors for the one-bit ST1 CMPT field.  Reads require the CMPT
 * knownness bit and preserve *value on UNKNOWN or INVALID_ARGUMENT.  Writes
 * accept only 0 or 1, replace CMPT in ST1, preserve all other ST1 bits and
 * knownness, and mark CMPT known.  NULL inputs and rejected writes leave the
 * state unchanged. */
ti_c54x_arch_state_status
ti_c54x_arch_state_read_cmpt(const ti_c54x_arch_state *state,
                             uint8_t *value);

ti_c54x_arch_state_status
ti_c54x_arch_state_write_cmpt(ti_c54x_arch_state *state,
                              uint8_t value);

#endif /* TI_C54X_STATE_ARCH_STATE_H */
