/*
 * Module contract
 *
 * Owns: executable proof of complete-instruction binding, copied bundle
 * ownership, every public validation dimension, and transactional output.
 * Does not own: catalog definitions, fetch or bus behavior, CPU/state or
 * program-counter mutation, operand interpretation, opcode dispatch, timing,
 * observers, diagnostics, or the test harness.
 */

#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_bytes_unchanged(const unsigned char *before,
                      const ti_c54x_decoded_instruction *after)
{
    TI_C54X_TEST_CHECK(memcmp(before, after, sizeof(*after)) == 0);
}

static ti_c54x_decode_result
nop_result(void)
{
    return ti_c54x_decode_first_word(UINT16_C(0xF495));
}

static ti_c54x_instruction_word_bundle
nop_bundle(void)
{
    return (ti_c54x_instruction_word_bundle){
        .program_words = {UINT16_C(0xF495), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
}

static void
test_nop_completion_copies_the_complete_bundle(void)
{
    ti_c54x_decode_result first = nop_result();
    ti_c54x_instruction_word_bundle bundle = nop_bundle();
    ti_c54x_decoded_instruction decoded = {0};

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &decoded) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(decoded.id == TI_C54X_OPCODE_ID_NOP);
    TI_C54X_TEST_CHECK(decoded.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(decoded.bundle.program_words[0] == UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(decoded.bundle.program_words[1] == UINT16_C(0));

    first.first_word = UINT16_C(0);
    bundle.program_words[0] = UINT16_C(0);
    bundle.program_words[1] = UINT16_C(0xBEEF);
    bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(decoded.id == TI_C54X_OPCODE_ID_NOP);
    TI_C54X_TEST_CHECK(decoded.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(decoded.bundle.program_words[0] == UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(decoded.bundle.program_words[1] == UINT16_C(0));
}

static void
check_failure(const ti_c54x_decode_result *first,
              const ti_c54x_instruction_word_bundle *bundle,
              ti_c54x_instruction_completion_status expected)
{
    unsigned char before[sizeof(ti_c54x_decoded_instruction)];
    ti_c54x_decoded_instruction output;

    memset(&output, 0xA5, sizeof(output));
    memcpy(before, &output, sizeof(before));

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) == expected);
    check_bytes_unchanged(before, &output);
}

static void
test_null_and_first_result_validation_is_transactional(void)
{
    const ti_c54x_decode_result first = nop_result();
    const ti_c54x_instruction_word_bundle bundle = nop_bundle();
    unsigned char before[sizeof(ti_c54x_decoded_instruction)];
    ti_c54x_decoded_instruction output;
    ti_c54x_decode_result forged = first;

    memset(&output, 0xA5, sizeof(output));
    memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           NULL, &bundle, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT);
    check_bytes_unchanged(before, &output);
    memset(&output, 0xA5, sizeof(output));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, NULL, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT);
    check_bytes_unchanged(before, &output);
    memset(&output, 0xA5, sizeof(output));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, NULL) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT);

    forged.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK);

    forged = first;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);

    forged = first;
    forged.id = (ti_c54x_opcode_id)UINT16_C(0xFFFF);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_OPCODE_ID);
}

static void
test_count_and_identity_validation_is_transactional(void)
{
    const ti_c54x_instruction_word_bundle valid_bundle = nop_bundle();
    ti_c54x_decode_result forged = nop_result();
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    forged.program_word_count = UINT8_C(0);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_INVALID);

    forged = nop_result();
    forged.program_word_count = UINT8_C(2);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    forged = nop_result();
    forged.program_word_count =
        (uint8_t)(TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY + UINT8_C(1));
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_INVALID);

    bundle.program_word_count = UINT8_C(0);
    forged = nop_result();
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID);

    bundle.program_word_count = UINT8_C(2);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle.program_word_count =
        (uint8_t)(TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY + UINT8_C(1));
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID);

    bundle = valid_bundle;
    bundle.program_words[0] = UINT16_C(0xF494);
    forged = nop_result();
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    forged.first_word = UINT16_C(0xFFFF);
    bundle.program_words[0] = UINT16_C(0xFFFF);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);
}

static void
test_catalog_metadata_invariants_and_valid_pattern(void)
{
    ti_c54x_decode_result forged = nop_result();
    ti_c54x_instruction_word_bundle bundle = nop_bundle();
    int id;

    for (id = (int)TI_C54X_OPCODE_ID_UNKNOWN;
         id < (int)TI_C54X_OPCODE_ID_COUNT; ++id) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id((ti_c54x_opcode_id)id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == (ti_c54x_opcode_id)id);
    }

    forged.id = (ti_c54x_opcode_id)TI_C54X_OPCODE_ID_COUNT;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_OPCODE_ID);

    forged = nop_result();
    forged.first_word = UINT16_C(0x1234);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    forged = nop_result();
    bundle.program_words[0] = UINT16_C(0xF495);
    /* NOP has an exact mask; this remains a positive pattern check. */
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &forged, &bundle, &(ti_c54x_decoded_instruction){0}) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
}

int
main(void)
{
    test_nop_completion_copies_the_complete_bundle();
    test_null_and_first_result_validation_is_transactional();
    test_count_and_identity_validation_is_transactional();
    test_catalog_metadata_invariants_and_valid_pattern();
    puts("instruction_bundle: ok");
    return 0;
}
