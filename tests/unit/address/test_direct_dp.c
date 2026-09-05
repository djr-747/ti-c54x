/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for the CPL=0 direct-data concatenation
 * helper and checked WORD16 planner over its complete DP9×dma7 domain.
 * Does not own: CPU/register mutation, bus mapping, invalid-input behavior,
 * opcode decoding, or test harness implementation.
 */

#include "address/direct_dp.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_pair(uint16_t dp_value, uint8_t dma_value)
{
    const ti_c54x_direct_dp9 dp = {.bits = dp_value};
    const ti_c54x_direct_dma7 dma = {.bits = dma_value};
    const uint16_t original_dp = dp.bits;
    const uint8_t original_dma = dma.bits;
    const uint32_t expected =
        (uint32_t)dp_value * UINT32_C(128) + (uint32_t)dma_value;
    const ti_c54x_data_word_address first =
        ti_c54x_direct_dp_address_word(dp, dma);
    const ti_c54x_data_word_address second =
        ti_c54x_direct_dp_address_word(dp, dma);

    TI_C54X_TEST_CHECK_U64(first, expected);
    TI_C54X_TEST_CHECK_U64(second, expected);
    TI_C54X_TEST_CHECK(dp.bits == original_dp);
    TI_C54X_TEST_CHECK(dma.bits == original_dma);
}

static void
check_planned_pair(uint16_t dp_value, uint8_t dma_value)
{
    const ti_c54x_direct_dp9 dp = {.bits = dp_value};
    const ti_c54x_direct_dma7 dma = {.bits = dma_value};
    const uint32_t expected =
        (uint32_t)dp_value * UINT32_C(128) + (uint32_t)dma_value;
    const ti_c54x_address_result result = ti_c54x_direct_dp_word(dp, dma);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, expected);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_DIRECT_DP);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.direct_dp.dp9,
                           dp_value);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.direct_dp.dma7,
                           dma_value);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0);
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
    check_pair(UINT16_C(0), UINT8_C(0));
    check_pair(UINT16_C(0), UINT8_C(127));
    check_pair(UINT16_C(511), UINT8_C(0));
    check_pair(UINT16_C(511), UINT8_C(127));
}

static void
test_complete_valid_domain(void)
{
    uint16_t dp_value;

    for (dp_value = UINT16_C(0); dp_value < UINT16_C(512); ++dp_value) {
        uint16_t dma_value;

        for (dma_value = UINT16_C(0); dma_value < UINT16_C(128);
             ++dma_value) {
            check_pair(dp_value, (uint8_t)dma_value);
        }
    }
}

static void
test_complete_planner_domain(void)
{
    uint16_t dp_value;

    for (dp_value = UINT16_C(0); dp_value < UINT16_C(512); ++dp_value) {
        uint16_t dma_value;

        for (dma_value = UINT16_C(0); dma_value < UINT16_C(128);
             ++dma_value) {
            check_planned_pair(dp_value, (uint8_t)dma_value);
        }
    }
}

int
main(void)
{
    test_explicit_corners();
    test_complete_valid_domain();
    test_complete_planner_domain();
    puts("direct_dp: ok");
    return 0;
}
