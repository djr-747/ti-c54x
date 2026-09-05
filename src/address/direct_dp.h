#ifndef TI_C54X_ADDRESS_DIRECT_DP_H
#define TI_C54X_ADDRESS_DIRECT_DP_H

/*
 * Module contract
 *
 * Owns: the pure CPL=0 direct-data address concatenation from the common
 * C54x address model.
 * Does not own: input validation or normalization, CPU/register mutation,
 * bus mapping, CPL=1 addressing, opcode decoding, status effects, or timing.
 *
 * Callers must provide a canonical 9-bit DP field (0..511) and a canonical
 * 7-bit dma field (0..127).  The result is a 16-bit data-memory word address;
 * C54x data addresses are words, not byte offsets.  Values outside those
 * domains have no contract.  This header-only primitive is intentionally
 * private to the core and has no public ABI.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

/* Canonical direct-address fields.  The caller owns the precondition that
 * `bits` contains only the named width; this primitive intentionally does not
 * mask, validate, or normalize inputs. */
typedef struct ti_c54x_direct_dp9 {
    uint16_t bits;
} ti_c54x_direct_dp9;

typedef struct ti_c54x_direct_dma7 {
    uint8_t bits;
} ti_c54x_direct_dma7;

/* Return (DP << 7) | dma for a valid CPL=0 direct-data operand. */
static inline ti_c54x_data_word_address
ti_c54x_direct_dp_address_word(ti_c54x_direct_dp9 dp,
                               ti_c54x_direct_dma7 dma)
{
    const uint32_t concatenated =
        ((uint32_t)dp.bits << UINT32_C(7)) | (uint32_t)dma.bits;
    return (ti_c54x_data_word_address)concatenated;
}

#endif /* TI_C54X_ADDRESS_DIRECT_DP_H */
