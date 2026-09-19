/*
 * Module contract
 *
 * Owns: executable proof for the block-repeat driver
 * that composes block start (BRAF plus RSA/REA
 * programming with BRC-knownness and delay-slot
 * guards) and body-retired (REA fall-through LOOP
 * versus END) decisions.
 * Does not own: catalog rows, decode, dispatch, MMR
 * accessors, PC writes, CPU run flow, timing, buses,
 * callbacks, allocation, or diagnostics.
 */

#include "sequencer/repeat/repeat_block.h"
#include "state/arch_state.h"
#include "state/mmr_file.h"
#include "state/status/registers.h"
#include "isa/ops/op_effect.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_arch_state
reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_effect
block_start_effect(uint16_t pmad)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_REPEAT_BLOCK_START;
    effect.payload.repeat_block_start.pmad = pmad;
    return effect;
}

static void
seed_brc(ti_c54x_arch_state *state, uint16_t brc)
{
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_brc(&state->mmr_file, brc) ==
                       TI_C54X_MMR_FILE_OK);
}

static uint16_t
read_cell(ti_c54x_mmr_file_status (*read)(
                  const ti_c54x_mmr_file_state *, uint16_t *),
              const ti_c54x_arch_state *state)
{
    uint16_t value = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(read(&state->mmr_file, &value) ==
                       TI_C54X_MMR_FILE_OK);
    return value;
}

static uint8_t
read_braf(const ti_c54x_arch_state *state)
{
    ti_c54x_status_register_field_result field =
        ti_c54x_status_register_read_braf(&state->status);

    TI_C54X_TEST_CHECK(field.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(field.known);
    return (uint8_t)(field.raw_value != UINT16_C(0));
}

static void
test_start_null_and_kind_reject(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_op_effect effect = block_start_effect(UINT16_C(0x803A));
    ti_c54x_op_effect wrong = {0};

    wrong.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           NULL, &effect, UINT16_C(0x8031), &state,
                           false) ==
                       TI_C54X_REPEAT_BLOCK_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, NULL, UINT16_C(0x8031), &state,
                           false) ==
                       TI_C54X_REPEAT_BLOCK_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &effect, UINT16_C(0x8031), NULL,
                           false) ==
                       TI_C54X_REPEAT_BLOCK_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &wrong, UINT16_C(0x8031), &state,
                           false) ==
                       TI_C54X_REPEAT_BLOCK_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_IDLE);
}

static void
test_start_unknown_brc_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_op_effect effect = block_start_effect(UINT16_C(0x803A));

    /* Reset leaves BRC unknown: no RSA/REA/BRAF mutation. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &effect, UINT16_C(0x8031), &state,
                           false) == TI_C54X_REPEAT_BLOCK_UNKNOWN_BRC);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_IDLE);
    TI_C54X_TEST_CHECK(read_braf(&state) == UINT8_C(0));
}

static void
test_start_in_delay_slots_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_op_effect effect = block_start_effect(UINT16_C(0x803A));

    seed_brc(&state, UINT16_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &effect, UINT16_C(0x8031), &state,
                           true) == TI_C54X_REPEAT_BLOCK_INVALID_STATE);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_IDLE);
    TI_C54X_TEST_CHECK(read_braf(&state) == UINT8_C(0));
}

static void
test_start_programs_block(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_op_effect effect = block_start_effect(UINT16_C(0x803A));

    seed_brc(&state, UINT16_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &effect, UINT16_C(0x8031), &state,
                           false) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_rsa, &state) ==
                       UINT16_C(0x8031));
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_rea, &state) ==
                       UINT16_C(0x803A));
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_brc, &state) ==
                       UINT16_C(3));
    TI_C54X_TEST_CHECK(read_braf(&state) == UINT8_C(1));
}

static void
test_nested_start_overwrites(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_op_effect first = block_start_effect(UINT16_C(0x803A));
    ti_c54x_op_effect second = block_start_effect(UINT16_C(0x8100));

    seed_brc(&state, UINT16_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &first, UINT16_C(0x8031), &state,
                           false) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &second, UINT16_C(0x8050), &state,
                           false) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_rsa, &state) ==
                       UINT16_C(0x8050));
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_rea, &state) ==
                       UINT16_C(0x8100));
}

static ti_c54x_repeat_block_progress
active_block(ti_c54x_arch_state *state, uint16_t brc)
{
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_op_effect effect = block_start_effect(UINT16_C(0x803A));

    seed_brc(state, brc);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_start(
                           &progress, &effect, UINT16_C(0x8031), state,
                           false) == TI_C54X_REPEAT_BLOCK_OK);
    return progress;
}

static void
test_retired_off_rea_noops(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress =
        active_block(&state, UINT16_C(3));
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0xBEEF);

    /* Mid-block retirement: no action, no BRC change. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x8035),
                           true, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE);
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_brc, &state) ==
                       UINT16_C(3));
    TI_C54X_TEST_CHECK(read_braf(&state) == UINT8_C(1));
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE);
}

static void
test_retired_rea_branch_taken_noops(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress =
        active_block(&state, UINT16_C(3));
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0xBEEF);

    /* REA retired but left non-sequentially: the block is
     * abandoned, not looped. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           false, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE);
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_brc, &state) ==
                       UINT16_C(3));
}

static void
test_retired_rea_armed_noops(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress =
        active_block(&state, UINT16_C(3));
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0xBEEF);

    /* REA fall-through inside a delay countdown: the
     * pending branch owns the PC, not the loop. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           true, true, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE);
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_brc, &state) ==
                       UINT16_C(3));
}

static void
test_retired_rea_loops_then_ends(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress =
        active_block(&state, UINT16_C(2));
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0xBEEF);

    /* BRC=2: two loop-backs, then END on the third pass. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           true, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_LOOP);
    TI_C54X_TEST_CHECK(loop_pc == UINT16_C(0x8031));
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_brc, &state) ==
                       UINT16_C(1));
    TI_C54X_TEST_CHECK(read_braf(&state) == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           true, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_LOOP);
    TI_C54X_TEST_CHECK(read_cell(ti_c54x_mmr_file_read_brc, &state) ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           true, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_END);
    TI_C54X_TEST_CHECK(read_braf(&state) == UINT8_C(0));
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_BLOCK_PHASE_IDLE);
    /* Ended: further REA hits stay NONE. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           true, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE);
}

static void
test_retired_idle_noops(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_block_progress progress = {0};
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_repeat_block_body_retired(
                           &progress, &state, UINT32_C(0x803A),
                           true, false, &action,
                           &loop_pc) == TI_C54X_REPEAT_BLOCK_OK);
    TI_C54X_TEST_CHECK(action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE);
}

int
main(void)
{
    test_start_null_and_kind_reject();
    test_start_unknown_brc_rejects();
    test_start_in_delay_slots_rejects();
    test_start_programs_block();
    test_nested_start_overwrites();
    test_retired_off_rea_noops();
    test_retired_rea_branch_taken_noops();
    test_retired_rea_armed_noops();
    test_retired_rea_loops_then_ends();
    test_retired_idle_noops();
    return 0;
}
