/*
 * Module contract
 *
 * Owns: executable equivalence proof that checked and trusted commits of one
 * successfully planned public/common PC transition agree for all admitted
 * carriers and widths, while checked planning retains all rejection and
 * no-mutation guarantees. Does not own: decode, opcode semantics, CPU
 * orchestration, buses, timing, diagnostics, or public address policy.
 */

#include "sequencer/program_pc_bridge.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_exhaustive_checked_trusted_equivalence(void)
{
    uint32_t source;

    for (source = UINT32_C(0); source <= (uint32_t)UINT16_MAX; ++source) {
        uint16_t width;

        for (width = UINT16_C(1); width <= UINT16_C(UINT8_MAX); ++width) {
            ti_c54x_program_pc_bridge_effect effect = {0};
            ti_c54x_program_pc_bridge_effect trusted_effect = {0};
            const ti_c54x_program_pc_bridge_status plan_status =
                ti_c54x_program_pc_bridge_plan(
                    source, (uint8_t)width, &effect);
            const ti_c54x_program_pc_bridge_status trusted_plan_status =
                ti_c54x_program_pc_bridge_plan_trusted(
                    source, (uint8_t)width, &trusted_effect);

            TI_C54X_TEST_CHECK(trusted_plan_status == plan_status);

            if (plan_status == TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                uint32_t checked = source;
                uint32_t trusted = source;

                TI_C54X_TEST_CHECK(
                    trusted_effect.public_pc_word_before ==
                    effect.public_pc_word_before);
                TI_C54X_TEST_CHECK(
                    trusted_effect.common_pc_word_before ==
                    effect.common_pc_word_before);
                TI_C54X_TEST_CHECK(
                    trusted_effect.common_pc_word_after ==
                    effect.common_pc_word_after);
                TI_C54X_TEST_CHECK(
                    trusted_effect.public_pc_word_after ==
                    effect.public_pc_word_after);
                TI_C54X_TEST_CHECK(trusted_effect.program_word_count ==
                                    effect.program_word_count);

                TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                                       &checked, &effect) ==
                                   TI_C54X_PROGRAM_PC_BRIDGE_OK);
                TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply_trusted(
                                       &trusted, &trusted_effect) ==
                                   TI_C54X_PROGRAM_PC_BRIDGE_OK);
                TI_C54X_TEST_CHECK(trusted == checked);
            } else {
                TI_C54X_TEST_CHECK(
                    plan_status ==
                    TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY);
            }
        }
    }
}

static void test_rejected_plans_preserve_output(void)
{
    const ti_c54x_program_pc_bridge_effect sentinel = {
        .public_pc_word_before = UINT32_C(0x1357),
        .common_pc_word_before = UINT16_C(0x1357),
        .common_pc_word_after = UINT16_C(0x2468),
        .public_pc_word_after = UINT32_C(0x2468),
        .program_word_count = UINT8_C(7)};
    const struct {
        uint32_t source;
        uint8_t width;
        ti_c54x_program_pc_bridge_status status;
    } rejected[] = {
        {UINT32_C(0x1234), UINT8_C(0),
         TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT},
        {UINT32_C(0xffff), UINT8_C(1),
         TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY},
        {UINT32_C(0xff01), UINT8_C(255),
         TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY},
        {UINT32_C(0x10000), UINT8_C(1),
         TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER},
        {UINT32_MAX, UINT8_C(255),
         TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER},
    };
    size_t index;

    for (index = 0U; index < sizeof(rejected) / sizeof(rejected[0]); ++index) {
        ti_c54x_program_pc_bridge_effect effect = sentinel;
        ti_c54x_program_pc_bridge_effect trusted_effect = sentinel;

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                               rejected[index].source, rejected[index].width,
                               &effect) == rejected[index].status);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan_trusted(
                               rejected[index].source, rejected[index].width,
                               &trusted_effect) == rejected[index].status);
        TI_C54X_TEST_CHECK(memcmp(&trusted_effect, &sentinel,
                                  sizeof(trusted_effect)) == 0);
    }

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(0), UINT8_C(1), NULL) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan_trusted(
                           UINT32_C(0), UINT8_C(1), NULL) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
}

static void test_stale_and_forged_commits_are_transactional(void)
{
    const ti_c54x_program_pc_bridge_effect forged[] = {
        {.public_pc_word_before = UINT32_C(12),
         .common_pc_word_before = UINT16_C(12),
         .common_pc_word_after = UINT16_C(14),
         .public_pc_word_after = UINT32_C(14),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(12),
         .common_pc_word_before = UINT16_C(12),
         .common_pc_word_after = UINT16_C(13),
         .public_pc_word_after = UINT32_C(14),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(12),
         .common_pc_word_before = UINT16_C(12),
         .common_pc_word_after = UINT16_C(13),
         .public_pc_word_after = UINT32_C(13),
         .program_word_count = UINT8_C(0)},
    };
    ti_c54x_program_pc_bridge_effect effect;
    uint32_t pc;
    size_t index;

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(12), UINT8_C(2), &effect) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_OK);

    pc = UINT32_C(12);
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply_trusted(
                       NULL, &effect) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply_trusted(
                           &pc, NULL) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
    pc = UINT32_C(13);
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply_trusted(
                           &pc, &effect) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE);
    TI_C54X_TEST_CHECK(pc == UINT32_C(13));

    pc = UINT32_C(13);
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                           &pc, &effect) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE);
    TI_C54X_TEST_CHECK(pc == UINT32_C(13));

    for (index = 0U; index < sizeof(forged) / sizeof(forged[0]); ++index) {
        pc = UINT32_C(12);
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &pc, &forged[index]) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT);
        TI_C54X_TEST_CHECK(pc == UINT32_C(12));
    }
}

int main(void)
{
    test_exhaustive_checked_trusted_equivalence();
    test_rejected_plans_preserve_output();
    test_stale_and_forged_commits_are_transactional();
    puts("trusted_pc_commit: ok");
    return 0;
}
