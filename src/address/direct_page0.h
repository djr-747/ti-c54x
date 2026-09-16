#ifndef TI_C54X_ADDRESS_DIRECT_PAGE0_H
#define TI_C54X_ADDRESS_DIRECT_PAGE0_H

/*
 * Module contract
 *
 * Owns: DP-independent page-0 direct field carrier, its pure
 * zero-extension helper, and the checked WORD16 address-effect
 * planner. The planned address is the zero-extended dma byte:
 * no DP, CPL, CMPT, or ARP dependence.
 * Does not own: CPU/register mutation, bus mapping, opcode
 * decoding, status effects beyond the returned NONE policy, or
 * timing.
 *
 * Callers of the inline helper must provide a canonical 7-bit dma
 * field (0..127); values outside that domain have no helper
 * contract. The checked planner validates the same carrier. Its
 * result is a 16-bit data-memory word address; C54x data
 * addresses are words, not byte offsets. Both interfaces are
 * private to the core, and the planner returns a value-owned
 * effect with no retained input.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical page-0 direct-address field.  The caller owns the
 * precondition that `bits` contains only the named width; this
 * primitive intentionally does not mask, validate, or normalize
 * inputs. */
typedef struct ti_c54x_direct_page0_dma7 {
    uint8_t bits;
} ti_c54x_direct_page0_dma7;

/* Return the zero-extended dma byte for a valid page-0 direct
 * operand. */
static inline ti_c54x_data_word_address
ti_c54x_direct_page0_address_word(ti_c54x_direct_page0_dma7 dma)
{
    return (ti_c54x_data_word_address)dma.bits;
}

/* Plan one checked page-0 direct WORD16 operand. */
ti_c54x_address_result
ti_c54x_direct_page0_word(ti_c54x_direct_page0_dma7 dma);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_DIRECT_PAGE0_H */
