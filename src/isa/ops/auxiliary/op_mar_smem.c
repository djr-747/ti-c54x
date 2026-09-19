/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word
 * `MAR Smem` auxiliary-register modify (TI SPRU172C section MAR:
 * first word 0x6D00 under mask 0xFF00 with the 8-bit Smem byte
 * in word 0, execution applies the addressed indirect
 * modification with no memory access and no status effects).
 * Only the evidenced indirect MOD1 postdecrement cookie
 * composes; direct words and other modifiers reject.
 * Both paths share one values core that stages a
 * fire-and-forget auxiliary-register write effect (addressed
 * index, frozen value-after). The addressed AR knownness gate
 * runs before any staging. Only the evidenced postdecrement
 * composes; every other Smem form rejects.
 * Borrowed instruction, operand, input, and effect objects
 * are used for one call only; no pointer is retained and
 * failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_mar_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define MAR_SMEM_FIRST_HIGH UINT16_C(0x6D00)
#define MAR_SMEM_FIRST_MASK UINT16_C(0xFF00)
#define MAR_SMEM_INDIRECT_BIT UINT8_C(0x80)
#define MAR_SMEM_POSTDEC_MOD UINT8_C(0x01)

typedef struct mar_smem_fields {
    uint8_t ar_index;
} mar_smem_fields;

static ti_c54x_op_status mar_smem_values(
    const mar_smem_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    uint8_t known_bit;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (fields->ar_index > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* The addressed AR value freezes here: unknownness gates
     * the postdecrement before any staging. */
    known_bit =
        (uint8_t)(UINT8_C(1) << fields->ar_index);
    if ((input->auxiliary_registers_known_mask & known_bit) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }

    staged.kind = TI_C54X_OP_EFFECT_AR_WRITE;
    staged.payload.ar_write.ar_index = fields->ar_index;
    staged.payload.ar_write.value_after =
        (uint16_t)(input->auxiliary_registers_raw16[fields->ar_index] -
                   UINT16_C(1));
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mar_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    mar_smem_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value smem_field = {0U, 0U};
    uint8_t smem;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_MAR_SMEM ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_MAR_SMEM8 ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (instruction->bundle.program_words[0] & MAR_SMEM_FIRST_MASK) !=
            MAR_SMEM_FIRST_HIGH) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_MAR_SMEM8,
            UINT8_C(1), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &smem_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            smem_field.width_bits != UINT8_C(8) ||
            smem_field.raw_value > UINT64_C(0xFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    smem = (uint8_t)smem_field.raw_value;
    /* Only the evidenced indirect MOD1 postdecrement cookie
     * composes: direct words and other modifiers reject. */
    if ((smem & MAR_SMEM_INDIRECT_BIT) == UINT8_C(0) ||
        (uint8_t)((smem >> UINT8_C(3)) & UINT8_C(0x0F)) !=
            MAR_SMEM_POSTDEC_MOD) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields->ar_index = (uint8_t)(smem & UINT8_C(0x07));
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_mar_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    mar_smem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mar_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mar_smem_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_mar_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    mar_smem_fields fields;
    uint8_t smem;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(8) ||
        operands->values[0].raw_value > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    smem = (uint8_t)operands->values[0].raw_value;
    if ((smem & MAR_SMEM_INDIRECT_BIT) == UINT8_C(0) ||
        (uint8_t)((smem >> UINT8_C(3)) & UINT8_C(0x0F)) !=
            MAR_SMEM_POSTDEC_MOD) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.ar_index = (uint8_t)(smem & UINT8_C(0x07));
    return mar_smem_values(&fields, input, effect);
}
