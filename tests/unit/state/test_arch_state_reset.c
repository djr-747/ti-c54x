/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof of the private common reset seed.
 * Does not own: CPU reset integration, configured bootstrap behavior, effect
 * application, bus transfers, opcode semantics, or project knownness policy.
 */

#include "state/arch_state.h"
#include "state/register_layout.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void check_common_reset(const ti_c54x_arch_state *state)
{
    TI_C54X_TEST_CHECK_U64(state->pc_word, UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK_U64(state->ifr, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(state->rc, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(state->xpc, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(state->status.st0, UINT16_C(0x1800));
    TI_C54X_TEST_CHECK_U64(state->status.st1, UINT16_C(0x2900));
    TI_C54X_TEST_CHECK_U64(state->status.pmst, UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK_U64(state->status.st0_known_mask, UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK_U64(state->status.st1_known_mask, UINT16_C(0xFBFF));
    TI_C54X_TEST_CHECK_U64(state->status.pmst_known_mask, UINT16_C(0xFFBC));
    TI_C54X_TEST_CHECK_U64(state->scalar_known_mask, UINT8_C(0x47));
    TI_C54X_TEST_CHECK_U64(state->ar_known_mask, UINT8_C(0x00));
}

static void test_layout_masks_are_exact_and_disjoint(void)
{
    const uint16_t st0_fields[] = {
        UINT16_C(0xE000), UINT16_C(0x1000), UINT16_C(0x0800),
        UINT16_C(0x0400), UINT16_C(0x0200), UINT16_C(0x01FF)};
    const uint16_t st1_fields[] = {
        UINT16_C(0x8000), UINT16_C(0x4000), UINT16_C(0x2000),
        UINT16_C(0x1000), UINT16_C(0x0800), UINT16_C(0x0400),
        UINT16_C(0x0200), UINT16_C(0x0100), UINT16_C(0x0080),
        UINT16_C(0x0040), UINT16_C(0x0020), UINT16_C(0x001F)};
    const uint16_t pmst_fields[] = {
        UINT16_C(0xFF80), UINT16_C(0x0040), UINT16_C(0x0020),
        UINT16_C(0x0010), UINT16_C(0x0008), UINT16_C(0x0004),
        UINT16_C(0x0002), UINT16_C(0x0001)};
    uint16_t st0_union = UINT16_C(0);
    uint16_t st1_union = UINT16_C(0);
    uint16_t pmst_union = UINT16_C(0);

    for (size_t index = 0U; index < sizeof(st0_fields) / sizeof(st0_fields[0]);
         ++index) {
        st0_union = (uint16_t)(st0_union | st0_fields[index]);
        for (size_t other = index + 1U;
             other < sizeof(st0_fields) / sizeof(st0_fields[0]); ++other) {
            TI_C54X_TEST_CHECK((st0_fields[index] & st0_fields[other]) == 0U);
        }
    }
    for (size_t index = 0U; index < sizeof(st1_fields) / sizeof(st1_fields[0]);
         ++index) {
        st1_union = (uint16_t)(st1_union | st1_fields[index]);
        for (size_t other = index + 1U;
             other < sizeof(st1_fields) / sizeof(st1_fields[0]); ++other) {
            TI_C54X_TEST_CHECK((st1_fields[index] & st1_fields[other]) == 0U);
        }
    }
    for (size_t index = 0U; index < sizeof(pmst_fields) / sizeof(pmst_fields[0]);
         ++index) {
        pmst_union = (uint16_t)(pmst_union | pmst_fields[index]);
        for (size_t other = index + 1U;
             other < sizeof(pmst_fields) / sizeof(pmst_fields[0]); ++other) {
            TI_C54X_TEST_CHECK((pmst_fields[index] & pmst_fields[other]) == 0U);
        }
    }
    TI_C54X_TEST_CHECK_U64(st0_union, UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK_U64(st1_union, UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK_U64(pmst_union, UINT16_C(0xFFFF));

    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_ARP_MASK, UINT16_C(0xE000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_TC_MASK, UINT16_C(0x1000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_C_MASK, UINT16_C(0x0800));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_OVA_MASK, UINT16_C(0x0400));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_OVB_MASK, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST0_DP_MASK, UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_BRAF_MASK, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_CPL_MASK, UINT16_C(0x4000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_XF_MASK, UINT16_C(0x2000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_HM_MASK, UINT16_C(0x1000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_INTM_MASK, UINT16_C(0x0800));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_RESERVED10_MASK, UINT16_C(0x0400));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_OVM_MASK, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_SXM_MASK, UINT16_C(0x0100));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_C16_MASK, UINT16_C(0x0080));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_FRCT_MASK, UINT16_C(0x0040));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_CMPT_MASK, UINT16_C(0x0020));
    TI_C54X_TEST_CHECK_U64(TI_C54X_ST1_ASM_MASK, UINT16_C(0x001F));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_IPTR_MASK, UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_MP_MC_MASK, UINT16_C(0x0040));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_OVLY_MASK, UINT16_C(0x0020));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_AVIS_MASK, UINT16_C(0x0010));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_DROM_MASK, UINT16_C(0x0008));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_CLKOFF_MASK, UINT16_C(0x0004));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_SMUL_MASK, UINT16_C(0x0002));
    TI_C54X_TEST_CHECK_U64(TI_C54X_PMST_SST_MASK, UINT16_C(0x0001));
}

static void test_reset_overwrites_dirty_state(void)
{
    ti_c54x_arch_state state;

    (void)memset(&state, 0xA5, sizeof(state));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    check_common_reset(&state);
}

int main(void)
{
    test_layout_masks_are_exact_and_disjoint();
    test_reset_overwrites_dirty_state();
    puts("arch_state_reset: ok");
    return 0;
}
