/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage that planner-produced MOD5/MOD6
 * WORD16 effects apply for reads and writes, changing exactly the selected
 * AR1..AR7 and never AR0.
 * Does not own: indexed planning arithmetic, architectural storage, opcode
 * decode, bus transfers, timing, diagnostics, or unresolved boundaries.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_indexed.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void test_planner_effects_apply_safe_domain(void)
{
    const uint16_t ar_values[] = {
        UINT16_C(0), UINT16_C(1), UINT16_C(0x1234), UINT16_C(0xfffe),
        UINT16_C(0xffff)
    };
    const uint16_t ar0_values[] = {
        UINT16_C(0), UINT16_C(1), UINT16_C(2), UINT16_C(0x1234),
        UINT16_C(0x7fff), UINT16_C(0xffff)
    };
    uint32_t mode_value;
    uint32_t access_value;
    uint32_t index_value;
    size_t ar_value_index;
    size_t ar0_value_index;

    for (mode_value = UINT32_C(0); mode_value < UINT32_C(2); ++mode_value) {
        const ti_c54x_indirect_indexed_mode mode =
            mode_value == UINT32_C(0)
                ? TI_C54X_INDIRECT_INDEXED_MOD5
                : TI_C54X_INDIRECT_INDEXED_MOD6;
        for (access_value = UINT32_C(0); access_value < UINT32_C(2);
             ++access_value) {
            const ti_c54x_address_access access =
                access_value == UINT32_C(0)
                    ? TI_C54X_ADDRESS_ACCESS_READ
                    : TI_C54X_ADDRESS_ACCESS_WRITE;
            for (index_value = UINT32_C(1); index_value <= UINT32_C(7);
                 ++index_value) {
                for (ar_value_index = 0U;
                     ar_value_index < sizeof(ar_values) / sizeof(ar_values[0]);
                     ++ar_value_index) {
                    for (ar0_value_index = 0U;
                         ar0_value_index <
                         sizeof(ar0_values) / sizeof(ar0_values[0]);
                         ++ar0_value_index) {
                        const uint16_t ar_value = ar_values[ar_value_index];
                        const uint16_t ar0_value =
                            ar0_values[ar0_value_index];
                        uint16_t expected_after;
                        ti_c54x_address_result result;
                        ti_c54x_arch_state state;
                        uint16_t observed;

                        if (mode == TI_C54X_INDIRECT_INDEXED_MOD5) {
                            if (ar_value < ar0_value) {
                                continue;
                            }
                            expected_after =
                                (uint16_t)(ar_value - ar0_value);
                        } else {
                            const uint32_t sum =
                                (uint32_t)ar_value + (uint32_t)ar0_value;
                            if (sum > UINT32_C(0xffff)) {
                                continue;
                            }
                            expected_after = (uint16_t)sum;
                        }

                        result = ti_c54x_indirect_indexed_word(
                            (uint8_t)index_value, ar_value, ar0_value,
                            mode, access);
                        TI_C54X_TEST_CHECK(
                            result.status == TI_C54X_ADDRESS_STATUS_OK);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_arch_state_reset_common(&state) ==
                            TI_C54X_ARCH_STATE_OK);
                        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                               &state, (uint8_t)index_value,
                                               ar_value) ==
                                           TI_C54X_ARCH_STATE_OK);
                        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                               &state, UINT8_C(0), ar0_value) ==
                                           TI_C54X_ARCH_STATE_OK);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_sequencer_apply_address_result(
                                &state, &result,
                                TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                            TI_C54X_ADDRESS_APPLY_OK);
                        TI_C54X_TEST_CHECK_U64(result.effect.operand.width,
                                               TI_C54X_DATA_OPERAND_WIDTH_WORD16);
                        TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                                               ar_value);
                        TI_C54X_TEST_CHECK_U64(
                            result.effect.update.value_after, expected_after);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_arch_state_read_ar(
                                &state, UINT8_C(0), &observed) ==
                            TI_C54X_ARCH_STATE_OK);
                        TI_C54X_TEST_CHECK_U64(observed, ar0_value);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_arch_state_read_ar(
                                &state, (uint8_t)index_value, &observed) ==
                            TI_C54X_ARCH_STATE_OK);
                        TI_C54X_TEST_CHECK_U64(
                            observed, expected_after);
                    }
                }
            }
        }
    }
}

int main(void)
{
    test_planner_effects_apply_safe_domain();
    puts("indirect_indexed_effect_apply: ok");
    return 0;
}
