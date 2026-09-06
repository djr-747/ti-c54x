/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof of BK reset unknownness and
 * zero representation, caller-output preservation, NULL rejection, exact
 * writes, sole-field/knownness mutation, stale-state reset clearing, and
 * bytewise nonmutation on rejected calls.
 * Does not own: TI reset conformance, MMR routing, circular arithmetic, CPU
 * lifecycle, bus transfers, opcode semantics, or test-harness behavior.
 */

#include "state/arch_state.h"
#include "state/block_size_register.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state
reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void
test_reset_is_zero_but_unknown(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xA5A5);

    TI_C54X_TEST_CHECK_U64(state.bk, UINT16_C(0));
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_BK) == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
}

static void
test_unknown_and_null_reads_preserve_outputs(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0x5AA5);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x5AA5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x5AA5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void
test_write_changes_only_bk_and_its_knownness(void)
{
    ti_c54x_arch_state state = reset_state();
    const ti_c54x_arch_state before = state;
    ti_c54x_arch_state expected = before;
    uint16_t value = UINT16_C(0xA5A5);

    expected.bk = UINT16_C(0xA5A5);
    expected.scalar_known_mask = (uint8_t)(
        expected.scalar_known_mask | TI_C54X_ARCH_STATE_KNOWN_BK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
}

static void
test_exact_project_values_round_trip(void)
{
    static const uint16_t values[] = {
        UINT16_C(0), UINT16_C(1), UINT16_C(0xffff)
    };
    ti_c54x_arch_state state = reset_state();
    size_t index;

    for (index = 0U; index < sizeof(values) / sizeof(values[0]); ++index) {
        uint16_t actual = UINT16_C(0xBEEF);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(
                               &state, values[index]) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &actual) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(actual, values[index]);
    }
}

static void
test_reset_clears_stale_value_and_knownness(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xCAFE);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(0xffff)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(state.bk, UINT16_C(0));
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_BK) == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xCAFE));
}

static void
test_rejected_calls_are_bytewise_nonmutating(void)
{
    ti_c54x_arch_state state = reset_state();
    const ti_c54x_arch_state before = state;
    uint16_t value = UINT16_C(0x1234);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(NULL, UINT16_C(0x5678)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int
main(void)
{
    test_reset_is_zero_but_unknown();
    test_unknown_and_null_reads_preserve_outputs();
    test_write_changes_only_bk_and_its_knownness();
    test_exact_project_values_round_trip();
    test_reset_clears_stale_value_and_knownness();
    test_rejected_calls_are_bytewise_nonmutating();
    puts("block_size_register_contract: ok");
    return 0;
}
