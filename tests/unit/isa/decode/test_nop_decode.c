/*
 * Module contract
 *
 * Owns: exhaustive decoder classification and catalog metadata checks for the
 * NOP-seeded ISA slice.  Does not own: opcode semantics, program-counter
 * sequencing, buses, memory, timing, observers, or the test harness.
 * Decoder results are copied values; metadata pointers are borrowed only for
 * the duration of each assertion and are never retained by this test.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
test_nop_decodes_exactly(void)
{
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(UINT16_C(0xF495));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_NOP);
    TI_C54X_TEST_CHECK(result.first_word == UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(result.program_word_count == UINT8_C(1));
}

static void
test_every_other_word_is_explicitly_unknown(void)
{
    uint32_t raw_word;

    for (raw_word = 0U; raw_word <= UINT16_MAX; ++raw_word) {
        const uint16_t first_word = (uint16_t)raw_word;
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(first_word);

        if (first_word == UINT16_C(0xF495)) {
            continue;
        }
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
        TI_C54X_TEST_CHECK(result.first_word == first_word);
        TI_C54X_TEST_CHECK(result.program_word_count == UINT8_C(0));
    }
}

static void
test_catalog_metadata_and_lookup_contract(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    const ti_c54x_opcode_metadata *unknown_metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_UNKNOWN);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_NOP);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_MAX);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(unknown_metadata != NULL);
    TI_C54X_TEST_CHECK(unknown_metadata->id == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(unknown_metadata->first_word_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(unknown_metadata->first_word_mask == UINT16_C(0));
    TI_C54X_TEST_CHECK(unknown_metadata->program_word_count == UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_COUNT) ==
                       NULL);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           (ti_c54x_opcode_id)UINT16_C(0xFFFF)) == NULL);

}

int
main(void)
{
    test_nop_decodes_exactly();
    test_every_other_word_is_explicitly_unknown();
    test_catalog_metadata_and_lookup_contract();
    puts("nop_decode: ok");
    return 0;
}
