/*
 * Module contract
 *
 * Owns: checked, field-local mutation and knownness-aware reads for the
 * private status-register bank.
 * Does not own: CPU lifecycle, opcode semantics, effect sequencing, buses,
 * timing, diagnostics, or public persistence formats.
 */

#include "registers.h"

#include <stddef.h>
#include <string.h>

static ti_c54x_status_register_field_result field_result(
    ti_c54x_status_register_status status,
    uint16_t raw_value,
    bool known)
{
    ti_c54x_status_register_field_result result = {status, raw_value, known};

    return result;
}

static ti_c54x_status_register_field_result read_field(
    const ti_c54x_status_register_state *state,
    uint16_t raw_register,
    uint16_t known_mask,
    uint16_t field_mask,
    uint8_t shift)
{
    if (state == NULL) {
        return field_result(TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT, 0U,
                            false);
    }
    if ((known_mask & field_mask) != field_mask) {
        return field_result(TI_C54X_STATUS_REGISTER_UNKNOWN, 0U, false);
    }
    return field_result(TI_C54X_STATUS_REGISTER_OK,
                        (uint16_t)((raw_register & field_mask) >> shift), true);
}

static ti_c54x_status_register_status write_field(
    uint16_t *raw_register,
    uint16_t *known_mask,
    uint16_t field_mask,
    uint8_t shift,
    uint16_t raw_value,
    uint16_t max_value)
{
    if (raw_register == NULL || known_mask == NULL || raw_value > max_value) {
        return TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT;
    }
    *raw_register = (uint16_t)((*raw_register & (uint16_t)~field_mask) |
                               (uint16_t)(raw_value << shift));
    *known_mask = (uint16_t)(*known_mask | field_mask);
    return TI_C54X_STATUS_REGISTER_OK;
}

ti_c54x_status_register_status
ti_c54x_status_register_reset_common(ti_c54x_status_register_state *state)
{
    if (state == NULL) {
        return TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT;
    }
    memset(state, 0, sizeof(*state));
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
    return TI_C54X_STATUS_REGISTER_OK;
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_dp(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st0,
                      state == NULL ? 0U : state->st0_known_mask,
                      TI_C54X_ST0_DP_MASK, 0U);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_asm(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st1,
                      state == NULL ? 0U : state->st1_known_mask,
                      TI_C54X_ST1_ASM_MASK, 0U);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_arp(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st0,
                      state == NULL ? 0U : state->st0_known_mask,
                      TI_C54X_ST0_ARP_MASK, TI_C54X_ST0_ARP_SHIFT);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_tc(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st0,
                      state == NULL ? 0U : state->st0_known_mask,
                      TI_C54X_ST0_TC_MASK, TI_C54X_ST0_TC_SHIFT);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_cpl(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st1,
                      state == NULL ? 0U : state->st1_known_mask,
                      TI_C54X_ST1_CPL_MASK, TI_C54X_ST1_CPL_SHIFT);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_cmpt(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st1,
                      state == NULL ? 0U : state->st1_known_mask,
                      TI_C54X_ST1_CMPT_MASK, TI_C54X_ST1_CMPT_SHIFT);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_sxm(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st1,
                      state == NULL ? 0U : state->st1_known_mask,
                      TI_C54X_ST1_SXM_MASK, TI_C54X_ST1_SXM_SHIFT);
}

ti_c54x_status_register_field_result
ti_c54x_status_register_read_c(const ti_c54x_status_register_state *state)
{
    return read_field(state, state == NULL ? 0U : state->st0,
                      state == NULL ? 0U : state->st0_known_mask,
                      TI_C54X_ST0_C_MASK, TI_C54X_ST0_C_SHIFT);
}

ti_c54x_status_register_status
ti_c54x_status_register_write_dp(ti_c54x_status_register_state *state,
                                  uint16_t raw_value)
{
    return state == NULL
               ? TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT
               : write_field(&state->st0, &state->st0_known_mask,
                             TI_C54X_ST0_DP_MASK, 0U, raw_value,
                             TI_C54X_ST0_DP_MASK);
}

ti_c54x_status_register_status
ti_c54x_status_register_write_asm(ti_c54x_status_register_state *state,
                                   uint16_t raw_value)
{
    return state == NULL
               ? TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT
               : write_field(&state->st1, &state->st1_known_mask,
                             TI_C54X_ST1_ASM_MASK, 0U, raw_value,
                             TI_C54X_ST1_ASM_MASK);
}

ti_c54x_status_register_status
ti_c54x_status_register_write_arp(ti_c54x_status_register_state *state,
                                   uint8_t raw_value)
{
    return state == NULL
               ? TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT
               : write_field(&state->st0, &state->st0_known_mask,
                             TI_C54X_ST0_ARP_MASK, TI_C54X_ST0_ARP_SHIFT,
                             raw_value, UINT8_C(7));
}

ti_c54x_status_register_status
ti_c54x_status_register_write_tc(ti_c54x_status_register_state *state,
                                  uint8_t raw_value)
{
    if (state == NULL || raw_value > UINT8_C(1)) {
        return TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT;
    }
    return write_field(&state->st0, &state->st0_known_mask,
                       TI_C54X_ST0_TC_MASK, TI_C54X_ST0_TC_SHIFT, raw_value,
                       UINT8_C(1));
}

ti_c54x_status_register_status
ti_c54x_status_register_write_cpl(ti_c54x_status_register_state *state,
                                   uint8_t raw_value)
{
    return state == NULL
               ? TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT
               : write_field(&state->st1, &state->st1_known_mask,
                             TI_C54X_ST1_CPL_MASK, TI_C54X_ST1_CPL_SHIFT,
                             raw_value, UINT8_C(1));
}

ti_c54x_status_register_status
ti_c54x_status_register_write_cmpt(ti_c54x_status_register_state *state,
                                    uint8_t raw_value)
{
    return state == NULL
               ? TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT
               : write_field(&state->st1, &state->st1_known_mask,
                             TI_C54X_ST1_CMPT_MASK, TI_C54X_ST1_CMPT_SHIFT,
                             raw_value, UINT8_C(1));
}

ti_c54x_status_register_status
ti_c54x_status_register_write_c(ti_c54x_status_register_state *state,
                                 uint8_t raw_value)
{
    if (state == NULL || raw_value > UINT8_C(1)) {
        return TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT;
    }
    return write_field(&state->st0, &state->st0_known_mask,
                       TI_C54X_ST0_C_MASK, TI_C54X_ST0_C_SHIFT, raw_value,
                       UINT8_C(1));
}

ti_c54x_status_register_status
ti_c54x_status_register_write_bit(ti_c54x_status_register_state *state,
                                   ti_c54x_status_register_target target,
                                   uint8_t bit_index,
                                   uint8_t raw_value)
{
    uint16_t *raw_register;
    uint16_t *known_mask;
    uint16_t bit_mask;

    if (state == NULL || bit_index >= UINT8_C(16) || raw_value > UINT8_C(1)) {
        return TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT;
    }
    bit_mask = (uint16_t)(UINT32_C(1) << bit_index);

    switch (target) {
    case TI_C54X_STATUS_REGISTER_TARGET_ST0:
        raw_register = &state->st0;
        known_mask = &state->st0_known_mask;
        break;
    case TI_C54X_STATUS_REGISTER_TARGET_ST1:
        raw_register = &state->st1;
        known_mask = &state->st1_known_mask;
        break;
    case TI_C54X_STATUS_REGISTER_TARGET_INVALID:
    default:
        return TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT;
    }

    if (raw_value == UINT8_C(0)) {
        *raw_register = (uint16_t)(*raw_register & (uint16_t)~bit_mask);
    } else {
        *raw_register = (uint16_t)(*raw_register | bit_mask);
    }
    *known_mask = (uint16_t)(*known_mask | bit_mask);
    return TI_C54X_STATUS_REGISTER_OK;
}
