/* Module contract
 * Owns: deterministic construction of the private common reset state and
 * checked knownness-aware AR/SP and ARP/DP/CPL/CMPT status-field access and
 * mutation. BK storage is part of the aggregate but its accessors live in
 * block_size_register.c.
 * Does not own: CPU lifecycle or configured bootstrap reset, raw ST0/ST1/PMST
 * status-register APIs, address planning or sequencer policy, effect sequencing,
 * bus transfers, opcode semantics, diagnostics, or public ABI declarations.
 */

#include "arch_state.h"

#include <stddef.h>
#include <string.h>

static uint8_t ar_known_bit(uint8_t index)
{
    return (uint8_t)(UINT8_C(1) << index);
}

ti_c54x_arch_state_status
ti_c54x_arch_state_reset_common(ti_c54x_arch_state *state)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }

    memset(state, 0, sizeof(*state));
    state->pc_word = UINT16_C(0xFF80);
    state->ifr = UINT16_C(0);
    state->rc = UINT16_C(0);
    state->st0 = (uint16_t)(TI_C54X_ST0_TC_MASK | TI_C54X_ST0_C_MASK);
    state->st1 = (uint16_t)(TI_C54X_ST1_XF_MASK | TI_C54X_ST1_INTM_MASK |
                            TI_C54X_ST1_SXM_MASK);
    state->pmst = TI_C54X_PMST_IPTR_MASK;
    state->st0_known_mask = (uint16_t)(TI_C54X_ST0_ARP_MASK |
                                       TI_C54X_ST0_TC_MASK |
                                       TI_C54X_ST0_C_MASK |
                                       TI_C54X_ST0_OVA_MASK |
                                       TI_C54X_ST0_OVB_MASK |
                                       TI_C54X_ST0_DP_MASK);
    state->st1_known_mask = (uint16_t)(TI_C54X_ST1_BRAF_MASK |
                                       TI_C54X_ST1_CPL_MASK |
                                       TI_C54X_ST1_XF_MASK |
                                       TI_C54X_ST1_HM_MASK |
                                       TI_C54X_ST1_INTM_MASK |
                                       TI_C54X_ST1_OVM_MASK |
                                       TI_C54X_ST1_SXM_MASK |
                                       TI_C54X_ST1_C16_MASK |
                                       TI_C54X_ST1_FRCT_MASK |
                                       TI_C54X_ST1_CMPT_MASK |
                                       TI_C54X_ST1_ASM_MASK);
    state->pmst_known_mask = (uint16_t)(TI_C54X_PMST_IPTR_MASK |
                                        TI_C54X_PMST_OVLY_MASK |
                                        TI_C54X_PMST_AVIS_MASK |
                                        TI_C54X_PMST_DROM_MASK |
                                        TI_C54X_PMST_CLKOFF_MASK);
    state->scalar_known_mask = (uint8_t)(TI_C54X_ARCH_STATE_KNOWN_PC |
                                         TI_C54X_ARCH_STATE_KNOWN_IFR |
                                         TI_C54X_ARCH_STATE_KNOWN_RC);
    state->ar_known_mask = UINT8_C(0);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_ar(const ti_c54x_arch_state *state,
                           uint8_t index,
                           uint16_t *value)
{
    if (state == NULL || value == NULL || index >= 8U) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->ar_known_mask & ar_known_bit(index)) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->ar[index];
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_ar(ti_c54x_arch_state *state,
                            uint8_t index,
                            uint16_t value)
{
    if (state == NULL || index >= 8U) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->ar[index] = value;
    state->ar_known_mask = (uint8_t)(state->ar_known_mask | ar_known_bit(index));
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_sp(const ti_c54x_arch_state *state,
                           uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_SP) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->sp;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_sp(ti_c54x_arch_state *state,
                            uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->sp = value;
    state->scalar_known_mask =
        (uint8_t)(state->scalar_known_mask | TI_C54X_ARCH_STATE_KNOWN_SP);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_arp(const ti_c54x_arch_state *state,
                            uint8_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->st0_known_mask & TI_C54X_ST0_ARP_MASK) !=
        TI_C54X_ST0_ARP_MASK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint8_t)((state->st0 & TI_C54X_ST0_ARP_MASK) >>
                       TI_C54X_ST0_ARP_SHIFT);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_arp(ti_c54x_arch_state *state,
                             uint8_t value)
{
    if (state == NULL || value > UINT8_C(7)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->st0 = (uint16_t)((state->st0 & (uint16_t)~TI_C54X_ST0_ARP_MASK) |
                            ((uint16_t)value << TI_C54X_ST0_ARP_SHIFT));
    state->st0_known_mask =
        (uint16_t)(state->st0_known_mask | TI_C54X_ST0_ARP_MASK);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_dp(const ti_c54x_arch_state *state,
                           uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->st0_known_mask & TI_C54X_ST0_DP_MASK) !=
        TI_C54X_ST0_DP_MASK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint16_t)(state->st0 & TI_C54X_ST0_DP_MASK);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_dp(ti_c54x_arch_state *state,
                            uint16_t value)
{
    if (state == NULL || value > TI_C54X_ST0_DP_MASK) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->st0 = (uint16_t)((state->st0 & (uint16_t)~TI_C54X_ST0_DP_MASK) |
                            value);
    state->st0_known_mask =
        (uint16_t)(state->st0_known_mask | TI_C54X_ST0_DP_MASK);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_cpl(const ti_c54x_arch_state *state,
                            uint8_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->st1_known_mask & TI_C54X_ST1_CPL_MASK) !=
        TI_C54X_ST1_CPL_MASK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint8_t)((state->st1 & TI_C54X_ST1_CPL_MASK) >>
                       TI_C54X_ST1_CPL_SHIFT);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_cpl(ti_c54x_arch_state *state,
                             uint8_t value)
{
    if (state == NULL || value > UINT8_C(1)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->st1 = (uint16_t)((state->st1 & (uint16_t)~TI_C54X_ST1_CPL_MASK) |
                            ((uint16_t)value << TI_C54X_ST1_CPL_SHIFT));
    state->st1_known_mask =
        (uint16_t)(state->st1_known_mask | TI_C54X_ST1_CPL_MASK);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_cmpt(const ti_c54x_arch_state *state,
                             uint8_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->st1_known_mask & TI_C54X_ST1_CMPT_MASK) !=
        TI_C54X_ST1_CMPT_MASK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint8_t)((state->st1 & TI_C54X_ST1_CMPT_MASK) >>
                       TI_C54X_ST1_CMPT_SHIFT);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_cmpt(ti_c54x_arch_state *state,
                              uint8_t value)
{
    if (state == NULL || value > UINT8_C(1)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->st1 = (uint16_t)((state->st1 & (uint16_t)~TI_C54X_ST1_CMPT_MASK) |
                            ((uint16_t)value << TI_C54X_ST1_CMPT_SHIFT));
    state->st1_known_mask =
        (uint16_t)(state->st1_known_mask | TI_C54X_ST1_CMPT_MASK);
    return TI_C54X_ARCH_STATE_OK;
}
