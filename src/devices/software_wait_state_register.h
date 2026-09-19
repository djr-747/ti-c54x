#ifndef TI_C54X_DEVICES_SOFTWARE_WAIT_STATE_REGISTER_H
#define TI_C54X_DEVICES_SOFTWARE_WAIT_STATE_REGISTER_H

/*
 * Module contract
 *
 * Owns: the instance-owned SWWSR cell (TI SPRU131 §10.3.1,
 * Tables 8-2/10-2, Figure 10-2): 16-bit storage plus
 * knownness with checked read/write access. Bit 15 is
 * reserved (diagram R marking): writes mask it to zero so
 * it always reads zero. Reset adopts the documented
 * all-fields-one value 0x7FFF as known.
 * Does not own: address mapping, request routing, opcode
 * semantics, CPU lifecycle, wait-state timing function, or
 * diagnostics. Callers own the cell struct and its lifetime.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_swwsr_status {
    TI_C54X_SWWSR_OK = 0,
    TI_C54X_SWWSR_UNKNOWN = 1,
    TI_C54X_SWWSR_INVALID_ARGUMENT = 2
} ti_c54x_swwsr_status;

typedef struct ti_c54x_swwsr_state {
    uint16_t value;
    uint8_t known;
} ti_c54x_swwsr_state;

/* SPRU131 Table 10-2: every field bit resets to 1, bit 15 to 0. */
#define TI_C54X_SWWSR_RESET_VALUE UINT16_C(0x7FFF)
/* Figure 10-2: bit 15 is R, bits 14:0 are R/W. */
#define TI_C54X_SWWSR_WRITABLE_MASK UINT16_C(0x7FFF)

/* Adopt the documented reset value as known. NULL rejects. */
ti_c54x_swwsr_status
ti_c54x_swwsr_reset(ti_c54x_swwsr_state *cell);

/* Checked cell access. Reads require knownness and preserve
 * *value on UNKNOWN or INVALID_ARGUMENT; writes mask to the
 * writable bits and mark the cell known. */
ti_c54x_swwsr_status
ti_c54x_swwsr_read(const ti_c54x_swwsr_state *cell,
                   uint16_t *value);
ti_c54x_swwsr_status
ti_c54x_swwsr_write(ti_c54x_swwsr_state *cell,
                    uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_DEVICES_SOFTWARE_WAIT_STATE_REGISTER_H */
