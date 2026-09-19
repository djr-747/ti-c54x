#ifndef TI_C54X_ISA_OPERAND_LAYOUT_INTERNAL_H
#define TI_C54X_ISA_OPERAND_LAYOUT_INTERNAL_H

/*
 * Module contract
 *
 * Owns: the private trusted hand-off from statically validated operand-layout
 * descriptors to one-pass packet extraction. Does not own: checked decoder
 * APIs, opcode semantics, CPU state, sequencing, buses, timing, diagnostics,
 * allocation, or caller-owned storage. Inputs are borrowed for one call and
 * all extracted values are copied into caller-owned fixed-capacity storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include "instruction_bundle.h"
#include "operand_layout.h"
#include "../catalog/opcode_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TI_C54X_TRUSTED_OPERAND_CAPACITY = 4
};

/* A plan is an immutable, process-lifetime hand-off selected by the binding
 * table.  Its identity and width are already checked when the table is built;
 * trusted extraction therefore consumes the pointer directly and never
 * resolves an operand-layout ID. */
typedef struct ti_c54x_operand_layout_plan {
    ti_c54x_operand_layout_id id;
    uint8_t program_word_count;
    const ti_c54x_operand_layout_set *descriptor;
} ti_c54x_operand_layout_plan;

/* Layout set tables live in operand_layout_sets.c with the
 * Smem-plus-lk and branch families split into
 * operand_layout_sets_smem_lk.c and operand_layout_sets_branch.c;
 * the plan table and resolver borrow them for the process
 * lifetime. */
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_extension16_mod4_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod4_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod_shift5_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_direct7_shift5_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_mod4_shift5_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_arf_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_immediate_k8_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_immediate_lk16_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_accumulator_immediate_shift4_lk16_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_source_destination_extension16_shift16_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_source_destination_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_source_destination_shift4_extension16_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_source_destination_shift5_set;
extern const ti_c54x_operand_layout_set ti_c54x_accumulator_source_set;
extern const ti_c54x_operand_layout_set ti_c54x_arf_extension16_mod4_set;
extern const ti_c54x_operand_layout_set ti_c54x_arf_extension16_mod_set;
extern const ti_c54x_operand_layout_set ti_c54x_arf_extension16_set;
extern const ti_c54x_operand_layout_set ti_c54x_arf_mod4_set;
extern const ti_c54x_operand_layout_set ti_c54x_arf_mod_set;
extern const ti_c54x_operand_layout_set ti_c54x_arf_set;
extern const ti_c54x_operand_layout_set ti_c54x_branch_cond8_set;
extern const ti_c54x_operand_layout_set ti_c54x_branch_cond_pmad16_set;
extern const ti_c54x_operand_layout_set ti_c54x_branch_sind_pmad16_set;
extern const ti_c54x_operand_layout_set ti_c54x_branch_pmad16_set;
extern const ti_c54x_operand_layout_set ti_c54x_condition_arf_set;
extern const ti_c54x_operand_layout_set ti_c54x_direct7_extension16_set;
extern const ti_c54x_operand_layout_set ti_c54x_mmr5_absolute16_set;
extern const ti_c54x_operand_layout_set ti_c54x_mmr4_mmr4_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_round_select_indirect_addr7_set;
extern const ti_c54x_operand_layout_set ti_c54x_dual_destination_xar_yar_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_dual_round_source_destination_xar_yar_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_accumulator_dual_xmod_xar_shift4_set;
extern const ti_c54x_operand_layout_set ti_c54x_indirect_addr7_set;
extern const ti_c54x_operand_layout_set ti_c54x_indirect_addr7_pa16_set;
extern const ti_c54x_operand_layout_set ti_c54x_n_condition8_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_select_indirect_addr7_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_select_dual_xmod_xar_cond4_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_source_destination_indirect_addr7_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_source_destination_shift5_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_destination_indirect_addr7_extension16_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_source_destination_indirect_addr7_extension16_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_extension16_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_absolute16_lk_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_indirect_addr7_absolute16_pmad_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_accumulator_indirect_addr7_absolute16_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_destination_indirect_addr7_set;
extern const ti_c54x_operand_layout_set ti_c54x_round_indirect_addr7_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_source_destination_round_set;
extern const ti_c54x_operand_layout_set
    ti_c54x_destination_indirect_addr7_pmad16_set;
extern const ti_c54x_operand_layout_set ti_c54x_ld_arp_set;
extern const ti_c54x_operand_layout_set ti_c54x_ld_asm_set;
extern const ti_c54x_operand_layout_set ti_c54x_ld_dp_set;
extern const ti_c54x_operand_layout_set ti_c54x_max_min_destination_set;
extern const ti_c54x_operand_layout_set ti_c54x_operand_layout_none;
extern const ti_c54x_operand_layout_set ti_c54x_frame_k_set;
extern const ti_c54x_operand_layout_set ti_c54x_rpt_k_set;
extern const ti_c54x_operand_layout_set ti_c54x_rpt_lk_set;
extern const ti_c54x_operand_layout_set ti_c54x_status_bit_set;

/* The binding table borrows these immutable plans for the process lifetime. */
extern const ti_c54x_operand_layout_plan ti_c54x_operand_layout_plans[
    TI_C54X_OPERAND_LAYOUT_COUNT];

/* Resolve an ID only at a checked boundary.  Trusted callers must use the
 * plan pointer carried by their immutable binding instead. */
const ti_c54x_operand_layout_plan *ti_c54x_operand_layout_plan_for_id(
    ti_c54x_operand_layout_id id);

/* Extract every field from one prevalidated static plan without revalidating
 * its descriptor or resolving an ID. False means only invalid call storage. */
bool ti_c54x_operand_layout_extract_all_trusted(
    const ti_c54x_operand_layout_plan *plan,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_operand_field_value values[TI_C54X_TRUSTED_OPERAND_CAPACITY],
    uint8_t *value_count);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPERAND_LAYOUT_INTERNAL_H */
