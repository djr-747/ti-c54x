/*
 * Module contract
 *
 * Owns: executable proofs for generic single-bit ST0/ST1 effect application,
 * including reserved ST1 bit 10, knownness publication, and rejection
 * atomicity.
 * Does not own: opcode decoding, handler semantics, CPU lifecycle, effect
 * transaction routing, PC effects, timing, memory, or public ABI behavior.
 */

#include "sequencer/status/bit_effect_apply.h"
#include "state/register_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_status_register_state reset_state(void)
{
    ti_c54x_status_register_state state;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    return state;
}

static ti_c54x_op_effect bit_effect(ti_c54x_op_status_register target_register,
                                    uint8_t bit_index,
                                    uint8_t raw_value)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE;
    effect.payload.status_bit_write.target_register = target_register;
    effect.payload.status_bit_write.bit_index = bit_index;
    effect.payload.status_bit_write.raw_value = raw_value;
    return effect;
}

static void test_none_and_null(void)
{
    ti_c54x_status_register_state state = reset_state();
    const ti_c54x_status_register_state before = state;
    const ti_c54x_op_effect none = {0};

    TI_C54X_TEST_CHECK(ti_c54x_status_bit_effect_apply(&none, &state) ==
                       TI_C54X_STATUS_BIT_EFFECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_status_bit_effect_apply(NULL, &state) ==
                       TI_C54X_STATUS_BIT_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_bit_effect_apply(&none, NULL) ==
                       TI_C54X_STATUS_BIT_EFFECT_INVALID_ARGUMENT);
}

static void test_exhaustive_st0_st1_bits_and_values(void)
{
    const ti_c54x_op_status_register targets[] = {
        TI_C54X_OP_STATUS_REGISTER_ST0,
        TI_C54X_OP_STATUS_REGISTER_ST1};

    for (size_t target_index = 0U; target_index < 2U; ++target_index) {
        for (uint8_t bit_index = 0U; bit_index < 16U; ++bit_index) {
            for (uint8_t raw_value = 0U; raw_value <= 1U; ++raw_value) {
                ti_c54x_status_register_state state = reset_state();
                const uint16_t before_st0 = UINT16_C(0x1357);
                const uint16_t before_st1 = UINT16_C(0x2468);
                const uint16_t before_st0_known = UINT16_C(0x5a5a);
                const uint16_t before_st1_known = UINT16_C(0xa5a5);
                const uint16_t bit_mask =
                    (uint16_t)(UINT16_C(1) << bit_index);
                const uint16_t expected =
                    raw_value == 0U
                        ? (uint16_t)((target_index == 0U ? before_st0
                                                          : before_st1) &
                                     (uint16_t)~bit_mask)
                        : (uint16_t)((target_index == 0U ? before_st0
                                                          : before_st1) |
                                     bit_mask);
                const uint16_t expected_known =
                    (uint16_t)((target_index == 0U ? before_st0_known
                                                   : before_st1_known) |
                               bit_mask);
                ti_c54x_op_effect effect;

                state.st0 = before_st0;
                state.st1 = before_st1;
                state.st0_known_mask = before_st0_known;
                state.st1_known_mask = before_st1_known;
                effect = bit_effect(targets[target_index], bit_index,
                                    raw_value);

                TI_C54X_TEST_CHECK(ti_c54x_status_bit_effect_apply(
                                       &effect, &state) ==
                                   TI_C54X_STATUS_BIT_EFFECT_OK);
                if (target_index == 0U) {
                    TI_C54X_TEST_CHECK_U64(state.st0, expected);
                    TI_C54X_TEST_CHECK_U64(state.st0_known_mask,
                                           expected_known);
                    TI_C54X_TEST_CHECK_U64(state.st1, before_st1);
                    TI_C54X_TEST_CHECK_U64(state.st1_known_mask,
                                           before_st1_known);
                } else {
                    TI_C54X_TEST_CHECK_U64(state.st1, expected);
                    TI_C54X_TEST_CHECK_U64(state.st1_known_mask,
                                           expected_known);
                    TI_C54X_TEST_CHECK_U64(state.st0, before_st0);
                    TI_C54X_TEST_CHECK_U64(state.st0_known_mask,
                                           before_st0_known);
                }
            }
        }
    }
}

static void test_rejected_effects_are_nonmutating(void)
{
    const ti_c54x_op_effect invalid_register = bit_effect(
        TI_C54X_OP_STATUS_REGISTER_INVALID, 0U, 1U);
    const ti_c54x_op_effect invalid_bit = bit_effect(
        TI_C54X_OP_STATUS_REGISTER_ST0, 16U, 1U);
    const ti_c54x_op_effect invalid_value = bit_effect(
        TI_C54X_OP_STATUS_REGISTER_ST1, 10U, 2U);
    const ti_c54x_op_effect field_effect = {TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE,
                                            {.status_field_write = {
                                                 TI_C54X_OP_STATUS_FIELD_TARGET_DP,
                                                 1U}}};
    const ti_c54x_op_effect repeat_effect = {
        TI_C54X_OP_EFFECT_REPEAT_START, {.repeat_start = {1U}}};
    ti_c54x_op_effect forged = {0};
    const ti_c54x_op_effect *effects[] = {
        &invalid_register, &invalid_bit, &invalid_value, &field_effect,
        &repeat_effect, &forged};

    forged.kind = (ti_c54x_op_effect_kind)UINT8_C(0x7f);
    for (size_t i = 0U; i < sizeof(effects) / sizeof(effects[0]); ++i) {
        ti_c54x_status_register_state state = reset_state();
        const ti_c54x_status_register_state before = state;
        const ti_c54x_status_bit_effect_status status =
            ti_c54x_status_bit_effect_apply(effects[i], &state);

        TI_C54X_TEST_CHECK(status ==
                           (i < 3U
                                ? TI_C54X_STATUS_BIT_EFFECT_INVALID_EFFECT
                                : TI_C54X_STATUS_BIT_EFFECT_UNSUPPORTED));
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

int main(void)
{
    test_none_and_null();
    test_exhaustive_st0_st1_bits_and_values();
    test_rejected_effects_are_nonmutating();
    return 0;
}
