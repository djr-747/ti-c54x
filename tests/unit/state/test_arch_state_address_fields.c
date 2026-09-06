/*
 * Module contract
 *
 * Owns: executable COMMON_TI proofs for the documented ARP/DP/CPL/CMPT reset
 * values, field positions, widths, and valid encode/decode domains.
 * Does not own: project policy for rejected calls, CPU reset integration,
 * effect application, bus transfers, opcode semantics, or public ABI behavior.
 */

#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static ti_c54x_arch_state reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void test_documented_reset_fields_are_zero(void)
{
    ti_c54x_arch_state state = reset_state();
    uint8_t arp = UINT8_C(0xA5);
    uint16_t dp = UINT16_C(0xA5A5);
    uint8_t cpl = UINT8_C(0xA5);
    uint8_t cmpt = UINT8_C(0xA5);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &arp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(arp, UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(&state, &dp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(dp, UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(&state, &cpl) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(cpl, UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, &cmpt) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(cmpt, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_ARP_SHIFT, UINT8_C(13));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_DP_MASK, UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_CPL_SHIFT, UINT8_C(14));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_CPL_MASK, UINT16_C(0x4000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_CMPT_SHIFT, UINT8_C(5));
}

static void test_arp_valid_values_encode_at_bits_15_to_13(void)
{
    const uint16_t surrounding = UINT16_C(0x135B);
    const uint16_t surrounding_raw = (uint16_t)(surrounding & UINT16_C(0x1FFF));

    for (uint8_t arp = UINT8_C(0); arp <= UINT8_C(7); ++arp) {
        ti_c54x_arch_state state = reset_state();
        uint8_t decoded = UINT8_C(0xA5);

        state.status.st0 = surrounding_raw;
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, arp) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(
            state.status.st0,
            (uint16_t)(surrounding_raw |
                       ((uint16_t)arp << 13)));
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &decoded) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(decoded, arp);
    }
}

static void test_cmpt_valid_values_encode_at_bit_5(void)
{
    const uint16_t surrounding = UINT16_C(0xA5DF);
    const uint16_t surrounding_raw = (uint16_t)(surrounding & UINT16_C(0xFFDF));

    for (uint8_t cmpt = UINT8_C(0); cmpt <= UINT8_C(1); ++cmpt) {
        ti_c54x_arch_state state = reset_state();
        uint8_t decoded = UINT8_C(0xA5);

        state.status.st1 = surrounding_raw;
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, cmpt) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(
            state.status.st1,
            (uint16_t)(surrounding_raw |
                       ((uint16_t)cmpt << 5)));
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, &decoded) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(decoded, cmpt);
    }
}

static void test_dp_valid_values_encode_at_bits_8_to_0(void)
{
    const uint16_t surrounding = UINT16_C(0x135B);
    const uint16_t surrounding_raw =
        (uint16_t)(surrounding & (uint16_t)~TI_C54X_ST0_DP_MASK);

    for (uint16_t dp = UINT16_C(0); dp <= TI_C54X_ST0_DP_MASK; ++dp) {
        ti_c54x_arch_state state = reset_state();
        uint16_t decoded = UINT16_C(0xA5A5);

        state.status.st0 = surrounding_raw;
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&state, dp) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(state.status.st0,
                               (uint16_t)(surrounding_raw | dp));
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_dp(&state, &decoded) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(decoded, dp);
    }
}

static void test_cpl_valid_values_encode_at_bit_14(void)
{
    const uint16_t surrounding = UINT16_C(0xA5DF);
    const uint16_t surrounding_raw =
        (uint16_t)(surrounding & (uint16_t)~TI_C54X_ST1_CPL_MASK);

    for (uint8_t cpl = UINT8_C(0); cpl <= UINT8_C(1); ++cpl) {
        ti_c54x_arch_state state = reset_state();
        uint8_t decoded = UINT8_C(0xA5);

        state.status.st1 = surrounding_raw;
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, cpl) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(
            state.status.st1,
            (uint16_t)(surrounding_raw |
                       ((uint16_t)cpl << TI_C54X_ST1_CPL_SHIFT)));
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cpl(&state, &decoded) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(decoded, cpl);
    }
}

int main(void)
{
    test_documented_reset_fields_are_zero();
    test_arp_valid_values_encode_at_bits_15_to_13();
    test_dp_valid_values_encode_at_bits_8_to_0();
    test_cpl_valid_values_encode_at_bit_14();
    test_cmpt_valid_values_encode_at_bit_5();
    puts("arch_state_address_fields: ok");
    return 0;
}
