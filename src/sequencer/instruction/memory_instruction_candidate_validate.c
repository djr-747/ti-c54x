/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied root memory
 * instruction candidates: dispatch across dual, DATA+PROGRAM, PORT,
 * LONG32, X-only, mode-independent STACK_PUSH/STACK_POP arms
 * with their delayed-branch twins, and STACK_PUSH_PAIR/STACK_POP_PAIR
 * arms, plus the shared single-leg tail
 * (generic resolution selection, CMPT=0/ARP=0 gating, and the
 * indirect-family liveness switch). Single-leg DATA resolution
 * shape lives in the memory_candidate_single_leg module, page-0 and
 * DP-relative direct arms in memory_candidate_direct, PORT pair
 * checks in memory_instruction_port_candidate_validate, and
 * stack-pair checks in
 * memory_instruction_stack_pair_candidate_validate.
 * Does not own: transaction lifecycle, endpoint I/O, architectural mutation,
 * effect completion, opcode dispatch, timing, observers, or diagnostics.
 */

#include "memory_instruction_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/block_size_register.h"
#include "memory_instruction_data_program_candidate_validate.h"
#include "memory_instruction_dual_candidate_validate.h"
#include "memory_instruction_port_candidate_validate.h"
#include "memory_instruction_stack_pair_candidate_validate.h"
#include "memory_instruction_smem_absolute_pair_candidate_validate.h"
#include "memory_instruction_smem_program_pair_candidate_validate.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_candidate_absolute_data.h"
#include "sequencer/instruction/memory_candidate_long32_single.h"
#include "sequencer/instruction/memory_candidate_single_leg.h"

bool
ti_c54x_memory_instruction_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate)
{
    ti_c54x_program_pc_bridge_effect planned = {0};

    if (candidate == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        !ti_c54x_op_memory_effect_is_valid(&candidate->effect.payload.memory) ||
        candidate->instructions_attempted_after == UINT64_C(0) ||
        candidate->instructions_retired_after == UINT64_C(0)) {
        return false;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16) {
        if (!ti_c54x_memory_instruction_dual_candidate_shape_is_valid(
                &candidate->dual_address_snapshot,
                &candidate->effect.payload.memory)) {
            return false;
        }
    } else if (candidate->effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        if (!ti_c54x_memory_instruction_port_candidate_shape_is_valid(
                candidate)) {
            return false;
        }
    } else if (candidate->effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 ||
               candidate->effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) {
        if (!ti_c54x_memory_instruction_stack_pair_candidate_shape_is_valid(
                candidate)) {
            return false;
        }
    } else if (candidate->effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
        if (!ti_c54x_memory_instruction_smem_absolute_pair_candidate_shape_is_valid(
                candidate)) {
            return false;
        }
    } else if (candidate->effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16 ||
               candidate->effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16 ||
               candidate->effect.payload.memory.address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
        if (!ti_c54x_memory_instruction_smem_program_pair_candidate_shape_is_valid(
                candidate)) {
            return false;
        }
    } else if (candidate->effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16) {
        if (!ti_c54x_memory_instruction_data_program_candidate_shape_is_valid(
                &candidate->dual_address_snapshot,
                &candidate->effect.payload.memory)) {
            return false;
        }
    } else if (candidate->effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16) {
        if (!ti_c54x_memory_candidate_absolute_data_shape_is_valid(
                &candidate->address_resolution,
                &candidate->effect.payload.memory)) {
            return false;
        }
    } else if (!ti_c54x_memory_candidate_single_leg_shape_is_valid(
                   &candidate->address_resolution,
                   &candidate->effect.payload.memory)) {
        return false;
    }
    if (ti_c54x_program_pc_bridge_plan(
            candidate->pc_effect.public_pc_word_before,
            candidate->pc_effect.program_word_count, &planned) !=
        TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return false;
    }
    if (planned.public_pc_word_before !=
            candidate->pc_effect.public_pc_word_before ||
        planned.common_pc_word_before !=
            candidate->pc_effect.common_pc_word_before ||
        planned.common_pc_word_after !=
            candidate->pc_effect.common_pc_word_after ||
        planned.public_pc_word_after !=
            candidate->pc_effect.public_pc_word_after ||
        planned.program_word_count != candidate->pc_effect.program_word_count ||
        candidate->instruction.bundle.program_word_count !=
            candidate->pc_effect.program_word_count) {
        return false;
    }
    if (candidate->effect.payload.memory.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        if (candidate->effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16 ||
            candidate->effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16) {
            return candidate->dual_address_snapshot.x_request
                           .write_value_word == UINT16_C(0) &&
                   candidate->dual_address_snapshot.y_request
                           .write_value_word == UINT16_C(0);
        }
        if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
            /* PORTW stages a zero IO template; the DATA leg it reads
             * carries no store value. */
            return candidate->port_address_snapshot.data_resolution.request
                       .write_value_word == UINT16_C(0);
        }
        if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) {
            /* POPD reads the TOS stack leg first; that frozen READ
             * template carries no store value. */
            return candidate->stack_pair_address_snapshot.stack_resolution
                       .request.write_value_word == UINT16_C(0);
        }
        if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
            /* MVKD reads the dmad leg first; that frozen READ
             * template carries no store value. */
            return candidate->smem_absolute_address_snapshot.dmad_resolution
                       .request.write_value_word == UINT16_C(0);
        }
        if (candidate->effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16 ||
            candidate->effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16 ||
            candidate->effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
            /* MVDP reads the Smem leg first; that frozen READ
             * template carries no store value (the PROGRAM WRITE
             * template below rides the built request, not the
             * Smem resolution). */
            return candidate->smem_program_address_snapshot.smem_resolution
                       .request.write_value_word == UINT16_C(0);
        }
        return candidate->address_resolution.request.write_value_word ==
               UINT16_C(0);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        return candidate->address_resolution.request.write_value_word ==
               candidate->effect.payload.memory.first_value_msw;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        /* PORTR resolves the DATA WRITE with a zero template until
         * the IO observation replaces it at second-leg issue. */
        return candidate->port_address_snapshot.data_resolution.request
                   .write_value_word ==
               candidate->effect.payload.memory.value_word;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16) {
        /* PSHD freezes the TOS stack-leg WRITE with a zero template;
         * the observed Smem word replaces it in the built second-leg
         * request at issue, never in the snapshot. */
        return candidate->stack_pair_address_snapshot.stack_resolution.request
                   .write_value_word ==
               candidate->effect.payload.memory.value_word;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
        /* MVDK freezes the dmad-leg WRITE with a zero template; the
         * observed Smem word replaces it in the built second-leg
         * request at issue, never in the snapshot. */
        return candidate->smem_absolute_address_snapshot.dmad_resolution
                   .request.write_value_word ==
               candidate->effect.payload.memory.value_word;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16 ||
            candidate->effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16) {
        /* MVPD freezes the Smem-leg WRITE with a zero template; the
         * observed PROGRAM word replaces it in the built
         * second-leg request at issue, never in the snapshot. */
        return candidate->smem_program_address_snapshot.smem_resolution
                   .request.write_value_word ==
               candidate->effect.payload.memory.value_word;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16 &&
        candidate->effect.payload.memory.address_spec.absolute_data
                .mmr_sidecar_kind ==
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE) {
        /* MVMD freezes the DATA-leg WRITE with a zero template; the
         * resume path snapshots the MMR source into the issued
         * request, never into the staged effect (the gate rejects
         * forged nonzero staged values). No value echo applies:
         * the address and routability echoes ride the absolute
         * shape arm above, and the snapshot is stable
         * single-writer state across WAIT retries. */
        return true;
    }
    return candidate->address_resolution.request.write_value_word ==
           candidate->effect.payload.memory.value_word;
}

static bool
x_only_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_op_memory_address_spec *spec;
    uint16_t selected = UINT16_C(0);

    /* The shape check above already pins the X-only operand/update
     * contract. Liveness observes no CMPT/ARP state: only the X selector
     * plus the MOD3 AR0/BK shadows. */
    address_effect = &candidate->address_resolution.address_result.effect;
    spec = &candidate->effect.payload.memory.address_spec;
    if (!ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect, state) ||
        address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
        address_effect->source.value.ar.index != spec->dual.x_ar ||
        ti_c54x_arch_state_read_ar(
            state, spec->dual.x_ar, &selected) !=
            TI_C54X_ARCH_STATE_OK ||
        selected != address_effect->source.value.ar.value_before) {
        return false;
    }
    if (spec->dual.x_mode != UINT8_C(3)) {
        return true;
    }
    if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &selected) !=
            TI_C54X_ARCH_STATE_OK ||
        selected != candidate->address_resolution.ar0_shadow) {
        return false;
    }
    if (ti_c54x_arch_state_read_bk(state, &selected) !=
        TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    return selected == candidate->address_resolution.bk_shadow;
}

static bool
stack_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;
    uint16_t live_sp = UINT16_C(0);

    /* Pushes and pops observe no CMPT/ARP state: only the live PC anchor
     * plus the frozen SP snapshot. The shape check above already pins
     * the operand/update contract. RTN pops additionally pin the live
     * RTN register against the value frozen at prepare. */
    address_effect = &candidate->address_resolution.address_result.effect;
    if (!ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect, state) ||
        address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_STACK_PUSH) {
        return false;
    }
    if (ti_c54x_arch_state_read_sp(state, &live_sp) !=
        TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    if (live_sp != address_effect->source.value.stack_push.sp_before) {
        return false;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        uint16_t live_rtn = UINT16_C(0);

        if (ti_c54x_arch_state_read_rtn(state, &live_rtn) !=
            TI_C54X_ARCH_STATE_OK) {
            return false;
        }
        return live_rtn == candidate->effect.payload.memory.value_word;
    }
    return true;
}

bool
ti_c54x_memory_instruction_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_memory_address_resolution *resolution;
    uint8_t cmpt = UINT8_C(0);
    uint8_t arp = UINT8_C(0);
    uint16_t selected_ar = UINT16_C(0);

    if (candidate == NULL || state == NULL) {
        return false;
    }
    /* Validate the copied candidate on every resume as well as before the
     * first issue.  This is deliberately structural only: it never invokes
     * the address resolver, so a pending candidate cannot be replanned. */
    if (!ti_c54x_memory_instruction_candidate_shape_is_valid(candidate)) {
        return false;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16) {
        return ti_c54x_memory_instruction_dual_candidate_matches_live(
            candidate, state);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16) {
        return ti_c54x_memory_instruction_data_program_candidate_matches_live(
            candidate, state);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY) {
        return x_only_candidate_matches_live(candidate, state);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16) {
        return ti_c54x_memory_instruction_stack_pair_matches_live(candidate,
                                                                  state);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
        return ti_c54x_memory_instruction_smem_absolute_pair_matches_live(
            candidate, state);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16) {
        return ti_c54x_memory_instruction_smem_program_pair_matches_live(
            candidate, state);
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 ||
        candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        return stack_candidate_matches_live(candidate, state);
    }
    /* The PORT DATA leg lives in the pair snapshot, not the generic
     * slot: that slot shares union storage with the snapshot, so the
     * frozen IO request would alias it. The remaining pair kinds
     * return through their own arms above. */
    resolution = &candidate->address_resolution;
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        resolution = &candidate->port_address_snapshot.data_resolution;
    }
    address_effect = &resolution->address_result.effect;

    /* The LONG32_SINGLE direct leg reads its dma echo from its own
     * arm: the DIRECT alias slot overlays reserved storage, so the
     * shared direct liveness would misread it. Indirect LONG32 legs
     * keep the generic CMPT=0/ARP=0 tail below. */
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE &&
        address_effect->source.kind ==
            TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0) {
        return ti_c54x_memory_candidate_long32_single_direct_matches_live(
            candidate, state);
    }
    /* Absolute DATA owns its liveness: the DIRECT_DP source echo
     * rides the frozen split, but no CPL/DP register is observed,
     * so the shared DIRECT_DP CPL0/DP gate must not run. */
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16) {
        return ti_c54x_memory_candidate_absolute_data_matches_live(
            candidate, state);
    }
    if (address_effect->source.kind ==
            TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0 ||
        address_effect->source.kind == TI_C54X_ADDRESS_SOURCE_DIRECT_DP) {
        if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
            return ti_c54x_memory_instruction_port_direct_data_matches_live(
                candidate, state, resolution);
        }
        return ti_c54x_memory_candidate_direct_matches_live(candidate,
                                                             state);
    }

    /* All remaining generic memory forms are CMPT=0/ARP=0
     * plans (dual-pair, X-only, and direct forms return through
     * their own arms above). Read the mode dependencies
     * directly; re-running the resolver here would redo address
     * arithmetic and could publish a different candidate after
     * a WAIT. */
    if (ti_c54x_arch_state_read_cmpt(state, &cmpt) !=
            TI_C54X_ARCH_STATE_OK ||
        cmpt != UINT8_C(0) ||
        ti_c54x_arch_state_read_arp(state, &arp) !=
            TI_C54X_ARCH_STATE_OK ||
        arp != UINT8_C(0) ||
        address_effect->state_policy !=
            TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        return false;
    }
    if (!ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect, state)) {
        return false;
    }
    switch (address_effect->source.kind) {
    case TI_C54X_ADDRESS_SOURCE_AR:
        if (ti_c54x_arch_state_read_ar(
                state, address_effect->source.value.ar.index,
                &selected_ar) != TI_C54X_ARCH_STATE_OK) {
            return false;
        }
        return selected_ar == address_effect->source.value.ar.value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_INDEXED:
        if (ti_c54x_arch_state_read_ar(
                state, address_effect->source.value.ar_indexed.index,
                &selected_ar) != TI_C54X_ARCH_STATE_OK ||
            selected_ar != address_effect->source.value.ar_indexed.value_before) {
            return false;
        }
        if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &selected_ar) !=
            TI_C54X_ARCH_STATE_OK) {
            return false;
        }
        return selected_ar ==
                   address_effect->source.value.ar_indexed.ar0_value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR:
        if (ti_c54x_arch_state_read_ar(
                state, address_effect->source.value.ar_circular.index,
                &selected_ar) != TI_C54X_ARCH_STATE_OK ||
            selected_ar != address_effect->source.value.ar_circular.value_before) {
            return false;
        }
        if (ti_c54x_arch_state_read_bk(state, &selected_ar) !=
            TI_C54X_ARCH_STATE_OK) {
            return false;
        }
        return selected_ar ==
                   address_effect->source.value.ar_circular.bk_value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY:
        if (ti_c54x_arch_state_read_ar(
                state, address_effect->source.value.ar_reverse_carry.index,
                &selected_ar) != TI_C54X_ARCH_STATE_OK ||
            selected_ar !=
                address_effect->source.value.ar_reverse_carry.value_before) {
            return false;
        }
        if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &selected_ar) !=
            TI_C54X_ARCH_STATE_OK) {
            return false;
        }
        return selected_ar == address_effect->source.value.ar_reverse_carry
                                  .ar0_value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET:
        if (ti_c54x_arch_state_read_ar(
                state, address_effect->source.value.ar_long_offset.index,
                &selected_ar) != TI_C54X_ARCH_STATE_OK) {
            return false;
        }
        return selected_ar ==
               address_effect->source.value.ar_long_offset.value_before;
    default:
        return false;
    }
}

/*
 * Snapshot matcher (resume path):
 * the prepared candidate snapshot must match the fresh shape
 * before the resume state is accepted.
 */
bool
ti_c54x_memory_instruction_candidate_snapshot_matches(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *const memory =
        transaction == NULL || transaction->candidate.effect.kind !=
                                  TI_C54X_OP_EFFECT_MEMORY
            ? NULL
            : &transaction->candidate.effect.payload.memory;

    if (memory == NULL) {
        return false;
    }
    if (transaction->child_kind == TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32) {
        return memory->address_spec.kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32 &&
               memory->first_value_msw ==
                   transaction->long32_first_value_msw &&
               memory->second_value_lsw ==
                   transaction->long32_second_value_lsw;
    }
    return transaction->child_kind == TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16 &&
           memory->rmw_operation == transaction->candidate_rmw_operation &&
           memory->value_word == transaction->candidate_rmw_value_word;
}
