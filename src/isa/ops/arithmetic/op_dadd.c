/* Module contract
 * Owns: pure double-precision DADD Lmem effects: one kind-21
 * LONG32 READ over a page-0 direct or standard-indirect MOD0-MOD3
 * Lmem leg, plus the C16-gated completion (40-bit add with
 * OVM/C/OV at C16=0; dual independent 16-bit adds with no
 * status effects at C16=1). The dual-mode guard fill is the
 * sign extension of result bit 31: the only TI worked example
 * pins the positive case, and sign fill is the consistent
 * completion of two independent 16-bit sums.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_dadd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct dadd_fields {
    uint8_t source;
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;
} dadd_fields;

static ti_c54x_op_status dadd_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t first_msw,
    uint16_t second_lsw, ti_c54x_op_effect *next_effect);

static ti_c54x_op_status dadd_extract(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, dadd_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(expected_id);
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value got[4] = {{0U, 0U}, {0U, 0U},
                                          {0U, 0U}, {0U, 0U}};
    uint8_t index;

    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_C16 ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
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
        static const uint8_t widths[4] = {1U, 1U, 1U, 7U};
        static const uint64_t maxima[4] = {UINT64_C(1), UINT64_C(1),
                                           UINT64_C(1),
                                           UINT64_C(0x7F)};

        for (index = UINT8_C(0); index < UINT8_C(4); ++index) {
            if (ti_c54x_operand_layout_set_extract_operand(
                    &layout_view, &words, index, &got[index]) !=
                    TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
                got[index].width_bits != widths[index] ||
                got[index].raw_value > maxima[index]) {
                return TI_C54X_OP_STATUS_INVALID_DECODE;
            }
        }
    }
    fields->source = (uint8_t)got[0].raw_value;
    fields->destination = (uint8_t)got[1].raw_value;
    fields->indirect = (uint8_t)got[2].raw_value;
    fields->addr7 = (uint8_t)got[3].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dadd_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t source,
    uint8_t destination)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((source == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) ||
        (destination != source &&
         (destination == UINT8_C(0) ? !input->accumulator_a_known
                                   : !input->accumulator_b_known)) ||
        !input->status_sxm_known || !input->status_ovm_known ||
        !input->status_c16_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1) ||
        input->status_c16 > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dadd_stage_read(const dadd_fields *fields,
                                         ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.first_value_msw = UINT16_C(0);
    staged.payload.memory.second_value_lsw = UINT16_C(0);
    staged.payload.memory.complete_long32 = dadd_complete;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE;
    if (fields->indirect == UINT8_C(0)) {
        staged.payload.memory.address_spec.long32_single.dma7 =
            fields->addr7;
    } else {
        const uint8_t mode4 = (uint8_t)(fields->addr7 >> 3U);
        const uint8_t arf = (uint8_t)(fields->addr7 & UINT8_C(7));

        if (mode4 > UINT8_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged.payload.memory.address_spec.long32_single.indirect =
            UINT8_C(1);
        staged.payload.memory.address_spec.long32_single.mode = mode4;
        staged.payload.memory.address_spec.long32_single.arf = arf;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static uint64_t dadd_dual_result(uint32_t lmem32, uint64_t src40)
{
    /* Two independent 16-bit adds; the guard fill sign-extends
     * result bit 31 (the only TI worked example pins positive). */
    const uint32_t hi = ((lmem32 >> 16U) & UINT32_C(0xFFFF)) +
                        (uint32_t)((src40 >> 16U) & UINT64_C(0xFFFF));
    const uint32_t lo = (lmem32 & UINT32_C(0xFFFF)) +
                        (uint32_t)(src40 & UINT64_C(0xFFFF));
    const uint32_t combined =
        ((hi & UINT32_C(0xFFFF)) << 16U) | (lo & UINT32_C(0xFFFF));

    if ((combined & UINT32_C(0x80000000)) != UINT32_C(0)) {
        return (uint64_t)combined | UINT64_C(0xFF00000000);
    }
    return (uint64_t)combined;
}

static ti_c54x_op_status dadd_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t first_msw,
    uint16_t second_lsw, ti_c54x_op_effect *next_effect)
{
    dadd_fields fields;
    ti_c54x_op_status status;
    uint64_t src40;
    uint32_t lmem32;
    ti_c54x_op_effect staged = {0};

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dadd_extract(instruction, TI_C54X_OPCODE_ID_DADD, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = dadd_validate_inputs(input, fields.source, fields.destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    src40 = fields.source == UINT8_C(0) ? input->accumulator_a_raw40
                                        : input->accumulator_b_raw40;
    lmem32 = ((uint32_t)first_msw << 16U) | (uint32_t)second_lsw;
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields.destination;
    if (input->status_c16 != UINT8_C(0)) {
        staged.payload.accumulator_write.value_raw40 =
            dadd_dual_result(lmem32, src40);
        staged.payload.accumulator_write.status_delta.c_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        staged.payload.accumulator_write.status_delta.tc_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        staged.payload.accumulator_write.status_delta
            .destination_overflow_action = TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    } else {
        ti_c54x_u40_t addend = ti_c54x_u40_from_unsigned(
            (uint64_t)lmem32);
        ti_c54x_u40_add_result add_result;
        uint64_t value;
        bool carry;
        ti_c54x_signed32_overflow_direction overflow;

        if (input->status_sxm != UINT8_C(0) &&
            (first_msw & UINT16_C(0x8000)) != UINT16_C(0)) {
            addend = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFFFFFF)) |
                     (uint64_t)lmem32;
        }
        if (ti_c54x_u40_add_checked(src40, addend, &add_result) !=
            TI_C54X_ARITHMETIC40_ADDSUB_OK) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        value = add_result.value;
        carry = add_result.low32_carry;
        overflow = add_result.signed32_overflow;
        if (input->status_ovm != UINT8_C(0)) {
            if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
                value = UINT64_C(0x007fffffff);
            } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
                value = UINT64_C(0xff80000000);
            }
        }
        staged.payload.accumulator_write.value_raw40 = value;
        staged.payload.accumulator_write.status_delta.c_action =
            carry ? TI_C54X_OP_STATUS_BIT_WRITE_1
                  : TI_C54X_OP_STATUS_BIT_WRITE_0;
        staged.payload.accumulator_write.status_delta.tc_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        staged.payload.accumulator_write.status_delta
            .destination_overflow_action =
            overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
                ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
                : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    }
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_dadd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dadd_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dadd_extract(instruction, TI_C54X_OPCODE_ID_DADD, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = dadd_validate_inputs(input, fields.source, fields.destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return dadd_stage_read(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_dadd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dadd_fields fields;

    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    if (operands->count != UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1) ||
        operands->values[2].width_bits != UINT8_C(1) ||
        operands->values[2].raw_value > UINT64_C(1) ||
        operands->values[3].width_bits != UINT8_C(7) ||
        operands->values[3].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.source = (uint8_t)operands->values[0].raw_value;
    fields.destination = (uint8_t)operands->values[1].raw_value;
    fields.indirect = (uint8_t)operands->values[2].raw_value;
    fields.addr7 = (uint8_t)operands->values[3].raw_value;
    {
        const ti_c54x_op_status status =
            dadd_validate_inputs(input, fields.source, fields.destination);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    }
    return dadd_stage_read(&fields, effect);
}
