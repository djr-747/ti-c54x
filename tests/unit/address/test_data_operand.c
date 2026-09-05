/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for the evidenced long32 data-operand
 * parity pairing.
 * Does not own: planner mode arithmetic, state application, memory, bus
 * ordering, timing, or test harness implementation.
 */

#include "address/data_operand.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
test_long32_complete_domain(void)
{
    uint32_t first;

    for (first = UINT32_C(0); first <= UINT32_C(0xffff); ++first) {
        const uint16_t first_word = (uint16_t)first;
        const uint16_t expected_second =
            (first_word & UINT16_C(1)) == UINT16_C(0)
                ? (uint16_t)(first_word + UINT16_C(1))
                : (uint16_t)(first_word - UINT16_C(1));
        const ti_c54x_data_operand operand =
            ti_c54x_data_operand_long32(first_word);

        TI_C54X_TEST_CHECK(operand.width ==
                           TI_C54X_DATA_OPERAND_WIDTH_LONG32);
        TI_C54X_TEST_CHECK_U64(operand.first_word, first_word);
        TI_C54X_TEST_CHECK_U64(operand.second_word, expected_second);
    }
}

int
main(void)
{
    test_long32_complete_domain();
    puts("data_operand: ok");
    return 0;
}
