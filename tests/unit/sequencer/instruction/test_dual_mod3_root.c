/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for the dual-indirect WORD16 MOD3
 * root commit contract: validator acceptance of exact indexed/circular
 * commits against AR0/BK shadows, rejection of forged or
 * boundary-violating commits, alias discipline, and the re-frozen carrier
 * sizes.
 * Evidence: E-TI-C54X-SPRU131G-S5-5-4-T5-8-P5-20-5-21-DUAL-MOD3,
 * E-TI-C54X-SPRU131G-S5-5-4-2-P5-21-5-22-DUAL-MOD3-INDEXED,
 * E-TI-C54X-SPRU131G-S5-5-4-3-P5-21-5-22-DUAL-MOD3-CIRCULAR, and
 * E-TI-C54X-SPRU131G-S5-5-3-4-P5-15-5-16-CIRCULAR-STEP-RULE.
 * Does not own: production sequencing, address arithmetic, opcode dispatch,
 * CPU lifecycle, endpoint storage, timing, or architectural policy. The
 * generic commit publication path is unchanged and stays covered by the
 * existing W51B lifecycle contracts.
 */

#include "sequencer/instruction/memory_instruction_dual_candidate_validate.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/dual_address_resolution.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

/* W52B widens the persistent snapshot with AR0/BK shadows: MOD3 commits
 * cannot be rechecked from mode/selector/address shadows alone. W76
 * grows the candidate with the RTN input snapshot fields, so the root
 * holds at 352; the dual snapshot arm is unchanged at 52. S3d adds
 * the read-phase T-load flag to the memory effect, so the root holds
 * at 360. S6 grows the PORT pair snapshot with the DATA-leg
 * resolution (the generic slot aliases the snapshot in the candidate
 * union), so the root holds at 384. S44 grows the embedded input
 * snapshot with the TRN fields, so the root holds at 392. */
_Static_assert(sizeof(ti_c54x_dual_memory_address_snapshot) == 52U,
               "W52B snapshot carries requests, commits, AR0/BK shadows");
_Static_assert(sizeof(ti_c54x_memory_instruction_transaction) == 392U,
               "S44 LP64 root holds the TRN input fields");

static ti_c54x_op_memory_address_spec
dual_spec_modes(uint8_t x_ar,
                uint8_t x_mode,
                uint8_t y_ar,
                uint8_t y_mode)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16,
        .dual = {
            .x_mode = x_mode,
            .x_ar = x_ar,
            .y_mode = y_mode,
            .y_ar = y_ar,
        },
    };

    return spec;
}

static ti_c54x_dual_memory_address_snapshot
indexed_snapshot(void)
{
    ti_c54x_dual_memory_address_snapshot snapshot = {0};

    snapshot.x_request.address_word = UINT32_C(0x1000);
    snapshot.y_request.address_word = UINT32_C(0x2000);
    snapshot.x_commit.target = TI_C54X_ADDRESS_UPDATE_AR;
    snapshot.x_commit.ar_index = UINT8_C(2);
    snapshot.x_commit.value_after = UINT16_C(0x1010);
    snapshot.y_commit.target = TI_C54X_ADDRESS_UPDATE_NONE;
    snapshot.ar0_shadow = UINT16_C(0x0010);
    snapshot.bk_shadow = UINT16_C(0);
    return snapshot;
}

static ti_c54x_dual_memory_address_snapshot
circular_snapshot(void)
{
    ti_c54x_dual_memory_address_snapshot snapshot = {0};

    snapshot.x_request.address_word = UINT32_C(0x0107);
    snapshot.y_request.address_word = UINT32_C(0x2000);
    snapshot.x_commit.target = TI_C54X_ADDRESS_UPDATE_AR;
    snapshot.x_commit.ar_index = UINT8_C(2);
    snapshot.x_commit.value_after = UINT16_C(0x0101);
    snapshot.y_commit.target = TI_C54X_ADDRESS_UPDATE_NONE;
    snapshot.ar0_shadow = UINT16_C(0x0002);
    snapshot.bk_shadow = UINT16_C(8);
    return snapshot;
}

static void
test_indexed_commit_accepted(void)
{
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(3), UINT8_C(0));
    const ti_c54x_dual_memory_address_snapshot snapshot = indexed_snapshot();

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
                           &snapshot, &spec));
}

static void
test_circular_commit_accepted(void)
{
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(3), UINT8_C(0));
    const ti_c54x_dual_memory_address_snapshot snapshot =
        circular_snapshot();

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
                           &snapshot, &spec));
}

static void
test_forged_commit_and_shadow_rejected(void)
{
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(3), UINT8_C(0));
    ti_c54x_dual_memory_address_snapshot bad_value = indexed_snapshot();
    ti_c54x_dual_memory_address_snapshot bad_ar0 = indexed_snapshot();
    ti_c54x_dual_memory_address_snapshot step_bound = circular_snapshot();

    bad_value.x_commit.value_after = UINT16_C(0x1011);
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
        &bad_value, &spec));
    bad_ar0.ar0_shadow = UINT16_C(0x0011);
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
        &bad_ar0, &spec));
    /* A shadow combination the planner would have rejected as a step-bound
     * violation cannot validate either. */
    step_bound.ar0_shadow = UINT16_C(0x0009);
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
        &step_bound, &spec));
}

static void
test_aliased_mod3_y_carries_no_update(void)
{
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(2), UINT8_C(3));
    ti_c54x_dual_memory_address_snapshot clean = circular_snapshot();
    ti_c54x_dual_memory_address_snapshot dirty = circular_snapshot();

    clean.y_request.address_word = UINT32_C(0x0107);
    dirty.y_request.address_word = UINT32_C(0x0107);
    dirty.y_commit.target = TI_C54X_ADDRESS_UPDATE_AR;
    dirty.y_commit.ar_index = UINT8_C(2);
    dirty.y_commit.value_after = UINT16_C(0x0101);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
                           &clean, &spec));
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
        &dirty, &spec));
}

static void
test_mod1_regression_unchanged(void)
{
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(1), UINT8_C(3), UINT8_C(0));
    ti_c54x_dual_memory_address_snapshot snapshot = {0};

    snapshot.x_request.address_word = UINT32_C(0x1000);
    snapshot.y_request.address_word = UINT32_C(0x2000);
    snapshot.x_commit.target = TI_C54X_ADDRESS_UPDATE_AR;
    snapshot.x_commit.ar_index = UINT8_C(2);
    snapshot.x_commit.value_after = UINT16_C(0x0FFF);
    snapshot.y_commit.target = TI_C54X_ADDRESS_UPDATE_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
                           &snapshot, &spec));
}

int
main(void)
{
    test_indexed_commit_accepted();
    test_circular_commit_accepted();
    test_forged_commit_and_shadow_rejected();
    test_aliased_mod3_y_carries_no_update();
    test_mod1_regression_unchanged();
    puts("dual_mod3_root: ok");
    return 0;
}
