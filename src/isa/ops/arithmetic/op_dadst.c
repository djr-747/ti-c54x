/* Module contract
 * Owns: pure double-precision DADST Lmem effects: one kind-21
 * LONG32 READ over a page-0 direct or standard-indirect MOD0-MOD3
 * Lmem leg, plus the C16-gated completion from TI SPRU172C DADST
 * (p.4-39 Execution/Status/Description): at C16=0 dst is Lmem
 * plus the 32-bit T:T composite with SXM widening, OVM
 * saturation, C from the bit-32 carry, and OVdst; at C16=1 the
 * high half adds T and the low half subtracts T with the guard
 * sign-filled from result bit 31, C set when the low half is
 * clean, OV/TC preserved, and no saturation regardless of OVM.
 * No catalog row exists for this opcode yet, so decode is keyed
 * off the raw first word (0x5A00 under 0xFE00) with the shared
 * source/destination/indirect/addr7 layout; catalog binding
 * belongs to a later owner.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_dadst.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct dadst_fields {
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;
} dadst_fields;

static ti_c54x_op_status dadst_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t first_msw,
    uint16_t second_lsw, ti_c54x_op_effect *next_effect);

static ti_c54x_op_status dadst_extract(
    const ti_c54x_decoded_instruction *instruction, dadst_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value got[4] = {{0U, 0U}, {0U, 0U},
                                          {0U, 0U}, {0U, 0U}};
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7 ||
        instruction->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        instruction->input_resources !=
            (ti_c54x_input_resource_mask)(
                TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_C16 |
                TI_C54X_INPUT_RESOURCES_T_REGISTER) ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFE00)) != UINT16_C(0x5A00)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7,
            UINT8_C(1), &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
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
    if (got[0].raw_value != UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    fields->destination = (uint8_t)got[1].raw_value;
    fields->indirect = (uint8_t)got[2].raw_value;
    fields->addr7 = (uint8_t)got[3].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dadst_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t destination)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((destination == UINT8_C(0) ? !input->accumulator_a_known
                                  : !input->accumulator_b_known) ||
        !input->status_sxm_known || !input->status_ovm_known ||
        !input->status_c16_known || !input->t_register_known) {
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

static ti_c54x_op_status dadst_stage_read(const dadst_fields *fields,
                                          ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.first_value_msw = UINT16_C(0);
    staged.payload.memory.second_value_lsw = UINT16_C(0);
    staged.payload.memory.complete_long32 = dadst_complete;
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

static uint64_t dadst_widen32(uint32_t value32, uint8_t sxm)
{
    if ((sxm != UINT8_C(0)) &&
        ((value32 & UINT32_C(0x80000000)) != UINT32_C(0))) {
        return (TI_C54X_U40_MASK & ~UINT64_C(0xFFFFFFFF)) |
               (uint64_t)value32;
    }
    return (uint64_t)value32;
}

static uint64_t dadst_dual_result(uint16_t high, uint16_t low)
{
    const uint32_t combined =
        ((uint32_t)high << 16U) | (uint32_t)low;

    if ((combined & UINT32_C(0x80000000)) != UINT32_C(0)) {
        return (uint64_t)combined | UINT64_C(0xFF00000000);
    }
    return (uint64_t)combined;
}

static ti_c54x_op_status dadst_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t first_msw,
    uint16_t second_lsw, ti_c54x_op_effect *next_effect)
{
    dadst_fields fields;
    ti_c54x_op_status status;
    uint32_t lmem32;
    uint16_t t;
    ti_c54x_op_effect staged = {0};

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dadst_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = dadst_validate_inputs(input, fields.destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    lmem32 = ((uint32_t)first_msw << 16U) | (uint32_t)second_lsw;
    t = input->t_register_raw16;
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields.destination;
    if (input->status_c16 != UINT8_C(0)) {
        const uint32_t lmem_hi =
            (lmem32 >> 16U) & UINT32_C(0xFFFF);
        const uint32_t lmem_lo = lmem32 & UINT32_C(0xFFFF);
        const uint32_t lane = (uint32_t)t;
        const uint32_t hi_sum = lmem_hi + lane;
        const bool lo_no_borrow = lmem_lo >= lane;
        const uint16_t hi = (uint16_t)(hi_sum & UINT32_C(0xFFFF));
        const uint16_t lo =
            (uint16_t)((lmem_lo - lane) & UINT32_C(0xFFFF));

        staged.payload.accumulator_write.value_raw40 =
            dadst_dual_result(hi, lo);
        staged.payload.accumulator_write.status_delta.c_action =
            lo_no_borrow ? TI_C54X_OP_STATUS_BIT_WRITE_1
                         : TI_C54X_OP_STATUS_BIT_WRITE_0;
        staged.payload.accumulator_write.status_delta.tc_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        staged.payload.accumulator_write.status_delta
            .destination_overflow_action = TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    } else {
        const uint32_t composite32 =
            ((uint32_t)t << 16U) | (uint32_t)t;
        uint64_t lmem40 = dadst_widen32(lmem32, input->status_sxm);
        uint64_t composite40 =
            dadst_widen32(composite32, input->status_sxm);
        ti_c54x_u40_add_result add_result;
        uint64_t value;
        bool carry;
        ti_c54x_signed32_overflow_direction overflow;

        if (ti_c54x_u40_add_checked(lmem40, composite40, &add_result) !=
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

ti_c54x_op_status ti_c54x_op_dadst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dadst_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dadst_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = dadst_validate_inputs(input, fields.destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return dadst_stage_read(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_dadst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dadst_fields fields;

    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    if (operands->count != UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value != UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1) ||
        operands->values[2].width_bits != UINT8_C(1) ||
        operands->values[2].raw_value > UINT64_C(1) ||
        operands->values[3].width_bits != UINT8_C(7) ||
        operands->values[3].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.destination = (uint8_t)operands->values[1].raw_value;
    fields.indirect = (uint8_t)operands->values[2].raw_value;
    fields.addr7 = (uint8_t)operands->values[3].raw_value;
    {
        const ti_c54x_op_status status =
            dadst_validate_inputs(input, fields.destination);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    }
    return dadst_stage_read(&fields, effect);
}
