#ifndef TI_C54X_STATE_STATUS_REGISTERS_H
#define TI_C54X_STATE_STATUS_REGISTERS_H

/*
 * Module contract
 *
 * Owns: the private ST0, ST1, and PMST register bank, common reset seed,
 * knownness masks, and checked field-local access for DP, ASM, ARP, TC, C,
 * CPL, and CMPT. Reads return canonical zero/unknown values when a field is
 * unknown.
 * Does not own: CPU lifecycle, program counter, opcode semantics, effect
 * sequencing, buses, timing, diagnostics, or public persistence formats.
 */

#include <stdbool.h>
#include <stdint.h>

#include "../register_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_status_register_status {
    TI_C54X_STATUS_REGISTER_OK = 0,
    TI_C54X_STATUS_REGISTER_UNKNOWN = 1,
    TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT = 2
} ti_c54x_status_register_status;

typedef struct ti_c54x_status_register_field_result {
    ti_c54x_status_register_status status;
    uint16_t raw_value;
    bool known;
} ti_c54x_status_register_field_result;

typedef struct ti_c54x_status_register_state {
    uint16_t st0;
    uint16_t st1;
    uint16_t pmst;
    uint16_t st0_known_mask;
    uint16_t st1_known_mask;
    uint16_t pmst_known_mask;
} ti_c54x_status_register_state;

ti_c54x_status_register_status
ti_c54x_status_register_reset_common(ti_c54x_status_register_state *state);

ti_c54x_status_register_field_result
ti_c54x_status_register_read_dp(const ti_c54x_status_register_state *state);
ti_c54x_status_register_field_result
ti_c54x_status_register_read_asm(const ti_c54x_status_register_state *state);
ti_c54x_status_register_field_result
ti_c54x_status_register_read_arp(const ti_c54x_status_register_state *state);
ti_c54x_status_register_field_result
ti_c54x_status_register_read_tc(const ti_c54x_status_register_state *state);
ti_c54x_status_register_field_result
ti_c54x_status_register_read_cpl(const ti_c54x_status_register_state *state);
ti_c54x_status_register_field_result
ti_c54x_status_register_read_cmpt(const ti_c54x_status_register_state *state);
ti_c54x_status_register_field_result
ti_c54x_status_register_read_c(const ti_c54x_status_register_state *state);

ti_c54x_status_register_status
ti_c54x_status_register_write_dp(ti_c54x_status_register_state *state,
                                  uint16_t raw_value);
ti_c54x_status_register_status
ti_c54x_status_register_write_asm(ti_c54x_status_register_state *state,
                                   uint16_t raw_value);
ti_c54x_status_register_status
ti_c54x_status_register_write_arp(ti_c54x_status_register_state *state,
                                   uint8_t raw_value);
ti_c54x_status_register_status
ti_c54x_status_register_write_tc(ti_c54x_status_register_state *state,
                                  uint8_t raw_value);
ti_c54x_status_register_status
ti_c54x_status_register_write_cpl(ti_c54x_status_register_state *state,
                                   uint8_t raw_value);
ti_c54x_status_register_status
ti_c54x_status_register_write_cmpt(ti_c54x_status_register_state *state,
                                    uint8_t raw_value);
ti_c54x_status_register_status
ti_c54x_status_register_write_c(ti_c54x_status_register_state *state,
                                 uint8_t raw_value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_STATUS_REGISTERS_H */
