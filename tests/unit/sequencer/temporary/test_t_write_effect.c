/* Module contract
 *
 * Owns: executable proofs for the value-owned T-register write effect leaf,
 * including uint16_t copying, knownness publication, and atomic rejection.
 * Does not own: T-register storage/reset, opcode semantics, decode, CPU
 * lifecycle, root effect routing, timing, diagnostics, memory, or observers.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/temporary/effect_apply.h"
#include "../../../../src/state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_t_register_state seed_state(void)
{
    ti_c54x_t_register_state state;

    state.raw_value = UINT16_C(0x1357);
    state.known = false;
    return state;
}

static ti_c54x_op_effect write_effect(uint16_t value)
{
    ti_c54x_op_effect effect = {0};
    const ti_c54x_op_t_register_write_effect payload = {value};

    effect.kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE;
    effect.payload.t_register_write = payload;
    return effect;
}

static void test_value_copy_marks_t_known(void)
{
    const uint16_t values[] = {UINT16_C(0), UINT16_C(1),
                               UINT16_C(0x8000), UINT16_C(0xffff)};

    for (size_t i = 0U; i < sizeof(values) / sizeof(values[0]); ++i) {
        ti_c54x_t_register_state state = seed_state();
        const ti_c54x_op_effect effect = write_effect(values[i]);

        TI_C54X_TEST_CHECK(ti_c54x_t_register_effect_apply(
                               &effect, &state) ==
                           TI_C54X_T_REGISTER_EFFECT_OK);
        TI_C54X_TEST_CHECK_U64(state.raw_value, values[i]);
        TI_C54X_TEST_CHECK(state.known);
        TI_C54X_TEST_CHECK(effect.kind ==
                           TI_C54X_OP_EFFECT_T_REGISTER_WRITE);
        TI_C54X_TEST_CHECK_U64(effect.payload.t_register_write.raw_value,
                               values[i]);
    }
}

static void test_none_is_nonmutating(void)
{
    ti_c54x_t_register_state state = seed_state();
    const ti_c54x_t_register_state before = state;
    const ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_t_register_effect_apply(&effect, &state) ==
                       TI_C54X_T_REGISTER_EFFECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void test_unrelated_effects_are_rejected_atomically(void)
{
    const ti_c54x_op_effect effects[] = {
        {TI_C54X_OP_EFFECT_REPEAT_START, {.repeat_start = {UINT16_C(7)}}},
        {TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE,
         {.status_field_write = {TI_C54X_OP_STATUS_FIELD_TARGET_DP, 1U}}},
        {TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
         {.accumulator_write = {0U, UINT64_C(1),
                                {TI_C54X_OP_STATUS_BIT_PRESERVE,
                                 TI_C54X_OP_STATUS_BIT_PRESERVE}}}},
    };

    for (size_t i = 0U; i < sizeof(effects) / sizeof(effects[0]); ++i) {
        ti_c54x_t_register_state state = seed_state();
        const ti_c54x_t_register_state before = state;

        TI_C54X_TEST_CHECK(ti_c54x_t_register_effect_apply(
                               &effects[i], &state) ==
                           TI_C54X_T_REGISTER_EFFECT_UNSUPPORTED);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

static void test_malformed_and_null_inputs_are_nonmutating(void)
{
    ti_c54x_t_register_state state = seed_state();
    const ti_c54x_t_register_state before = state;
    ti_c54x_op_effect malformed = write_effect(UINT16_C(0x2468));

    malformed.kind = (ti_c54x_op_effect_kind)UINT16_C(0x7f);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_effect_apply(&malformed, &state) ==
                       TI_C54X_T_REGISTER_EFFECT_UNSUPPORTED);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_effect_apply(NULL, &state) ==
                       TI_C54X_T_REGISTER_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_effect_apply(&malformed, NULL) ==
                       TI_C54X_T_REGISTER_EFFECT_INVALID_ARGUMENT);
}

int main(void)
{
    test_value_copy_marks_t_known();
    test_none_is_nonmutating();
    test_unrelated_effects_are_rejected_atomically();
    test_malformed_and_null_inputs_are_nonmutating();
    return 0;
}
