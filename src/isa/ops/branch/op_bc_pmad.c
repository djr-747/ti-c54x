/*
 * Module contract
 *
 * Owns: pure checked/trusted BC pmad,cond semantics over the sixteen
 * evidenced single conditions with taken PC_WRITE and not-taken NONE
 * emission and consumed-input-only knownness.
 * Does not own: catalog lookup generation, program-counter commit,
 * CPU sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_bc_pmad.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

/* Image-evidenced single condition codes (TI SPRU172C Table 2-12). */
#define BC_COND_TC UINT8_C(0x30)
#define BC_COND_NTC UINT8_C(0x20)
#define BC_COND_C UINT8_C(0x0C)
#define BC_COND_NC UINT8_C(0x08)
#define BC_COND_AEQ UINT8_C(0x45)
#define BC_COND_ANEQ UINT8_C(0x44)
#define BC_COND_AGT UINT8_C(0x46)
#define BC_COND_AGEQ UINT8_C(0x42)
#define BC_COND_ALT UINT8_C(0x43)
#define BC_COND_ALEQ UINT8_C(0x47)
#define BC_COND_BEQ UINT8_C(0x4D)
#define BC_COND_BNEQ UINT8_C(0x4C)
#define BC_COND_BGR UINT8_C(0x4E)
#define BC_COND_BGEQ UINT8_C(0x4A)
#define BC_COND_BLT UINT8_C(0x4B)
#define BC_COND_BLEQ UINT8_C(0x4F)

static bool bc_pmad_is_zero(uint64_t value)
{
    return (value & TI_C54X_U40_MASK) == UINT64_C(0);
}

static bool bc_pmad_is_negative(uint64_t value)
{
    return ((value >> 39U) & UINT64_C(1)) != UINT64_C(0);
}

static ti_c54x_op_status bc_pmad_evaluate(
    uint8_t cond, const ti_c54x_op_input_snapshot *input, bool *taken)
{
    uint64_t accumulator;

    switch (cond) {
    case BC_COND_TC:
    case BC_COND_NTC:
        if (!input->status_tc_known || input->status_tc > UINT8_C(1)) {
            return input->status_tc_known ? TI_C54X_OP_STATUS_INVALID_OPERAND
                                          : TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        *taken = (cond == BC_COND_TC) ? input->status_tc != UINT8_C(0)
                                      : input->status_tc == UINT8_C(0);
        return TI_C54X_OP_STATUS_OK;
    case BC_COND_C:
    case BC_COND_NC:
        if (!input->status_c_known || input->status_c > UINT8_C(1)) {
            return input->status_c_known ? TI_C54X_OP_STATUS_INVALID_OPERAND
                                         : TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        *taken = (cond == BC_COND_C) ? input->status_c != UINT8_C(0)
                                     : input->status_c == UINT8_C(0);
        return TI_C54X_OP_STATUS_OK;
    case BC_COND_AEQ:
    case BC_COND_ANEQ:
    case BC_COND_AGT:
    case BC_COND_AGEQ:
    case BC_COND_ALT:
    case BC_COND_ALEQ:
        if (!input->accumulator_a_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        accumulator = input->accumulator_a_raw40 & TI_C54X_U40_MASK;
        break;
    case BC_COND_BEQ:
    case BC_COND_BNEQ:
    case BC_COND_BGR:
    case BC_COND_BGEQ:
    case BC_COND_BLT:
    case BC_COND_BLEQ:
        if (!input->accumulator_b_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        accumulator = input->accumulator_b_raw40 & TI_C54X_U40_MASK;
        break;
    default:
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    /* Full-40-bit two's-complement signed compare against zero. */
    switch (cond) {
    case BC_COND_AEQ:
    case BC_COND_BEQ:
        *taken = bc_pmad_is_zero(accumulator);
        break;
    case BC_COND_ANEQ:
    case BC_COND_BNEQ:
        *taken = !bc_pmad_is_zero(accumulator);
        break;
    case BC_COND_AGT:
    case BC_COND_BGR:
        *taken = !bc_pmad_is_negative(accumulator) &&
                 !bc_pmad_is_zero(accumulator);
        break;
    case BC_COND_AGEQ:
    case BC_COND_BGEQ:
        *taken = !bc_pmad_is_negative(accumulator);
        break;
    case BC_COND_ALT:
    case BC_COND_BLT:
        *taken = bc_pmad_is_negative(accumulator);
        break;
    default:
        *taken = bc_pmad_is_negative(accumulator) ||
                 bc_pmad_is_zero(accumulator);
        break;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bc_pmad_emit(bool taken, uint16_t pmad,
                                      ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (taken) {
        staged = (ti_c54x_op_effect){
            .kind = TI_C54X_OP_EFFECT_PC_WRITE,
            .payload.pc_write = {.pmad = pmad}};
    } else {
        staged = (ti_c54x_op_effect){.kind = TI_C54X_OP_EFFECT_NONE};
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bc_pmad_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *cond,
    uint16_t *pmad)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value cond_field = {0U, 0U};
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || cond == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BC_PMAD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_BC_PMAD ||
        metadata->first_word_value != UINT16_C(0xF800) ||
        metadata->first_word_mask != UINT16_C(0xFF00) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC ||
        instruction->id != TI_C54X_OPCODE_ID_BC_PMAD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFF00)) != UINT16_C(0xF800)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &cond_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            cond_field.width_bits != UINT8_C(8) ||
            cond_field.raw_value > UINT64_C(0xFF) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *cond = (uint8_t)cond_field.raw_value;
    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_bc_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t cond;
    uint16_t pmad;
    bool taken;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = bc_pmad_extract(instruction, &cond, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = bc_pmad_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return bc_pmad_emit(taken, pmad, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_bc_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t cond;
    uint16_t pmad;
    bool taken;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(8) ||
        operands->values[0].raw_value > UINT64_C(0xFF) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    cond = (uint8_t)operands->values[0].raw_value;
    pmad = (uint16_t)operands->values[1].raw_value;
    status = bc_pmad_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return bc_pmad_emit(taken, pmad, effect);
}
