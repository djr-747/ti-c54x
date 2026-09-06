/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage that planner-produced MOD4/MOD7
 * WORD16 effects apply for reads and writes, retain the old operand address,
 * and commit exactly one selected-AR POST_ACCESS update without changing AR0
 * or any other architectural field.
 * Does not own: reverse-carry arithmetic, architectural storage, opcode
 * decode, bus transfers, timing, diagnostics, or unresolved boundaries.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_reverse_carry.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_planner_effects_apply_safe_transitions(void)
{
    /* These are interior points from the TI worked bit-reversed sequence for
     * the one-hot AR0 value 8.  The MOD4 list is the inverse of MOD7. */
    const uint16_t mod7_before[] = {
        UINT16_C(0x0060), UINT16_C(0x0068), UINT16_C(0x0064),
        UINT16_C(0x006c), UINT16_C(0x0062), UINT16_C(0x006a),
        UINT16_C(0x0066)
    };
    const uint16_t mod7_after[] = {
        UINT16_C(0x0068), UINT16_C(0x0064), UINT16_C(0x006c),
        UINT16_C(0x0062), UINT16_C(0x006a), UINT16_C(0x0066),
        UINT16_C(0x006e)
    };
    const uint16_t mod4_before[] = {
        UINT16_C(0x0068), UINT16_C(0x0064), UINT16_C(0x006c),
        UINT16_C(0x0062), UINT16_C(0x006a), UINT16_C(0x0066),
        UINT16_C(0x006e)
    };
    const uint16_t mod4_after[] = {
        UINT16_C(0x0060), UINT16_C(0x0068), UINT16_C(0x0064),
        UINT16_C(0x006c), UINT16_C(0x0062), UINT16_C(0x006a),
        UINT16_C(0x0066)
    };
    const ti_c54x_indirect_reverse_carry_mode modes[] = {
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7
    };
    uint32_t mode_index;
    uint32_t access_index;
    uint32_t ar_index;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        const bool is_mod4 = mode_index == 0U;
        const uint16_t *before = is_mod4 ? mod4_before : mod7_before;
        const uint16_t *after = is_mod4 ? mod4_after : mod7_after;

        for (access_index = 0U; access_index < 2U; ++access_index) {
            const ti_c54x_address_access access =
                access_index == 0U ? TI_C54X_ADDRESS_ACCESS_READ
                                   : TI_C54X_ADDRESS_ACCESS_WRITE;

            for (ar_index = 1U; ar_index <= 7U; ++ar_index) {
                const uint8_t index = (uint8_t)ar_index;
                ti_c54x_address_result result =
                    ti_c54x_indirect_reverse_carry_word16(
                        index, before[ar_index - 1U], UINT16_C(0x0008),
                        modes[mode_index], access);
                ti_c54x_arch_state state;
                ti_c54x_arch_state expected_state;
                uint16_t observed;

                TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
                TI_C54X_TEST_CHECK(result.effect.operand.width ==
                                    TI_C54X_DATA_OPERAND_WIDTH_WORD16);
                TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                                       before[ar_index - 1U]);
                TI_C54X_TEST_CHECK(result.effect.source.kind ==
                                    TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY);
                TI_C54X_TEST_CHECK_U64(
                    result.effect.source.value.ar_reverse_carry.index, index);
                TI_C54X_TEST_CHECK_U64(
                    result.effect.source.value.ar_reverse_carry.modification,
                    modes[mode_index]);
                TI_C54X_TEST_CHECK_U64(
                    result.effect.source.value.ar_reverse_carry.value_before,
                    before[ar_index - 1U]);
                TI_C54X_TEST_CHECK_U64(
                    result.effect.source.value.ar_reverse_carry
                        .ar0_value_before,
                    UINT16_C(0x0008));
                TI_C54X_TEST_CHECK(result.effect.update.target ==
                                    TI_C54X_ADDRESS_UPDATE_AR);
                TI_C54X_TEST_CHECK(result.effect.update.phase ==
                                    TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
                TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, index);
                TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                       before[ar_index - 1U]);
                TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                                       after[ar_index - 1U]);

                TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                                   TI_C54X_ARCH_STATE_OK);
                TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                       &state, index, before[ar_index - 1U]) ==
                                   TI_C54X_ARCH_STATE_OK);
                TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                       &state, UINT8_C(0), UINT16_C(0x0008)) ==
                                   TI_C54X_ARCH_STATE_OK);
                expected_state = state;
                TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                       &expected_state, index,
                                       after[ar_index - 1U]) ==
                                   TI_C54X_ARCH_STATE_OK);
                TI_C54X_TEST_CHECK(
                    ti_c54x_sequencer_apply_address_result(
                        &state, &result,
                        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                    TI_C54X_ADDRESS_APPLY_OK);
                TI_C54X_TEST_CHECK(memcmp(&state, &expected_state,
                                          sizeof(state)) == 0);
                TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                                       &state, UINT8_C(0), &observed) ==
                                   TI_C54X_ARCH_STATE_OK);
                TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x0008));
            }
        }
    }
}

int main(void)
{
    test_planner_effects_apply_safe_transitions();
    puts("indirect_reverse_carry_effect_apply: ok");
    return 0;
}
