/*
 * Module contract
 *
 * Owns: executable proof of the checked IFR register accessor, one-bit
 * set/clear validation, and unrelated-state preservation. Does not own:
 * interrupt sources, vectors, priority, repeat arbitration, CPU lifecycle,
 * timing, buses, allocation, observers, or diagnostics.
 */

#include "state/arch_state.h"
#include "state/interrupt_flag_register.h"
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
test_read_unknown_and_null_preserves_output(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xA5A5);

    state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_IFR);
    state.ifr = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void
test_write_and_one_bit_mutations_preserve_unrelated_state(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_arch_state expected = state;
    ti_c54x_arch_state before;
    uint16_t value = UINT16_C(0);

    state.pc_word = UINT16_C(0x1234);
    state.ifr = UINT16_C(0x8001);
    expected = state;
    expected.ifr = UINT16_C(0x8033);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ifr(&state, UINT16_C(0x8033)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x8033));
    before = state;
    expected = state;
    expected.ifr = (uint16_t)(expected.ifr | UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(
                           &state, UINT16_C(0x0004)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(state.pc_word == before.pc_word);
    expected.ifr = (uint16_t)(expected.ifr & (uint16_t)~UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_clear_ifr_bit(
                           &state, UINT16_C(0x0004)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
}

static void
test_every_one_bit_mask(void)
{
    uint8_t shift;

    for (shift = 0U; shift < 16U; ++shift) {
        const uint16_t mask = (uint16_t)(UINT16_C(1) << shift);
        ti_c54x_arch_state state = reset_state();
        ti_c54x_arch_state expected;
        const uint16_t baseline = (uint16_t)(UINT16_C(0xA5A5) &
                                             (uint16_t)~mask);

        state.pc_word = UINT16_C(0x3456);
        state.bk = UINT16_C(0x789A);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ifr(&state, baseline) ==
                           TI_C54X_ARCH_STATE_OK);
        expected = state;
        expected.ifr = (uint16_t)(baseline | mask);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(&state, mask) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
        expected.ifr = baseline;
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_clear_ifr_bit(&state, mask) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
    }
}

static void
test_invalid_masks_and_nulls_are_transactional(void)
{
    ti_c54x_arch_state state = reset_state();
    const ti_c54x_arch_state before = state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(&state, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(&state, UINT16_C(3)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_clear_ifr_bit(NULL, UINT16_C(1)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ifr(NULL, UINT16_C(1)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int
main(void)
{
    test_read_unknown_and_null_preserves_output();
    test_write_and_one_bit_mutations_preserve_unrelated_state();
    test_every_one_bit_mask();
    test_invalid_masks_and_nulls_are_transactional();
    puts("interrupt_flag_register_contract: ok");
    return 0;
}
