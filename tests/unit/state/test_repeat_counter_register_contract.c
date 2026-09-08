/*
 * Module contract
 *
 * Owns: executable proof of RC's checked read/write and dedicated knownness
 * contract. Does not own: repeat sequencing, opcode effects, CPU lifecycle,
 * program control flow, timing, buses, allocation, or diagnostics.
 */

#include "state/arch_state.h"
#include "state/repeat_counter_register.h"
#include "../../support/test_harness.h"

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
test_unknown_and_null_reads_preserve_output(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xA5A5);

    state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_RC);
    state.rc = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void
test_write_changes_only_rc_and_knownness(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_arch_state expected = state;

    state.pc_word = UINT16_C(0x1234);
    state.ifr = UINT16_C(0x5678);
    state.bk = UINT16_C(0x9ABC);
    state.sp = UINT16_C(0xDEF0);
    state.ar[3] = UINT16_C(0xCAFE);
    state.status.st0 = UINT16_C(0x1357);
    state.status.st1 = UINT16_C(0x2468);
    state.status.pmst = UINT16_C(0xAAAA);
    expected = state;
    expected.rc = UINT16_C(0xBEEF);
    expected.scalar_known_mask = (uint8_t)(expected.scalar_known_mask |
                                           TI_C54X_ARCH_STATE_KNOWN_RC);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(0xBEEF)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
}

static void
test_round_trip_and_rejected_write(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_arch_state before;
    uint16_t value = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_MAX) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_MAX);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(NULL, UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int
main(void)
{
    test_unknown_and_null_reads_preserve_output();
    test_write_changes_only_rc_and_knownness();
    test_round_trip_and_rejected_write();
    puts("repeat_counter_register_contract: ok");
    return 0;
}
