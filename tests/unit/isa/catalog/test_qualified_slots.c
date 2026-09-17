/*
 * Module contract
 *
 * Owns: executable proof that second-word slot resolution reports UNKNOWN
 * while the catalog carries no qualified rows, and that the LD Smem,SHIFT
 * metadata record is still unqualified. Lane 2b qualifies the LD row and
 * extends this file with match/mismatch vectors.
 * Does not own: lookup generation, decode policy, dispatch, CPU/state
 * mutation, sequencing, timing, or diagnostics.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_empty_table_has_no_slot(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_first_word_has_qualified_slot(
                           UINT16_C(0x6F80)) == false);
    TI_C54X_TEST_CHECK(ti_c54x_first_word_has_qualified_slot(
                           UINT16_C(0x0000)) == false);
}

static void
test_empty_table_resolves_unknown(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(UINT16_C(0x6F80),
                                                     UINT16_C(0x0C40)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(UINT16_C(0x6F80),
                                                     UINT16_C(0x0C60)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(UINT16_C(0x0000),
                                                     UINT16_C(0x0000)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
}

int
main(void)
{
    test_empty_table_has_no_slot();
    test_empty_table_resolves_unknown();
    return 0;
}
