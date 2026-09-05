/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for planner-produced CMPT=0 MOD13
 * WORD16 and LONG32 effects across both access kinds and selected ARs.
 * Does not own: signed offset arithmetic, architectural storage, decode,
 * buses, timing, diagnostics, or unresolved wrap policy.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_long_offset.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int32_t signed_offset_value(uint16_t offset_word)
{
    return (offset_word & UINT16_C(0x8000)) != UINT16_C(0)
               ? (int32_t)offset_word - INT32_C(0x10000)
               : (int32_t)offset_word;
}

static uint16_t expected_second(uint16_t first)
{
    return (first & UINT16_C(1)) == UINT16_C(0)
               ? (uint16_t)(first + UINT16_C(1))
               : (uint16_t)(first - UINT16_C(1));
}

static void check_apply(const ti_c54x_address_result *result,
                        uint8_t ar_index,
                        uint16_t ar_before,
                        uint16_t offset,
                        uint16_t expected_after,
                        ti_c54x_data_operand_width width)
{
    ti_c54x_arch_state state;
    ti_c54x_arch_state expected_state;
    uint16_t observed;

    TI_C54X_TEST_CHECK(result->status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result->effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET);
    TI_C54X_TEST_CHECK_U64(result->effect.source.value.ar_long_offset.index,
                           ar_index);
    TI_C54X_TEST_CHECK_U64(
        result->effect.source.value.ar_long_offset.value_before, ar_before);
    TI_C54X_TEST_CHECK_U64(
        result->effect.source.value.ar_long_offset.long_offset_word, offset);
    TI_C54X_TEST_CHECK_U64(result->effect.source.value.ar_long_offset.modification,
                           TI_C54X_INDIRECT_LONG_OFFSET_MOD13);
    TI_C54X_TEST_CHECK(result->effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result->effect.operand.width == width);
    TI_C54X_TEST_CHECK_U64(result->effect.operand.first_word, expected_after);
    if (width == TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
        TI_C54X_TEST_CHECK_U64(result->effect.operand.second_word,
                               UINT16_C(0));
    } else {
        TI_C54X_TEST_CHECK_U64(result->effect.operand.second_word,
                               expected_second(expected_after));
    }
    TI_C54X_TEST_CHECK(result->effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(result->effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS);
    TI_C54X_TEST_CHECK_U64(result->effect.update.ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(result->effect.update.value_before, ar_before);
    TI_C54X_TEST_CHECK_U64(result->effect.update.value_after, expected_after);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, ar_index,
                                                   ar_before) ==
                       TI_C54X_ARCH_STATE_OK);
    expected_state = state;
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &expected_state, ar_index, expected_after) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state,
                              sizeof(expected_state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, ar_index, &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, expected_after);
}

static void test_planner_effects_apply(void)
{
    const uint16_t ar_values[] = {
        UINT16_C(0), UINT16_C(1), UINT16_C(0x1234), UINT16_C(0x7fff),
        UINT16_C(0x8000), UINT16_C(0xfffe), UINT16_C(0xffff)};
    const uint16_t offsets[] = {
        UINT16_C(0), UINT16_C(1), UINT16_C(0xffff), UINT16_C(0x7fff),
        UINT16_C(0x8000)};
    uint32_t width_value;
    uint32_t access_value;
    uint32_t index_value;
    size_t ar_value_index;
    size_t offset_index;

    for (width_value = UINT32_C(0); width_value < UINT32_C(2);
         ++width_value) {
        const ti_c54x_data_operand_width width =
            width_value == UINT32_C(0)
                ? TI_C54X_DATA_OPERAND_WIDTH_WORD16
                : TI_C54X_DATA_OPERAND_WIDTH_LONG32;
        for (access_value = UINT32_C(0); access_value < UINT32_C(2);
             ++access_value) {
            const ti_c54x_address_access access =
                access_value == UINT32_C(0)
                    ? TI_C54X_ADDRESS_ACCESS_READ
                    : TI_C54X_ADDRESS_ACCESS_WRITE;
            for (index_value = UINT32_C(0); index_value <= UINT32_C(7);
                 ++index_value) {
                for (ar_value_index = 0U;
                     ar_value_index < sizeof(ar_values) / sizeof(ar_values[0]);
                     ++ar_value_index) {
                    for (offset_index = 0U;
                         offset_index < sizeof(offsets) / sizeof(offsets[0]);
                         ++offset_index) {
                        const uint16_t ar_value = ar_values[ar_value_index];
                        const uint16_t offset = offsets[offset_index];
                        const int32_t signed_offset =
                            signed_offset_value(offset);
                        const int32_t sum = (int32_t)ar_value + signed_offset;

                        if (sum < INT32_C(0) ||
                            sum > INT32_C(0xffff)) {
                            continue;
                        }

                        {
                            const ti_c54x_address_result result =
                                width == TI_C54X_DATA_OPERAND_WIDTH_WORD16
                                    ? ti_c54x_indirect_long_offset_mod13_word16(
                                          (uint8_t)index_value, ar_value,
                                          offset, access)
                                    : ti_c54x_indirect_long_offset_mod13_long32(
                                          (uint8_t)index_value, ar_value,
                                          offset, access);
                            check_apply(&result, (uint8_t)index_value,
                                        ar_value, offset, (uint16_t)sum,
                                        width);
                        }
                    }
                }
            }
        }
    }
}

int main(void)
{
    test_planner_effects_apply();
    puts("indirect_long_offset_effect_apply: ok");
    return 0;
}
