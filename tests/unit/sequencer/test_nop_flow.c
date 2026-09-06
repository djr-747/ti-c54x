/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof for the private sequential 16-bit PC
 * retirement seam used by the one-word NOP flow. It covers every non-boundary
 * source, the unresolved boundary, malformed staged effects, stale sources,
 * and bytewise transactional behavior.
 * Does not own: opcode catalog/decode, NOP semantic dispatch, public CPU
 * carrier policy, timing, bus transfers, memory, observers, or diagnostics.
 */

#include "sequencer/sequential_pc.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_every_non_boundary_source_plans_and_commits(void)
{
    uint32_t source;

    for (source = UINT32_C(0); source <= UINT32_C(0xfffe); ++source) {
        const uint16_t source_word = (uint16_t)source;
        const uint16_t expected_next = (uint16_t)(source + UINT32_C(1));
        ti_c54x_sequential_pc_effect staged = {0};
        uint16_t pc_word = source_word;

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                               source_word, UINT8_C(1), &staged) ==
                           TI_C54X_SEQUENTIAL_PC_OK);
        TI_C54X_TEST_CHECK_U64(staged.pc_word_before, source_word);
        TI_C54X_TEST_CHECK_U64(staged.pc_word_after, expected_next);
        TI_C54X_TEST_CHECK_U64(staged.program_word_count, UINT8_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                               &pc_word, &staged) ==
                           TI_C54X_SEQUENTIAL_PC_OK);
        TI_C54X_TEST_CHECK_U64(pc_word, expected_next);
    }
}

static void
test_boundary_and_invalid_arguments_preserve_outputs(void)
{
    ti_c54x_sequential_pc_effect staged = {
        .pc_word_before = UINT16_C(0x1357),
        .pc_word_after = UINT16_C(0x2468),
        .program_word_count = UINT8_C(7)
    };
    const ti_c54x_sequential_pc_effect before = staged;
    uint16_t pc_word = UINT16_C(0xffff);

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                           UINT16_C(0xffff), UINT8_C(1), &staged) ==
                       TI_C54X_SEQUENTIAL_PC_UNRESOLVED_BOUNDARY);
    TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof(staged)) == 0);
    TI_C54X_TEST_CHECK_U64(pc_word, UINT16_C(0xffff));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_sequential_pc(
                           UINT16_C(0), UINT8_C(1), NULL) ==
                       TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                           NULL, &staged) ==
                       TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                           &pc_word, NULL) ==
                       TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT);
}

static void
test_malformed_and_stale_effects_are_transactional(void)
{
    const ti_c54x_sequential_pc_effect malformed[] = {
        {.pc_word_before = UINT16_C(0),
         .pc_word_after = UINT16_C(1),
         .program_word_count = UINT8_C(0)},
        {.pc_word_before = UINT16_C(0),
         .pc_word_after = UINT16_C(1),
         .program_word_count = UINT8_C(2)},
        {.pc_word_before = UINT16_C(0xffff),
         .pc_word_after = UINT16_C(0),
         .program_word_count = UINT8_C(1)},
        {.pc_word_before = UINT16_C(0x1234),
         .pc_word_after = UINT16_C(0x1236),
         .program_word_count = UINT8_C(1)},
        {.pc_word_before = UINT16_C(0x1234),
         .pc_word_after = UINT16_C(0x1233),
         .program_word_count = UINT8_C(1)},
    };
    size_t index;

    for (index = 0; index < sizeof(malformed) / sizeof(malformed[0]); ++index) {
        uint16_t pc_word = UINT16_C(0x1234);
        const uint16_t before = pc_word;

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                               &pc_word, &malformed[index]) ==
                           TI_C54X_SEQUENTIAL_PC_INVALID_EFFECT);
        TI_C54X_TEST_CHECK_U64(pc_word, before);
    }

    {
        const ti_c54x_sequential_pc_effect staged = {
            .pc_word_before = UINT16_C(0x1234),
            .pc_word_after = UINT16_C(0x1235),
            .program_word_count = UINT8_C(1)
        };
        uint16_t pc_word = UINT16_C(0x4321);
        const uint16_t before = pc_word;

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_sequential_pc(
                               &pc_word, &staged) ==
                           TI_C54X_SEQUENTIAL_PC_STALE_SOURCE);
        TI_C54X_TEST_CHECK_U64(pc_word, before);
    }
}

int
main(void)
{
    test_every_non_boundary_source_plans_and_commits();
    test_boundary_and_invalid_arguments_preserve_outputs();
    test_malformed_and_stale_effects_are_transactional();
    puts("nop_flow: ok");
    return 0;
}
