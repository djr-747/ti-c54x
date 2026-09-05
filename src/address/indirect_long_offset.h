#ifndef TI_C54X_INDIRECT_LONG_OFFSET_H
#define TI_C54X_INDIRECT_LONG_OFFSET_H

/*
 * Module contract
 *
 * Owns: checked CMPT=0 signed long-offset MOD13 effective-address planning
 * for WORD16 and LONG32 data operands, including the copied raw extension,
 * the computed address, and one selected-AR pre-access update.
 * Does not own: CPU/register storage, ARP/CMPT validation, instruction fetch,
 * opcode decoding, bus transfers, address wrapping, MOD12/MOD14/MOD15 modes,
 * instruction scheduling, timing, or diagnostics.
 * Inputs are copied scalar values. Results are returned by value and retain no
 * borrowed storage. The validator borrows an effect read-only only until it
 * returns and retains no pointer. Every non-OK result has an all-zero effect.
 */

#include <stdbool.h>
#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The raw modifier field represented by this module's supported mode. */
enum { TI_C54X_INDIRECT_LONG_OFFSET_MOD13 = 13 };

/*
 * Plan CMPT=0 MOD13 `*+ARx(lk)` for one 16-bit data operand. `lk` is copied
 * verbatim and interpreted as a signed 16-bit two's-complement displacement.
 * The old ARx value is copied as the architectural register source; the
 * resulting sum is both the operand address and selected ARx PRE_ACCESS value.
 */
ti_c54x_address_result ti_c54x_indirect_long_offset_mod13_word16(
    uint8_t ar_index,
    uint16_t ar_value,
    uint16_t long_offset_word,
    ti_c54x_address_access access);

/* The same checked MOD13 effect shape for a canonical two-word LONG32 data
 * operand. The signed offset remains one data-word displacement; the paired
 * address is supplied by the canonical LONG32 descriptor. */
ti_c54x_address_result ti_c54x_indirect_long_offset_mod13_long32(
    uint8_t ar_index,
    uint16_t ar_value,
    uint16_t long_offset_word,
    ti_c54x_address_access access);

/*
 * Validate the intrinsic MOD13 effect shape without reading CPU state. The
 * caller owns the effect storage and must keep it valid only for this call;
 * no pointer is retained. This accepts planner-produced effects and rejects
 * malformed source, operand, update, policy, mode, or arithmetic relations.
 */
bool ti_c54x_indirect_long_offset_effect_is_valid(
    const ti_c54x_address_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_LONG_OFFSET_H */
