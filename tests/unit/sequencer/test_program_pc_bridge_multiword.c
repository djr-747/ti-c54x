/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for explicit sequential
 * program-word widths through the private PC planner and public-carrier
 * bridge. It proves all widths, checked boundaries, high-carrier rejection,
 * and transactional malformed/stale effect behavior.
 * Does not own: CPU orchestration, bus fetch, opcode decode or dispatch,
 * timing, XPC/page policy, architectural state, observers, or diagnostics.
 */

#include "sequencer/program_pc_bridge.h"
#include "sequencer/sequential_pc.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_sequential_all_width_boundaries(void)
{
    uint32_t width;

    for (width = UINT32_C(1); width <= (uint32_t)UINT8_MAX; ++width) {
        const uint8_t count = (uint8_t)width;
        const uint16_t max_source =
            (uint16_t)((uint32_t)UINT16_MAX - width);
        ti_c54x_sequential_pc_effect staged = {0};
        uint16_t pc_word;

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                               UINT16_C(0), count, &staged) ==
                           TI_C54X_SEQUENTIAL_PC_OK);
        TI_C54X_TEST_CHECK_U64(staged.pc_word_before, UINT16_C(0));
        TI_C54X_TEST_CHECK_U64(staged.pc_word_after, width);
        TI_C54X_TEST_CHECK_U64(staged.program_word_count, count);
        pc_word = UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                               &pc_word, &staged) ==
                           TI_C54X_SEQUENTIAL_PC_OK);
        TI_C54X_TEST_CHECK_U64(pc_word, width);

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                               max_source, count, &staged) ==
                           TI_C54X_SEQUENTIAL_PC_OK);
        TI_C54X_TEST_CHECK_U64(staged.pc_word_after, UINT16_MAX);
        pc_word = max_source;
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                               &pc_word, &staged) ==
                           TI_C54X_SEQUENTIAL_PC_OK);
        TI_C54X_TEST_CHECK_U64(pc_word, UINT16_MAX);

        if (max_source < UINT16_MAX) {
            const uint16_t unresolved_source = (uint16_t)(max_source + 1U);
            const ti_c54x_sequential_pc_effect before = staged;

            TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                                   unresolved_source, count, &staged) ==
                               TI_C54X_SEQUENTIAL_PC_UNRESOLVED_BOUNDARY);
            TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof(staged)) == 0);
        }
    }
}

static void
test_sequential_rejects_zero_and_preserves_malformed_effects(void)
{
    const ti_c54x_sequential_pc_effect sentinel = {
        .pc_word_before = UINT16_C(0x1234),
        .pc_word_after = UINT16_C(0x5678),
        .program_word_count = UINT8_C(99)
    };
    ti_c54x_sequential_pc_effect staged = sentinel;
    uint16_t pc_word = UINT16_C(0x1234);

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                           UINT16_C(0), UINT8_C(0), &staged) ==
                       TI_C54X_SEQUENTIAL_PC_UNSUPPORTED_PROGRAM_WORD_COUNT);
    TI_C54X_TEST_CHECK(memcmp(&staged, &sentinel, sizeof(staged)) == 0);

    {
        const ti_c54x_sequential_pc_effect malformed[] = {
            {.pc_word_before = UINT16_C(0),
             .pc_word_after = UINT16_C(1),
             .program_word_count = UINT8_C(0)},
            {.pc_word_before = UINT16_C(0xfffe),
             .pc_word_after = UINT16_C(0xffff),
             .program_word_count = UINT8_C(2)},
            {.pc_word_before = UINT16_C(0x1234),
             .pc_word_after = UINT16_C(0x1235),
             .program_word_count = UINT8_C(2)},
            {.pc_word_before = UINT16_C(0xfffe),
             .pc_word_after = UINT16_C(0),
             .program_word_count = UINT8_C(1)},
        };
        size_t index;

        for (index = 0; index < sizeof(malformed) / sizeof(malformed[0]);
             ++index) {
            const uint16_t before = pc_word;

            TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                                   &pc_word, &malformed[index]) ==
                               TI_C54X_SEQUENTIAL_PC_INVALID_EFFECT);
            TI_C54X_TEST_CHECK_U64(pc_word, before);
        }
    }
}

static void
test_bridge_all_width_boundaries_and_high_carriers(void)
{
    uint32_t width;

    for (width = UINT32_C(1); width <= (uint32_t)UINT8_MAX; ++width) {
        const uint8_t count = (uint8_t)width;
        const uint32_t max_source = (uint32_t)UINT16_MAX - width;
        ti_c54x_program_pc_bridge_effect staged = {0};
        uint32_t public_pc;

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                               UINT32_C(0), count, &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK_U64(staged.public_pc_word_after, width);
        TI_C54X_TEST_CHECK_U64(staged.program_word_count, count);
        public_pc = UINT32_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK_U64(public_pc, width);

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                               max_source, count, &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK_U64(staged.public_pc_word_after, UINT16_MAX);
        public_pc = max_source;
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK_U64(public_pc, UINT16_MAX);
        if (max_source < (uint32_t)UINT16_MAX) {
            const uint32_t unresolved_source = max_source + UINT32_C(1);
            const ti_c54x_program_pc_bridge_effect before = staged;

            TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                                   unresolved_source, count, &staged) ==
                               TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY);
            TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof(staged)) == 0);
        }
    }

    {
        const uint32_t high_carriers[] = {
            UINT32_C(0x10000), UINT32_C(0x10001), UINT32_MAX};
        ti_c54x_program_pc_bridge_effect staged = {
            .public_pc_word_before = UINT32_C(0x1357),
            .common_pc_word_before = UINT16_C(0x1357),
            .common_pc_word_after = UINT16_C(0x2468),
            .public_pc_word_after = UINT32_C(0x2468),
            .program_word_count = UINT8_C(77)
        };
        const ti_c54x_program_pc_bridge_effect before = staged;
        size_t index;

        for (index = 0; index < sizeof(high_carriers) /
                                  sizeof(high_carriers[0]); ++index) {
            staged = before;
            TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                                   high_carriers[index], UINT8_C(2), &staged) ==
                               TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER);
            TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof(staged)) == 0);
        }
    }
}

static void
test_bridge_rejects_zero_and_transactionally_validates_effects(void)
{
    const ti_c54x_program_pc_bridge_effect sentinel = {
        .public_pc_word_before = UINT32_C(0x1234),
        .common_pc_word_before = UINT16_C(0x1234),
        .common_pc_word_after = UINT16_C(0x5678),
        .public_pc_word_after = UINT32_C(0x5678),
        .program_word_count = UINT8_C(99)
    };
    ti_c54x_program_pc_bridge_effect staged = sentinel;

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(0), UINT8_C(0), &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT);
    TI_C54X_TEST_CHECK(memcmp(&staged, &sentinel, sizeof(staged)) == 0);

    {
        const ti_c54x_program_pc_bridge_effect malformed[] = {
            {.public_pc_word_before = UINT32_C(0),
             .common_pc_word_before = UINT16_C(0),
             .common_pc_word_after = UINT16_C(1),
             .public_pc_word_after = UINT32_C(1),
             .program_word_count = UINT8_C(0)},
            {.public_pc_word_before = UINT32_C(0xfffe),
             .common_pc_word_before = UINT16_C(0xfffe),
             .common_pc_word_after = UINT16_C(0xffff),
             .public_pc_word_after = UINT32_C(0xffff),
             .program_word_count = UINT8_C(2)},
            {.public_pc_word_before = UINT32_C(0x1234),
             .common_pc_word_before = UINT16_C(0x1234),
             .common_pc_word_after = UINT16_C(0x1235),
             .public_pc_word_after = UINT32_C(0x1235),
             .program_word_count = UINT8_C(2)},
            {.public_pc_word_before = UINT32_C(0x1234),
             .common_pc_word_before = UINT16_C(0x1234),
             .common_pc_word_after = UINT16_C(0x1235),
             .public_pc_word_after = UINT32_C(0x1236),
             .program_word_count = UINT8_C(1)},
            {.public_pc_word_before = UINT32_C(0xffff),
             .common_pc_word_before = UINT16_C(0xffff),
             .common_pc_word_after = UINT16_C(0),
             .public_pc_word_after = UINT32_C(0),
             .program_word_count = UINT8_C(1)},
        };
        size_t index;

        for (index = 0; index < sizeof(malformed) / sizeof(malformed[0]);
             ++index) {
            uint32_t public_pc = UINT32_C(0x4321);

            TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                                   &public_pc, &malformed[index]) ==
                               TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT);
            TI_C54X_TEST_CHECK_U64(public_pc, UINT32_C(0x4321));
        }
    }

    {
        ti_c54x_program_pc_bridge_effect stale_staged = {0};
        uint32_t public_pc = UINT32_C(0xfffc);

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                               UINT32_C(0xfffd), UINT8_C(2),
                               &stale_staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, &stale_staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE);
        TI_C54X_TEST_CHECK_U64(public_pc, UINT32_C(0xfffc));
    }
}

int
main(void)
{
    test_sequential_all_width_boundaries();
    test_sequential_rejects_zero_and_preserves_malformed_effects();
    test_bridge_all_width_boundaries_and_high_carriers();
    test_bridge_rejects_zero_and_transactionally_validates_effects();
    puts("program_pc_bridge_multiword: ok");
    return 0;
}
