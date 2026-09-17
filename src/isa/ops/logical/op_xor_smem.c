/* Module contract
 * Owns: checked and trusted pure XOR Smem semantics for the single
 * FORM-T2-09-01 row (TI SPRU172C Sec4 XOR, p. 4-201: dst ^= Smem)
 * over direct page-0 and standard-indirect MOD0--MOD3 reads through
 * the memory effect. The 16-bit Smem word is zero-extended unshifted
 * and XORed full-width with the destination accumulator; C and TC
 * are preserved and no SXM/OVM input is consumed. Exotic indirect
 * modes (mode bits above MOD3) defer as INVALID_OPERAND.
 * Does not own: catalog rows, operand-layout definitions, decode,
 * dispatch binding, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_xor_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/memory/memory_effect.h"

typedef struct xor_smem_fields {
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;
} xor_smem_fields;

static ti_c54x_op_status xor_smem_validate_fields(
    uint8_t destination, uint8_t indirect, uint8_t addr7)
{
    if (destination > UINT8_C(1) || indirect > UINT8_C(1) ||
        addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status xor_smem_validate_destination(
    const ti_c54x_op_input_snapshot *input, uint8_t destination)
{
    ti_c54x_u40_t raw40;

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    raw40 = destination == UINT8_C(0) ? input->accumulator_a_raw40
                                     : input->accumulator_b_raw40;
    if (destination == UINT8_C(0) ? !input->accumulator_a_known
                                 : !input->accumulator_b_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status xor_smem_complete_for(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint8_t destination,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t destination_raw40;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    ti_c54x_op_status status;

    (void)instruction;
    status = xor_smem_validate_destination(input, destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    destination_raw40 = destination == UINT8_C(0)
                            ? input->accumulator_a_raw40
                            : input->accumulator_b_raw40;
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 =
        destination_raw40 ^ (ti_c54x_u40_t)read_value_word;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status xor_smem_complete_dst_a(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    return xor_smem_complete_for(instruction, input, UINT8_C(0),
                                 read_value_word, next_effect);
}

static ti_c54x_op_status xor_smem_complete_dst_b(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    return xor_smem_complete_for(instruction, input, UINT8_C(1),
                                 read_value_word, next_effect);
}

static ti_c54x_op_status xor_smem_stage_read(
    const xor_smem_fields *fields, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (fields == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete =
        fields->destination == UINT8_C(0) ? xor_smem_complete_dst_a
                                          : xor_smem_complete_dst_b;
    staged.payload.memory.load_t_with_read = false;
    if (fields->indirect == UINT8_C(0)) {
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        staged.payload.memory.address_spec.direct_dma7 = fields->addr7;
    } else {
        const uint8_t mode = (uint8_t)(fields->addr7 >> 3);
        const uint8_t arf = (uint8_t)(fields->addr7 & UINT8_C(7));

        if (mode > UINT8_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
        staged.payload.memory.address_spec.mode =
            (ti_c54x_indirect_single_mode)mode;
        staged.payload.memory.address_spec.arf = arf;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_xor_smem_values(
    uint8_t destination,
    uint8_t indirect,
    uint8_t addr7,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    xor_smem_fields fields;
    ti_c54x_op_status status;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = xor_smem_validate_fields(destination, indirect, addr7);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = xor_smem_validate_destination(input, destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    fields.destination = destination;
    fields.indirect = indirect;
    fields.addr7 = addr7;
    return xor_smem_stage_read(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_xor_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[2].width_bits != UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    destination = (uint8_t)operands->values[0].raw_value;
    indirect = (uint8_t)operands->values[1].raw_value;
    addr7 = (uint8_t)operands->values[2].raw_value;
    return ti_c54x_op_xor_smem_values(destination, indirect, addr7, input,
                                      effect);
}
