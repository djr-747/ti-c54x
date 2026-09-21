/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof of XPC reset knownness and
 * zero value, caller-output preservation, NULL rejection, exact writes,
 * sole-field/knownness mutation, and bytewise nonmutation on rejected calls.
 * Does not own: TI reset conformance beyond the pinned zero-init, MMR
 * routing (XPC 0x1E stays outside the MMR file), far control semantics,
 * CPU lifecycle, bus transfers, opcode semantics, or test-harness behavior.
 */

#include "state/arch_state.h"
#include "state/program_counter_extension.h"
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
test_reset_is_zero_and_known(void)
{
    ti_c54x_arch_state state = reset_state();
    uint16_t value = UINT16_C(0xA5A5);

    TI_C54X_TEST_CHECK_U64(state.xpc, UINT16_C(0));
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_XPC) != 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0));
}

static void
test_unknown_and_null_reads_preserve_outputs(void)
{
    ti_c54x_arch_state state;
    uint16_t value = UINT16_C(0x5AA5);

    (void)memset(&state, 0, sizeof(state));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&state, &value) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x5AA5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(NULL, &value) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x5AA5));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&state, NULL) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

static void
test_write_changes_only_xpc_and_its_knownness(void)
{
    ti_c54x_arch_state state;
    ti_c54x_arch_state before;
    ti_c54x_arch_state expected;
    uint16_t value = UINT16_C(0xA5A5);

    (void)memset(&state, 0, sizeof(state));
    before = state;
    expected = before;
    expected.xpc = UINT16_C(0x007F);
    expected.scalar_known_mask = (uint8_t)(
        before.scalar_known_mask | TI_C54X_ARCH_STATE_KNOWN_XPC);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&state,
                                                   UINT16_C(0x007F)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x007F));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(NULL,
                                                   UINT16_C(1)) ==
                       TI_C54X_ARCH_STATE_INVALID_ARGUMENT);
}

int
main(void)
{
    test_reset_is_zero_and_known();
    test_unknown_and_null_reads_preserve_outputs();
    test_write_changes_only_xpc_and_its_knownness();
    puts("program_counter_extension: ok");
    return 0;
}
