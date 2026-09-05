#ifndef TI_C54X_ADDRESS_DIRECT_DP_H
#define TI_C54X_ADDRESS_DIRECT_DP_H

/*
 * Module contract
 *
 * Owns: CPL=0 direct-data field carriers, their pure concatenation helper,
 * and the checked WORD16 address-effect planner.
 * Does not own: CPU/register mutation, bus mapping, CPL=1 addressing, opcode
 * decoding, status effects beyond the returned CPL policy, or timing.
 *
 * Callers of the inline helper must provide a canonical 9-bit DP field
 * (0..511) and canonical 7-bit dma field (0..127); values outside those
 * domains have no helper contract. The checked planner validates the same
 * carriers. Its result is a 16-bit data-memory word address; C54x data
 * addresses are words, not byte offsets. Both interfaces are private to the
 * core, and the planner returns a value-owned effect with no retained input.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* Plan one checked CPL=0 direct WORD16 operand. */
ti_c54x_address_result ti_c54x_direct_dp_word(ti_c54x_direct_dp9 dp,
                                              ti_c54x_direct_dma7 dma);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_DIRECT_DP_H */
