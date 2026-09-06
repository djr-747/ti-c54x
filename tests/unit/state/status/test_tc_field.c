/*
 * Module contract
 *
 * Owns: executable proof for the private ST0 TC field accessor, including
 * reset knownness, canonical unknown reads, checked writes, and field-local
 * preservation.
 * Does not own: status-register storage, CPU lifecycle, opcode semantics,
 * effect sequencing, buses, timing, diagnostics, or public persistence.
 */

#include "state/status/registers.h"
#include "state/register_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static void test_null_arguments_and_reset_knownness(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result result;

    result = ti_c54x_status_register_read_tc(NULL);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!result.known);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(NULL, UINT8_C(0)) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_status_register_read_tc(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(1));
    TI_C54X_TEST_CHECK(result.known);
}

static void test_unknown_read_is_canonical(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result result;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    state.st0 = (uint16_t)(state.st0 | TI_C54X_ST0_TC_MASK);
    state.st0_known_mask = (uint16_t)(state.st0_known_mask &
                                      (uint16_t)~TI_C54X_ST0_TC_MASK);

    result = ti_c54x_status_register_read_tc(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!result.known);
}

static void test_valid_writes_change_only_tc(void)
{
    const uint16_t unrelated_st0 =
        (uint16_t)(UINT16_C(0xA5A5) & (uint16_t)~TI_C54X_ST0_TC_MASK);
    const uint16_t unrelated_known =
        (uint16_t)(UINT16_C(0x1357) & (uint16_t)~TI_C54X_ST0_TC_MASK);
    ti_c54x_status_register_state before;
    ti_c54x_status_register_state after;
    uint8_t value;

    for (value = UINT8_C(0); value <= UINT8_C(1); ++value) {
        TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&before) ==
                           TI_C54X_STATUS_REGISTER_OK);
        before.st0 = unrelated_st0;
        before.st0_known_mask = unrelated_known;
        after = before;

        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&after, value) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK_U64(
            after.st0,
            (uint16_t)(unrelated_st0 |
                       (value == UINT8_C(0) ? UINT16_C(0)
                                            : TI_C54X_ST0_TC_MASK)));
        TI_C54X_TEST_CHECK_U64(
            after.st0_known_mask,
            (uint16_t)(unrelated_known | TI_C54X_ST0_TC_MASK));
        TI_C54X_TEST_CHECK(after.st1 == before.st1);
        TI_C54X_TEST_CHECK(after.pmst == before.pmst);
        TI_C54X_TEST_CHECK(after.st1_known_mask == before.st1_known_mask);
        TI_C54X_TEST_CHECK(after.pmst_known_mask == before.pmst_known_mask);
    }
}

static void test_invalid_write_is_byte_identical(void)
{
    ti_c54x_status_register_state before;
    ti_c54x_status_register_state after;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&before) ==
                       TI_C54X_STATUS_REGISTER_OK);
    before.st0 = UINT16_C(0xA5A5);
    before.st1 = UINT16_C(0x5AA5);
    before.pmst = UINT16_C(0xC33C);
    before.st0_known_mask = UINT16_C(0x1357);
    before.st1_known_mask = UINT16_C(0x2468);
    before.pmst_known_mask = UINT16_C(0x369C);
    after = before;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&after, UINT8_C(2)) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&before, &after, sizeof(before)) == 0);
}

int main(void)
{
    test_null_arguments_and_reset_knownness();
    test_unknown_read_is_canonical();
    test_valid_writes_change_only_tc();
    test_invalid_write_is_byte_identical();
    return 0;
}
