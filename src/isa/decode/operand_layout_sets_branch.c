/*
 * Module contract
 *
 * Owns: the static immutable piece/field/operand tables behind the
 * branch-family generic operand layouts (BRANCH_PMAD16,
 * BRANCH_COND_PMAD16, BRANCH_COND8, BRANCH_SIND_PMAD16, and
 * the XC N_CONDITION8). Each set object has external linkage
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

static const ti_c54x_operand_field_piece
    ti_c54x_branch_pmad16_pmad_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_branch_pmad16_operands[1] = {
        {&ti_c54x_branch_pmad16_pmad_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_branch_pmad16_set = {
    ti_c54x_branch_pmad16_operands, UINT8_C(1)};

static const ti_c54x_operand_field_piece
    ti_c54x_branch_cond_pmad16_cond_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_branch_cond_pmad16_pmad_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_branch_cond_pmad16_operands[2] = {
        {&ti_c54x_branch_cond_pmad16_cond_piece, UINT8_C(1),
         UINT8_C(8)},
        {&ti_c54x_branch_cond_pmad16_pmad_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_branch_cond_pmad16_set = {
    ti_c54x_branch_cond_pmad16_operands, UINT8_C(2)};

/* RC condition field: raw cond[7:0] from the single first word. */
static const ti_c54x_operand_field_piece
    ti_c54x_branch_cond8_cond_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_branch_cond8_operands[1] = {
        {&ti_c54x_branch_cond8_cond_piece, UINT8_C(1),
         UINT8_C(8)}};
const ti_c54x_operand_layout_set ti_c54x_branch_cond8_set = {
    ti_c54x_branch_cond8_operands, UINT8_C(1)};

/* BANZ Sind fields: raw Sind[7:0], then the copied pmad[+1].
 * The piece shape matches the condition layout exactly; the
 * distinct set keeps Sind and condition namespaces separate. */
static const ti_c54x_operand_field_piece
    ti_c54x_branch_sind_pmad16_sind_piece = {
        UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_piece
    ti_c54x_branch_sind_pmad16_pmad_piece = {
        UINT8_C(1), UINT8_C(0), UINT8_C(16), UINT8_C(0)};
static const ti_c54x_operand_field_layout
    ti_c54x_branch_sind_pmad16_operands[2] = {
        {&ti_c54x_branch_sind_pmad16_sind_piece, UINT8_C(1),
         UINT8_C(8)},
        {&ti_c54x_branch_sind_pmad16_pmad_piece, UINT8_C(1),
         UINT8_C(16)}};
const ti_c54x_operand_layout_set ti_c54x_branch_sind_pmad16_set = {
    ti_c54x_branch_sind_pmad16_operands, UINT8_C(2)};

/* XC fields: execute-count select N[9:9], then 8-bit condition
 * code C[7:0]. */
static const ti_c54x_operand_field_piece ti_c54x_n_condition8_n_piece = {
    UINT8_C(0), UINT8_C(9), UINT8_C(1), UINT8_C(0)};
static const ti_c54x_operand_field_piece ti_c54x_n_condition8_cond_piece = {
    UINT8_C(0), UINT8_C(0), UINT8_C(8), UINT8_C(0)};
static const ti_c54x_operand_field_layout ti_c54x_n_condition8_operands[2] = {
    {&ti_c54x_n_condition8_n_piece, UINT8_C(1), UINT8_C(1)},
    {&ti_c54x_n_condition8_cond_piece, UINT8_C(1), UINT8_C(8)}};
const ti_c54x_operand_layout_set ti_c54x_n_condition8_set = {
    ti_c54x_n_condition8_operands, UINT8_C(2)};
