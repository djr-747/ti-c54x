/*
 * Module contract
 *
 * Owns: the pure, allocation-free operand-field validator and raw bit
 * assembler. Does not own: instruction encodings, bus/fetch operations, CPU
 * or architectural state, program-counter sequencing, timing, memory,
 * observers, diagnostics, or semantic interpretation.
 */

#include "operand_field.h"

#include <stddef.h>

static uint64_t
ti_c54x_operand_field_mask(uint8_t width_bits)
{
    if (width_bits == UINT8_C(64)) {
        return UINT64_MAX;
    }
    return (UINT64_C(1) << width_bits) - UINT64_C(1);
}

ti_c54x_operand_field_status
ti_c54x_operand_field_validate_layout(
    const ti_c54x_operand_field_layout *layout,
    uint8_t program_word_count)
{
    uint64_t destination_mask = UINT64_C(0);
    uint8_t piece_index;

    if (layout == NULL) {
        return TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT;
    }
    if (program_word_count == 0U) {
        return TI_C54X_OPERAND_FIELD_STATUS_INVALID_WORD_VIEW;
    }
    if (layout->pieces == NULL || layout->piece_count == 0U ||
        layout->value_width_bits == 0U || layout->value_width_bits > 64U) {
        return TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT;
    }

    for (piece_index = 0U; piece_index < layout->piece_count; ++piece_index) {
        const ti_c54x_operand_field_piece *piece =
            &layout->pieces[piece_index];
        uint64_t piece_mask;

        if (piece->width_bits == 0U || piece->width_bits > 16U ||
            piece->source_lsb > 15U ||
            piece->width_bits > (uint8_t)(16U - piece->source_lsb) ||
            piece->value_lsb > 63U ||
            piece->width_bits >
                (uint8_t)(64U - piece->value_lsb) ||
            piece->value_lsb + piece->width_bits >
                layout->value_width_bits) {
            return TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT;
        }
        if (piece->program_word_index >= program_word_count) {
            return TI_C54X_OPERAND_FIELD_STATUS_SOURCE_WORD_INDEX_OUT_OF_RANGE;
        }

        piece_mask = ti_c54x_operand_field_mask(piece->width_bits)
                     << piece->value_lsb;
        if ((destination_mask & piece_mask) != UINT64_C(0)) {
            return TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT;
        }
        destination_mask |= piece_mask;
    }

    for (piece_index = 0U; piece_index < layout->piece_count; ++piece_index) {
        const ti_c54x_operand_field_piece *piece =
            &layout->pieces[piece_index];
        uint8_t other_index;

        for (other_index = 0U; other_index < piece_index; ++other_index) {
            const ti_c54x_operand_field_piece *other =
                &layout->pieces[other_index];
            const uint8_t piece_end =
                (uint8_t)(piece->source_lsb + piece->width_bits);
            const uint8_t other_end =
                (uint8_t)(other->source_lsb + other->width_bits);

            if (piece->program_word_index == other->program_word_index &&
                piece->source_lsb < other_end &&
                other->source_lsb < piece_end) {
                return TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT;
            }
        }
    }

    if (destination_mask != ti_c54x_operand_field_mask(
                                layout->value_width_bits)) {
        return TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT;
    }

    return TI_C54X_OPERAND_FIELD_STATUS_OK;
}

ti_c54x_operand_field_status
ti_c54x_operand_field_extract(
    const ti_c54x_program_words_view *word_view,
    const ti_c54x_operand_field_layout *layout,
    ti_c54x_operand_field_value *value)
{
    uint64_t raw_value = UINT64_C(0);
    uint8_t piece_index;
    ti_c54x_operand_field_value result = {0};

    if (word_view == NULL || layout == NULL || value == NULL) {
        return TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT;
    }
    if (word_view->program_words == NULL ||
        word_view->program_word_count == 0U) {
        return TI_C54X_OPERAND_FIELD_STATUS_INVALID_WORD_VIEW;
    }
    {
        const ti_c54x_operand_field_status status =
            ti_c54x_operand_field_validate_layout(
                layout, word_view->program_word_count);
        if (status != TI_C54X_OPERAND_FIELD_STATUS_OK) {
            return status;
        }
    }

    result.raw_value = UINT64_C(0);
    result.width_bits = layout->value_width_bits;
    for (piece_index = 0U; piece_index < layout->piece_count; ++piece_index) {
        const ti_c54x_operand_field_piece *piece =
            &layout->pieces[piece_index];
        const uint32_t source_mask =
            (UINT32_C(1) << piece->width_bits) - UINT32_C(1);
        const uint16_t source_value = (uint16_t)(
            (word_view->program_words[piece->program_word_index] >>
             piece->source_lsb) &
            source_mask);

        raw_value |= ((uint64_t)source_value &
                      ti_c54x_operand_field_mask(piece->width_bits))
                     << piece->value_lsb;
    }
    result.raw_value = raw_value;
    *value = result;
    return TI_C54X_OPERAND_FIELD_STATUS_OK;
}
