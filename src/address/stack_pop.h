#ifndef TI_C54X_ADDRESS_STACK_POP_H
#define TI_C54X_ADDRESS_STACK_POP_H

/*
 * Module contract
 *
 * Owns: the pure stack-pop WORD16 planner mapping one old stack-pointer
 * value to the TOS data operand at old SP with a single POST_ACCESS
 * SP update. Wrap at SP=0xFFFF is an explicit project-interpretation
 * pin: the manual states no stack status, and the 16-bit SP increment
 * wraps.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, CPL/CMPT/ARP policy, opcode decoding, instruction sequencing,
 * timing, or diagnostics. Inputs are copied; all result fields are
 * value-owned by the returned object.
 */

#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Plan one checked stack-pop WORD16 operand with wrapping SP arithmetic. */
ti_c54x_address_result ti_c54x_stack_pop_word16(uint16_t sp_before);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_STACK_POP_H */
