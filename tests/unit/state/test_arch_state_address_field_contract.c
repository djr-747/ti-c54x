/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for checked ARP/DP/CPL/CMPT field
 * access, output-preserving unknown/invalid reads, rejected-call nonmutation,
 * knownness ownership, and cross-field isolation.
 * Does not own: TI reset conformance, CPU reset integration, effect
 * application, bus transfers, opcode semantics, or public ABI behavior.
 */

#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void test_unknown_reads_preserve_outputs(void)
{
    ti_c54x_arch_state state = reset_state();
    uint8_t value = UINT8_C(0xA5);
    uint16_t dp_value = UINT16_C(0xA5A5);

    /* Leave two ARP bits known and withhold one: the whole field is required. */
    state.status.st0_known_mask = UINT16_C(0xDFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT8_C(0xA5));

    state.status.st1_known_mask = UINT16_C(0xFFDF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT8_C(0xA5));

    state.status.st0_known_mask = UINT16_C(0xFE00);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(&state, &dp_value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(dp_value, UINT16_C(0xA5A5));

    state.status.st1_known_mask = UINT16_C(0xBFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT8_C(0xA5));
}

static void test_null_reads_preserve_outputs(void)
{
    ti_c54x_arch_state state = reset_state();
    uint8_t value = UINT8_C(0x5A);
    uint16_t dp_value = UINT16_C(0x5AA5);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT8_C(0x5A));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT8_C(0x5A));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(NULL, &dp_value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(dp_value, UINT16_C(0x5AA5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT8_C(0x5A));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void test_invalid_writes_are_bytewise_unchanged(void)
{
    ti_c54x_arch_state state = reset_state();
    const ti_c54x_arch_state before = state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(8)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(255)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(2)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(255)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&state, UINT16_C(0x0200)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&state, UINT16_C(0xFFFF)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(2)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(255)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(NULL, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(NULL, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(NULL, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(NULL, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void test_writes_preserve_unrelated_raw_and_known_bits(void)
{
    ti_c54x_arch_state state = reset_state();
    const uint16_t st0_raw = UINT16_C(0x135B);
    const uint16_t st1_raw = UINT16_C(0xA5DF);
    const uint16_t st0_known = UINT16_C(0x1357);
    const uint16_t st1_known = UINT16_C(0xA5A5);
    const uint16_t after_arp_st0 =
        (uint16_t)((st0_raw & UINT16_C(0x1FFF)) | UINT16_C(0xA000));
    const uint16_t after_arp_st0_known =
        (uint16_t)((st0_known & UINT16_C(0x1FFF)) | UINT16_C(0xE000));

    state.status.st0 = st0_raw;
    state.status.st1 = st1_raw;
    state.status.st0_known_mask = st0_known;
    state.status.st1_known_mask = st1_known;
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(5)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(state.status.st0,
                           (uint16_t)((st0_raw & UINT16_C(0x1FFF)) |
                                      UINT16_C(0xA000)));
    TI_C54X_TEST_CHECK_U64(state.status.st1, st1_raw);
    TI_C54X_TEST_CHECK_U64(state.status.st0_known_mask,
                           (uint16_t)((st0_known & UINT16_C(0x1FFF)) |
                                      UINT16_C(0xE000)));
    TI_C54X_TEST_CHECK_U64(state.status.st1_known_mask, st1_known);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&state, UINT16_C(0x01A5)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(
        state.status.st0,
        (uint16_t)((after_arp_st0 & UINT16_C(0xFE00)) | UINT16_C(0x01A5)));
    TI_C54X_TEST_CHECK_U64(state.status.st1, st1_raw);
    TI_C54X_TEST_CHECK_U64(
        state.status.st0_known_mask,
        (uint16_t)((after_arp_st0_known & UINT16_C(0xFE00)) |
                   TI_C54X_ST0_DP_MASK));
    TI_C54X_TEST_CHECK_U64(state.status.st1_known_mask,
                           (uint16_t)((st1_known & UINT16_C(0xFFDF)) |
                                      UINT16_C(0x0020)));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(state.status.st0,
                           (uint16_t)((after_arp_st0 & UINT16_C(0xFE00)) |
                                      UINT16_C(0x01A5)));
    TI_C54X_TEST_CHECK_U64(state.status.st1,
                           (uint16_t)((st1_raw & UINT16_C(0xFFDF)) |
                                      UINT16_C(0x0020)));
    TI_C54X_TEST_CHECK_U64(state.status.st0_known_mask,
                           (uint16_t)((after_arp_st0_known & UINT16_C(0xFE00)) |
                                      TI_C54X_ST0_DP_MASK));
    TI_C54X_TEST_CHECK_U64(state.status.st1_known_mask,
                           (uint16_t)((st1_known & UINT16_C(0xFFDF)) |
                                      UINT16_C(0x0020)));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(state.status.st0,
                           (uint16_t)((after_arp_st0 & UINT16_C(0xFE00)) |
                                      UINT16_C(0x01A5)));
    TI_C54X_TEST_CHECK_U64(state.status.st1,
                           (uint16_t)((st1_raw & UINT16_C(0xBFFF)) |
                                      UINT16_C(0x4020)));
    TI_C54X_TEST_CHECK_U64(state.status.st0_known_mask,
                           (uint16_t)((after_arp_st0_known & UINT16_C(0xFE00)) |
                                      TI_C54X_ST0_DP_MASK));
    TI_C54X_TEST_CHECK_U64(state.status.st1_known_mask,
                           (uint16_t)((st1_known & UINT16_C(0xBFFF)) |
                                      UINT16_C(0x4020)));
}

static void test_repeated_values_and_cross_field_reads(void)
{
    ti_c54x_arch_state state = reset_state();
    uint8_t arp = UINT8_C(0);
    uint16_t dp = UINT16_C(0);
    uint8_t cpl = UINT8_C(0);
    uint8_t cmpt = UINT8_C(0);

    state.status.st0 = UINT16_C(0x0000);
    state.status.st1 = UINT16_C(0x0000);
    state.status.st0_known_mask = UINT16_C(0x0000);
    state.status.st1_known_mask = UINT16_C(0x0000);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(3)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(3)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &arp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(arp, UINT8_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(&state, &dp) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(&state, &cpl) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&state, UINT16_C(3)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&state, UINT16_C(3)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(&state, &dp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(dp, UINT16_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, &cmpt) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, &cmpt) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(cmpt, UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(&state, &cpl) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(cpl, UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &arp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(arp, UINT8_C(3));
}

int main(void)
{
    test_unknown_reads_preserve_outputs();
    test_null_reads_preserve_outputs();
    test_invalid_writes_are_bytewise_unchanged();
    test_writes_preserve_unrelated_raw_and_known_bits();
    test_repeated_values_and_cross_field_reads();
    puts("arch_state_address_field_contract: ok");
    return 0;
}
