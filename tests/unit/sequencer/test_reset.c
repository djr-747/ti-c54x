/*
 * Module contract
 *
 * Owns: COMMON_TI executable proof of the documented reset subset
 * (SPRU131G §6.9): PC=FF80h, IPTR=1FFh, IFR=0, RC=0, ARP=0, ASM=0,
 * AVIS=0, BRAF=0, C16=0, C=1, CLKOFF=0, CMPT=0, CPL=0, DP=0, DROM=0,
 * FRCT=0, HM=0, INTM=1, OVA=0, OVB=0, OVM=0, OVLY=0, SXM=1, TC=1,
 * XF=1, each read back known through the checked accessors after the
 * common reset seed. SP, AR0-AR7, and BK stay architecturally unknown.
 * Does not own: state storage, reset seeding, CPU lifecycle, configured
 * bootstrap reset, XPC, pin-sampled MP/MC, family-dependent SMUL/SST,
 * reserved ST1 bit 10, opcode semantics, or bus transfers.
 */

#include "../../../src/state/arch_state.h"
#include "../../../src/state/block_size_register.h"
#include "../../../src/state/status/registers.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static void check_field(ti_c54x_status_register_field_result result,
                        uint16_t expected)
{
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(result.known);
    TI_C54X_TEST_CHECK_U64(result.raw_value, expected);
}

static void test_documented_subset_is_known(void)
{
    ti_c54x_arch_state state;

    (void)memset(&state, 0xA5, sizeof(state));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);

    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK_U64(state.ifr, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(state.rc, UINT16_C(0));
    TI_C54X_TEST_CHECK(
        (state.scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_PC) != 0U);
    TI_C54X_TEST_CHECK(
        (state.scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_IFR) != 0U);
    TI_C54X_TEST_CHECK(
        (state.scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_RC) != 0U);

    check_field(ti_c54x_status_register_read_arp(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_tc(&state.status),
                UINT16_C(1));
    check_field(ti_c54x_status_register_read_c(&state.status),
                UINT16_C(1));
    check_field(ti_c54x_status_register_read_ova(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_ovb(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_dp(&state.status),
                UINT16_C(0));

    check_field(ti_c54x_status_register_read_braf(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_cpl(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_xf(&state.status),
                UINT16_C(1));
    check_field(ti_c54x_status_register_read_hm(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_intm(&state.status),
                UINT16_C(1));
    check_field(ti_c54x_status_register_read_ovm(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_sxm(&state.status),
                UINT16_C(1));
    check_field(ti_c54x_status_register_read_c16(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_frct(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_cmpt(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_asm(&state.status),
                UINT16_C(0));

    check_field(ti_c54x_status_register_read_iptr(&state.status),
                UINT16_C(0x01FF));
    check_field(ti_c54x_status_register_read_ovly(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_avis(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_drom(&state.status),
                UINT16_C(0));
    check_field(ti_c54x_status_register_read_clkoff(&state.status),
                UINT16_C(0));
}

static void test_unspecified_registers_stay_unknown(void)
{
    ti_c54x_arch_state state;
    uint16_t ignored = UINT16_C(0xFFFF);
    uint8_t index;

    (void)memset(&state, 0, sizeof(state));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);

    TI_C54X_TEST_CHECK(
        (state.scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_SP) == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&state, &ignored) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &ignored) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    for (index = UINT8_C(0); index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, index,
                                                     &ignored) ==
                           TI_C54X_ARCH_STATE_UNKNOWN);
    }
}

static void test_null_reset_rejects(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

int main(void)
{
    test_documented_subset_is_known();
    test_unspecified_registers_stay_unknown();
    test_null_reset_rejects();
    return 0;
}
