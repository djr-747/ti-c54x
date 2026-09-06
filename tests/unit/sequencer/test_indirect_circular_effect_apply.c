/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage that planner-produced circular MOD8/
 * MOD10 WORD16 effects apply through the sequencer for reads and writes.
 * Positive BK cases prove exactly one selected-AR result and no BK/other-AR
 * mutation; BK=0 proves a successful canonical NONE effect is non-mutating.
 * Does not own: circular planning arithmetic, architectural dependency
 * rejection, opcode decoding, bus transfers, timing, diagnostics, or tests
 * of malformed effects.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_circular.h"
#include "state/block_size_register.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state state_with_mode(uint8_t selected,
                                          uint16_t ar_value,
                                          uint16_t bk_value)
{
    ti_c54x_arch_state state;
    uint8_t index;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    for (index = UINT8_C(0); index <= UINT8_C(7); ++index) {
        const uint16_t value =
            index == selected ? ar_value : (uint16_t)(UINT16_C(0x6000) + index);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, value) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, bk_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void run_positive_case(uint8_t selected,
                              uint16_t ar_value,
                              uint16_t bk_value,
                              ti_c54x_indirect_circular_mode mode,
                              ti_c54x_address_access access,
                              uint16_t expected_after)
{
    ti_c54x_arch_state state;
    uint16_t before_ar[8];
    uint16_t before_bk;
    uint8_t index;
    const ti_c54x_address_result result =
        ti_c54x_indirect_circular_word16(selected, ar_value, bk_value, mode,
                                         access);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    state = state_with_mode(selected, ar_value, bk_value);
    for (index = UINT8_C(0); index <= UINT8_C(7); ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, index,
                                                       &before_ar[index]) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &before_bk) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);

    for (index = UINT8_C(0); index <= UINT8_C(7); ++index) {
        uint16_t observed;
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, index,
                                                       &observed) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(observed,
                               index == selected ? expected_after
                                                 : before_ar[index]);
    }
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &before_bk) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(before_bk, bk_value);
}

static void test_positive_bk_updates_only_selected_ar(void)
{
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE};
    static const ti_c54x_indirect_circular_mode modes[] = {
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INDIRECT_CIRCULAR_MOD10};
    size_t mode_index;
    size_t access_index;
    uint8_t selected;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            for (selected = UINT8_C(0); selected <= UINT8_C(7); ++selected) {
                const uint16_t ar_value =
                    mode_index == 0U ? UINT16_C(0x1240)
                                     : UINT16_C(0x1244);
                run_positive_case(selected, ar_value, UINT16_C(5),
                                  modes[mode_index], accesses[access_index],
                                  mode_index == 0U ? UINT16_C(0x1244)
                                                   : UINT16_C(0x1240));
                run_positive_case(selected, UINT16_C(0x1242), UINT16_C(5),
                                  modes[mode_index], accesses[access_index],
                                  mode_index == 0U ? UINT16_C(0x1241)
                                                   : UINT16_C(0x1243));
            }
        }
    }
}

static void test_bk_zero_success_is_non_mutating(void)
{
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE};
    static const ti_c54x_indirect_circular_mode modes[] = {
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INDIRECT_CIRCULAR_MOD10};
    size_t mode_index;
    size_t access_index;
    uint8_t selected;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            for (selected = UINT8_C(0); selected <= UINT8_C(7); ++selected) {
                ti_c54x_arch_state state = state_with_mode(
                    selected, (uint16_t)(UINT16_C(0xa500) + selected),
                    UINT16_C(0));
                const ti_c54x_arch_state before = state;
                const ti_c54x_address_result result =
                    ti_c54x_indirect_circular_word16(
                        selected, (uint16_t)(UINT16_C(0xa500) + selected),
                        UINT16_C(0), modes[mode_index], accesses[access_index]);

                TI_C54X_TEST_CHECK(result.status ==
                                   TI_C54X_ADDRESS_STATUS_OK);
                TI_C54X_TEST_CHECK(result.effect.update.target ==
                                   TI_C54X_ADDRESS_UPDATE_NONE);
                TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                                       &state, &result,
                                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                                   TI_C54X_ADDRESS_APPLY_OK);
                TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) ==
                                   0);
            }
        }
    }
}

int main(void)
{
    test_positive_bk_updates_only_selected_ar();
    test_bk_zero_success_is_non_mutating();
    puts("indirect_circular_effect_apply: ok");
    return 0;
}
