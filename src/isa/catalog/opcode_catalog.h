#ifndef TI_C54X_ISA_OPCODE_CATALOG_H
#define TI_C54X_ISA_OPCODE_CATALOG_H

/*
 * Module contract
 *
 * Owns: opcode identity, immutable first-word/program-word metadata, and the
 * catalog-owned scalar operand-layout, repeatability, and fixed-width
 * input-resource classifications derived from the one catalog definition.
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
    TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16,
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16,
    TI_C54X_OPERAND_LAYOUT_COUNT
} ti_c54x_operand_layout_id;

typedef enum ti_c54x_repeatability_class {
    TI_C54X_REPEATABILITY_UNRESOLVED = 0,
    TI_C54X_REPEATABILITY_ELIGIBLE,
    TI_C54X_REPEATABILITY_INELIGIBLE,
    TI_C54X_REPEATABILITY_COUNT
} ti_c54x_repeatability_class;

typedef uint8_t ti_c54x_input_resource_mask;
/* Resource values are powers of two; T_REGISTER is the sixth resource (32). */
#define TI_C54X_INPUT_RESOURCES_NONE ((ti_c54x_input_resource_mask)0U)
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS ((ti_c54x_input_resource_mask)1U)
#define TI_C54X_INPUT_RESOURCES_STATUS_C ((ti_c54x_input_resource_mask)2U)
#define TI_C54X_INPUT_RESOURCES_STATUS_TC ((ti_c54x_input_resource_mask)4U)
#define TI_C54X_INPUT_RESOURCES_STATUS_SXM ((ti_c54x_input_resource_mask)8U)
#define TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS \
    ((ti_c54x_input_resource_mask)16U)
#define TI_C54X_INPUT_RESOURCES_T_REGISTER \
    ((ti_c54x_input_resource_mask)32U)
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C))
#define TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_TC))
#define TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_T_REGISTER))
#define TI_C54X_INPUT_RESOURCES_KNOWN_MASK \
    ((ti_c54x_input_resource_mask)(TI_C54X_INPUT_RESOURCES_ACCUMULATORS | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_C | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_TC | \
                                   TI_C54X_INPUT_RESOURCES_STATUS_SXM | \
                                   TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS | \
                                   TI_C54X_INPUT_RESOURCES_T_REGISTER))

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
