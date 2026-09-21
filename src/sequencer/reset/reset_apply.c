/*
 * Module contract
 *
 * Owns: the RESET field-list application described in
 * reset_apply.h.
 * Does not own: opcode decoding, handler semantics, CPU lifecycle,
 * repeat/take latch clearing, delayed-branch cancellation,
 * retirement counting, timing, memory, allocation, callbacks,
 * diagnostics, or global state.
 */

#include "reset_apply.h"

#include <stddef.h>
#include <stdint.h>

#include "../../state/arch_state.h"
#include "../../state/interrupt_flag_register.h"
#include "../../state/register_layout.h"
#include "../../state/status/registers.h"

/* ST1 bit indices for the single-bit reset targets. */
#define RESET_ST1_BIT_BRAF UINT8_C(15)
#define RESET_ST1_BIT_XF UINT8_C(13)
#define RESET_ST1_BIT_HM UINT8_C(12)
#define RESET_ST1_BIT_INTM UINT8_C(11)
#define RESET_ST1_BIT_OVM UINT8_C(9)
#define RESET_ST1_BIT_SXM UINT8_C(8)
#define RESET_ST1_BIT_C16 UINT8_C(7)
#define RESET_ST1_BIT_FRCT UINT8_C(6)

static ti_c54x_reset_apply_status
apply_status_fields(ti_c54x_status_register_state *status)
{
    /* ST0: ARP/DP clear, TC/C set, OVA/OVB clear. */
    if (ti_c54x_status_register_write_arp(status, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_tc(status, UINT8_C(1)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_c(status, UINT8_C(1)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_ova(status, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_ovb(status, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_dp(status, UINT16_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_RESET_APPLY_INVALID_STATE;
    }
    /* ST1 multi-bit fields: ASM/CMPT/CPL clear. */
    if (ti_c54x_status_register_write_asm(status, UINT16_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_cmpt(status, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_cpl(status, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_RESET_APPLY_INVALID_STATE;
    }
    /* ST1 single bits: BRAF/HM/OVM/C16/FRCT clear, XF/INTM/SXM set.
     * Reserved bit 10 is preserved. */
    if (ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_BRAF, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_XF, UINT8_C(1)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_HM, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_INTM, UINT8_C(1)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_OVM, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_SXM, UINT8_C(1)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_C16, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_status_register_write_bit(
            status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
            RESET_ST1_BIT_FRCT, UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_RESET_APPLY_INVALID_STATE;
    }
    return TI_C54X_RESET_APPLY_OK;
}

ti_c54x_reset_apply_status ti_c54x_reset_apply(
    ti_c54x_arch_state *state, uint16_t *pc_word_out)
{
    ti_c54x_status_register_field_result iptr;
    ti_c54x_arch_state candidate;
    ti_c54x_reset_apply_status status;

    if (state == NULL || pc_word_out == NULL) {
        return TI_C54X_RESET_APPLY_INVALID_ARGUMENT;
    }
    /* TI lists the IPTR-derived PC first: snapshot before reset
     * mutation. An unknown IPTR proves no successor. */
    iptr = ti_c54x_status_register_read_iptr(&state->status);
    if (iptr.status != TI_C54X_STATUS_REGISTER_OK || !iptr.known) {
        return TI_C54X_RESET_APPLY_UNKNOWN_IPTR;
    }
    candidate = *state;
    status = apply_status_fields(&candidate.status);
    if (status != TI_C54X_RESET_APPLY_OK) {
        return status;
    }
    if (ti_c54x_arch_state_write_ifr(&candidate, UINT16_C(0)) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_RESET_APPLY_INVALID_STATE;
    }
    *state = candidate;
    /* IPTR is 9 bits; the shifted successor always fits 16 bits. */
    *pc_word_out =
        (uint16_t)((uint32_t)(iptr.raw_value & UINT16_C(0x01FF)) << 7U);
    return TI_C54X_RESET_APPLY_OK;
}
