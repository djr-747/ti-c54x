/*
 * Module contract
 *
 * Owns: executable proof for private full-40-bit accumulator storage,
 * knownness, reset, checked access, and cross-lane isolation.
 * Does not own: CPU lifecycle, opcode semantics, status mutation, sequencing,
 * buses, timing, diagnostics, or public bootstrap APIs.
 */

#include "state/accumulator/accumulator.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static void test_reset_is_zero_but_unknown(void)
{
    ti_c54x_accumulator_state state;
    ti_c54x_accumulator_snapshot snapshot;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(&state, &snapshot) ==
                       TI_C54X_ACCUMULATOR_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(snapshot.a_raw40, UINT64_C(0));
    TI_C54X_TEST_CHECK_U64(snapshot.b_raw40, UINT64_C(0));
    TI_C54X_TEST_CHECK(!snapshot.a_known);
    TI_C54X_TEST_CHECK(!snapshot.b_known);
}

static void test_checked_values_and_isolation(void)
{
    ti_c54x_accumulator_state state;
    uint64_t value = UINT64_C(0);
    unsigned char before[sizeof(state)];

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &state, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xFFFFFFFFFF)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_A, &value) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT64_C(0xFFFFFFFFFF));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_B, &value) ==
                       TI_C54X_ACCUMULATOR_UNKNOWN);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &state, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x123456789A)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_B, &value) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT64_C(0x123456789A));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_A, &value) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT64_C(0xFFFFFFFFFF));

    memcpy(before, &state, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &state, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x10000000000)) ==
                       TI_C54X_ACCUMULATOR_INVALID_VALUE);
    TI_C54X_TEST_CHECK(memcmp(before, &state, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &state, TI_C54X_ACCUMULATOR_INVALID,
                           UINT64_C(0)) == TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);

    value = UINT64_C(0xDEADBEEF);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_A, &value) ==
                       TI_C54X_ACCUMULATOR_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT64_C(0xDEADBEEF));

    memcpy(before, &state, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, (ti_c54x_accumulator_id)-1, &value) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(before, &state, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &state, (ti_c54x_accumulator_id)-1,
                           UINT64_C(1)) == TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(before, &state, sizeof(state)) == 0);
}

static void test_null_arguments(void)
{
    ti_c54x_accumulator_state state;
    ti_c54x_accumulator_snapshot snapshot;
    uint64_t value = UINT64_C(7);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(NULL) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(NULL,
                                                 TI_C54X_ACCUMULATOR_A,
                                                 &value) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(&state,
                                                 TI_C54X_ACCUMULATOR_A,
                                                 NULL) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(NULL, &snapshot) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(&state, NULL) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
}

int main(void)
{
    test_reset_is_zero_but_unknown();
    test_checked_values_and_isolation();
    test_null_arguments();
    return 0;
}
