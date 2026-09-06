/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for the private program-PC
 * bridge. It proves public-carrier validation, one-word width policy,
 * boundary handling, value-owned effect shape, stale-source rejection, and
 * bytewise transactional behavior across the complete 16-bit domain.
 * Does not own: CPU orchestration, bus fetch, opcode decode or dispatch,
 * timing, XPC/page policy, architectural state, observers, or diagnostics.
 */

#include "sequencer/program_pc_bridge.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_every_admitted_source_plans_and_applies(void)
{
    uint32_t source;

    for (source = UINT32_C(0); source <= UINT32_C(0xfffe); ++source) {
        ti_c54x_program_pc_bridge_effect staged = {0};
        uint32_t public_pc = source;

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                               source, UINT8_C(1), &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK_U64(staged.public_pc_word_before, source);
        TI_C54X_TEST_CHECK_U64(staged.common_pc_word_before, source);
        TI_C54X_TEST_CHECK_U64(staged.common_pc_word_after, source + 1U);
        TI_C54X_TEST_CHECK_U64(staged.public_pc_word_after, source + 1U);
        TI_C54X_TEST_CHECK_U64(staged.program_word_count, UINT8_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_OK);
        TI_C54X_TEST_CHECK_U64(public_pc, source + 1U);
    }
}

static void
test_boundary_width_carrier_and_null_precedence(void)
{
    const ti_c54x_program_pc_bridge_effect sentinel = {
        .public_pc_word_before = UINT32_C(0x1357),
        .common_pc_word_before = UINT16_C(0x1357),
        .common_pc_word_after = UINT16_C(0x2468),
        .public_pc_word_after = UINT32_C(0x2468),
        .program_word_count = UINT8_C(7)
    };
    ti_c54x_program_pc_bridge_effect staged = sentinel;
    const ti_c54x_program_pc_bridge_effect before = staged;
    const uint8_t invalid_widths[] = {UINT8_C(0), UINT8_C(2), UINT8_MAX};
    const size_t invalid_width_count =
        sizeof(invalid_widths) / sizeof(invalid_widths[0]);
    size_t index;

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(0xffff), UINT8_C(1), &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY);
    TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof(staged)) == 0);

    staged = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(0xfffe), UINT8_C(1), &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_OK);
    TI_C54X_TEST_CHECK_U64(staged.common_pc_word_after, UINT16_C(0xffff));
    TI_C54X_TEST_CHECK_U64(staged.public_pc_word_after, UINT32_C(0xffff));

    for (index = 0; index < invalid_width_count; ++index) {
        staged = sentinel;
        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                               UINT32_C(0x1234), invalid_widths[index],
                               &staged) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT);
        TI_C54X_TEST_CHECK(memcmp(&staged, &sentinel, sizeof(staged)) == 0);
    }

    staged = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(0x10000), UINT8_C(1), &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER);
    TI_C54X_TEST_CHECK(memcmp(&staged, &sentinel, sizeof(staged)) == 0);

    staged = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_MAX, UINT8_C(1), &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER);
    TI_C54X_TEST_CHECK(memcmp(&staged, &sentinel, sizeof(staged)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_plan(
                           UINT32_C(0), UINT8_C(1), NULL) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
}

static void
test_invalid_effect_precedes_stale_and_is_transactional(void)
{
    const ti_c54x_program_pc_bridge_effect valid = {
        .public_pc_word_before = UINT32_C(0x1234),
        .common_pc_word_before = UINT16_C(0x1234),
        .common_pc_word_after = UINT16_C(0x1235),
        .public_pc_word_after = UINT32_C(0x1235),
        .program_word_count = UINT8_C(1)
    };
    ti_c54x_program_pc_bridge_effect malformed[] = {
        {.public_pc_word_before = UINT32_C(0x1234),
         .common_pc_word_before = UINT16_C(0x1234),
         .common_pc_word_after = UINT16_C(0x1235),
         .public_pc_word_after = UINT32_C(0x1235),
         .program_word_count = UINT8_C(0)},
        {.public_pc_word_before = UINT32_C(0x1234),
         .common_pc_word_before = UINT16_C(0x1234),
         .common_pc_word_after = UINT16_C(0x1235),
         .public_pc_word_after = UINT32_C(0x1235),
         .program_word_count = UINT8_C(2)},
        {.public_pc_word_before = UINT32_C(0x10000),
         .common_pc_word_before = UINT16_C(0),
         .common_pc_word_after = UINT16_C(1),
         .public_pc_word_after = UINT32_C(1),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(0x1234),
         .common_pc_word_before = UINT16_C(0x1234),
         .common_pc_word_after = UINT16_C(0x1235),
         .public_pc_word_after = UINT32_C(0x1236),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(0x1234),
         .common_pc_word_before = UINT16_C(0x1234),
         .common_pc_word_after = UINT16_C(0x1235),
         .public_pc_word_after = UINT32_C(0x10000),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(0x1234),
         .common_pc_word_before = UINT16_C(0x1233),
         .common_pc_word_after = UINT16_C(0x1234),
         .public_pc_word_after = UINT32_C(0x1234),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(0x1234),
         .common_pc_word_before = UINT16_C(0x1234),
         .common_pc_word_after = UINT16_C(0x1236),
         .public_pc_word_after = UINT32_C(0x1236),
         .program_word_count = UINT8_C(1)},
        {.public_pc_word_before = UINT32_C(0xffff),
         .common_pc_word_before = UINT16_C(0xffff),
         .common_pc_word_after = UINT16_C(0),
         .public_pc_word_after = UINT32_C(0),
         .program_word_count = UINT8_C(1)}
    };
    size_t index;

    for (index = 0; index < sizeof(malformed) / sizeof(malformed[0]);
         ++index) {
        uint32_t public_pc = UINT32_C(0x4321);
        const uint32_t before = public_pc;

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, &malformed[index]) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT);
        TI_C54X_TEST_CHECK_U64(public_pc, before);
    }

    {
        uint32_t public_pc = UINT32_C(0x4321);
        const uint32_t before = public_pc;

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, &valid) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE);
        TI_C54X_TEST_CHECK_U64(public_pc, before);
    }

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                           NULL, &valid) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
    {
        uint32_t public_pc = UINT32_C(0x1234);

        TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                               &public_pc, NULL) ==
                           TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK_U64(public_pc, UINT32_C(0x1234));
    }
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                           NULL, NULL) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT);
}

static void
test_apply_commits_public_carrier_once(void)
{
    const ti_c54x_program_pc_bridge_effect staged = {
        .public_pc_word_before = UINT32_C(0xfffe),
        .common_pc_word_before = UINT16_C(0xfffe),
        .common_pc_word_after = UINT16_C(0xffff),
        .public_pc_word_after = UINT32_C(0xffff),
        .program_word_count = UINT8_C(1)
    };
    uint32_t public_pc = UINT32_C(0xfffe);

    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                           &public_pc, &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_OK);
    TI_C54X_TEST_CHECK_U64(public_pc, UINT32_C(0xffff));
    TI_C54X_TEST_CHECK(ti_c54x_program_pc_bridge_apply(
                           &public_pc, &staged) ==
                       TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE);
    TI_C54X_TEST_CHECK_U64(public_pc, UINT32_C(0xffff));
}

int
main(void)
{
    test_every_admitted_source_plans_and_applies();
    test_boundary_width_carrier_and_null_precedence();
    test_invalid_effect_precedes_stale_and_is_transactional();
    test_apply_commits_public_carrier_once();
    puts("program_pc_bridge: ok");
    return 0;
}
