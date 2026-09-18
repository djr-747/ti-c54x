/* Module contract
 *
 * Owns: executable proofs for the value-owned stack-pointer write
 * effect leaf, including value-after copying, SP knownness
 * publication, and atomic rejection.
 * Does not own: SP storage/reset, opcode semantics, decode, CPU
 * lifecycle, root effect routing, timing, diagnostics, memory, or
 * observers.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/stack/sp_write_apply.h"
#include "../../../../src/state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
seed_state(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state_status status =
        ti_c54x_arch_state_write_sp(&state, UINT16_C(0x1357));

    TI_C54X_TEST_CHECK(status == TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_effect
write_effect(uint16_t value_after)
{
    ti_c54x_op_effect effect = {0};
    const ti_c54x_op_sp_write_effect payload = {value_after};

    effect.kind = TI_C54X_OP_EFFECT_SP_WRITE;
    effect.payload.sp_write = payload;
    return effect;
}

static uint16_t
read_sp(const ti_c54x_arch_state *state)
{
    uint16_t value = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    return value;
}

static void
test_value_copy_marks_sp_known(void)
{
    const uint16_t values[] = {UINT16_C(0), UINT16_C(1),
                               UINT16_C(0x8000), UINT16_C(0xFFFF)};

    for (size_t i = 0U; i < sizeof(values) / sizeof(values[0]); ++i) {
        ti_c54x_arch_state state = {0};
        const ti_c54x_op_effect effect = write_effect(values[i]);

        TI_C54X_TEST_CHECK(ti_c54x_sp_write_effect_apply(&effect,
                                                        &state) ==
                           TI_C54X_SP_WRITE_EFFECT_OK);
        TI_C54X_TEST_CHECK(read_sp(&state) == values[i]);
        TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_SP_WRITE);
        TI_C54X_TEST_CHECK(effect.payload.sp_write.value_after ==
                           values[i]);
    }
}

static void
test_none_is_nonmutating(void)
{
    ti_c54x_arch_state state = seed_state();
    const ti_c54x_arch_state before = state;
    const ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sp_write_effect_apply(&effect, &state) ==
                       TI_C54X_SP_WRITE_EFFECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_unrelated_effects_are_rejected_atomically(void)
{
    const ti_c54x_op_effect effects[] = {
        {TI_C54X_OP_EFFECT_REPEAT_START, {.repeat_start = {UINT16_C(7)}}},
        {TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
         {.t_register_write = {UINT16_C(9)}}},
        {TI_C54X_OP_EFFECT_STATUS_BIT_WRITE,
         {.status_bit_write = {TI_C54X_OP_STATUS_REGISTER_ST0, UINT8_C(0),
                               UINT8_C(1)}}},
    };

    for (size_t i = 0U; i < sizeof(effects) / sizeof(effects[0]); ++i) {
        ti_c54x_arch_state state = seed_state();
        const ti_c54x_arch_state before = state;

        TI_C54X_TEST_CHECK(ti_c54x_sp_write_effect_apply(
                               &effects[i], &state) ==
                           TI_C54X_SP_WRITE_EFFECT_UNSUPPORTED);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

static void
test_malformed_and_null_inputs_are_nonmutating(void)
{
    ti_c54x_arch_state state = seed_state();
    const ti_c54x_arch_state before = state;
    ti_c54x_op_effect malformed = write_effect(UINT16_C(0x2468));

    malformed.kind = (ti_c54x_op_effect_kind)UINT16_C(0x7F);
    TI_C54X_TEST_CHECK(ti_c54x_sp_write_effect_apply(&malformed,
                                                    &state) ==
                       TI_C54X_SP_WRITE_EFFECT_UNSUPPORTED);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_sp_write_effect_apply(NULL, &state) ==
                       TI_C54X_SP_WRITE_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_sp_write_effect_apply(&malformed, NULL) ==
                       TI_C54X_SP_WRITE_EFFECT_INVALID_ARGUMENT);
}

int main(void)
{
    test_value_copy_marks_sp_known();
    test_none_is_nonmutating();
    test_unrelated_effects_are_rejected_atomically();
    test_malformed_and_null_inputs_are_nonmutating();
    return 0;
}
