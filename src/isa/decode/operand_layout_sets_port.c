/*
 * Module contract
 *
 * Owns: the static immutable piece/field/operand tables behind the
 * PORT pair generic operand layouts (INDIRECT_ADDR7_PA16 and the
 * absolute INDIRECT_ADDR7_ABSOLUTE16_PA16). Each set object has
 * external linkage so the plan table and resolver in
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

/* PORT absolute Smem fields: indirect flag I[7:7] and 7-bit Smem
 * cookie A[6:0] from word 0 with the copied 16-bit absolute DATA
 * address from word 1 and the copied 16-bit absolute I/O port
 * address from word 2. */
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pa16_indirect_piece = {
        UINT8_C(0), UINT8_C(7), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pa16_address_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(7), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pa16_absolute_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_indirect_addr7_absolute16_pa16_pa_piece = {
        UINT8_C(2), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_indirect_addr7_absolute16_pa16_operands[4] = {
        {&ti_c54x_indirect_addr7_absolute16_pa16_indirect_piece,
         UINT8_C(1), UINT8_C(1)},
        {&ti_c54x_indirect_addr7_absolute16_pa16_address_piece,
         UINT8_C(1), UINT8_C(7)},
        {&ti_c54x_indirect_addr7_absolute16_pa16_absolute_piece,
         UINT8_C(1), UINT8_C(16)},
        {&ti_c54x_indirect_addr7_absolute16_pa16_pa_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_indirect_addr7_absolute16_pa16_set = {
    ti_c54x_indirect_addr7_absolute16_pa16_operands, UINT8_C(4)};
