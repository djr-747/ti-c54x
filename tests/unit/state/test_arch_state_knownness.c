/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof that unknown AR/SP/RTN
 * storage is not observed as an architectural zero and that checked
 * accessors preserve caller outputs on rejection.
 * Does not own: TI reset conformance, CPU reset integration, effect
 * application, bus transfers, opcode semantics, or public ABI behavior.
 */

#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void test_unknown_reads_preserve_outputs(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, UINT8_C(0), &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rtn(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xBEEF));
}

static void test_writes_establish_known_values(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0);

    for (uint8_t index = 0U; index < 8U; ++index) {
        const uint16_t expected = (uint16_t)(UINT16_C(0x1200) + index);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, expected) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, index, &value) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(value, expected);
    }

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, UINT16_C(0x4567)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x4567));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rtn(&state, UINT16_C(0x89AB)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rtn(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x89AB));
    TI_C54X_TEST_CHECK((state.ar_known_mask & UINT8_C(0x08)) != 0U);
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_SP) != 0U);
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_RTN) != 0U);
}

static void test_invalid_reads_preserve_outputs(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xCAFE);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, UINT8_C(8), &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, UINT8_C(255), &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(NULL, UINT8_C(0), &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, UINT8_C(0), NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rtn(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rtn(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void test_invalid_writes_do_not_mutate(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_arch_state before = state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(8), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(255), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           NULL, UINT8_C(0), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(NULL, UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rtn(NULL, UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void test_reset_clears_stale_knownness(void)
{
    ti_c54x_arch_state state = reset_state();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(7), UINT16_C(0xFFFF)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, UINT16_C(0xFFFF)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rtn(&state, UINT16_C(0xFFFF)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(state.ar_known_mask, UINT8_C(0));
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_SP) == 0U);
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_RTN) == 0U);
    TI_C54X_TEST_CHECK_U64(state.rtn, UINT16_C(0));
}

static void test_reset_overwrites_project_storage(void)
{
    ti_c54x_arch_state first;
    ti_c54x_arch_state second;

    (void)memset(&first, 0x5A, sizeof(first));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&first) ==
                       TI_C54X_ARCH_STATE_OK);
    first.ar[3] = UINT16_C(0x1234);
    first.ar_known_mask = UINT8_C(0x08);
    first.scalar_known_mask |= TI_C54X_ARCH_STATE_KNOWN_SP;
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&first) ==
                       TI_C54X_ARCH_STATE_OK);

    (void)memset(&second, 0xC3, sizeof(second));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&second) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&first, &second, sizeof(first)) == 0);
    TI_C54X_TEST_CHECK_U64(first.sp, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(first.rtn, UINT16_C(0));
    for (uint8_t index = 0U; index < 8U; ++index) {
        TI_C54X_TEST_CHECK_U64(first.ar[index], UINT16_C(0));
    }
}

static void test_invalid_reset_is_checked(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

int main(void)
{
    test_unknown_reads_preserve_outputs();
    test_writes_establish_known_values();
    test_invalid_reads_preserve_outputs();
    test_invalid_writes_do_not_mutate();
    test_reset_clears_stale_knownness();
    test_reset_overwrites_project_storage();
    test_invalid_reset_is_checked();
    puts("arch_state_knownness: ok");
    return 0;
}
