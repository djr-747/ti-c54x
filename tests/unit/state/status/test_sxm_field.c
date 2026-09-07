/*
 * Module contract
 *
 * Owns: executable proof for the private ST1 SXM field read, including reset
 * knownness, raw known values, canonical unknown results, NULL handling, and
 * field-local non-mutation.
 * Does not own: status-register storage, CPU lifecycle, opcode semantics,
 * effect sequencing, buses, timing, diagnostics, or any SXM write API.
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

    result = ti_c54x_status_register_read_sxm(NULL);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!result.known);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_status_register_read_sxm(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(1));
    TI_C54X_TEST_CHECK(result.known);
}

static void test_known_raw_values_preserve_status(void)
{
    ti_c54x_status_register_state before;
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result result;
    uint8_t value;

    for (value = UINT8_C(0); value <= UINT8_C(1); ++value) {
        TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                           TI_C54X_STATUS_REGISTER_OK);
        state.st1 = (uint16_t)(state.st1 &
                               (uint16_t)~TI_C54X_ST1_SXM_MASK);
        if (value != UINT8_C(0)) {
            state.st1 = (uint16_t)(state.st1 | TI_C54X_ST1_SXM_MASK);
        }
        before = state;

        result = ti_c54x_status_register_read_sxm(&state);
        TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK(result.raw_value == (uint16_t)value);
        TI_C54X_TEST_CHECK(result.known);
        TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);
    }
}

static void test_unknown_read_uses_canonical_result_and_preserves_status(void)
{
    ti_c54x_status_register_state before;
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result result;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    state.st1_known_mask = (uint16_t)(state.st1_known_mask &
                                      (uint16_t)~TI_C54X_ST1_SXM_MASK);
    state.st1 = (uint16_t)(state.st1 | TI_C54X_ST1_SXM_MASK);
    before = state;

    result = ti_c54x_status_register_read_sxm(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(result.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!result.known);
    TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);
}

int main(void)
{
    test_null_arguments_and_reset_knownness();
    test_known_raw_values_preserve_status();
    test_unknown_read_uses_canonical_result_and_preserves_status();
    return 0;
}
