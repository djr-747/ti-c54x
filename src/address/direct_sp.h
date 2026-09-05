#ifndef TI_C54X_ADDRESS_DIRECT_SP_H
#define TI_C54X_ADDRESS_DIRECT_SP_H

/*
 * Module contract
 *
 * Owns: CPL=1 direct-data SP16 field carriers and their checked non-wrapping
 * WORD16 planner.
 * Does not own: CPU/register storage, status-register reads, memory or bus
 * access, CPL=0 DP addressing, opcode decoding, instruction sequencing,
 * timing, or diagnostics. Inputs are copied; no caller storage is retained.
 */

#include <stdint.h>

#include "address_effect.h"
#include "direct_dp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_direct_sp16 {
    uint16_t bits;
} ti_c54x_direct_sp16;

/* Plan one checked CPL=1 direct WORD16 operand without address wrap. */
ti_c54x_address_result ti_c54x_direct_sp_word(ti_c54x_direct_sp16 sp,
                                              ti_c54x_direct_dma7 dma);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_DIRECT_SP_H */
