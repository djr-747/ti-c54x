/*
 * Module contract
 *
 * Owns: the static immutable piece/field/operand tables behind every
 * catalog-declared generic operand layout. Each set object has external
 * linkage so the plan table and resolver in operand_layout.c can borrow
 * it; pieces and per-operand arrays stay translation-unit private.
 * Does not own: plan selection, ID resolution, operand extraction,
 * opcode semantics, CPU/state, bus/memory, timing, diagnostics,
 * allocation, environment, or caller-owned storage.
 */

#include "operand_layout.h"

#include "operand_layout_internal.h"

#include <stddef.h>
#include <stdint.h>

const ti_c54x_operand_layout_set ti_c54x_operand_layout_none = {
    NULL, UINT8_C(0)};

static const ti_c54x_operand_field_piece ti_c54x_rpt_k_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_rpt_k_operand = {
    &ti_c54x_rpt_k_piece, UINT8_C(1), UINT8_C(8)};
const ti_c54x_operand_layout_set ti_c54x_rpt_k_set = {
    &ti_c54x_rpt_k_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_rpt_lk_piece = {
    UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_rpt_lk_operand = {
    &ti_c54x_rpt_lk_piece, UINT8_C(1), UINT8_C(16)};
const ti_c54x_operand_layout_set ti_c54x_rpt_lk_set = {
    &ti_c54x_rpt_lk_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_status_bit_n_piece = {
    UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece ti_c54x_status_bit_sbit_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_status_bit_operands[2] = {
    {&ti_c54x_status_bit_n_piece, UINT8_C(1), UINT8_C(1)},
    {&ti_c54x_status_bit_sbit_piece, UINT8_C(1), UINT8_C(4)}};
const ti_c54x_operand_layout_set ti_c54x_status_bit_set = {
    ti_c54x_status_bit_operands, UINT8_C(2)};

static const ti_c54x_operand_field_piece ti_c54x_condition_arf_cc_piece = {
    UINT8_C(0), UINT8_C(8), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece ti_c54x_condition_arf_arf_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_condition_arf_operands[2] = {
    {&ti_c54x_condition_arf_cc_piece, UINT8_C(1), UINT8_C(2)},
    {&ti_c54x_condition_arf_arf_piece, UINT8_C(1), UINT8_C(3)}};
const ti_c54x_operand_layout_set ti_c54x_condition_arf_set = {
    ti_c54x_condition_arf_operands, UINT8_C(2)};

static const ti_c54x_operand_field_piece ti_c54x_ld_dp_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(9), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_ld_dp_operand = {
    &ti_c54x_ld_dp_piece, UINT8_C(1), UINT8_C(9)};
const ti_c54x_operand_layout_set ti_c54x_ld_dp_set = {
    &ti_c54x_ld_dp_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_ld_asm_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(5), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_ld_asm_operand = {
    &ti_c54x_ld_asm_piece, UINT8_C(1), UINT8_C(5)};
const ti_c54x_operand_layout_set ti_c54x_ld_asm_set = {
    &ti_c54x_ld_asm_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_ld_arp_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_ld_arp_operand = {
    &ti_c54x_ld_arp_piece, UINT8_C(1), UINT8_C(3)};
const ti_c54x_operand_layout_set ti_c54x_ld_arp_set = {
    &ti_c54x_ld_arp_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_arf_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_arf_operand = {
    &ti_c54x_arf_piece, UINT8_C(1), UINT8_C(3)};
const ti_c54x_operand_layout_set ti_c54x_arf_set = {
    &ti_c54x_arf_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_arf_extension16_arf_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_arf_extension16_extension_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_arf_extension16_operands[2] = {
    {&ti_c54x_arf_extension16_arf_piece, UINT8_C(1), UINT8_C(3)},
    {&ti_c54x_arf_extension16_extension_piece, UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_arf_extension16_set = {
    ti_c54x_arf_extension16_operands, UINT8_C(2)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_arf_accumulator_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece ti_c54x_accumulator_arf_arf_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_arf_operands[2] = {
        {&ti_c54x_accumulator_arf_accumulator_piece, UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_arf_arf_piece, UINT8_C(1), UINT8_C(3)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_set = {
    ti_c54x_accumulator_arf_operands, UINT8_C(2)};

static const ti_c54x_operand_field_piece ti_c54x_standard_indirect_mod_piece = {
    UINT8_C(0), UINT8_C(3), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece ti_c54x_standard_indirect_mod4_piece = {
    UINT8_C(0), UINT8_C(3), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_arf_mod_operands[2] = {
    {&ti_c54x_arf_piece, UINT8_C(1), UINT8_C(3)},
    {&ti_c54x_standard_indirect_mod_piece, UINT8_C(1), UINT8_C(2)}};
const ti_c54x_operand_layout_set ti_c54x_arf_mod_set = {
    ti_c54x_arf_mod_operands, UINT8_C(2)};

static const ti_c54x_operand_field_layout
    ti_c54x_arf_extension16_mod_operands[3] = {
        {&ti_c54x_arf_extension16_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_arf_extension16_extension_piece, UINT8_C(1), UINT8_C(16)},
        {&ti_c54x_standard_indirect_mod_piece, UINT8_C(1), UINT8_C(2)}};
const ti_c54x_operand_layout_set ti_c54x_arf_extension16_mod_set = {
    ti_c54x_arf_extension16_mod_operands, UINT8_C(3)};

static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_arf_mod_operands[3] = {
        {&ti_c54x_accumulator_arf_accumulator_piece, UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_arf_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_standard_indirect_mod_piece, UINT8_C(1), UINT8_C(2)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod_set = {
    ti_c54x_accumulator_arf_mod_operands, UINT8_C(3)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_arf_mod_shift5_destination_piece = {
        UINT8_C(1), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_arf_mod_shift5_shift_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(5), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_arf_mod_shift5_operands[4] = {
        {&ti_c54x_accumulator_arf_mod_shift5_destination_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_accumulator_arf_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_standard_indirect_mod_piece, UINT8_C(1), UINT8_C(2)},
        {&ti_c54x_accumulator_arf_mod_shift5_shift_piece, UINT8_C(1),
         UINT8_C(5)}};
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_arf_mod_shift5_set = {
        ti_c54x_accumulator_arf_mod_shift5_operands, UINT8_C(4)};

static const ti_c54x_operand_field_layout ti_c54x_arf_mod4_operands[2] = {
    {&ti_c54x_arf_piece, UINT8_C(1), UINT8_C(3)},
    {&ti_c54x_standard_indirect_mod4_piece, UINT8_C(1), UINT8_C(4)}};
const ti_c54x_operand_layout_set ti_c54x_arf_mod4_set = {
    ti_c54x_arf_mod4_operands, UINT8_C(2)};

static const ti_c54x_operand_field_layout
    ti_c54x_arf_extension16_mod4_operands[3] = {
        {&ti_c54x_arf_extension16_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_arf_extension16_extension_piece, UINT8_C(1), UINT8_C(16)},
        {&ti_c54x_standard_indirect_mod4_piece, UINT8_C(1), UINT8_C(4)}};
const ti_c54x_operand_layout_set ti_c54x_arf_extension16_mod4_set = {
    ti_c54x_arf_extension16_mod4_operands, UINT8_C(3)};

static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_arf_mod4_operands[3] = {
        {&ti_c54x_accumulator_arf_accumulator_piece, UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_arf_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_standard_indirect_mod4_piece, UINT8_C(1), UINT8_C(4)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod4_set = {
    ti_c54x_accumulator_arf_mod4_operands, UINT8_C(3)};

static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_arf_extension16_mod4_operands[4] = {
        {&ti_c54x_accumulator_arf_accumulator_piece, UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_arf_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_arf_extension16_extension_piece, UINT8_C(1), UINT8_C(16)},
        {&ti_c54x_standard_indirect_mod4_piece, UINT8_C(1), UINT8_C(4)}};
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_arf_extension16_mod4_set = {
        ti_c54x_accumulator_arf_extension16_mod4_operands, UINT8_C(4)};

static const ti_c54x_operand_field_piece ti_c54x_max_min_destination_piece = {
    UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_max_min_destination_operand = {
    &ti_c54x_max_min_destination_piece, UINT8_C(1), UINT8_C(1)};
const ti_c54x_operand_layout_set ti_c54x_max_min_destination_set = {
    &ti_c54x_max_min_destination_operand, UINT8_C(1)};

static const ti_c54x_operand_field_piece ti_c54x_accumulator_source_piece = {
    UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_accumulator_source_operand = {
    &ti_c54x_accumulator_source_piece, UINT8_C(1), UINT8_C(1)};
const ti_c54x_operand_layout_set ti_c54x_accumulator_source_set = {
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
const ti_c54x_operand_layout_set
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
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_source_destination_shift5_set = {
        ti_c54x_accumulator_source_destination_shift5_operands, UINT8_C(3)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_k8_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_k8_immediate_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_immediate_k8_operands[2] = {
        {&ti_c54x_accumulator_immediate_k8_destination_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_accumulator_immediate_k8_immediate_piece, UINT8_C(1),
         UINT8_C(8)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_immediate_k8_set = {
    ti_c54x_accumulator_immediate_k8_operands, UINT8_C(2)};

static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_lk16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_lk16_immediate_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_immediate_lk16_operands[2] = {
        {&ti_c54x_accumulator_immediate_lk16_destination_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_accumulator_immediate_lk16_immediate_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_immediate_lk16_set = {
    ti_c54x_accumulator_immediate_lk16_operands, UINT8_C(2)};

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
const ti_c54x_operand_layout_set
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
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_source_destination_extension16_shift16_set = {
        ti_c54x_accumulator_source_destination_extension16_shift16_operands,
        UINT8_C(3)};

/* Syntax-6 dual-memory fields: D[8], Xar[5:4], Yar[1:0].  X/Y MOD bits
 * [7:6]/[3:2] are fixed to MOD0 by the catalog mask and therefore are not
 * exposed as operands. */
static const ti_c54x_operand_field_piece
    ti_c54x_dual_destination_xar_yar_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_destination_xar_yar_xar_piece = {
        UINT8_C(0), UINT8_C(4), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_destination_xar_yar_yar_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_dual_destination_xar_yar_operands[3] = {
        {&ti_c54x_dual_destination_xar_yar_destination_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_dual_destination_xar_yar_xar_piece, UINT8_C(1), UINT8_C(2)},
        {&ti_c54x_dual_destination_xar_yar_yar_piece, UINT8_C(1), UINT8_C(2)}};
const ti_c54x_operand_layout_set
    ti_c54x_dual_destination_xar_yar_set = {
        ti_c54x_dual_destination_xar_yar_operands, UINT8_C(3)};

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

/* Syntax-7 long-immediate fields: D[8], unsigned SHFT[3:0], lk[+1]. */
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_shift4_lk16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_shift4_lk16_shift_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_immediate_shift4_lk16_immediate_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_immediate_shift4_lk16_operands[3] = {
        {&ti_c54x_accumulator_immediate_shift4_lk16_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_immediate_shift4_lk16_shift_piece, UINT8_C(1),
         UINT8_C(4)},
        {&ti_c54x_accumulator_immediate_shift4_lk16_immediate_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_immediate_shift4_lk16_set = {
        ti_c54x_accumulator_immediate_shift4_lk16_operands, UINT8_C(3)};
