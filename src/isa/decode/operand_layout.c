/*
 * Module contract
 *
 * Owns: the immutable plan table over catalog-declared generic operand
 * layouts plus their pure resolution and extraction seams. The static
 * piece/set tables live in operand_layout_sets.c and are borrowed here.
 * Does not own:
 * opcode semantics, instruction lookup, CPU/state, bus/memory, timing,
 * diagnostics, allocation, environment, or caller-owned storage.
 */

#include "operand_layout.h"

#include "operand_layout_internal.h"

#include "instruction_bundle.h"

#include <stddef.h>


const ti_c54x_operand_layout_plan ti_c54x_operand_layout_plans[
    TI_C54X_OPERAND_LAYOUT_COUNT] = {
        [TI_C54X_OPERAND_LAYOUT_UNKNOWN] = {
            TI_C54X_OPERAND_LAYOUT_UNKNOWN, UINT8_C(0), NULL},
        [TI_C54X_OPERAND_LAYOUT_NONE] = {
            TI_C54X_OPERAND_LAYOUT_NONE, UINT8_C(1),
            &ti_c54x_operand_layout_none},
        [TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K] = {
            TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, UINT8_C(1),
            &ti_c54x_rpt_k_set},
        [TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK] = {
            TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, UINT8_C(2),
            &ti_c54x_rpt_lk_set},
        [TI_C54X_OPERAND_LAYOUT_STATUS_BIT] = {
            TI_C54X_OPERAND_LAYOUT_STATUS_BIT, UINT8_C(1),
            &ti_c54x_status_bit_set},
        [TI_C54X_OPERAND_LAYOUT_CONDITION_ARF] = {
            TI_C54X_OPERAND_LAYOUT_CONDITION_ARF, UINT8_C(1),
            &ti_c54x_condition_arf_set},
        [TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP] = {
            TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP, UINT8_C(1),
            &ti_c54x_ld_dp_set},
        [TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM] = {
            TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM, UINT8_C(1),
            &ti_c54x_ld_asm_set},
        [TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP] = {
            TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP, UINT8_C(1),
            &ti_c54x_ld_arp_set},
        [TI_C54X_OPERAND_LAYOUT_ARF] = {
            TI_C54X_OPERAND_LAYOUT_ARF, UINT8_C(1), &ti_c54x_arf_set},
        [TI_C54X_OPERAND_LAYOUT_ARF_MOD] = {
            TI_C54X_OPERAND_LAYOUT_ARF_MOD, UINT8_C(1),
            &ti_c54x_arf_mod_set},
        [TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16] = {
            TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16, UINT8_C(2),
            &ti_c54x_arf_extension16_set},
        [TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD] = {
            TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD, UINT8_C(2),
            &ti_c54x_arf_extension16_mod_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF, UINT8_C(1),
            &ti_c54x_accumulator_arf_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD, UINT8_C(1),
            &ti_c54x_accumulator_arf_mod_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5, UINT8_C(2),
            &ti_c54x_accumulator_arf_mod_shift5_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5, UINT8_C(2),
            &ti_c54x_accumulator_direct7_shift5_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5, UINT8_C(2),
            &ti_c54x_accumulator_arf_mod4_shift5_set},
        [TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC] = {
            TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC, UINT8_C(1),
            &ti_c54x_max_min_destination_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE, UINT8_C(1),
            &ti_c54x_accumulator_source_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION, UINT8_C(1),
            &ti_c54x_accumulator_source_destination_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5,
            UINT8_C(1), &ti_c54x_accumulator_source_destination_shift5_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8, UINT8_C(1),
            &ti_c54x_accumulator_immediate_k8_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16, UINT8_C(2),
            &ti_c54x_accumulator_immediate_lk16_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16,
            UINT8_C(2),
            &ti_c54x_accumulator_immediate_shift4_lk16_set},
        [TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16] = {
            TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16, UINT8_C(2),
            &ti_c54x_branch_pmad16_set},
        [TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16] = {
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16, UINT8_C(2),
            &ti_c54x_branch_cond_pmad16_set},
        [TI_C54X_OPERAND_LAYOUT_BRANCH_COND8] = {
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND8, UINT8_C(1),
            &ti_c54x_branch_cond8_set},
        [TI_C54X_OPERAND_LAYOUT_N_CONDITION8] = {
            TI_C54X_OPERAND_LAYOUT_N_CONDITION8, UINT8_C(1),
            &ti_c54x_n_condition8_set},
        [TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16] = {
            TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16, UINT8_C(2),
            &ti_c54x_branch_sind_pmad16_set},
        [TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16] = {
            TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16, UINT8_C(2),
            &ti_c54x_direct7_extension16_set},
        [TI_C54X_OPERAND_LAYOUT_ROUND_SELECT_INDIRECT_ADDR7] = {
            TI_C54X_OPERAND_LAYOUT_ROUND_SELECT_INDIRECT_ADDR7, UINT8_C(1),
            &ti_c54x_round_select_indirect_addr7_set},
        [TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7] = {
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7, UINT8_C(1),
            &ti_c54x_indirect_addr7_set},
        [TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7] = {
            TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7, UINT8_C(1),
            &ti_c54x_select_indirect_addr7_set},
        [TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7] = {
            TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7,
            UINT8_C(1),
            &ti_c54x_source_destination_indirect_addr7_set},
        [TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5] = {
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5,
            UINT8_C(2),
            &ti_c54x_indirect_addr7_source_destination_shift5_set},
        [TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7] = {
            TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7, UINT8_C(1),
            &ti_c54x_destination_indirect_addr7_set},
        [TI_C54X_OPERAND_LAYOUT_ROUND_INDIRECT_ADDR7] = {
            TI_C54X_OPERAND_LAYOUT_ROUND_INDIRECT_ADDR7, UINT8_C(1),
            &ti_c54x_round_indirect_addr7_set},
        [TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND] = {
            TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND, UINT8_C(1),
            &ti_c54x_source_destination_round_set},
        [TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16] = {
            TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16,
            UINT8_C(2),
            &ti_c54x_destination_indirect_addr7_pmad16_set},
        [TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_PA16] = {
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_PA16, UINT8_C(2),
            &ti_c54x_indirect_addr7_pa16_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
            UINT8_C(2), &ti_c54x_accumulator_source_destination_shift4_extension16_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16,
            UINT8_C(2), &ti_c54x_accumulator_source_destination_extension16_shift16_set},
        [TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR] = {
            TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR, UINT8_C(1),
            &ti_c54x_dual_destination_xar_yar_set},
        [TI_C54X_OPERAND_LAYOUT_DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR] = {
            TI_C54X_OPERAND_LAYOUT_DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR,
            UINT8_C(1),
            &ti_c54x_dual_round_source_destination_xar_yar_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4,
            UINT8_C(1), &ti_c54x_accumulator_dual_xmod_xar_shift4_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, UINT8_C(1),
            &ti_c54x_accumulator_arf_mod4_set},
        [TI_C54X_OPERAND_LAYOUT_ARF_MOD4] = {
            TI_C54X_OPERAND_LAYOUT_ARF_MOD4, UINT8_C(1),
            &ti_c54x_arf_mod4_set},
        [TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4] = {
            TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4, UINT8_C(2),
            &ti_c54x_arf_extension16_mod4_set},
        [TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4] = {
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4,
            UINT8_C(2), &ti_c54x_accumulator_arf_extension16_mod4_set}
};

const ti_c54x_operand_layout_plan *
ti_c54x_operand_layout_plan_for_id(ti_c54x_operand_layout_id id)
{
    if (id <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        id >= TI_C54X_OPERAND_LAYOUT_COUNT) {
        return NULL;
    }
    return &ti_c54x_operand_layout_plans[(size_t)id];
}

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
    case TI_C54X_OPERAND_LAYOUT_STATUS_BIT:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_status_bit_set;
    case TI_C54X_OPERAND_LAYOUT_CONDITION_ARF:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_condition_arf_set;
    case TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_ld_dp_set;
    case TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_ld_asm_set;
    case TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_ld_arp_set;
    case TI_C54X_OPERAND_LAYOUT_ARF:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_arf_set;
    case TI_C54X_OPERAND_LAYOUT_ARF_MOD:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_arf_mod_set;
    case TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_arf_extension16_set;
    case TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_arf_extension16_mod_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_arf_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_arf_mod_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_arf_mod_shift5_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_direct7_shift5_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_arf_mod4_shift5_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_immediate_k8_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_immediate_lk16_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_immediate_shift4_lk16_set;
    case TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_branch_pmad16_set;
    case TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_branch_cond_pmad16_set;
    case TI_C54X_OPERAND_LAYOUT_BRANCH_COND8:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_branch_cond8_set;
    case TI_C54X_OPERAND_LAYOUT_N_CONDITION8:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_n_condition8_set;
    case TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_branch_sind_pmad16_set;
    case TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_direct7_extension16_set;
    case TI_C54X_OPERAND_LAYOUT_ROUND_SELECT_INDIRECT_ADDR7:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_round_select_indirect_addr7_set;
    case TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_indirect_addr7_set;
    case TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_select_indirect_addr7_set;
    case TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_source_destination_indirect_addr7_set;
    case TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_indirect_addr7_source_destination_shift5_set;
    case TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_destination_indirect_addr7_set;
    case TI_C54X_OPERAND_LAYOUT_ROUND_INDIRECT_ADDR7:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_round_indirect_addr7_set;
    case TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_source_destination_round_set;
    case TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_destination_indirect_addr7_pmad16_set;
    case TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_PA16:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_indirect_addr7_pa16_set;
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
    case TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_dual_destination_xar_yar_set;
    case TI_C54X_OPERAND_LAYOUT_DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_dual_round_source_destination_xar_yar_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_dual_xmod_xar_shift4_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_accumulator_arf_mod4_set;
    case TI_C54X_OPERAND_LAYOUT_ARF_MOD4:
        *expected_word_count = UINT8_C(1);
        return &ti_c54x_arf_mod4_set;
    case TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_arf_extension16_mod4_set;
    case TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4:
        *expected_word_count = UINT8_C(2);
        return &ti_c54x_accumulator_arf_extension16_mod4_set;
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

bool ti_c54x_operand_layout_extract_all_trusted(
    const ti_c54x_operand_layout_plan *plan,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_operand_field_value values[TI_C54X_TRUSTED_OPERAND_CAPACITY],
    uint8_t *value_count)
{
    const ti_c54x_operand_layout_set *descriptor;
    uint8_t operand_index;

    if (plan == NULL || bundle == NULL || values == NULL || value_count == NULL) {
        return false;
    }
    descriptor = plan->descriptor;
    if (descriptor == NULL ||
        descriptor->operand_count >
            (uint8_t)TI_C54X_TRUSTED_OPERAND_CAPACITY) {
        return false;
    }

    for (operand_index = UINT8_C(0);
         operand_index < descriptor->operand_count; ++operand_index) {
        const ti_c54x_operand_field_layout *field =
            &descriptor->operands[operand_index];
        uint64_t raw_value = UINT64_C(0);
        uint8_t piece_index;

        for (piece_index = UINT8_C(0); piece_index < field->piece_count;
             ++piece_index) {
            const ti_c54x_operand_field_piece *piece =
                &field->pieces[piece_index];
            const uint32_t source_mask =
                (UINT32_C(1) << piece->width_bits) - UINT32_C(1);
            const uint16_t source_value = (uint16_t)(
                (bundle->program_words[piece->program_word_index] >>
                 piece->source_lsb) & source_mask);

            raw_value |= (uint64_t)source_value << piece->value_lsb;
        }
        values[operand_index].raw_value = raw_value;
        values[operand_index].width_bits = field->value_width_bits;
    }
    *value_count = descriptor->operand_count;
    return true;
}
