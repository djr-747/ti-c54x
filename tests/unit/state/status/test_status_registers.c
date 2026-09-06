/*
 * Module contract
 *
 * Owns: executable proof for the private status-register bank reset,
 * knownness, field widths, and field-preserving writes.
 * Does not own: CPU lifecycle, opcode semantics, effect application, bus
 * transfers, timing, diagnostics, or public persistence formats.
 */

#include "state/status/registers.h"
#include "state/register_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void test_reset_and_unknown_reads(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result result;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK_U64(state.st0, UINT16_C(0x1800));
    TI_C54X_TEST_CHECK_U64(state.st1, UINT16_C(0x2900));
    TI_C54X_TEST_CHECK_U64(state.pmst, UINT16_C(0xff80));

    state.st0_known_mask = UINT16_C(0);
    result = ti_c54x_status_register_read_dp(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!result.known);
}

static void test_exhaustive_field_writes_preserve_surrounding_bits(void)
{
    ti_c54x_status_register_state state;
    uint16_t before_st0;
    uint16_t before_st1;
    uint16_t before_st0_known;
    uint16_t before_st1_known;
    uint16_t value;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);

    before_st0 = (uint16_t)(UINT16_C(0xA5A5) &
                            (uint16_t)~TI_C54X_ST0_DP_MASK);
    before_st0_known = UINT16_C(0x1357);
    state.st0 = before_st0;
    state.st0_known_mask = before_st0_known;
    for (value = UINT16_C(0); value <= TI_C54X_ST0_DP_MASK; ++value) {
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_dp(&state, value) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK_U64(state.st0,
                               (uint16_t)(before_st0 | value));
        TI_C54X_TEST_CHECK_U64(
            state.st0_known_mask,
            (uint16_t)((before_st0_known &
                        (uint16_t)~TI_C54X_ST0_DP_MASK) |
                       TI_C54X_ST0_DP_MASK));
    }

    before_st1 = (uint16_t)(UINT16_C(0xA5A5) &
                            (uint16_t)~TI_C54X_ST1_ASM_MASK);
    before_st1_known = UINT16_C(0x2468);
    state.st1 = before_st1;
    state.st1_known_mask = before_st1_known;
    for (value = UINT16_C(0); value <= UINT16_C(0x1f); ++value) {
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_asm(&state, value) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK_U64(state.st1,
                               (uint16_t)(before_st1 | value));
        TI_C54X_TEST_CHECK_U64(
            state.st1_known_mask,
            (uint16_t)((before_st1_known &
                        (uint16_t)~TI_C54X_ST1_ASM_MASK) |
                       TI_C54X_ST1_ASM_MASK));
    }

    for (value = UINT16_C(0); value <= UINT16_C(7); ++value) {
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_arp(
                               &state, (uint8_t)value) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK_U64(
            state.st0 & TI_C54X_ST0_ARP_MASK,
            (uint16_t)(value << TI_C54X_ST0_ARP_SHIFT));
    }
}

static void test_cpl_cmpt_and_invalid_writes(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result c;
    uint16_t before_st0;
    uint16_t before_known;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_cpl(&state, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_cmpt(&state, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK((state.st1 & TI_C54X_ST1_CPL_MASK) != 0U);
    TI_C54X_TEST_CHECK((state.st1 & TI_C54X_ST1_CMPT_MASK) != 0U);
    c = ti_c54x_status_register_read_c(&state);
    TI_C54X_TEST_CHECK(c.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(c.raw_value == 1U);
    before_st0 = state.st0;
    before_known = state.st0_known_mask;
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&state, 0U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_c(&state).raw_value == 0U);
    TI_C54X_TEST_CHECK((state.st0 & (uint16_t)~TI_C54X_ST0_C_MASK) ==
                       (before_st0 & (uint16_t)~TI_C54X_ST0_C_MASK));
    TI_C54X_TEST_CHECK((state.st0_known_mask & (uint16_t)~TI_C54X_ST0_C_MASK) ==
                       (before_known & (uint16_t)~TI_C54X_ST0_C_MASK));
    before_st0 = state.st0;
    before_known = state.st0_known_mask;
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&state, 2U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(state.st0 == before_st0);
    TI_C54X_TEST_CHECK(state.st0_known_mask == before_known);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_c(NULL).status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(NULL, 0U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_dp(&state, 0x200U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_asm(&state, 0x20U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
}

int main(void)
{
    test_reset_and_unknown_reads();
    test_exhaustive_field_writes_preserve_surrounding_bits();
    test_cpl_cmpt_and_invalid_writes();
    return 0;
}
