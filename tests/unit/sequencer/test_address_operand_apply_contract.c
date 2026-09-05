/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for canonical operand
 * validation, rejection precedence, and bytewise nonmutation at the
 * sequencer application boundary.
 * Does not own: address planning, TI mode evidence, bus transfers, timing,
 * opcode decoding, CPU lifecycle, or diagnostics.
 */

#include "sequencer/address_effect_apply.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state
known_state(uint8_t index, uint16_t value)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_address_result
valid_none(ti_c54x_data_operand operand, uint8_t index)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = operand,
            .selected_ar_index = index,
            .update = {
                .target = TI_C54X_ADDRESS_UPDATE_NONE,
                .phase = TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                .ar_index = UINT8_C(0),
                .value_before = UINT16_C(0),
                .value_after = UINT16_C(0)
            }
        }
    };
    return result;
}

static ti_c54x_address_result
valid_long_update(uint16_t first,
                  uint8_t index,
                  ti_c54x_address_update_phase phase,
                  uint16_t before,
                  uint16_t after)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {
                .width = TI_C54X_DATA_OPERAND_WIDTH_LONG32,
                .first_word = first,
                .second_word =
                    (first & UINT16_C(1)) == UINT16_C(0)
                        ? (uint16_t)(first + UINT16_C(1))
                        : (uint16_t)(first - UINT16_C(1))
            },
            .selected_ar_index = index,
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {
                .target = TI_C54X_ADDRESS_UPDATE_AR,
                .phase = phase,
                .ar_index = index,
                .value_before = before,
                .value_after = after
            }
        }
    };
    return result;
}

static ti_c54x_address_result
valid_pre(ti_c54x_data_operand operand,
          uint8_t index,
          uint16_t before,
          uint16_t after)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = operand,
            .selected_ar_index = index,
            .update = {
                .target = TI_C54X_ADDRESS_UPDATE_AR,
                .phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                .ar_index = index,
                .value_before = before,
                .value_after = after
            }
        }
    };
    return result;
}

static void
expect_rejected(ti_c54x_arch_state *state,
                const ti_c54x_address_result *result,
                ti_c54x_address_update_phase requested_phase,
                ti_c54x_address_apply_status expected)
{
    const ti_c54x_arch_state before = *state;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           state, result, requested_phase) == expected);
    TI_C54X_TEST_CHECK(memcmp(&before, state, sizeof(before)) == 0);
}

static void
test_canonical_word_and_long_shapes_apply(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    ti_c54x_address_result result = valid_none(
        ti_c54x_data_operand_word16(UINT16_C(0x2200)), UINT8_C(2));

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    result = valid_none(ti_c54x_data_operand_long32(UINT16_C(0x2200)),
                        UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    result = valid_none(ti_c54x_data_operand_long32(UINT16_C(0x2201)),
                        UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                       TI_C54X_ADDRESS_APPLY_OK);
}

static void
test_canonical_long_pre_and_post_updates_apply(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(3), UINT16_C(0x2200));
    ti_c54x_arch_state expected_state = state;
    ti_c54x_address_result result = valid_long_update(
        UINT16_C(0x2202), UINT8_C(3),
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        UINT16_C(0x2200), UINT16_C(0x2202));
    uint16_t observed = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &expected_state, UINT8_C(3), UINT16_C(0x2202)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.second_word ==
                       UINT16_C(0x2203));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(3), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x2202));

    state = known_state(UINT8_C(3), UINT16_C(0x2202));
    expected_state = state;
    result = valid_long_update(
        UINT16_C(0x2202), UINT8_C(3),
        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        UINT16_C(0x2202), UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &expected_state, UINT8_C(3), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.second_word ==
                       UINT16_C(0x2203));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(3), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x2200));
}

static void
test_malformed_operands_reject_before_other_payload(void)
{
    ti_c54x_data_operand malformed[] = {
        {.width = TI_C54X_DATA_OPERAND_WIDTH_INVALID,
         .first_word = UINT16_C(0x2200), .second_word = UINT16_C(0)},
        {.width = (ti_c54x_data_operand_width)-1,
         .first_word = UINT16_C(0x2200), .second_word = UINT16_C(0)},
        {.width = (ti_c54x_data_operand_width)3,
         .first_word = UINT16_C(0x2200), .second_word = UINT16_C(0)},
        {.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
         .first_word = UINT16_C(0x2200), .second_word = UINT16_C(0x2201)},
        {.width = TI_C54X_DATA_OPERAND_WIDTH_LONG32,
         .first_word = UINT16_C(0x2200), .second_word = UINT16_C(0x2200)},
        {.width = TI_C54X_DATA_OPERAND_WIDTH_LONG32,
         .first_word = UINT16_C(0x2201), .second_word = UINT16_C(0x2202)}
    };
    size_t operand_index;

    for (operand_index = 0;
         operand_index < sizeof(malformed) / sizeof(malformed[0]);
         ++operand_index) {
        ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
        ti_c54x_address_result result =
            valid_none(malformed[operand_index], UINT8_C(255));

        result.effect.update.phase = (ti_c54x_address_update_phase)99;
        result.effect.state_policy =
            (ti_c54x_address_state_policy)99;
        expect_rejected(&state, &result,
                        TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    }
}

static void
test_non_ok_result_still_precedes_malformed_operand(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    ti_c54x_address_result result = valid_none(
        (ti_c54x_data_operand){
            .width = TI_C54X_DATA_OPERAND_WIDTH_INVALID,
            .first_word = UINT16_C(0x2200),
            .second_word = UINT16_C(0)
        }, UINT8_C(255));

    result.status = TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS;
    expect_rejected(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                    TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
}

static void
test_invalid_operand_precedes_phase_mode_and_ar_checks(void)
{
    const ti_c54x_data_operand malformed_word = {
        .width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
        .first_word = UINT16_C(0x2201),
        .second_word = UINT16_C(0x2202)
    };
    ti_c54x_arch_state state;
    ti_c54x_address_result result;

    /* The update itself is valid PRE with first_word == value_after; only
     * the operand shape is malformed, and requested NONE would otherwise
     * produce a phase mismatch. */
    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    result = valid_pre(malformed_word, UINT8_C(2), UINT16_C(0x2200),
                       UINT16_C(0x2201));
    expect_rejected(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    /* Unknown and violating CMPT0/ARP0 policy are not reached. */
    (void)memset(&state, 0, sizeof(state));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2201)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = valid_none(malformed_word, UINT8_C(2));
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;
    expect_rejected(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = valid_none(malformed_word, UINT8_C(2));
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;
    expect_rejected(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    /* Unknown and stale selected AR state are likewise not reached. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    result = valid_none(malformed_word, UINT8_C(2));
    expect_rejected(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    result = valid_none(malformed_word, UINT8_C(2));
    result.effect.operand.first_word = UINT16_C(0x2200);
    expect_rejected(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
}

int
main(void)
{
    test_canonical_word_and_long_shapes_apply();
    test_canonical_long_pre_and_post_updates_apply();
    test_malformed_operands_reject_before_other_payload();
    test_non_ok_result_still_precedes_malformed_operand();
    test_invalid_operand_precedes_phase_mode_and_ar_checks();
    puts("address_operand_apply_contract: ok");
    return 0;
}
