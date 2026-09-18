/*
 * Module contract
 *
 * Owns: executable proof of the LD Xmem,SHFT,dst catalog row
 * (syntax 5), including its exact masked identity, D/XXXX/SHFT
 * population, dedicated dual-operand layout, auxiliary-register
 * plus SXM/OVM input resources and repeatability metadata, and
 * fixed-bit separation.
 * Does not own: opcode decoding, operand extraction, AR reads,
 * CPU/state mutation, sequencing, timing, generated lookup
 * production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t LD_XMEM_SHIFT_VALUE = UINT16_C(0x9400);
static const uint16_t LD_XMEM_SHIFT_SAMPLE_WORD = UINT16_C(0x95AF);
static const uint16_t LD_XMEM_SHIFT_MASK = UINT16_C(0xFE00);

static bool
matches_ld_xmem_shift(uint16_t word)
{
    return (word & LD_XMEM_SHIFT_MASK) == LD_XMEM_SHIFT_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 269);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LD_XMEM_SHIFT_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_XMEM_SHIFT_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_XMEM_SHIFT_SAMPLE_WORD) ==
                       TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);

    /* D(8), XXXX(7:4), and SHFT(3:0) are variable; bits 15:9 are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x01FF));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_ld_xmem_shift(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 512U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_XMEM_SHIFT_VALUE) ==
                       TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
