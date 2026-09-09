/* Module contract
 * Owns: exhaustive metadata and first-word lookup proofs for the sixteen
 * non-MOD0/MOD0 syntax-6 dual-memory ADD/SUB rows, plus proof that every
 * MOD3-involving neighborhood stays undecoded.
 * Does not own: dual addressing, arithmetic semantics, CPU mutation, or
 * inventory policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

typedef struct dual_modes_row {
    ti_c54x_opcode_id id;
    uint16_t base;
} dual_modes_row;

static const dual_modes_row add_rows[] = {
    {TI_C54X_OPCODE_ID_ADD_DUAL_M0_M1, UINT16_C(0xA004)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M0_M2, UINT16_C(0xA008)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M1_M0, UINT16_C(0xA040)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M1_M1, UINT16_C(0xA044)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M1_M2, UINT16_C(0xA048)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M2_M0, UINT16_C(0xA080)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M2_M1, UINT16_C(0xA084)},
    {TI_C54X_OPCODE_ID_ADD_DUAL_M2_M2, UINT16_C(0xA088)},
};

static const dual_modes_row sub_rows[] = {
    {TI_C54X_OPCODE_ID_SUB_DUAL_M0_M1, UINT16_C(0xA204)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M0_M2, UINT16_C(0xA208)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M1_M0, UINT16_C(0xA240)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M1_M1, UINT16_C(0xA244)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M1_M2, UINT16_C(0xA248)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M2_M0, UINT16_C(0xA280)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M2_M1, UINT16_C(0xA284)},
    {TI_C54X_OPCODE_ID_SUB_DUAL_M2_M2, UINT16_C(0xA288)},
};

static void check_row(ti_c54x_opcode_id id, uint16_t base)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    uint8_t destination;
    uint8_t xar;
    uint8_t yar;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == base);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFECC));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_C_SXM_OVM);

    for (destination = 0U; destination < 2U; ++destination) {
        for (xar = 0U; xar < 4U; ++xar) {
            for (yar = 0U; yar < 4U; ++yar) {
                const uint16_t word = (uint16_t)(base |
                    ((uint16_t)destination << 8U) |
                    ((uint16_t)xar << 4U) | yar);
                TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                                       word) == id);
            }
        }
    }
}

static void check_all_rows(const dual_modes_row *rows, size_t count)
{
    size_t index;

    for (index = 0U; index < count; ++index) {
        check_row(rows[index].id, rows[index].base);
    }
}

static void check_mod3_neighborhoods_stay_unknown(void)
{
    /* Every MOD3-involving (xmode, ymode) combination per operation. */
    static const uint16_t bases[] = {
        UINT16_C(0xA00C), UINT16_C(0xA04C), UINT16_C(0xA08C),
        UINT16_C(0xA0C0), UINT16_C(0xA0C4), UINT16_C(0xA0C8),
        UINT16_C(0xA0CC), UINT16_C(0xA20C), UINT16_C(0xA24C),
        UINT16_C(0xA28C), UINT16_C(0xA2C0), UINT16_C(0xA2C4),
        UINT16_C(0xA2C8), UINT16_C(0xA2CC)};
    size_t index;

    for (index = 0U; index < sizeof(bases) / sizeof(bases[0]); ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                               bases[index]) == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int main(void)
{
    check_all_rows(add_rows, sizeof(add_rows) / sizeof(add_rows[0]));
    check_all_rows(sub_rows, sizeof(sub_rows) / sizeof(sub_rows[0]));
    check_mod3_neighborhoods_stay_unknown();
    return 0;
}
