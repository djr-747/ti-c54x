/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S1 MMR owner-access
 * seam: checked whole-word ST0/ST1/PMST read/write composed from the
 * status bank with full-mask knownness, and checked accumulator
 * low/high/guard word read/write composed from the canonical 40-bit
 * value with accumulator-granularity knownness. NULL inputs reject;
 * unknown owners read UNKNOWN; out-of-range word values reject.
 * Does not own: status or accumulator storage, MMR address mapping,
 * request routing, opcode semantics, CPU lifecycle, or timing.
 */

#include "state/accumulator/accumulator.h"
#include "state/status/registers.h"
#include "../../support/test_harness.h"

#include <stdint.h>

static void
test_status_whole_word_round_trip(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result read_back;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_st0(&state,
                                                         UINT16_C(0x01A5)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_st1(&state,
                                                         UINT16_C(0x3F00)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_pmst(&state,
                                                          UINT16_C(0x00E0)) ==
                       TI_C54X_STATUS_REGISTER_OK);

    read_back = ti_c54x_status_register_read_st0(&state);
    TI_C54X_TEST_CHECK(read_back.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(read_back.known);
    TI_C54X_TEST_CHECK_U64(read_back.raw_value, UINT16_C(0x01A5));
    read_back = ti_c54x_status_register_read_st1(&state);
    TI_C54X_TEST_CHECK(read_back.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(read_back.known);
    TI_C54X_TEST_CHECK_U64(read_back.raw_value, UINT16_C(0x3F00));
    read_back = ti_c54x_status_register_read_pmst(&state);
    TI_C54X_TEST_CHECK(read_back.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(read_back.known);
    TI_C54X_TEST_CHECK_U64(read_back.raw_value, UINT16_C(0x00E0));

    /* Whole-word writes compose through the field readers. */
    read_back = ti_c54x_status_register_read_dp(&state);
    TI_C54X_TEST_CHECK(read_back.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(read_back.known);
    TI_C54X_TEST_CHECK_U64(read_back.raw_value, UINT16_C(0x01A5));
}

static void
test_status_whole_word_partial_knownness(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result read_back;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    /* All-but-one known bits is not a known word. */
    state.st0_known_mask = (uint16_t)~UINT16_C(0x0001);
    read_back = ti_c54x_status_register_read_st0(&state);
    TI_C54X_TEST_CHECK(read_back.status == TI_C54X_STATUS_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(!read_back.known);
}

static void
test_status_whole_word_null_rejects(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_field_result read_back;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_st0(NULL,
                                                         UINT16_C(0)) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_st1(NULL,
                                                         UINT16_C(0)) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_pmst(NULL,
                                                          UINT16_C(0)) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    read_back = ti_c54x_status_register_read_st0(NULL);
    TI_C54X_TEST_CHECK(read_back.status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    read_back = ti_c54x_status_register_read_st1(NULL);
    TI_C54X_TEST_CHECK(read_back.status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    read_back = ti_c54x_status_register_read_pmst(NULL);
    TI_C54X_TEST_CHECK(read_back.status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
}

static void
test_accumulator_words_compose_canonical(void)
{
    ti_c54x_accumulator_state state;
    uint64_t canonical;
    uint16_t word;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AL,
                           UINT16_C(0x4321)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AH,
                           UINT16_C(0x8765)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AG,
                           UINT16_C(0x00FF)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_A,
                           &canonical) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(canonical, UINT64_C(0x00FF87654321));

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AL,
                           &word) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(word, UINT16_C(0x4321));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AH,
                           &word) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(word, UINT16_C(0x8765));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AG,
                           &word) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(word, UINT16_C(0x00FF));

    /* B words stay unknown and zero-touching until written. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_BL,
                           &word) == TI_C54X_ACCUMULATOR_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_BH,
                           UINT16_C(0x1111)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_B,
                           &canonical) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(canonical, UINT64_C(0x000011110000));
}

static void
test_accumulator_word_write_to_unknown_bases_zero(void)
{
    ti_c54x_accumulator_state state;
    uint64_t canonical;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_BG,
                           UINT16_C(0x0002)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &state, TI_C54X_ACCUMULATOR_B,
                           &canonical) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(canonical, UINT64_C(0x000200000000));
}

static void
test_accumulator_word_rejects(void)
{
    ti_c54x_accumulator_state state;
    uint16_t word = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           NULL, TI_C54X_ACCUMULATOR_WORD_AL,
                           &word) == TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AL,
                           NULL) == TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_INVALID,
                           &word) == TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           NULL, TI_C54X_ACCUMULATOR_WORD_AL,
                           UINT16_C(0)) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_INVALID,
                           UINT16_C(0)) ==
                       TI_C54X_ACCUMULATOR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write_word(
                           &state, TI_C54X_ACCUMULATOR_WORD_AG,
                           UINT16_C(0x0100)) ==
                       TI_C54X_ACCUMULATOR_INVALID_VALUE);
    TI_C54X_TEST_CHECK_U64(word, UINT16_C(0xBEEF));
}

int
main(void)
{
    test_status_whole_word_round_trip();
    test_status_whole_word_partial_knownness();
    test_status_whole_word_null_rejects();
    test_accumulator_words_compose_canonical();
    test_accumulator_word_write_to_unknown_bases_zero();
    test_accumulator_word_rejects();
    return 0;
}
