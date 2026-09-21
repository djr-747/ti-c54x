/*
 * Module contract
 *
 * Owns: the static immutable piece/field/operand tables behind the
 * dual-XMOD single-operand generic layouts
 * (ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4, SELECT_DUAL_XMOD_XAR_COND4,
 * DUAL_XMOD_XAR_BITC, DUAL_XMOD_XAR_YMOD_YAR, and
 * DUAL_XMOD_XAR_YMOD_YAR_PMAD16). Each set object
 * has external linkage
 * so the plan table and resolver in operand_layout.c can
 * borrow it; pieces and per-operand arrays stay
 * translation-unit private.
 * Does not own: plan selection, ID resolution, operand extraction,
 * opcode semantics, CPU/state, bus/memory, timing, diagnostics,
 * allocation, environment, or caller-owned storage.
 */

#include "operand_layout.h"

#include "operand_layout_internal.h"

#include <stddef.h>
#include <stdint.h>

/* Syntax-5 LD Xmem fields: D[8], Xmod[7:6], raw Xar[5:4], SHFT[3:0].
 * The handler maps the raw Xar field to AR2..AR5; decode exposes raw
 * bits exactly like the dual syntax-6 family. */
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_dual_xmod_xar_shift4_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_dual_xmod_xar_shift4_xmod_piece = {
        UINT8_C(0), UINT8_C(6), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_dual_xmod_xar_shift4_xar_piece = {
        UINT8_C(0), UINT8_C(4), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_dual_xmod_xar_shift4_shift_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_dual_xmod_xar_shift4_operands[4] = {
        {&ti_c54x_accumulator_dual_xmod_xar_shift4_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_dual_xmod_xar_shift4_xmod_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_accumulator_dual_xmod_xar_shift4_xar_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_accumulator_dual_xmod_xar_shift4_shift_piece, UINT8_C(1),
         UINT8_C(4)}};
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_dual_xmod_xar_shift4_set = {
        ti_c54x_accumulator_dual_xmod_xar_shift4_operands, UINT8_C(4)};

/* Conditional-store fields: source select S[8:8], dual-X modifier
 * XMOD[7:6] and register XAR[5:4], and 4-bit condition COND[3:0]. */
static const ti_c54x_operand_field_piece
    ti_c54x_select_dual_xmod_xar_cond4_select_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_select_dual_xmod_xar_cond4_xmod_piece = {
        UINT8_C(0), UINT8_C(6), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_select_dual_xmod_xar_cond4_xar_piece = {
        UINT8_C(0), UINT8_C(4), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_select_dual_xmod_xar_cond4_cond_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_select_dual_xmod_xar_cond4_operands[4] = {
        {&ti_c54x_select_dual_xmod_xar_cond4_select_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_select_dual_xmod_xar_cond4_xmod_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_select_dual_xmod_xar_cond4_xar_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_select_dual_xmod_xar_cond4_cond_piece, UINT8_C(1),
         UINT8_C(4)}};
const ti_c54x_operand_layout_set ti_c54x_select_dual_xmod_xar_cond4_set = {
    ti_c54x_select_dual_xmod_xar_cond4_operands, UINT8_C(4)};

/* BIT Xmem fields: dual-X modifier XMOD[7:6] and register
 * XAR[5:4], and 4-bit bit code BITC[3:0]. */
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_bitc_xmod_piece = {
        UINT8_C(0), UINT8_C(6), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_bitc_xar_piece = {
        UINT8_C(0), UINT8_C(4), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_bitc_bitc_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_dual_xmod_xar_bitc_operands[3] = {
        {&ti_c54x_dual_xmod_xar_bitc_xmod_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_dual_xmod_xar_bitc_xar_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_dual_xmod_xar_bitc_bitc_piece, UINT8_C(1),
         UINT8_C(4)}};
const ti_c54x_operand_layout_set ti_c54x_dual_xmod_xar_bitc_set = {
    ti_c54x_dual_xmod_xar_bitc_operands, UINT8_C(3)};

/* Dual-memory fields: dual-X modifier XMOD[7:6] and register
 * XAR[5:4] over dual-Y modifier YMOD[3:2] and register
 * YAR[1:0]. Both raw register fields map to AR2..AR5 at the
 * handler, as in the MAC-dual precedent. */
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_ymod_yar_xmod_piece = {
        UINT8_C(0), UINT8_C(6), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_ymod_yar_xar_piece = {
        UINT8_C(0), UINT8_C(4), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_ymod_yar_ymod_piece = {
        UINT8_C(0), UINT8_C(2), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_ymod_yar_yar_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_dual_xmod_xar_ymod_yar_operands[4] = {
        {&ti_c54x_dual_xmod_xar_ymod_yar_xmod_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_dual_xmod_xar_ymod_yar_xar_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_dual_xmod_xar_ymod_yar_ymod_piece, UINT8_C(1),
         UINT8_C(2)},
        {&ti_c54x_dual_xmod_xar_ymod_yar_yar_piece, UINT8_C(1),
         UINT8_C(2)}};
const ti_c54x_operand_layout_set ti_c54x_dual_xmod_xar_ymod_yar_set = {
    ti_c54x_dual_xmod_xar_ymod_yar_operands, UINT8_C(4)};

/* Dual-memory filter fields: the word-0 dual XMOD/XAR/YMOD/YAR
 * byte packed as one 8-bit operand with the copied 16-bit
 * absolute PROGRAM coefficient address from word 1. Packing
 * keeps the layout within the frozen trusted-operand
 * capacity, as in the MAC-dual RSD precedent; the leaf
 * unpacks the four 2-bit selectors. */
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_ymod_yar_pmad16_dualxy_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_xmod_xar_ymod_yar_pmad16_pmad_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_dual_xmod_xar_ymod_yar_pmad16_operands[2] = {
        {&ti_c54x_dual_xmod_xar_ymod_yar_pmad16_dualxy_piece,
         UINT8_C(1), UINT8_C(8)},
        {&ti_c54x_dual_xmod_xar_ymod_yar_pmad16_pmad_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_dual_xmod_xar_ymod_yar_pmad16_set = {
        ti_c54x_dual_xmod_xar_ymod_yar_pmad16_operands,
        UINT8_C(2)};
