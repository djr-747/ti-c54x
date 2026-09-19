/*
 * Module contract
 *
 * Owns: the static immutable piece/field/operand tables behind the
 * two-word Smem-plus-lk generic operand layouts
 * (DESTINATION_INDIRECT_ADDR7_EXTENSION16 and
 * SOURCE_DESTINATION_INDIRECT_ADDR7_EXTENSION16) plus the
 * three-word absolute members (the store
 * INDIRECT_ADDR7_ABSOLUTE16_LK and the move
 * INDIRECT_ADDR7_ABSOLUTE16_PMAD) and the two-word
 * accumulator absolute member
 * (ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16). Each set object
 * has external linkage so the plan table and resolver in
 * operand_layout.c can borrow it; pieces and per-operand arrays
 * stay translation-unit private.
 * Does not own: plan selection, ID resolution, operand extraction,
 * opcode semantics, CPU/state, bus/memory, timing, diagnostics,
 * allocation, environment, or caller-owned storage.
 */

#include "operand_layout.h"

#include "operand_layout_internal.h"

#include <stddef.h>
#include <stdint.h>

/* MPY Smem,#lk fields: destination select D[8:8], indirect flag
 * I[7:7], 7-bit Smem address A[6:0], then the copied 16-bit
 * immediate from word 1; T is written. */
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_extension16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_extension16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_extension16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_destination_indirect_addr7_extension16_lk_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_destination_indirect_addr7_extension16_operands[4] = {
        {&ti_c54x_destination_indirect_addr7_extension16_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_destination_indirect_addr7_extension16_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_destination_indirect_addr7_extension16_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_destination_indirect_addr7_extension16_lk_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_destination_indirect_addr7_extension16_set = {
        ti_c54x_destination_indirect_addr7_extension16_operands,
        UINT8_C(4)};

/* MAC Smem,#lk fields: source select S[9:9], destination select
 * D[8:8], one 8-bit Smem specifier cookie (indirect flag I[7:7]
 * over the 7-bit address cookie A[6:0]) from word 0, then the
 * copied 16-bit immediate from word 1; T is written. The cookie
 * rides one operand so trusted packets stay within the
 * four-operand capacity; leaves split the flag from the
 * address. */
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_extension16_source_piece = {
        UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_extension16_destination_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_extension16_smem_pieces[2] = {
        {UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(7)},
        {UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)}};
static const ti_c54x_operand_field_piece
    ti_c54x_source_destination_indirect_addr7_extension16_lk_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_source_destination_indirect_addr7_extension16_operands[4] = {
        {&ti_c54x_source_destination_indirect_addr7_extension16_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_source_destination_indirect_addr7_extension16_destination_piece,
         UINT8_C(1), UINT8_C(1)},
        {ti_c54x_source_destination_indirect_addr7_extension16_smem_pieces,
         UINT8_C(2), UINT8_C(8)},
        {&ti_c54x_source_destination_indirect_addr7_extension16_lk_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_source_destination_indirect_addr7_extension16_set = {
        ti_c54x_source_destination_indirect_addr7_extension16_operands,
        UINT8_C(4)};

/* Test/store Smem,#lk fields: indirect flag I[7:7] and 7-bit
 * Smem address A[6:0] from word 0 with the copied 16-bit
 * immediate from word 1. Three separate operands keep the
 * trusted packet simple; no cookie is needed. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_extension16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_extension16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_extension16_lk_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_extension16_operands[3] = {
        {&ti_c54x_indirect_addr7_extension16_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_indirect_addr7_extension16_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_indirect_addr7_extension16_lk_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_indirect_addr7_extension16_set = {
    ti_c54x_indirect_addr7_extension16_operands, UINT8_C(3)};

/* Absolute store Smem,#lk fields: indirect flag I[7:7] and
 * 7-bit absolute selector A[6:0] from word 0 with the copied
 * 16-bit absolute DATA address from word 1 and the copied
 * 16-bit immediate from word 2. Four separate operands keep
 * the trusted packet simple; the selector rides its own
 * operand so leaves pin the absolute cookie explicitly. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_lk_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_lk_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_lk_absolute_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_lk_lk_piece = {
        UINT8_C(2), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_absolute16_lk_operands[4] = {
        {&ti_c54x_indirect_addr7_absolute16_lk_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_indirect_addr7_absolute16_lk_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_indirect_addr7_absolute16_lk_absolute_piece,
         UINT8_C(1), UINT8_C(16)},
        {&ti_c54x_indirect_addr7_absolute16_lk_lk_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_absolute16_lk_set = {
        ti_c54x_indirect_addr7_absolute16_lk_operands, UINT8_C(4)};

/* Absolute move Smem,pmad fields: indirect flag I[7:7] and
 * 7-bit absolute selector A[6:0] from word 0 with the copied
 * 16-bit absolute DATA address from word 1 and the copied
 * 16-bit program address from word 2. Four separate operands
 * keep the trusted packet simple; the selector rides its own
 * operand so leaves pin the absolute cookie explicitly. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pmad_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pmad_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pmad_absolute_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pmad_pmad_piece = {
        UINT8_C(2), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_absolute16_pmad_operands[4] = {
        {&ti_c54x_indirect_addr7_absolute16_pmad_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_indirect_addr7_absolute16_pmad_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_indirect_addr7_absolute16_pmad_absolute_piece,
         UINT8_C(1), UINT8_C(16)},
        {&ti_c54x_indirect_addr7_absolute16_pmad_pmad_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_absolute16_pmad_set = {
        ti_c54x_indirect_addr7_absolute16_pmad_operands, UINT8_C(4)};

/* Absolute DST src,Smem fields: source select S[8:8],
 * indirect flag I[7:7], and 7-bit absolute selector
 * A[6:0] from word 0 with the copied 16-bit absolute DATA
 * address from word 1. Four separate operands keep the
 * trusted packet simple; the selector rides its own
 * operand so leaves pin the absolute cookie explicitly. */
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_indirect_addr7_absolute16_source_piece = {
        UINT8_C(0), UINT8_C(8), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_indirect_addr7_absolute16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_indirect_addr7_absolute16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_accumulator_indirect_addr7_absolute16_absolute_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_accumulator_indirect_addr7_absolute16_operands[4] = {
        {&ti_c54x_accumulator_indirect_addr7_absolute16_source_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_indirect_addr7_absolute16_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_accumulator_indirect_addr7_absolute16_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_accumulator_indirect_addr7_absolute16_absolute_piece,
         UINT8_C(1), UINT8_C(16)}};
const ti_c54x_operand_layout_set
    ti_c54x_accumulator_indirect_addr7_absolute16_set = {
        ti_c54x_accumulator_indirect_addr7_absolute16_operands,
        UINT8_C(4)};
