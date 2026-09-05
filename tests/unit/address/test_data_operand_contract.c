/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for descriptor shape
 * validation and rejection of noncanonical values.
 * Does not own: TI evidence, planner mode semantics, state application,
 * memory, bus ordering, timing, or test harness implementation.
 */

#include "address/data_operand.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_validator(const ti_c54x_data_operand *operand, bool expected)
{
    const ti_c54x_data_operand before = *operand;

    TI_C54X_TEST_CHECK(ti_c54x_data_operand_is_valid(operand) == expected);
    TI_C54X_TEST_CHECK(memcmp(&before, operand, sizeof(before)) == 0);
}

static void
test_invalid_widths(void)
{
    ti_c54x_data_operand operand = {
        .width = TI_C54X_DATA_OPERAND_WIDTH_INVALID,
        .first_word = UINT16_C(0x1234),
        .second_word = UINT16_C(0x1235)
    };

    TI_C54X_TEST_CHECK(!ti_c54x_data_operand_is_valid(NULL));
    check_validator(&operand, false);
    operand.width = (ti_c54x_data_operand_width)-1;
    check_validator(&operand, false);
    operand.width = (ti_c54x_data_operand_width)3;
    check_validator(&operand, false);
}

static void
test_noncanonical_word_shape(void)
{
    ti_c54x_data_operand operand =
        ti_c54x_data_operand_word16(UINT16_C(0x1234));

    operand.second_word = UINT16_C(0x1235);
    check_validator(&operand, false);
}

static void
test_noncanonical_long_shapes(void)
{
    ti_c54x_data_operand operand =
        ti_c54x_data_operand_long32(UINT16_C(0x1234));

    operand.second_word = operand.first_word;
    check_validator(&operand, false);
    operand.second_word = UINT16_C(0x1236);
    check_validator(&operand, false);

    operand = ti_c54x_data_operand_long32(UINT16_C(0x1235));
    operand.second_word = UINT16_C(0x1236);
    check_validator(&operand, false);
}

static void
test_canonical_validator_complete_domain(void)
{
    uint32_t first;

    for (first = UINT32_C(0); first <= UINT32_C(0xffff); ++first) {
        const uint16_t first_word = (uint16_t)first;
        const uint16_t expected_second =
            (first_word & UINT16_C(1)) == UINT16_C(0)
                ? (uint16_t)(first_word + UINT16_C(1))
                : (uint16_t)(first_word - UINT16_C(1));
        const ti_c54x_data_operand word =
            ti_c54x_data_operand_word16(first_word);
        const ti_c54x_data_operand long32 =
            ti_c54x_data_operand_long32(first_word);

        TI_C54X_TEST_CHECK(word.width ==
                           TI_C54X_DATA_OPERAND_WIDTH_WORD16);
        TI_C54X_TEST_CHECK_U64(word.first_word, first_word);
        TI_C54X_TEST_CHECK_U64(word.second_word, UINT16_C(0));
        check_validator(&word, true);
        TI_C54X_TEST_CHECK(long32.width ==
                           TI_C54X_DATA_OPERAND_WIDTH_LONG32);
        TI_C54X_TEST_CHECK_U64(long32.first_word, first_word);
        TI_C54X_TEST_CHECK_U64(long32.second_word, expected_second);
        check_validator(&long32, true);
    }
}

int
main(void)
{
    test_invalid_widths();
    test_noncanonical_word_shape();
    test_noncanonical_long_shapes();
    test_canonical_validator_complete_domain();
    puts("data_operand_contract: ok");
    return 0;
}
