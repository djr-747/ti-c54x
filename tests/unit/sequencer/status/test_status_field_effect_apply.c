/*
 * Module contract
 *
 * Owns: executable proofs for status-field effect dispatch, field limits,
 * surrounding-bit preservation, and transactional rejection.
 * Does not own: opcode decoding, CPU lifecycle, PC effects, timing, memory,
 * bus transfers, or public ABI behavior.
 */

#include "sequencer/status/field_effect_apply.h"
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

static ti_c54x_op_effect field_effect(ti_c54x_op_status_field_target target,
                                      uint16_t value)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE;
    effect.payload.status_field_write.target = target;
    effect.payload.status_field_write.raw_value = value;
    return effect;
}

static void test_none_and_null(void)
{
    ti_c54x_status_register_state state = reset_state();
    const ti_c54x_status_register_state before = state;
    const ti_c54x_op_effect none = {0};

    TI_C54X_TEST_CHECK(ti_c54x_status_field_effect_apply(&none, &state) ==
                       TI_C54X_STATUS_FIELD_EFFECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_status_field_effect_apply(NULL, &state) ==
                       TI_C54X_STATUS_FIELD_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_field_effect_apply(&none, NULL) ==
                       TI_C54X_STATUS_FIELD_EFFECT_INVALID_ARGUMENT);
}

static void test_exhaustive_fields(void)
{
    const ti_c54x_op_status_field_target targets[] = {
        TI_C54X_OP_STATUS_FIELD_TARGET_DP,
        TI_C54X_OP_STATUS_FIELD_TARGET_ASM,
        TI_C54X_OP_STATUS_FIELD_TARGET_ARP};
    const uint16_t limits[] = {UINT16_C(0x01FF), UINT16_C(0x001F),
                               UINT16_C(0x0007)};

    for (size_t target_index = 0; target_index < 3U; ++target_index) {
        for (uint16_t value = 0; value <= limits[target_index]; ++value) {
            ti_c54x_status_register_state state = reset_state();
            ti_c54x_op_effect effect =
                field_effect(targets[target_index], value);
            TI_C54X_TEST_CHECK(
                ti_c54x_status_field_effect_apply(&effect, &state) ==
                TI_C54X_STATUS_FIELD_EFFECT_OK);
            if (target_index == 0U) {
                uint16_t dp = UINT16_C(0);
                TI_C54X_TEST_CHECK(
                    ti_c54x_status_register_read_dp(&state).status ==
                    TI_C54X_STATUS_REGISTER_OK);
                dp = ti_c54x_status_register_read_dp(&state).raw_value;
                TI_C54X_TEST_CHECK_U64(dp, value);
            } else if (target_index == 1U) {
                TI_C54X_TEST_CHECK_U64(
                    ti_c54x_status_register_read_asm(&state).raw_value, value);
            } else {
                TI_C54X_TEST_CHECK_U64(
                    ti_c54x_status_register_read_arp(&state).raw_value, value);
            }
        }
    }
}

static void test_rejected_effects_are_nonmutating(void)
{
    const ti_c54x_op_effect invalid_target =
        field_effect(TI_C54X_OP_STATUS_FIELD_TARGET_INVALID, 0U);
    const ti_c54x_op_effect invalid_dp =
        field_effect(TI_C54X_OP_STATUS_FIELD_TARGET_DP, UINT16_C(0x0200));
    const ti_c54x_op_effect invalid_asm =
        field_effect(TI_C54X_OP_STATUS_FIELD_TARGET_ASM, UINT16_C(0x0020));
    const ti_c54x_op_effect invalid_arp =
        field_effect(TI_C54X_OP_STATUS_FIELD_TARGET_ARP, UINT16_C(0x0008));
    const ti_c54x_op_effect wide_arp =
        field_effect(TI_C54X_OP_STATUS_FIELD_TARGET_ARP, UINT16_C(0x0107));
    const ti_c54x_op_effect repeat = {TI_C54X_OP_EFFECT_REPEAT_START,
                                      {.repeat_start = {1U}}};
    ti_c54x_op_effect forged = {0};
    forged.kind = (ti_c54x_op_effect_kind)0x7f;
    const ti_c54x_op_effect *effects[] = {
        &invalid_target, &invalid_dp, &invalid_asm, &invalid_arp, &wide_arp,
        &repeat, &forged};

    for (size_t i = 0; i < sizeof(effects) / sizeof(effects[0]); ++i) {
        ti_c54x_status_register_state state = reset_state();
        const ti_c54x_status_register_state before = state;
        const ti_c54x_status_field_effect_status status =
            ti_c54x_status_field_effect_apply(effects[i], &state);
        TI_C54X_TEST_CHECK(status ==
                           (i == 5U || i == 6U
                                ? TI_C54X_STATUS_FIELD_EFFECT_UNSUPPORTED
                                : TI_C54X_STATUS_FIELD_EFFECT_INVALID_EFFECT));
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

int main(void)
{
    test_none_and_null();
    test_exhaustive_fields();
    test_rejected_effects_are_nonmutating();
    return 0;
}
