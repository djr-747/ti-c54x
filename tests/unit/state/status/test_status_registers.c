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

#include <string.h>
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

static void test_pmst_sst_read_write_and_common_reset_unknownness(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_state before;
    ti_c54x_status_register_field_result result;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_status_register_read_sst(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!result.known);
    TI_C54X_TEST_CHECK((state.pmst_known_mask & TI_C54X_PMST_SST_MASK) == 0U);

    state.pmst = UINT16_C(0xA5A4);
    state.pmst_known_mask = UINT16_C(0x5A5A);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_sst(&state, 2U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_sst(&state, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK((state.pmst & TI_C54X_PMST_SST_MASK) != 0U);
    TI_C54X_TEST_CHECK((state.pmst_known_mask & TI_C54X_PMST_SST_MASK) != 0U);
    result = ti_c54x_status_register_read_sst(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(1));
    TI_C54X_TEST_CHECK(result.known);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_sst(&state, 0U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK((state.pmst & TI_C54X_PMST_SST_MASK) == 0U);
    TI_C54X_TEST_CHECK((state.pmst_known_mask & TI_C54X_PMST_SST_MASK) != 0U);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_sst(NULL).status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_sst(NULL, 0U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
}

static void test_checked_single_status_bit_writes(void)
{
    const ti_c54x_status_register_target targets[] = {
        TI_C54X_STATUS_REGISTER_TARGET_ST0,
        TI_C54X_STATUS_REGISTER_TARGET_ST1};

    for (size_t target_index = 0; target_index < 2U; ++target_index) {
        for (uint8_t bit_index = 0U; bit_index < 16U; ++bit_index) {
            for (uint8_t raw_value = 0U; raw_value <= 1U; ++raw_value) {
                ti_c54x_status_register_state state;
                uint16_t before_raw;
                uint16_t before_known;
                uint16_t expected_raw;
                uint16_t expected_known;
                const uint16_t bit_mask = (uint16_t)(UINT16_C(1) << bit_index);

                TI_C54X_TEST_CHECK(
                    ti_c54x_status_register_reset_common(&state) ==
                    TI_C54X_STATUS_REGISTER_OK);
                state.st0 = UINT16_C(0x1357);
                state.st1 = UINT16_C(0x2468);
                state.st0_known_mask = UINT16_C(0x5a5a);
                state.st1_known_mask = UINT16_C(0xa5a5);
                before_raw = target_index == 0U ? state.st0 : state.st1;
                before_known = target_index == 0U ? state.st0_known_mask
                                                   : state.st1_known_mask;
                expected_raw = raw_value == 0U
                                   ? (uint16_t)(before_raw & (uint16_t)~bit_mask)
                                   : (uint16_t)(before_raw | bit_mask);
                expected_known = (uint16_t)(before_known | bit_mask);

                TI_C54X_TEST_CHECK(ti_c54x_status_register_write_bit(
                                       &state, targets[target_index], bit_index,
                                       raw_value) ==
                                   TI_C54X_STATUS_REGISTER_OK);
                if (target_index == 0U) {
                    TI_C54X_TEST_CHECK_U64(state.st0, expected_raw);
                    TI_C54X_TEST_CHECK_U64(state.st0_known_mask, expected_known);
                    TI_C54X_TEST_CHECK_U64(state.st1, UINT16_C(0x2468));
                    TI_C54X_TEST_CHECK_U64(state.st1_known_mask,
                                           UINT16_C(0xa5a5));
                } else {
                    TI_C54X_TEST_CHECK_U64(state.st1, expected_raw);
                    TI_C54X_TEST_CHECK_U64(state.st1_known_mask, expected_known);
                    TI_C54X_TEST_CHECK_U64(state.st0, UINT16_C(0x1357));
                    TI_C54X_TEST_CHECK_U64(state.st0_known_mask,
                                           UINT16_C(0x5a5a));
                }
            }
        }
    }
}

static void test_checked_single_status_bit_rejections_are_nonmutating(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_state before;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    state.st0 = UINT16_C(0x1357);
    state.st1 = UINT16_C(0x2468);
    state.st0_known_mask = UINT16_C(0x5a5a);
    state.st1_known_mask = UINT16_C(0xa5a5);

    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_bit(
                           &state,
                           (ti_c54x_status_register_target)UINT8_C(0xff), 0U,
                           1U) == TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_bit(
                           &state, TI_C54X_STATUS_REGISTER_TARGET_ST0, 16U,
                           1U) == TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_bit(
                           &state, TI_C54X_STATUS_REGISTER_TARGET_ST1, 10U,
                           2U) == TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_bit(
                           NULL, TI_C54X_STATUS_REGISTER_TARGET_ST0, 0U,
                           1U) == TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
}

int main(void)
{
    test_reset_and_unknown_reads();
    test_exhaustive_field_writes_preserve_surrounding_bits();
    test_cpl_cmpt_and_invalid_writes();
    test_pmst_sst_read_write_and_common_reset_unknownness();
    test_checked_single_status_bit_writes();
    test_checked_single_status_bit_rejections_are_nonmutating();
    return 0;
}
