/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof that planner-produced CMPT=0 word effects
 * accept known CMPT=0/ARP=0 state and reject the other standard-mode states.
 * Does not own: register layout, address arithmetic, opcode decoding, bus
 * mapping, timing, diagnostics, or unresolved address-boundary policy.
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
test_planner_effects_accept_known_cmpt0_arp0(void)
{
    uint32_t mode_value;
    uint32_t index_value;

    for (mode_value = UINT32_C(0); mode_value <= UINT32_C(3);
         ++mode_value) {
        for (index_value = UINT32_C(0); index_value <= UINT32_C(7);
             ++index_value) {
            const uint8_t index = (uint8_t)index_value;
            const uint16_t current = (uint16_t)(UINT16_C(0x2000) + index);
            const ti_c54x_indirect_single_mode mode =
                (ti_c54x_indirect_single_mode)mode_value;
            const ti_c54x_address_result result =
                ti_c54x_indirect_single_word(
                    index, current, mode,
                    TI_C54X_ADDRESS_ACCESS_WRITE);
            ti_c54x_arch_state state;
            ti_c54x_arch_state expected;
            ti_c54x_address_update_target expected_target;
            ti_c54x_address_update_phase expected_phase;
            uint16_t expected_address;
            uint16_t expected_after = current;
            uint8_t arp = UINT8_C(0);
            uint8_t cmpt = UINT8_C(0);

            switch (mode) {
            case TI_C54X_INDIRECT_SINGLE_MOD0:
                expected_target = TI_C54X_ADDRESS_UPDATE_NONE;
                expected_phase = TI_C54X_ADDRESS_UPDATE_PHASE_NONE;
                expected_address = current;
                break;
            case TI_C54X_INDIRECT_SINGLE_MOD1:
                expected_target = TI_C54X_ADDRESS_UPDATE_AR;
                expected_phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
                expected_address = current;
                expected_after = (uint16_t)(current - UINT16_C(1));
                break;
            case TI_C54X_INDIRECT_SINGLE_MOD2:
                expected_target = TI_C54X_ADDRESS_UPDATE_AR;
                expected_phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
                expected_address = current;
                expected_after = (uint16_t)(current + UINT16_C(1));
                break;
            case TI_C54X_INDIRECT_SINGLE_MOD3:
                expected_target = TI_C54X_ADDRESS_UPDATE_AR;
                expected_phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
                expected_address = (uint16_t)(current + UINT16_C(1));
                expected_after = (uint16_t)(current + UINT16_C(1));
                break;
            default:
                TI_C54X_TEST_CHECK(false);
                return;
            }

            TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
            TI_C54X_TEST_CHECK(
                result.effect.state_policy ==
                TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
            TI_C54X_TEST_CHECK(result.effect.source.kind ==
                               TI_C54X_ADDRESS_SOURCE_AR);
            TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar.index,
                                   index);
            TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                                   expected_address);
            TI_C54X_TEST_CHECK(result.effect.update.target == expected_target);
            TI_C54X_TEST_CHECK(result.effect.update.phase == expected_phase);
            TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                   expected_target ==
                                           TI_C54X_ADDRESS_UPDATE_AR
                                       ? index
                                       : UINT8_C(0));
            TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                   expected_target ==
                                           TI_C54X_ADDRESS_UPDATE_AR
                                       ? current
                                       : UINT16_C(0));
            TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                                   expected_target ==
                                           TI_C54X_ADDRESS_UPDATE_AR
                                       ? expected_after
                                       : UINT16_C(0));
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                   &state, index, current) ==
                               TI_C54X_ARCH_STATE_OK);
            expected = state;

            if (expected_target == TI_C54X_ADDRESS_UPDATE_AR) {
                TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                       &expected,
                                       index, expected_after) ==
                                   TI_C54X_ARCH_STATE_OK);
            }
            TI_C54X_TEST_CHECK(
                ti_c54x_sequencer_apply_address_result(
                    &state, &result, expected_phase) ==
                TI_C54X_ADDRESS_APPLY_OK);
            TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &arp) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK_U64(arp, UINT8_C(0));
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(
                                   &state, &cmpt) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK_U64(cmpt, UINT8_C(0));
        }
    }
}

static void
test_nonzero_arp_rejected_before_selected_ar(void)
{
    uint8_t arp_value;

    for (arp_value = UINT8_C(1); arp_value <= UINT8_C(7); ++arp_value) {
        ti_c54x_arch_state state = {0};
        const ti_c54x_address_result result =
            ti_c54x_indirect_single_word(
                UINT8_C(0), UINT16_C(0x2200),
                TI_C54X_INDIRECT_SINGLE_MOD0,
                TI_C54X_ADDRESS_ACCESS_READ);
        ti_c54x_arch_state before;
        uint8_t observed = UINT8_C(0);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, arp_value) ==
                           TI_C54X_ARCH_STATE_OK);
        before = state;
        TI_C54X_TEST_CHECK(
            ti_c54x_sequencer_apply_address_result(
                &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
            TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(&state, &observed) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(observed, arp_value);
    }
}

static void
test_cmpt_one_rejected_without_repair(void)
{
    ti_c54x_arch_state state = {0};
    const ti_c54x_address_result result = ti_c54x_indirect_single_word(
        UINT8_C(0), UINT16_C(0x2200), TI_C54X_INDIRECT_SINGLE_MOD0,
        TI_C54X_ADDRESS_ACCESS_READ);
    ti_c54x_arch_state before;
    uint8_t observed = UINT8_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
        TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(&state, &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT8_C(1));
}

int
main(void)
{
    test_planner_effects_accept_known_cmpt0_arp0();
    test_nonzero_arp_rejected_before_selected_ar();
    test_cmpt_one_rejected_without_repair();
    puts("indirect_cmpt0_state: ok");
    return 0;
}
