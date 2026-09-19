/*
 * Module contract
 *
 * Owns: Smem-leg spec synthesis and pair snapshot preparation
 * for one Smem-to-program DATA pair (MVPD/MVDP, including the
 * absolute-Smem arm); see
 * memory_instruction_smem_program_pair.h.
 * Does not own: transaction orchestration, leg issue, endpoint
 * traffic, architectural mutation, opcode semantics, timing,
 * observers, diagnostics, allocation, I/O, or global state.
 * Transaction storage is borrowed for each call.
 */

#include "memory_instruction_smem_program_pair.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "foundation/arithmetic40.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"

static bool
synthesize_smem_spec_from_fields(uint8_t indirect, uint8_t smem_mode,
                                 uint8_t smem_arf, uint8_t smem_dma7,
                                 const ti_c54x_op_input_snapshot *input,
                                 ti_c54x_op_memory_address_spec *smem_spec)
{
    ti_c54x_op_memory_address_spec staged = {0};

    if (input == NULL || smem_spec == NULL) {
        return false;
    }
    if (indirect == UINT8_C(0)) {
        if (smem_mode != UINT8_C(0) || smem_arf != UINT8_C(0) ||
            smem_dma7 > UINT8_C(0x7F) || !input->status_dp_known) {
            return false;
        }
        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16;
        staged.long_offset_word = input->status_dp;
        staged.direct_dma7 = smem_dma7;
        *smem_spec = staged;
        return true;
    }
    /* Single-indirect Smem leg: the DATA-leg direction gates the
     * mode exactly like the effect validation (MOD0--MOD3; the
     * leaf already gates this, the check here is defense in
     * depth). */
    if (indirect != UINT8_C(1) || smem_dma7 != UINT8_C(0) ||
        smem_arf > UINT8_C(7) || smem_mode > UINT8_C(3)) {
        return false;
    }
    staged.kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    staged.mode = (ti_c54x_indirect_single_mode)smem_mode;
    staged.arf = smem_arf;
    *smem_spec = staged;
    return true;
}

bool
ti_c54x_memory_instruction_smem_program_pair_synthesize_smem_spec(
    const ti_c54x_op_memory_address_spec *arm,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_memory_address_spec *smem_spec)
{
    if (arm == NULL) {
        return false;
    }
    if (arm->kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
        /* Absolute Smem leg: the 16-bit absolute DATA address
         * names the word with no state dependence (MVDM/MVMD
         * absolute precedent). */
        ti_c54x_op_memory_address_spec staged = {0};

        staged.kind = TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16;
        staged.absolute_data.address =
            arm->smem_absolute_program.absolute;
        staged.absolute_data.mmr_sidecar_kind =
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE;
        staged.absolute_data.mmr_sidecar_number = UINT8_C(0);
        *smem_spec = staged;
        return true;
    }
    if (arm->kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16) {
        return synthesize_smem_spec_from_fields(
            arm->smem_program_a.indirect, arm->smem_program_a.smem_mode,
            arm->smem_program_a.smem_arf, arm->smem_program_a.smem_dma7,
            input, smem_spec);
    }
    return synthesize_smem_spec_from_fields(
        arm->smem_program.indirect, arm->smem_program.smem_mode,
        arm->smem_program.smem_arf, arm->smem_program.smem_dma7,
        input, smem_spec);
}

/* The PROGRAM leg is absolute: the 16-bit address names the word
 * with no state dependence and no post-update (DATA_PROGRAM
 * precedent). The template value stays zero until the observed
 * first-leg word replaces it at second-leg issue. */
static bool
build_program_request(uint16_t program_address_word,
                      ti_c54x_bus_word16_request_access access,
                      ti_c54x_bus_word16_request *out_request)
{
    ti_c54x_bus_word16_request_result built;

    if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        built = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
            (uint32_t)program_address_word);
    } else if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
            (uint32_t)program_address_word, UINT16_C(0));
    } else {
        return false;
    }
    if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK ||
        !ti_c54x_bus_word16_request_is_valid(&built.request)) {
        return false;
    }
    *out_request = built.request;
    return true;
}

bool
ti_c54x_memory_instruction_smem_program_pair_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *arm;
    ti_c54x_op_memory_address_spec smem_spec = {0};
    ti_c54x_memory_address_resolution smem_resolution = {0};
    ti_c54x_bus_word16_request program_request = {0};
    ti_c54x_bus_word16_request_access program_access;
    ti_c54x_smem_program_pair_address_snapshot *snapshot;
    uint16_t program_address_word;

    if (transaction == NULL || state == NULL ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    arm = &memory->address_spec;
    if (arm->kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16) {
        /* Immediate-pmad pair: the PROGRAM address rides the arm. */
        if (!synthesize_smem_spec_from_fields(
                arm->smem_program.indirect,
                arm->smem_program.smem_mode, arm->smem_program.smem_arf,
                arm->smem_program.smem_dma7,
                &transaction->candidate.input, &smem_spec)) {
            return false;
        }
        program_address_word = arm->smem_program.pmad;
    } else if (arm->kind ==
               TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16) {
        /* Accumulator-addressed pair: PAR <- A resolves from the
         * frozen input (the leaf gates A knownness; the u40 mask
         * check here is defense in depth). */
        if (!synthesize_smem_spec_from_fields(
                arm->smem_program_a.indirect,
                arm->smem_program_a.smem_mode,
                arm->smem_program_a.smem_arf,
                arm->smem_program_a.smem_dma7,
                &transaction->candidate.input, &smem_spec)) {
            return false;
        }
        if (!transaction->candidate.input.accumulator_a_known ||
            transaction->candidate.input.accumulator_a_raw40 >
                TI_C54X_U40_MASK) {
            return false;
        }
        program_address_word =
            (uint16_t)transaction->candidate.input.accumulator_a_raw40;
    } else if (arm->kind ==
               TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
        /* Absolute-Smem pair: the Smem leg synthesizes the
         * absolute DATA spec with no state dependence; the
         * PROGRAM address rides the arm. */
        if (!ti_c54x_memory_instruction_smem_program_pair_synthesize_smem_spec(
                arm, &transaction->candidate.input, &smem_spec)) {
            return false;
        }
        program_address_word = arm->smem_absolute_program.pmad;
    } else {
        return false;
    }
    /* The Smem leg runs at the effect access (the access names
     * the Smem-leg direction); the PROGRAM leg runs inverse. */
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &smem_spec, memory->access, UINT16_C(0),
            &smem_resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return false;
    }
    program_access =
        (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE)
            ? TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            : TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    if (!build_program_request(program_address_word, program_access,
                               &program_request)) {
        return false;
    }
    snapshot =
        ti_c54x_memory_instruction_smem_program_pair_mutable_snapshot_for_kind(
            &transaction->candidate);
    snapshot->smem_resolution = smem_resolution;
    snapshot->program_request = program_request;
    snapshot->second_leg_issued = UINT8_C(0);
    transaction->child_kind =
        TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_PROGRAM_PAIR_WORD16;
    return true;
}
