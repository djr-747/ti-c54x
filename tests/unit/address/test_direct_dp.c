/*
 * Module contract
 *
 * Owns: executable tests for the private CPL=0 direct-data address helper.
 * Does not own: address semantics, CPU/register mutation, bus mapping, test
 * harness implementation, or any invalid-input behavior.
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

int
main(void)
{
    test_explicit_corners();
    test_complete_valid_domain();
    puts("direct_dp: ok");
    return 0;
}
