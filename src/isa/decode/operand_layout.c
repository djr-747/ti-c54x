/*
 * Module contract
 *
 * Owns: static immutable descriptors for catalog-declared generic operand
 * layouts, plus their pure resolution and extraction seams. Does not own:
 * opcode semantics, instruction lookup, CPU/state, bus/memory, timing,
 * diagnostics, allocation, environment, or caller-owned storage.
 */

#include "operand_layout.h"

#include "instruction_bundle.h"

#include <stddef.h>

static const ti_c54x_operand_layout_set ti_c54x_operand_layout_none = {
    NULL, UINT8_C(0)};

static const ti_c54x_operand_field_piece ti_c54x_rpt_k_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_rpt_k_operand = {
    &ti_c54x_rpt_k_piece, UINT8_C(1), UINT8_C(8)};
static const ti_c54x_operand_layout_set ti_c54x_rpt_k_set = {
    &ti_c54x_rpt_k_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_rpt_lk_piece = {
    UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_rpt_lk_operand = {
    &ti_c54x_rpt_lk_piece, UINT8_C(1), UINT8_C(16)};
static const ti_c54x_operand_layout_set ti_c54x_rpt_lk_set = {
    &ti_c54x_rpt_lk_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_ld_dp_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(9), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_ld_dp_operand = {
    &ti_c54x_ld_dp_piece, UINT8_C(1), UINT8_C(9)};
static const ti_c54x_operand_layout_set ti_c54x_ld_dp_set = {
    &ti_c54x_ld_dp_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_ld_asm_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(5), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_ld_asm_operand = {
    &ti_c54x_ld_asm_piece, UINT8_C(1), UINT8_C(5)};
static const ti_c54x_operand_layout_set ti_c54x_ld_asm_set = {
    &ti_c54x_ld_asm_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_ld_arp_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_ld_arp_operand = {
    &ti_c54x_ld_arp_piece, UINT8_C(1), UINT8_C(3)};
static const ti_c54x_operand_layout_set ti_c54x_ld_arp_set = {
    &ti_c54x_ld_arp_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_max_min_destination_piece = {
    UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_max_min_destination_operand = {
    &ti_c54x_max_min_destination_piece, UINT8_C(1), UINT8_C(1)};
static const ti_c54x_operand_layout_set ti_c54x_max_min_destination_set = {
    &ti_c54x_max_min_destination_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_accumulator_source_piece = {
    UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_accumulator_source_operand = {
    &ti_c54x_accumulator_source_piece, UINT8_C(1), UINT8_C(1)};
static const ti_c54x_operand_layout_set ti_c54x_accumulator_source_set = {
    &ti_c54x_accumulator_source_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_source_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_source_destination_source_operand = {
        &ti_c54x_accumulator_source_destination_source_piece, UINT8_C(1),
        UINT8_C(1)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_source_destination_destination_operand = {
        &ti_c54x_accumulator_source_destination_destination_piece, UINT8_C(1),
        UINT8_C(1)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_source_destination_operands[2] = {
        ti_c54x_accumulator_source_destination_source_operand,
        ti_c54x_accumulator_source_destination_destination_operand};
static const ti_c54x_operand_layout_set
    ti_c54x_accumulator_source_destination_set = {
        ti_c54x_accumulator_source_destination_operands, UINT8_C(2)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift5_source_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift5_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift5_shift_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(5), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_source_destination_shift5_operands[3] = {
        {&ti_c54x_accumulator_source_destination_shift5_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_source_destination_shift5_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_source_destination_shift5_shift_piece,
         UINT8_C(1), UINT8_C(5)}};
static const ti_c54x_operand_layout_set
    ti_c54x_accumulator_source_destination_shift5_set = {
        ti_c54x_accumulator_source_destination_shift5_operands, UINT8_C(3)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift4_extension16_source_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift4_extension16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift4_extension16_shift_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_shift4_extension16_immediate_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_source_destination_shift4_extension16_operands[4] = {
        {&ti_c54x_accumulator_source_destination_shift4_extension16_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_source_destination_shift4_extension16_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_source_destination_shift4_extension16_shift_piece,
         UINT8_C(1), UINT8_C(4)},
        {&ti_c54x_accumulator_source_destination_shift4_extension16_immediate_piece,
         UINT8_C(1), UINT8_C(16)}};
static const ti_c54x_operand_layout_set
    ti_c54x_accumulator_source_destination_shift4_extension16_set = {
        ti_c54x_accumulator_source_destination_shift4_extension16_operands,
        UINT8_C(4)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_extension16_shift16_source_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_extension16_shift16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_source_destination_extension16_shift16_immediate_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_source_destination_extension16_shift16_operands[3] = {
        {&ti_c54x_accumulator_source_destination_extension16_shift16_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_source_destination_extension16_shift16_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_source_destination_extension16_shift16_immediate_piece,
         UINT8_C(1), UINT8_C(16)}};
static const ti_c54x_operand_layout_set
    ti_c54x_accumulator_source_destination_extension16_shift16_set = {
        ti_c54x_accumulator_source_destination_extension16_shift16_operands,
        UINT8_C(3)};

static ti_c54x_operand_layout_status
ti_c54x_operand_layout_map_field_status(
    ti_c54x_operand_field_status status)
{
    if (status == TI_C54X_OPERAND_FIELD_STATUS_SOURCE_WORD_INDEX_OUT_OF_RANGE) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_FIELD_SOURCE_WORD_INDEX_OUT_OF_RANGE;
    }
    if (status != TI_C54X_OPERAND_FIELD_STATUS_OK) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_FIELD_LAYOUT_INVALID;
    }
    return TI_C54X_OPERAND_LAYOUT_STATUS_OK;
}

ti_c54x_operand_layout_status
ti_c54x_operand_layout_set_validate_descriptor(
    const ti_c54x_operand_layout_set *descriptor,
    uint8_t program_word_count)
{
    uint8_t operand_index;

    if (descriptor == NULL) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT;
    }
    if (program_word_count == UINT8_C(0)) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_INVALID;
    }
    if (descriptor->operand_count == UINT8_C(0)) {
        if (descriptor->operands != NULL) {
            return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_SET;
        }
        return TI_C54X_OPERAND_LAYOUT_STATUS_OK;
    }
    if (descriptor->operands == NULL) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_SET;
    }

    for (operand_index = UINT8_C(0);
         operand_index < descriptor->operand_count;
         ++operand_index) {
        const ti_c54x_operand_field_status field_status =
            ti_c54x_operand_field_validate_layout(
                &descriptor->operands[operand_index], program_word_count);
        const ti_c54x_operand_layout_status status =
            ti_c54x_operand_layout_map_field_status(field_status);
        if (status != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return status;
        }
    }
    return TI_C54X_OPERAND_LAYOUT_STATUS_OK;
}

static const ti_c54x_operand_layout_set *
ti_c54x_operand_layout_descriptor_for_id(ti_c54x_operand_layout_id id,
                                         uint8_t *expected_word_count)
{
    switch (id) {
    case TI_C54X_OPERAND_LAYOUT_NONE:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_operand_layout_none;
    case TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_rpt_k_set;
    case TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_rpt_lk_set;
    case TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_ld_dp_set;
    case TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_ld_asm_set;
    case TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_ld_arp_set;
    case TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_max_min_destination_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_source_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_source_destination_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_source_destination_shift5_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_source_destination_shift4_extension16_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_source_destination_extension16_shift16_set;
    default:
        return NULL;
    }
}

ti_c54x_operand_layout_status
ti_c54x_operand_layout_set_resolve(
    ti_c54x_operand_layout_id id,
    uint8_t program_word_count,
    ti_c54x_operand_layout_set_view *output)
{
    const ti_c54x_operand_layout_set *descriptor;
    ti_c54x_operand_layout_set_view result;
    uint8_t expected_word_count = UINT8_C(0);

    if (output == NULL) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT;
    }
    if (id == TI_C54X_OPERAND_LAYOUT_UNKNOWN) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_UNKNOWN_ID;
    }
    if (id < TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        id >= TI_C54X_OPERAND_LAYOUT_COUNT) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ID;
    }
    if (program_word_count == UINT8_C(0) ||
        program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_INVALID;
    }

    descriptor = ti_c54x_operand_layout_descriptor_for_id(
        id, &expected_word_count);
    if (descriptor == NULL) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ID;
    }
    if (id != TI_C54X_OPERAND_LAYOUT_NONE &&
        program_word_count != expected_word_count) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH;
    }
    {
        const ti_c54x_operand_layout_status status =
            ti_c54x_operand_layout_set_validate_descriptor(
                descriptor, program_word_count);
        if (status != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return status;
        }
    }

    result.id = id;
    result.program_word_count = program_word_count;
    result.descriptor = descriptor;
    *output = result;
    return TI_C54X_OPERAND_LAYOUT_STATUS_OK;
}

ti_c54x_operand_layout_status
ti_c54x_operand_layout_set_extract_operand(
    const ti_c54x_operand_layout_set_view *view,
    const ti_c54x_program_words_view *word_view,
    uint8_t operand_index,
    ti_c54x_operand_field_value *value)
{
    const ti_c54x_operand_layout_set *expected_descriptor;
    uint8_t expected_word_count = UINT8_C(0);

    if (view == NULL || value == NULL) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT;
    }
    if (view->id == TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        view->id < TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        view->id >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        view->program_word_count == UINT8_C(0) ||
        view->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_VIEW;
    }
    expected_descriptor = ti_c54x_operand_layout_descriptor_for_id(
        view->id, &expected_word_count);
    if (expected_descriptor == NULL ||
        expected_descriptor != view->descriptor ||
        (view->id != TI_C54X_OPERAND_LAYOUT_NONE &&
         view->program_word_count != expected_word_count)) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_VIEW;
    }
    if (view->descriptor->operand_count == UINT8_C(0) ||
        operand_index >= view->descriptor->operand_count) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_OPERAND_INDEX_OUT_OF_RANGE;
    }
    if (word_view == NULL) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT;
    }
    if (word_view->program_words == NULL ||
        word_view->program_word_count == UINT8_C(0)) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT;
    }
    if (word_view->program_word_count != view->program_word_count) {
        return TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH;
    }
    {
        const ti_c54x_operand_field_status field_status =
            ti_c54x_operand_field_extract(
                word_view, &view->descriptor->operands[operand_index], value);
        return ti_c54x_operand_layout_map_field_status(field_status);
    }
}
