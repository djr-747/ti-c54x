#ifndef TI_C54X_ADDRESS_ABSOLUTE_H
#define TI_C54X_ADDRESS_ABSOLUTE_H

/*
 * Module contract
 *
 * Owns: mode-independent absolute DATA field carrier and the
 * checked WORD16 address-effect planner. The planned address is
 * the carried 16-bit word unchanged: no DP, CPL, CMPT, ARP, or
 * AR dependence.
 * Does not own: CPU/register mutation, bus mapping, opcode
 * decoding, status effects beyond the returned NONE policy, or
 * timing.
 *
 * Callers of the inline helper must provide a canonical 16-bit
 * DATA word address; C54x data addresses are words, not byte
 * offsets. Both interfaces are private to the core, and the
 * planner returns a value-owned effect with no retained input.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical absolute DATA address field. The caller owns the
 * precondition that `bits` holds the full 16-bit address; this
 * primitive intentionally does not mask, validate, or normalize
 * inputs. */
typedef struct ti_c54x_absolute_address16 {
    uint16_t bits;
} ti_c54x_absolute_address16;

/* Return the carried address word unchanged. */
static inline ti_c54x_data_word_address
ti_c54x_absolute_address_word(ti_c54x_absolute_address16 address)
{
    return (ti_c54x_data_word_address)address.bits;
}

/* Plan one checked absolute DATA WORD16 operand. */
ti_c54x_address_result
ti_c54x_absolute_word(ti_c54x_absolute_address16 address);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_ABSOLUTE_H */
