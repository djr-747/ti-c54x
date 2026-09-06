/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof for the private 16-bit T
 * register state leaf. The test freezes reset's physical-zero but
 * architecturally-unknown boundary, checked read status/nonmutation, and
 * checked writes across the complete uint16_t value boundary.
 * Does not own: MMR or data-memory routing, CPU lifecycle, opcode semantics,
 * effect sequencing, timing, public inspection APIs, diagnostics, or the test
 * harness implementation.
 */

#include "../../../../src/state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_t_register_state sentinel_state(void)
{
    ti_c54x_t_register_state state;

    state.raw_value = UINT16_C(0xA55A);
    state.known = true;
    return state;
}

static void test_reset(void)
{
    ti_c54x_t_register_state state = sentinel_state();

    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&state) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK_U64(state.raw_value, UINT16_C(0));
    TI_C54X_TEST_CHECK(!state.known);

    /* Reset must clear stale storage and knownness on every invocation. */
    state.raw_value = UINT16_C(0xFFFF);
    state.known = true;
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&state) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK_U64(state.raw_value, UINT16_C(0));
    TI_C54X_TEST_CHECK(!state.known);

    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(NULL) ==
                       TI_C54X_T_REGISTER_INVALID_ARGUMENT);
}

static void test_unknown_read_preserves_output(void)
{
    const ti_c54x_t_register_state state = {
        UINT16_C(0xBEEF),
        false,
    };
    uint16_t output = UINT16_C(0x5AA5);
    const uint16_t before = output;

    TI_C54X_TEST_CHECK(ti_c54x_t_register_read(&state, &output) ==
                       TI_C54X_T_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(output, before);

    /* A rejected read must not mutate the state either. */
    TI_C54X_TEST_CHECK_U64(state.raw_value, UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(!state.known);
}

static void test_known_read(void)
{
    const ti_c54x_t_register_state state = {
        UINT16_C(0xC33C),
        true,
    };
    uint16_t output = UINT16_C(0x5AA5);

    TI_C54X_TEST_CHECK(ti_c54x_t_register_read(&state, &output) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK_U64(output, UINT16_C(0xC33C));
}

static void test_invalid_reads_preserve_output(void)
{
    const ti_c54x_t_register_state state = {
        UINT16_C(0xC33C),
        true,
    };
    uint16_t output = UINT16_C(0x5AA5);
    const uint16_t before = output;

    TI_C54X_TEST_CHECK(ti_c54x_t_register_read(NULL, &output) ==
                       TI_C54X_T_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(output, before);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_read(&state, NULL) ==
                       TI_C54X_T_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(output, before);
}

static void test_writes_accept_complete_uint16_domain(void)
{
    ti_c54x_t_register_state state = {
        UINT16_C(0x1357),
        false,
    };
    uint32_t value;

    for (value = UINT32_C(0); value <= UINT32_C(0xffff); ++value) {
        const uint16_t expected = (uint16_t)value;

        TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&state, expected) ==
                           TI_C54X_T_REGISTER_OK);
        TI_C54X_TEST_CHECK_U64(state.raw_value, expected);
        TI_C54X_TEST_CHECK(state.known);
    }
}

static void test_invalid_write_rejected_without_mutation(void)
{
    ti_c54x_t_register_state state = sentinel_state();
    const ti_c54x_t_register_state before = state;

    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(NULL, UINT16_C(0x0000)) ==
                       TI_C54X_T_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int main(void)
{
    test_reset();
    test_unknown_read_preserves_output();
    test_known_read();
    test_invalid_reads_preserve_output();
    test_writes_accept_complete_uint16_domain();
    test_invalid_write_rejected_without_mutation();
    puts("t_register: ok");
    return 0;
}
