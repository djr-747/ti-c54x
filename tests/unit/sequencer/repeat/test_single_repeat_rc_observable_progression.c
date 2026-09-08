/*
 * Module contract
 *
 * Owns: executable proof that one repeat start and each retired body expose
 * the expected RC sequence for required uint16_t boundary samples.
 * Does not own: CPU/PC/IRQ sequencing, timing, buses, memory, allocation,
 * observers, or opcode decode.
 */

#include "sequencer/repeat/repeat_progress.h"
#include "state/repeat_counter_register.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_op_effect
repeat_effect(uint16_t operand)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    effect.payload.repeat_start.operand = operand;
    return effect;
}

static void
test_operand_progression(uint16_t operand)
{
    ti_c54x_arch_state state;
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_effect(operand);
    uint32_t body_index;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK_U64(progress.loaded_rc_operand, operand);
    TI_C54X_TEST_CHECK_U64(progress.body_executions_remaining,
                           (uint32_t)operand + UINT32_C(1));
    for (body_index = (uint32_t)operand + UINT32_C(1); body_index > 0U;
         --body_index) {
        ti_c54x_arch_state state_before = state;
        ti_c54x_arch_state state_expected = state;
        uint16_t rc = UINT16_C(0xFFFF);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, &rc) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(rc, body_index - UINT32_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(
                               &state, &progress) ==
                           TI_C54X_REPEAT_PROGRESS_OK);
        state_expected.rc = (uint16_t)(body_index > UINT32_C(1)
                                           ? body_index - UINT32_C(2)
                                           : UINT32_C(0));
        TI_C54X_TEST_CHECK(memcmp(&state, &state_expected, sizeof(state)) == 0);
        TI_C54X_TEST_CHECK_U64(state_before.rc, body_index - UINT32_C(1));
        TI_C54X_TEST_CHECK_U64(progress.body_executions_remaining,
                               body_index - UINT32_C(1));
        TI_C54X_TEST_CHECK(progress.phase ==
                           (body_index == UINT32_C(1)
                                ? TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE
                                : TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE));
    }
    {
        uint16_t rc = UINT16_C(0xFFFF);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, &rc) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(rc, UINT16_C(0));
    }
}

int
main(void)
{
    test_operand_progression(UINT16_C(0));
    test_operand_progression(UINT16_C(1));
    test_operand_progression(UINT16_C(2));
    test_operand_progression(UINT16_MAX);
    puts("single_repeat_rc_observable_progression: ok");
    return 0;
}
