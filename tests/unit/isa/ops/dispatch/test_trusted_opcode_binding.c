/*
 * Module contract
 *
 * Owns: exhaustive executable proof that immutable trusted opcode bindings
 * have the same identity, unknown classification, scalar metadata, and
 * handler presence as the canonical catalog for all 65,536 first words.
 * Does not own: packet construction, opcode semantics, CPU execution,
 * sequencing, timing, diagnostics, or benchmark policy.
 */

#include "isa/catalog/opcode_catalog.h"
#include "isa/catalog/opcode_catalog_internal.h"
#include "isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void check_metadata(const ti_c54x_opcode_metadata *expected,
                           const ti_c54x_opcode_metadata *actual)
{
    TI_C54X_TEST_CHECK(expected != NULL);
    TI_C54X_TEST_CHECK(actual != NULL);
    TI_C54X_TEST_CHECK(actual->id == expected->id);
    TI_C54X_TEST_CHECK(actual->first_word_value == expected->first_word_value);
    TI_C54X_TEST_CHECK(actual->first_word_mask == expected->first_word_mask);
    TI_C54X_TEST_CHECK(actual->program_word_count ==
                       expected->program_word_count);
    TI_C54X_TEST_CHECK(actual->operand_layout == expected->operand_layout);
    TI_C54X_TEST_CHECK(actual->repeatability == expected->repeatability);
    TI_C54X_TEST_CHECK(actual->input_resources == expected->input_resources);
}

static void test_all_ids(void)
{
    int id;

    for (id = (int)TI_C54X_OPCODE_ID_UNKNOWN;
         id < (int)TI_C54X_OPCODE_ID_COUNT; ++id) {
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id((ti_c54x_opcode_id)id);

        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(
            ti_c54x_trusted_opcode_binding_is_canonical(binding));
        check_metadata(
            ti_c54x_opcode_metadata_for_id((ti_c54x_opcode_id)id),
            &binding->metadata);
        if (id == (int)TI_C54X_OPCODE_ID_UNKNOWN) {
            TI_C54X_TEST_CHECK(binding->operand_plan == NULL);
        } else {
            TI_C54X_TEST_CHECK(binding->operand_plan != NULL);
            TI_C54X_TEST_CHECK(binding->operand_plan->id ==
                               binding->metadata.operand_layout);
            TI_C54X_TEST_CHECK(binding->operand_plan->program_word_count ==
                               binding->metadata.program_word_count);
            TI_C54X_TEST_CHECK(binding->operand_plan ==
                               ti_c54x_operand_layout_plan_for_id(
                                   binding->metadata.operand_layout));
        }
        TI_C54X_TEST_CHECK((binding->handler == NULL) ==
                           (id == (int)TI_C54X_OPCODE_ID_UNKNOWN));
    }
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_for_id(
                           (ti_c54x_opcode_id)-1) == NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_for_id(
                           TI_C54X_OPCODE_ID_COUNT) == NULL);
    TI_C54X_TEST_CHECK(!ti_c54x_trusted_opcode_binding_is_canonical(NULL));
}

static void test_all_first_words(void)
{
    uint32_t raw;

    for (raw = UINT32_C(0); raw <= (uint32_t)UINT16_MAX; ++raw) {
        const uint16_t first_word = (uint16_t)raw;
        const ti_c54x_opcode_id expected_id =
            ti_c54x_opcode_catalog_lookup_first_word(first_word);
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_first_word(first_word);

        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(binding ==
                           ti_c54x_trusted_opcode_binding_for_id(expected_id));
        TI_C54X_TEST_CHECK(binding->metadata.id == expected_id);
        TI_C54X_TEST_CHECK((binding->handler == NULL) ==
                           (expected_id == TI_C54X_OPCODE_ID_UNKNOWN));
    }
}

int main(void)
{
    test_all_ids();
    test_all_first_words();
    puts("trusted_opcode_binding: ok");
    return 0;
}
