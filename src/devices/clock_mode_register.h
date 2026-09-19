#ifndef TI_C54X_DEVICES_CLOCK_MODE_REGISTER_H
#define TI_C54X_DEVICES_CLOCK_MODE_REGISTER_H

/*
 * Module contract
 *
 * Owns: the instance-owned CLKMD cell (TI SPRU131 §8
 * clock-generator module, Figure 8-4 diagram, Tables
 * 8-18/8-19): 16-bit storage plus knownness with checked
 * read/write access. Bits 15:1 are R/W PLL configuration;
 * bit 0 (PLLSTATUS) is R status, so writes mask it to zero.
 * Reset values are CLKMD-pin-dependent (Table 8-16), so no
 * reset value is adopted: reset leaves the cell UNKNOWN per
 * standing zero-plus-unknown policy.
 * Does not own: address mapping, request routing, opcode
 * semantics, CPU lifecycle, PLL lock dynamics, clock timing
 * function, or diagnostics. Callers own the cell struct and
 * its lifetime.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_clkmd_status {
    TI_C54X_CLKMD_OK = 0,
    TI_C54X_CLKMD_UNKNOWN = 1,
    TI_C54X_CLKMD_INVALID_ARGUMENT = 2
} ti_c54x_clkmd_status;

typedef struct ti_c54x_clkmd_state {
    uint16_t value;
    uint8_t known;
} ti_c54x_clkmd_state;

/* Figure 8-4: bit 0 is R status, bits 15:1 are R/W. */
#define TI_C54X_CLKMD_WRITABLE_MASK UINT16_C(0xFFFE)

/* Drop storage and knownness: reset is pin-dependent. NULL
 * rejects. */
ti_c54x_clkmd_status
ti_c54x_clkmd_reset(ti_c54x_clkmd_state *cell);

/* Checked cell access. Reads require knownness and preserve
 * *value on UNKNOWN or INVALID_ARGUMENT; writes mask to the
 * writable bits and mark the cell known. */
ti_c54x_clkmd_status
ti_c54x_clkmd_read(const ti_c54x_clkmd_state *cell,
                   uint16_t *value);
ti_c54x_clkmd_status
ti_c54x_clkmd_write(ti_c54x_clkmd_state *cell,
                    uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_DEVICES_CLOCK_MODE_REGISTER_H */
