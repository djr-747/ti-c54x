/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for applying planner-produced long32
 * single-indirect effects through the transactional sequencer seam.
 * Does not own: address planning arithmetic, bus order, timing, opcode decode,
 * CPU lifecycle, or diagnostics.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_single.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint16_t
expected_second(uint16_t first)
{
    return (first & UINT16_C(1)) == UINT16_C(0)
               ? (uint16_t)(first + UINT16_C(1))
               : (uint16_t)(first - UINT16_C(1));
}

static void
test_planner_long32_effects_apply(void)
{
    uint32_t mode_value;

    for (mode_value = UINT32_C(0); mode_value <= UINT32_C(3); ++mode_value) {
        uint32_t access_value;

        for (access_value = UINT32_C(0); access_value <= UINT32_C(1);
             ++access_value) {
            const ti_c54x_indirect_single_mode mode =
                (ti_c54x_indirect_single_mode)mode_value;
            const ti_c54x_address_access access =
                (ti_c54x_address_access)access_value;
            uint32_t ar_index_value;
            uint32_t first_value = mode == TI_C54X_INDIRECT_SINGLE_MOD1
                                       ? UINT32_C(2)
                                       : UINT32_C(0);
            uint32_t last_value =
                mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
                        mode == TI_C54X_INDIRECT_SINGLE_MOD3
                    ? UINT32_C(0xfffd)
                    : UINT32_C(0xffff);

            if (mode == TI_C54X_INDIRECT_SINGLE_MOD3 &&
                access == TI_C54X_ADDRESS_ACCESS_READ) {
                continue;
            }
            for (ar_index_value = UINT32_C(0);
                 ar_index_value <= UINT32_C(7); ++ar_index_value) {
                uint32_t ar_value;

                for (ar_value = first_value; ar_value <= last_value;
                     ++ar_value) {
                    const uint8_t ar_index = (uint8_t)ar_index_value;
                    const uint16_t current = (uint16_t)ar_value;
                    const ti_c54x_address_result result =
                        ti_c54x_indirect_single_long32(
                            ar_index, current, mode, access);
                    ti_c54x_arch_state state;
                    ti_c54x_arch_state expected_state;
                    uint16_t expected_first = current;
                    uint16_t expected_after = current;

                    TI_C54X_TEST_CHECK(result.status ==
                                       TI_C54X_ADDRESS_STATUS_OK);
                    if (mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
                        expected_first = (uint16_t)(current + UINT16_C(2));
                    }
                    if (mode == TI_C54X_INDIRECT_SINGLE_MOD1) {
                        expected_after = (uint16_t)(current - UINT16_C(2));
                    } else if (mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
                               mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
                        expected_after = (uint16_t)(current + UINT16_C(2));
                    }
                    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                                       TI_C54X_DATA_OPERAND_WIDTH_LONG32);
                    TI_C54X_TEST_CHECK_U64(result.effect.selected_ar_index,
                                           ar_index);
                    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
                    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                                           expected_first);
                    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word,
                                           expected_second(expected_first));
                    TI_C54X_TEST_CHECK(
                        result.effect.update.target ==
                        (mode == TI_C54X_INDIRECT_SINGLE_MOD0
                             ? TI_C54X_ADDRESS_UPDATE_NONE
                             : TI_C54X_ADDRESS_UPDATE_AR));
                    TI_C54X_TEST_CHECK(
                        result.effect.update.phase ==
                        (mode == TI_C54X_INDIRECT_SINGLE_MOD0
                             ? TI_C54X_ADDRESS_UPDATE_PHASE_NONE
                         : mode == TI_C54X_INDIRECT_SINGLE_MOD3
                             ? TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS
                             : TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS));
                    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                           mode == TI_C54X_INDIRECT_SINGLE_MOD0
                                               ? UINT8_C(0)
                                               : ar_index);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                           mode == TI_C54X_INDIRECT_SINGLE_MOD0
                                               ? UINT16_C(0)
                                               : current);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                                           mode == TI_C54X_INDIRECT_SINGLE_MOD0
                                               ? UINT16_C(0)
                                               : expected_after);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_arch_state_reset_common(&state) ==
                        TI_C54X_ARCH_STATE_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                           &state, ar_index, current) ==
                                       TI_C54X_ARCH_STATE_OK);
                    expected_state = state;
                    if (mode != TI_C54X_INDIRECT_SINGLE_MOD0) {
                        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                               &expected_state, ar_index,
                                               expected_after) ==
                                           TI_C54X_ARCH_STATE_OK);
                    }
                    TI_C54X_TEST_CHECK(
                        ti_c54x_sequencer_apply_address_result(
                            &state, &result,
                            mode == TI_C54X_INDIRECT_SINGLE_MOD0
                                ? TI_C54X_ADDRESS_UPDATE_PHASE_NONE
                            : mode == TI_C54X_INDIRECT_SINGLE_MOD3
                                ? TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS
                                : TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                        TI_C54X_ADDRESS_APPLY_OK);
                    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state,
                                              sizeof(state)) == 0);
                }
            }
        }
    }
}

int
main(void)
{
    test_planner_long32_effects_apply();
    puts("indirect_long32_effect_apply: ok");
    return 0;
}
