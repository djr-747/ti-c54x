/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for transactional application of every
 * supported CMPT=0 single-indirect word effect.
 * Does not own: address planning, architectural-state storage, opcode decode,
 * bus transfers, timing, diagnostics, or unresolved address-boundary policy.
 * The test drives the private-module seam through planner-produced value-owned
 * effects and checks that only the selected AR changes.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_single.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_planner_effects_apply_across_supported_word_domain(void)
{
    uint32_t mode_value;

    for (mode_value = UINT32_C(0); mode_value <= UINT32_C(3);
         ++mode_value) {
        const ti_c54x_indirect_single_mode mode =
            (ti_c54x_indirect_single_mode)mode_value;
        uint32_t ar_index_value;
        uint32_t ar_value;

        for (ar_index_value = UINT32_C(0);
             ar_index_value <= UINT32_C(7);
             ++ar_index_value) {
            for (ar_value = UINT32_C(0); ar_value <= UINT32_C(0xffff);
                 ++ar_value) {
                const uint8_t ar_index = (uint8_t)ar_index_value;
                const uint16_t current = (uint16_t)ar_value;
                const ti_c54x_address_result result =
                    ti_c54x_indirect_single_word(
                        ar_index,
                        current,
                        mode,
                        TI_C54X_ADDRESS_ACCESS_WRITE);
                ti_c54x_arch_state state;
                ti_c54x_arch_state expected_state;

                if (result.status != TI_C54X_ADDRESS_STATUS_OK) {
                    continue;
                }

                TI_C54X_TEST_CHECK(
                    ti_c54x_arch_state_reset_common(&state) ==
                    TI_C54X_ARCH_STATE_OK);
                TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                       &state, ar_index, current) ==
                                   TI_C54X_ARCH_STATE_OK);
                expected_state = state;

                TI_C54X_TEST_CHECK(result.effect.source.kind ==
                                   TI_C54X_ADDRESS_SOURCE_AR);
                TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar.index,
                                       ar_index);
                switch (mode) {
                case TI_C54X_INDIRECT_SINGLE_MOD0:
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.operand.first_word, current);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.target ==
                        TI_C54X_ADDRESS_UPDATE_NONE);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.phase ==
                        TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                           UINT8_C(0));
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                           UINT16_C(0));
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                                           UINT16_C(0));
                    TI_C54X_TEST_CHECK(
                        ti_c54x_sequencer_apply_address_result(
                            &state, &result,
                            TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                        TI_C54X_ADDRESS_APPLY_OK);
                    break;
                case TI_C54X_INDIRECT_SINGLE_MOD1:
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.operand.first_word, current);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.target ==
                        TI_C54X_ADDRESS_UPDATE_AR);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.phase ==
                        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                           ar_index);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                           current);
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.update.value_after,
                        (uint16_t)(current - UINT16_C(1)));
                    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                           &expected_state,
                                           ar_index,
                                           (uint16_t)(current - UINT16_C(1))) ==
                                       TI_C54X_ARCH_STATE_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_sequencer_apply_address_result(
                            &state, &result,
                            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                        TI_C54X_ADDRESS_APPLY_OK);
                    break;
                case TI_C54X_INDIRECT_SINGLE_MOD2:
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.operand.first_word, current);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.target ==
                        TI_C54X_ADDRESS_UPDATE_AR);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.phase ==
                        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                           ar_index);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                           current);
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.update.value_after,
                        (uint16_t)(current + UINT16_C(1)));
                    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                           &expected_state,
                                           ar_index,
                                           (uint16_t)(current + UINT16_C(1))) ==
                                       TI_C54X_ARCH_STATE_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_sequencer_apply_address_result(
                            &state, &result,
                            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                        TI_C54X_ADDRESS_APPLY_OK);
                    break;
                case TI_C54X_INDIRECT_SINGLE_MOD3:
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.operand.first_word,
                        (uint16_t)(current + UINT16_C(1)));
                    TI_C54X_TEST_CHECK(
                        result.effect.update.target ==
                        TI_C54X_ADDRESS_UPDATE_AR);
                    TI_C54X_TEST_CHECK(
                        result.effect.update.phase ==
                        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                           ar_index);
                    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                           current);
                    TI_C54X_TEST_CHECK_U64(
                        result.effect.update.value_after,
                        (uint16_t)(current + UINT16_C(1)));
                    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                           &expected_state,
                                           ar_index,
                                           (uint16_t)(current + UINT16_C(1))) ==
                                       TI_C54X_ARCH_STATE_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_sequencer_apply_address_result(
                            &state, &result,
                            TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS) ==
                        TI_C54X_ADDRESS_APPLY_OK);
                    break;
                default:
                    TI_C54X_TEST_CHECK(false);
                    break;
                }

                TI_C54X_TEST_CHECK(memcmp(&state, &expected_state,
                                          sizeof(state)) == 0);
            }
        }
    }
}

int
main(void)
{
    test_planner_effects_apply_across_supported_word_domain();
    puts("address_effect_apply: ok");
    return 0;
}
