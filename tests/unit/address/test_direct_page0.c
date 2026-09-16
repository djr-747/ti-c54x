/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for the DP-independent page-0
 * direct concatenation helper and checked WORD16 planner over its
 * complete dma7 domain: the planned address is the zero-extended
 * dma byte with no DP, CPL, CMPT, or ARP dependence.
 * Does not own: CPU/register mutation, bus mapping, invalid-input
 * behavior, opcode decoding, or test harness implementation.
 */

#include "address/direct_page0.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_address(uint8_t dma_value)
{
    const ti_c54x_direct_page0_dma7 dma = {.bits = dma_value};
    const uint8_t original_dma = dma.bits;
    const ti_c54x_data_word_address first =
        ti_c54x_direct_page0_address_word(dma);
    const ti_c54x_data_word_address second =
        ti_c54x_direct_page0_address_word(dma);

    TI_C54X_TEST_CHECK_U64(first, dma_value);
    TI_C54X_TEST_CHECK_U64(second, dma_value);
    TI_C54X_TEST_CHECK(dma.bits == original_dma);
}

static void
check_planned(uint8_t dma_value)
{
    const ti_c54x_direct_page0_dma7 dma = {.bits = dma_value};
    const ti_c54x_address_result result = ti_c54x_direct_page0_word(dma);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, dma_value);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.direct_page0.dma7,
                           dma_value);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, UINT16_C(0));
}

static void
test_explicit_corners(void)
{
    check_address(UINT8_C(0));
    check_address(UINT8_C(0x1F));
    check_address(UINT8_C(0x20));
    check_address(UINT8_C(127));
    check_planned(UINT8_C(0));
    check_planned(UINT8_C(0x1F));
    check_planned(UINT8_C(0x20));
    check_planned(UINT8_C(127));
}

static void
test_complete_valid_domain(void)
{
    unsigned dma_value;

    for (dma_value = 0U; dma_value <= 127U; ++dma_value) {
        check_address((uint8_t)dma_value);
        check_planned((uint8_t)dma_value);
    }
}

static void
test_out_of_range_rejects(void)
{
    const ti_c54x_direct_page0_dma7 dma = {.bits = UINT8_C(0x80)};
    const ti_c54x_address_result result = ti_c54x_direct_page0_word(dma);

    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_explicit_corners();
    test_complete_valid_domain();
    test_out_of_range_rejects();
    return 0;
}
