/*
 * Module contract
 *
 * Owns: executable proof that the RC catalog row carries the
 * 0xFC00 value under the 0xFF00 mask with one program word, the
 * 1-word condition layout, combined accumulator and status
 * plus stack-pointer inputs, and ineligible repeatability,
 * covering all 256 condition encodings with the 0xFC00 word
 * keeping its RET identity under the single-word shadow seam.
 * Does not own: catalog generation, decode, condition
 * evaluation, stack reads, CPU/state mutation, fetch or bus
 * behavior, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t RC_VALUE = UINT16_C(0xFC00);
static const uint16_t RC_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RC);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 295);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_RC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == RC_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == RC_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND8);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population_with_ret_shadow(void)
{
    unsigned matches = 0U;
    unsigned rc_words = 0U;

    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & RC_MASK) == RC_VALUE;
        ti_c54x_opcode_id id;

        if (matches_row) {
            ++matches;
        }
        id = ti_c54x_opcode_catalog_lookup_first_word(word);
        if (word == RC_VALUE) {
            /* The unconditional word keeps its RET identity under
             * the single-word shadow seam. */
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_RET);
        } else {
            TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_RC) ==
                               matches_row);
            if (matches_row) {
                ++rc_words;
            }
        }
    }
    TI_C54X_TEST_CHECK(matches == 256U);
    TI_C54X_TEST_CHECK(rc_words == 255U);
}

static void
test_delayed_variant_separation(void)
{
    /* Setting bit 9 reaches the RETD word; the RCD variant stays
     * deferred, so no RC identity may claim it. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xFE00)) == TI_C54X_OPCODE_ID_RETD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(RC_VALUE | UINT16_C(0x0030))) ==
                       TI_C54X_OPCODE_ID_RC);
}

int
main(void)
{
    test_metadata();
    test_exact_population_with_ret_shadow();
    test_delayed_variant_separation();
    return 0;
}
