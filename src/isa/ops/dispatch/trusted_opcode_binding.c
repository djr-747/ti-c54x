/*
 * Module contract
 *
 * Owns: the immutable X-macro-derived opcode binding table and its checked ID
 * and direct-first-word selectors. Does not own: the generated lookup table,
 * packet construction, operand extraction, checked dispatch, CPU state,
 * sequencing, buses, timing, diagnostics, allocation, or mutable globals.
 */

#include "trusted_opcode_binding.h"

#include <stddef.h>

#include "isa/catalog/opcode_catalog_internal.h"
#include "trusted_semantics.h"

static const ti_c54x_trusted_opcode_binding ti_c54x_trusted_bindings[
    TI_C54X_OPCODE_ID_COUNT] = {
        [TI_C54X_OPCODE_ID_UNKNOWN] = {
            {TI_C54X_OPCODE_ID_UNKNOWN, UINT16_C(0), UINT16_C(0),
             UINT8_C(0), TI_C54X_OPERAND_LAYOUT_UNKNOWN,
             TI_C54X_REPEATABILITY_UNRESOLVED,
             TI_C54X_INPUT_RESOURCES_NONE},
            NULL,
            NULL},
#define TI_C54X_OPCODE_EXACT(symbol, handler, first_word, program_words, \
                             operand_layout, repeatability, input_resources) \
    [TI_C54X_OPCODE_ID_##symbol] = { \
        {TI_C54X_OPCODE_ID_##symbol, (uint16_t)(first_word), UINT16_MAX, \
         (uint8_t)(program_words), \
         (ti_c54x_operand_layout_id)(operand_layout), \
         (ti_c54x_repeatability_class)(repeatability), \
         (ti_c54x_input_resource_mask)(input_resources)}, \
        &ti_c54x_operand_layout_plans[(operand_layout)], \
        ti_c54x_trusted_op_##handler},
#define TI_C54X_OPCODE_MASKED(symbol, handler, first_word_value, \
                              first_word_mask, program_words, operand_layout, \
                              repeatability, input_resources) \
    [TI_C54X_OPCODE_ID_##symbol] = { \
        {TI_C54X_OPCODE_ID_##symbol, (uint16_t)(first_word_value), \
         (uint16_t)(first_word_mask), (uint8_t)(program_words), \
         (ti_c54x_operand_layout_id)(operand_layout), \
         (ti_c54x_repeatability_class)(repeatability), \
         (ti_c54x_input_resource_mask)(input_resources)}, \
        &ti_c54x_operand_layout_plans[(operand_layout)], \
        ti_c54x_trusted_op_##handler},
#include "isa/catalog/opcode_catalog.def"
#undef TI_C54X_OPCODE_EXACT
#undef TI_C54X_OPCODE_MASKED
};

const ti_c54x_trusted_opcode_binding *
ti_c54x_trusted_opcode_binding_for_id(ti_c54x_opcode_id id)
{
    if (id < TI_C54X_OPCODE_ID_UNKNOWN || id >= TI_C54X_OPCODE_ID_COUNT) {
        return NULL;
    }
    return &ti_c54x_trusted_bindings[(size_t)id];
}

const ti_c54x_trusted_opcode_binding *
ti_c54x_trusted_opcode_binding_for_first_word(uint16_t first_word)
{
    return ti_c54x_trusted_opcode_binding_for_id(
        ti_c54x_opcode_catalog_lookup_first_word(first_word));
}

bool ti_c54x_trusted_opcode_binding_is_canonical(
    const ti_c54x_trusted_opcode_binding *binding)
{
    const ti_c54x_trusted_opcode_binding *expected;

    if (binding == NULL) {
        return false;
    }
    expected = ti_c54x_trusted_opcode_binding_for_id(binding->metadata.id);
    if (expected == NULL || binding != expected) {
        return false;
    }
    if (binding->metadata.id == TI_C54X_OPCODE_ID_UNKNOWN) {
        return binding->operand_plan == NULL;
    }
    return binding->operand_plan != NULL &&
           binding->operand_plan ==
               ti_c54x_operand_layout_plan_for_id(
                   binding->metadata.operand_layout) &&
           binding->operand_plan->id == binding->metadata.operand_layout &&
           binding->operand_plan->program_word_count ==
               binding->metadata.program_word_count;
}
