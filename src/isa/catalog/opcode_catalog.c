/*
 * Module contract
 *
 * Owns: X-macro-derived opcode IDs and immutable metadata, including scalar
 * operand-layout/repeatability/input-resource classifications, plus the generated direct
 * 16-bit first-word lookup. The lookup is a fixed 65,536-entry value table;
 * unspecified entries remain the zero/UNKNOWN identity.
 * Does not own: opcode strings, semantic dispatch, timing, CPU state, buses,
 * program-counter sequencing, or diagnostics. All exported results are
 * copied scalars or borrowed pointers to immutable process-lifetime data.
 */

#include "opcode_catalog_internal.h"

#include <stddef.h>

static const ti_c54x_opcode_metadata ti_c54x_opcode_metadata_table[
    TI_C54X_OPCODE_ID_COUNT] = {
        [TI_C54X_OPCODE_ID_UNKNOWN] = {
            TI_C54X_OPCODE_ID_UNKNOWN,
            UINT16_C(0),
            UINT16_C(0),
            UINT8_C(0),
            TI_C54X_OPERAND_LAYOUT_UNKNOWN,
            TI_C54X_REPEATABILITY_UNRESOLVED,
            TI_C54X_INPUT_RESOURCES_NONE},
#define TI_C54X_OPCODE_EXACT(symbol, handler, first_word, program_words, \
                             operand_layout, repeatability, input_resources) \
    [TI_C54X_OPCODE_ID_##symbol] = {TI_C54X_OPCODE_ID_##symbol, \
                                    (uint16_t)(first_word), UINT16_MAX, \
                                    (uint8_t)(program_words), \
                                    (ti_c54x_operand_layout_id)(operand_layout), \
                                    (ti_c54x_repeatability_class)(repeatability), \
                                    (ti_c54x_input_resource_mask)(input_resources)},
#define TI_C54X_OPCODE_MASKED(symbol, handler, first_word_value, \
                              first_word_mask, program_words, operand_layout, \
                              repeatability, input_resources) \
    [TI_C54X_OPCODE_ID_##symbol] = {TI_C54X_OPCODE_ID_##symbol, \
                                    (uint16_t)(first_word_value), \
                                    (uint16_t)(first_word_mask), \
                                    (uint8_t)(program_words), \
                                    (ti_c54x_operand_layout_id)(operand_layout), \
                                    (ti_c54x_repeatability_class)(repeatability), \
                                    (ti_c54x_input_resource_mask)(input_resources)},
#define TI_C54X_OPCODE_MASKED2(symbol, handler, first_word_value, \
                               first_word_mask, program_words, \
                               operand_layout, repeatability, \
                               input_resources, second_word_value, \
                               second_word_mask) \
    [TI_C54X_OPCODE_ID_##symbol] = {TI_C54X_OPCODE_ID_##symbol, \
                                    (uint16_t)(first_word_value), \
                                    (uint16_t)(first_word_mask), \
                                    (uint8_t)(program_words), \
                                    (ti_c54x_operand_layout_id)(operand_layout), \
                                    (ti_c54x_repeatability_class)(repeatability), \
                                    (ti_c54x_input_resource_mask)(input_resources)},
#include "opcode_catalog.def"
#undef TI_C54X_OPCODE_EXACT
#undef TI_C54X_OPCODE_MASKED
#undef TI_C54X_OPCODE_MASKED2
};

/* Second-word qualifiers live only in the slot table below, so the
 * metadata carrier keeps its pinned 24-byte size; a first word owns a
 * qualified slot exactly when at least one entry matches it. */
static const ti_c54x_qualified_slot ti_c54x_qualified_slots[] = {
#include "../../../generated/isa/catalog/opcode_qualified_slots.inc"
};

bool
ti_c54x_first_word_has_qualified_slot(uint16_t first_word)
{
    size_t index;

    for (index = 0; index < sizeof(ti_c54x_qualified_slots) /
                              sizeof(ti_c54x_qualified_slots[0]); ++index) {
        const ti_c54x_qualified_slot *slot = &ti_c54x_qualified_slots[index];
        if (slot->id == TI_C54X_OPCODE_ID_UNKNOWN) {
            break;
        }
        if (((uint16_t)(first_word & slot->first_word_mask)) ==
            slot->first_word_value) {
            return true;
        }
    }
    return false;
}

ti_c54x_opcode_id
ti_c54x_resolve_qualified_slot(uint16_t first_word, uint16_t second_word)
{
    size_t index;

    for (index = 0; index < sizeof(ti_c54x_qualified_slots) /
                              sizeof(ti_c54x_qualified_slots[0]); ++index) {
        const ti_c54x_qualified_slot *slot = &ti_c54x_qualified_slots[index];
        if (slot->id == TI_C54X_OPCODE_ID_UNKNOWN) {
            break;
        }
        if (((uint16_t)(first_word & slot->first_word_mask)) ==
                slot->first_word_value &&
            ((uint16_t)(second_word & slot->second_word_mask)) ==
                slot->second_word_value) {
            return slot->id;
        }
    }
    return TI_C54X_OPCODE_ID_UNKNOWN;
}

static const uint16_t ti_c54x_opcode_direct_lookup[UINT16_MAX + 1U] = {
#include "../../../generated/isa/catalog/opcode_first_word.inc"
};

_Static_assert(TI_C54X_OPCODE_ID_COUNT <= (UINT16_MAX + 1U),
               "opcode identities must fit in uint16_t lookup values");
_Static_assert((sizeof(ti_c54x_opcode_direct_lookup) /
                sizeof(ti_c54x_opcode_direct_lookup[0])) ==
                   (UINT16_MAX + 1U),
               "direct lookup must classify every 16-bit first word");

const ti_c54x_opcode_metadata *
ti_c54x_opcode_metadata_for_id(ti_c54x_opcode_id id)
{
    if (id < TI_C54X_OPCODE_ID_UNKNOWN || id >= TI_C54X_OPCODE_ID_COUNT) {
        return NULL;
    }
    return &ti_c54x_opcode_metadata_table[id];
}

ti_c54x_opcode_id
ti_c54x_opcode_catalog_lookup_first_word(uint16_t first_word)
{
    return (ti_c54x_opcode_id)ti_c54x_opcode_direct_lookup[first_word];
}
