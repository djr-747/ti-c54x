#ifndef TI_C54X_ISA_OPCODE_CATALOG_H
#define TI_C54X_ISA_OPCODE_CATALOG_H

/*
 * Module contract
 *
 * Owns: opcode identity, immutable first-word/program-word metadata, and the
 * catalog-owned scalar operand-layout, repeatability, and fixed-width
 * input-resource classifications derived from the one catalog definition,
 * including private AR0-indexed MOD5/MOD6 and circular MOD8/MOD10 identities.
 * NONE is explicit and zero-valued so zero-initialized decode values are safe.
 * Does not own: opcode strings, timing, semantic effects, buses, CPU state,
 * program-counter sequencing, or diagnostics. Metadata pointers returned by
 * the lookup function are borrowed read-only and remain valid for the process
 * lifetime; callers retain no ownership and the catalog retains no caller
 * pointer.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_opcode_id {
    TI_C54X_OPCODE_ID_UNKNOWN = 0,
#define TI_C54X_OPCODE_EXACT(symbol, handler, first_word, program_words, \
                             operand_layout, repeatability, input_resources) \
    TI_C54X_OPCODE_ID_##symbol,
#define TI_C54X_OPCODE_MASKED(symbol, handler, first_word_value, \
                              first_word_mask, program_words, operand_layout, \
                              repeatability, input_resources) \
    TI_C54X_OPCODE_ID_##symbol,
#include "opcode_catalog.def"
#undef TI_C54X_OPCODE_EXACT
#undef TI_C54X_OPCODE_MASKED
    TI_C54X_OPCODE_ID_COUNT
} ti_c54x_opcode_id;

typedef enum ti_c54x_operand_layout_id {
    TI_C54X_OPERAND_LAYOUT_UNKNOWN = 0,
    TI_C54X_OPERAND_LAYOUT_NONE,
    /* Immediate repeat and status-field operand identities. */
    TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K,
    TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK,
    TI_C54X_OPERAND_LAYOUT_STATUS_BIT,
    TI_C54X_OPERAND_LAYOUT_CONDITION_ARF,
    TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP,
    TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM,
    TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP,
    /* Generic one-word auxiliary-register field (first-word bits 2:0). */
    TI_C54X_OPERAND_LAYOUT_ARF,
    /* Two-word auxiliary-register plus raw extension-word field. */
    TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF,
    /* Standard-indirect auxiliary-register fields with MOD bits (4:3). */
    TI_C54X_OPERAND_LAYOUT_ARF_MOD,
    TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
    TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16,
    /* One-word dual-memory ADD/SUB fields: destination, X AR, Y AR. */
    TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR,
    /* One-word dual-memory MAC fields: combined R/S/D control field,
     * X AR, Y AR. */
    TI_C54X_OPERAND_LAYOUT_DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR,
    /* One-word LD Xmem fields: destination, Xmod, raw Xar, SHFT. */
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4,
    /* Indexed standard-indirect fields retain existing operands and widen
     * MOD [4:3] to carry the MOD5/MOD6 discriminator in bits [6:3]. */
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
    TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
    TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
    /* Two-word long32 standard-indirect fields: D/S, ARF, lk, MOD[6:3]. */
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5,
    /* One-word destination-accumulator plus unsigned 8-bit immediate. */
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8,
    /* Two-word destination-accumulator plus copied 16-bit extension. */
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16,
    /* Two-word destination-accumulator plus 4-bit shift and 16-bit extension. */
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16,
    /* Two-word branch with copied 16-bit program-memory target. */
    TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16,
    /* Two-word branch with copied 8-bit condition and 16-bit target. */
    TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16,
    /* One-word branch with copied 8-bit condition only. */
    TI_C54X_OPERAND_LAYOUT_BRANCH_COND8,
    /* Two-word branch with copied 8-bit Sind and 16-bit target. */
    TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16,
    /* Two-word direct store with copied indirect flag, 7-bit direct
     * address, and 16-bit extension. */
    TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16,
    /* One-word multiply fields: rounding flag, accumulator select,
     * indirect flag, and 7-bit Smem address. */
    TI_C54X_OPERAND_LAYOUT_ROUND_SELECT_INDIRECT_ADDR7,
    /* One-word MPYA Smem fields: indirect flag and 7-bit Smem
     * address; the B destination is fixed and T is written. */
    TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7,
    /* One-word SQUR Smem fields: D destination bit, indirect flag,
     * and 7-bit Smem address; T is written. */
    TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7,
    /* One-word MACA Smem fields: round bit, indirect flag, and 7-bit
     * Smem address; the B destination is fixed and T is written. */
    TI_C54X_OPERAND_LAYOUT_ROUND_INDIRECT_ADDR7,
    /* One-word MACA T,src fields: source select, destination select,
     * and round bit; register-only with T preset and preserved. */
    TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND,
    TI_C54X_OPERAND_LAYOUT_COUNT
} ti_c54x_operand_layout_id;

typedef enum ti_c54x_repeatability_class {
    TI_C54X_REPEATABILITY_UNRESOLVED = 0,
    TI_C54X_REPEATABILITY_ELIGIBLE,
    TI_C54X_REPEATABILITY_INELIGIBLE,
    TI_C54X_REPEATABILITY_COUNT
} ti_c54x_repeatability_class;

typedef uint16_t ti_c54x_input_resource_mask;
/* Resource values are powers of two; RETURN_REGISTER is the eleventh
 * resource (1024) and STATUS_FRCT is the twelfth resource (2048). */
#define TI_C54X_INPUT_RESOURCES_NONE ((ti_c54x_input_resource_mask)0U)
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS ((ti_c54x_input_resource_mask)1U)
#define TI_C54X_INPUT_RESOURCES_STATUS_C ((ti_c54x_input_resource_mask)2U)
#define TI_C54X_INPUT_RESOURCES_STATUS_TC ((ti_c54x_input_resource_mask)4U)
#define TI_C54X_INPUT_RESOURCES_STATUS_SXM ((ti_c54x_input_resource_mask)8U)
#define TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS \
    ((ti_c54x_input_resource_mask)16U)
#define TI_C54X_INPUT_RESOURCES_T_REGISTER \
    ((ti_c54x_input_resource_mask)32U)
#define TI_C54X_INPUT_RESOURCES_STATUS_SST ((ti_c54x_input_resource_mask)64U)
#define TI_C54X_INPUT_RESOURCES_STATUS_OVM ((ti_c54x_input_resource_mask)128U)
#define TI_C54X_INPUT_RESOURCES_STATUS_ASM ((ti_c54x_input_resource_mask)256U)
#define TI_C54X_INPUT_RESOURCES_STACK_POINTER ((ti_c54x_input_resource_mask)512U)
#define TI_C54X_INPUT_RESOURCES_RETURN_REGISTER \
    ((ti_c54x_input_resource_mask)1024U)
#define TI_C54X_INPUT_RESOURCES_STATUS_FRCT \
    ((ti_c54x_input_resource_mask)2048U)
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_TC))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_TC))
#define TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_T_REGISTER))
#define TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_SST \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SST))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_ASM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_ASM))
#define TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_C_SXM_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
/* Fast interrupt returns read SP plus the RTN return register. */
#define TI_C54X_INPUT_RESOURCES_STACK_POINTER_RETURN_REGISTER \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_STACK_POINTER | \
                                   TI_C54X_INPUT_RESOURCES_RETURN_REGISTER))
/* Delayed conditional calls read condition inputs plus SP. */
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_TC | \
                                   TI_C54X_INPUT_RESOURCES_STACK_POINTER))
/* Dual Xmem loads read ARs plus SXM/OVM but never carry. */
#define TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
/* MPYA bodies read accumulator A plus FRCT/OVM; T is written, not read. */
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_FRCT | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
/* Multiply-accumulate bodies read accumulators plus T plus FRCT/OVM. */
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_T_REGISTER | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_FRCT | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
#define TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_FRCT_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_FRCT | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_FRCT | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM))
#define TI_C54X_INPUT_RESOURCES_KNOWN_MASK \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_TC | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS | \
                                   TI_C54X_INPUT_RESOURCES_T_REGISTER | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SST | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_OVM | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_ASM | \
                                   TI_C54X_INPUT_RESOURCES_STACK_POINTER | \
                                   TI_C54X_INPUT_RESOURCES_RETURN_REGISTER | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_FRCT))

typedef struct ti_c54x_opcode_metadata {
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    uint16_t first_word_mask;
    uint8_t program_word_count;
    ti_c54x_operand_layout_id operand_layout;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_input_resource_mask input_resources;
} ti_c54x_opcode_metadata;

/* Return immutable borrowed metadata, or NULL for an ID outside the enum. */
const ti_c54x_opcode_metadata *ti_c54x_opcode_metadata_for_id(
    ti_c54x_opcode_id id);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPCODE_CATALOG_H */
