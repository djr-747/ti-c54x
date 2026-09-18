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

/* STH syntax-4 second-word source select S at word 1 bit 8. */
static const ti_c54x_operand_field_piece
    ti_c54x_shift_source_piece = {
        UINT8_C(1), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
/* STH syntax-4 second-word signed 5-bit SHIFT at word 1 bits 4:0. */
static const ti_c54x_operand_field_piece
    ti_c54x_shift_shift5_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(5), UINT8_C(0)};
/* STH-direct 7-bit direct address A[6:0] at word 0. */
static const ti_c54x_operand_field_piece
    ti_c54x_shift_direct_addr7_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
/* STH shift-Smem MOD4-family ARF at word 0 bits 2:0. */
static const ti_c54x_operand_field_piece
    ti_c54x_shift_arf_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(3), UINT8_C(0)};
/* STH shift-Smem MOD4-family 4-bit MOD at word 0 bits 6:3. */
static const ti_c54x_operand_field_piece
    ti_c54x_shift_mod4_piece = {
        UINT8_C(0), UINT8_C(3), UINT8_C(4), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_direct7_shift5_operands[3] = {
        {&ti_c54x_shift_source_piece, UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_shift_direct_addr7_piece, UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_shift_shift5_piece, UINT8_C(1), UINT8_C(5)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_direct7_shift5_set = {
    ti_c54x_accumulator_direct7_shift5_operands, UINT8_C(3)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_arf_mod4_shift5_operands[4] = {
        {&ti_c54x_shift_source_piece, UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_shift_arf_piece, UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_shift_mod4_piece, UINT8_C(1), UINT8_C(4)},
        {&ti_c54x_shift_shift5_piece, UINT8_C(1), UINT8_C(5)}};
const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod4_shift5_set = {
    ti_c54x_accumulator_arf_mod4_shift5_operands, UINT8_C(4)};

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

/* Syntax-2 dual-memory MAC fields: combined RSD[10:8] (R rounding
 * flag, S source select, D destination select), Xar[5:4], Yar[1:0].
 * X/Y MOD bits [7:6]/[3:2] are fixed to one combination by the
 * catalog mask and therefore are not exposed as operands. The
 * three-bit RSD carrier keeps the layout within the frozen
 * trusted-operand capacity. */
static const ti_c54x_operand_field_piece
    ti_c54x_dual_round_source_destination_xar_yar_rsd_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(3), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_round_source_destination_xar_yar_xar_piece = {
        UINT8_C(0), UINT8_C(4), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_dual_round_source_destination_xar_yar_yar_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(2), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_dual_round_source_destination_xar_yar_operands[3] = {
        {&ti_c54x_dual_round_source_destination_xar_yar_rsd_piece,
         UINT8_C(1), UINT8_C(3)},
        {&ti_c54x_dual_round_source_destination_xar_yar_xar_piece,
         UINT8_C(1), UINT8_C(2)},
        {&ti_c54x_dual_round_source_destination_xar_yar_yar_piece,
         UINT8_C(1), UINT8_C(2)}};
const ti_c54x_operand_layout_set
    ti_c54x_dual_round_source_destination_xar_yar_set = {
        ti_c54x_dual_round_source_destination_xar_yar_operands,
        UINT8_C(3)};

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

/* Branch-family sets live in operand_layout_sets_branch.c (S48
 * split decision: this file approached the 800-line review
 * ceiling, so the branch/XC tables moved to their own
 * translation unit with pieces still private there). */

/* STM direct fields: raw indirect flag I[7:7], 7-bit direct address
 * dma[6:0], then the copied lk[+1]. */
static const ti_c54x_operand_field_piece
    ti_c54x_direct7_extension16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_direct7_extension16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_direct7_extension16_lk_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_direct7_extension16_operands[3] = {
        {&ti_c54x_direct7_extension16_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_direct7_extension16_address_piece, UINT8_C(1),
         UINT8_C(7)},
        {&ti_c54x_direct7_extension16_lk_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_direct7_extension16_set = {
    ti_c54x_direct7_extension16_operands, UINT8_C(3)};

/* MAC/MPY Smem fields: rounding flag R[9:9], accumulator select
 * S/D[8:8], indirect flag I[7:7], then 7-bit Smem address A[6:0]. */
static const ti_c54x_operand_field_piece
    ti_c54x_round_select_indirect_addr7_round_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_round_select_indirect_addr7_select_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_round_select_indirect_addr7_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_round_select_indirect_addr7_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_round_select_indirect_addr7_operands[4] = {
        {&ti_c54x_round_select_indirect_addr7_round_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_round_select_indirect_addr7_select_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_round_select_indirect_addr7_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_round_select_indirect_addr7_address_piece, UINT8_C(1),
         UINT8_C(7)}};
const ti_c54x_operand_layout_set ti_c54x_round_select_indirect_addr7_set = {
    ti_c54x_round_select_indirect_addr7_operands, UINT8_C(4)};

/* MPYA Smem fields: indirect flag I[7:7], then 7-bit Smem
 * address A[6:0]. The B destination is fixed by the opcode. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_operands[2] = {
        {&ti_c54x_indirect_addr7_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_indirect_addr7_address_piece, UINT8_C(1),
         UINT8_C(7)}};
const ti_c54x_operand_layout_set ti_c54x_indirect_addr7_set = {
    ti_c54x_indirect_addr7_operands, UINT8_C(2)};

/* PORT pair fields: indirect flag I[7:7] and 7-bit Smem cookie
 * A[6:0] from word 0 with the copied 16-bit absolute I/O port
 * address from word 1. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_pa16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_pa16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_pa16_pa_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_pa16_operands[3] = {
        {&ti_c54x_indirect_addr7_pa16_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_indirect_addr7_pa16_address_piece, UINT8_C(1),
         UINT8_C(7)},
        {&ti_c54x_indirect_addr7_pa16_pa_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_indirect_addr7_pa16_set = {
    ti_c54x_indirect_addr7_pa16_operands, UINT8_C(3)};

/* ADD Smem fields: accumulator select S[8:8], indirect flag I[7:7],
 * then 7-bit Smem address A[6:0]. */
static const ti_c54x_operand_field_piece
    ti_c54x_select_indirect_addr7_select_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_select_indirect_addr7_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_select_indirect_addr7_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_select_indirect_addr7_operands[3] = {
        {&ti_c54x_select_indirect_addr7_select_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_select_indirect_addr7_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_select_indirect_addr7_address_piece, UINT8_C(1),
         UINT8_C(7)}};
const ti_c54x_operand_layout_set ti_c54x_select_indirect_addr7_set = {
    ti_c54x_select_indirect_addr7_operands, UINT8_C(3)};

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

/* ADD Smem<<16 fields: source select S[9:9], destination select
 * D[8:8], indirect flag I[7:7], then 7-bit Smem address A[6:0]. */
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_source_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_source_destination_indirect_addr7_operands[4] = {
        {&ti_c54x_source_destination_indirect_addr7_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_source_destination_indirect_addr7_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_source_destination_indirect_addr7_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_source_destination_indirect_addr7_address_piece,
         UINT8_C(1), UINT8_C(7)}};
const ti_c54x_operand_layout_set
    ti_c54x_source_destination_indirect_addr7_set = {
        ti_c54x_source_destination_indirect_addr7_operands, UINT8_C(4)};

/* ADD/SUB Smem SHIFT fields: source select S[9:9], destination
 * select D[8:8], one 8-bit Smem specifier cookie (indirect flag
 * I[7:7] over the 7-bit address cookie A[6:0]) from word 0, and
 * signed 5-bit SHIFT[4:0] from word 1. The cookie rides one
 * operand so trusted packets stay within the four-operand
 * capacity; leaves split the flag from the address. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_source_destination_shift5_source_piece = {
        UINT8_C(1), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_source_destination_shift5_destination_piece = {
        UINT8_C(1), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_source_destination_shift5_shift_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(5), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_source_destination_shift5_smem_pieces[2] = {
        {UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(7)},
        {UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)}};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_source_destination_shift5_operands[4] = {
        {&ti_c54x_indirect_addr7_source_destination_shift5_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_indirect_addr7_source_destination_shift5_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {ti_c54x_indirect_addr7_source_destination_shift5_smem_pieces,
         UINT8_C(2), UINT8_C(8)},
        {&ti_c54x_indirect_addr7_source_destination_shift5_shift_piece,
         UINT8_C(1), UINT8_C(5)}};
const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_source_destination_shift5_set = {
        ti_c54x_indirect_addr7_source_destination_shift5_operands,
        UINT8_C(4)};

/* SQUR Smem fields: D destination bit D[8:8], indirect flag I[7:7],
 * then 7-bit Smem address A[6:0]. Unlike the round/select/indirect
 * shape, fixed bit 9 carries no operand. */
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_destination_indirect_addr7_operands[3] = {
        {&ti_c54x_destination_indirect_addr7_destination_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_destination_indirect_addr7_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_destination_indirect_addr7_address_piece, UINT8_C(1),
         UINT8_C(7)}};
const ti_c54x_operand_layout_set ti_c54x_destination_indirect_addr7_set = {
    ti_c54x_destination_indirect_addr7_operands, UINT8_C(3)};

/* MACP Smem fields: D destination bit S[8:8], indirect flag I[7:7],
 * 7-bit Smem address A[6:0], then the copied 16-bit program-memory
 * coefficient address from word 1. */
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_pmad16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_pmad16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_pmad16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_pmad16_pmad_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_destination_indirect_addr7_pmad16_operands[4] = {
        {&ti_c54x_destination_indirect_addr7_pmad16_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_destination_indirect_addr7_pmad16_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_destination_indirect_addr7_pmad16_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_destination_indirect_addr7_pmad16_pmad_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_destination_indirect_addr7_pmad16_set = {
        ti_c54x_destination_indirect_addr7_pmad16_operands, UINT8_C(4)};

/* MACA Smem fields: round bit R[9:9], indirect flag I[7:7],
 * then 7-bit Smem address A[6:0]. Unlike the round/select/indirect
 * shape, fixed bit 8 carries no operand. */
static const ti_c54x_operand_field_piece
    ti_c54x_round_indirect_addr7_round_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_round_indirect_addr7_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_round_indirect_addr7_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_round_indirect_addr7_operands[3] = {
        {&ti_c54x_round_indirect_addr7_round_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_round_indirect_addr7_indirect_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_round_indirect_addr7_address_piece, UINT8_C(1),
         UINT8_C(7)}};
const ti_c54x_operand_layout_set ti_c54x_round_indirect_addr7_set = {
    ti_c54x_round_indirect_addr7_operands, UINT8_C(3)};

/* MACA T,src fields: source select S[9:9], destination select D[8:8],
 * round R[0:0]; fixed bits 15:10 and 7:1 carry no operand. */
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_round_src_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_round_dst_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_round_round_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_source_destination_round_operands[3] = {
        {&ti_c54x_source_destination_round_src_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_source_destination_round_dst_piece, UINT8_C(1),
         UINT8_C(1)},
        {&ti_c54x_source_destination_round_round_piece, UINT8_C(1),
         UINT8_C(1)}};
const ti_c54x_operand_layout_set ti_c54x_source_destination_round_set = {
    ti_c54x_source_destination_round_operands, UINT8_C(3)};

/* FRAME fields: signed 8-bit stack-pointer offset K[7:0] from word 0;
 * fixed bits 15:8 carry no operand. */
static const ti_c54x_operand_field_piece ti_c54x_frame_k_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_frame_k_operand = {
    &ti_c54x_frame_k_piece, UINT8_C(1), UINT8_C(8)};
const ti_c54x_operand_layout_set ti_c54x_frame_k_set = {
    &ti_c54x_frame_k_operand, UINT8_C(1)};
