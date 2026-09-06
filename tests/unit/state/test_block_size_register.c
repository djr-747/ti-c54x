/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof that the checked BK access seam round-trips
 * every 16-bit value exactly. This test deliberately makes no reset claim.
 * Does not own: BK reset policy, MMR routing, circular arithmetic, CPU state
 * construction, bus transfers, opcode semantics, or test-harness behavior.
 */

#include "state/arch_state.h"
#include "state/block_size_register.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_all_uint16_values_round_trip(void)
{
    ti_c54x_arch_state state;
    uint32_t raw_value;

    (void)memset(&state, 0, sizeof(state));
    for (raw_value = UINT32_C(0); raw_value <= UINT32_C(0xffff);
         ++raw_value) {
        const uint16_t expected = (uint16_t)raw_value;
        uint16_t actual = UINT16_C(0xA5A5);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, expected) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &actual) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(actual, expected);
    }
}

int
main(void)
{
    test_all_uint16_values_round_trip();
    puts("block_size_register: ok");
    return 0;
}
