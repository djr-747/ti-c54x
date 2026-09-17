/*
 * Module contract
 *
 * Owns: ordered, non-mutating resolution of a generic CMPT=0 W16 data
 * address (single-indirect MOD0--MOD3, AR0-indexed MOD5/MOD6, fixed-step
 * circular MOD8/MOD10, reverse-carry MOD4/MOD7, X-only dual-indirect
 * MOD0--MOD3, or mode-independent STACK_PUSH/STACK_POP/STACK_POP_RTN)
 * into the existing checked address planner and request constructor.
 * STACK_POP_RTN additionally gates RTN knownness; its planned READ
 * is never issued by the root.
 * Does not own: opcode semantics, bus callbacks, physical memory, CPU state
 * mutation, timing, lifecycle, or diagnostics.
 */

#include "address_resolution.h"

#include <stddef.h>

#include "address/direct_page0.h"
#include "address/indirect_dual.h"
#include "address/indirect_long_offset.h"
#include "address/indirect_reverse_carry.h"
#include "address/stack_pop.h"
#include "address/stack_push.h"
#include "state/block_size_register.h"

static ti_c54x_memory_address_resolution_status
invalid_output(ti_c54x_memory_address_resolution *out,
               ti_c54x_memory_address_resolution_status status)
{
    if (out != NULL) {
        *out = (ti_c54x_memory_address_resolution){0};
    }
    return status;
}

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_memory_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_memory_address_resolution *out)
{
    uint8_t cmpt = UINT8_C(0);
    uint8_t arp = UINT8_C(0);
    uint16_t ar_value = UINT16_C(0);
    ti_c54x_address_result address_result;
    ti_c54x_sequencer_address_commit address_commit;
    ti_c54x_bus_word16_request_result request_result;
    ti_c54x_address_access address_access;
    ti_c54x_address_apply_status address_status;

    if (state == NULL || spec == NULL || out == NULL) {
        return invalid_output(out,
                              TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
    }
    *out = (ti_c54x_memory_address_resolution){0};
    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16) {
        if (spec->indexed_mode != 0 || spec->circular_mode != 0 ||
            spec->reverse_carry_mode != 0 ||
            spec->mode < TI_C54X_INDIRECT_SINGLE_MOD0 ||
            spec->mode > TI_C54X_INDIRECT_SINGLE_MOD3 ||
            spec->arf > UINT8_C(7)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind ==
               TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
            spec->arf > UINT8_C(7)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16) {
        if (spec->mode != TI_C54X_INDIRECT_SINGLE_MOD0 ||
            spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
            (spec->indexed_mode != TI_C54X_INDIRECT_INDEXED_MOD5 &&
             spec->indexed_mode != TI_C54X_INDIRECT_INDEXED_MOD6) ||
            spec->arf == UINT8_C(0) || spec->arf > UINT8_C(7)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16) {
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->reverse_carry_mode != 0 ||
            (spec->circular_mode != TI_C54X_INDIRECT_CIRCULAR_MOD8 &&
             spec->circular_mode != TI_C54X_INDIRECT_CIRCULAR_MOD10) ||
            spec->arf > UINT8_C(7)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16) {
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->circular_mode != 0 ||
            (spec->reverse_carry_mode !=
                 TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 &&
             spec->reverse_carry_mode !=
                 TI_C54X_INDIRECT_REVERSE_CARRY_MOD7) ||
            spec->arf == UINT8_C(0) || spec->arf > UINT8_C(7)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
               spec->kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16) {
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
            spec->arf != UINT8_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
               spec->kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
        /* Accumulator-targeted pushes share the TOS contract with the
         * immediate pushes; only the accumulator select rides `arf`
         * (0 for A, 1 for B) while every other first-arm field stays
         * zero. The PC target itself travels outside this spec. */
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
            spec->long_offset_word != UINT16_C(0) ||
            spec->direct_dma7 != UINT8_C(0) ||
            spec->arf > UINT8_C(1)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
               spec->kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 ||
               spec->kind ==
                   TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
            spec->arf != UINT8_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16) {
        if (spec->mode != 0 || spec->indexed_mode != 0 ||
            spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
            spec->arf != UINT8_C(0) ||
            spec->long_offset_word != UINT16_C(0) ||
            spec->direct_dma7 > UINT8_C(0x7f)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind ==
               TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY) {
        if (spec->dual.x_mode > UINT8_C(3) || spec->dual.x_ar < UINT8_C(2) ||
            spec->dual.x_ar > UINT8_C(5) ||
            spec->dual.y_mode != spec->dual.x_mode ||
            spec->dual.y_ar != spec->dual.x_ar ||
            spec->dual.reserved_word0 != UINT32_C(0) ||
            spec->dual.reserved_word1 != UINT32_C(0) ||
            spec->dual.reserved_word2 != UINT32_C(0) ||
            spec->dual.reserved_word3 != UINT32_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
    }
    if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        write_value_word != UINT16_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        /* MOD13 LONG32 consumes the same CMPT=0/ARP=0 dependencies as the
         * existing generic forms. The planner supplies the canonical MSW/LSW
         * pair; the bus request intentionally targets only the MSW here. */
        if (ti_c54x_arch_state_read_cmpt(state, &cmpt) !=
                TI_C54X_ARCH_STATE_OK ||
            cmpt != UINT8_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE;
        }
        if (ti_c54x_arch_state_read_arp(state, &arp) !=
                TI_C54X_ARCH_STATE_OK ||
            arp != UINT8_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE;
        }
        if (ti_c54x_arch_state_read_ar(state, spec->arf, &ar_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        address_access = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? TI_C54X_ADDRESS_ACCESS_READ
                             : TI_C54X_ADDRESS_ACCESS_WRITE;
        address_result = ti_c54x_indirect_long_offset_mod13_long32(
            spec->arf, ar_value, spec->long_offset_word, address_access);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS, &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? ti_c54x_bus_word16_request_read(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word)
                             : ti_c54x_bus_word16_request_write(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word,
                                   write_value_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
        spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 ||
        spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        uint16_t sp_value = UINT16_C(0);
        uint16_t rtn_value = UINT16_C(0);

        /* Pops observe only SP: no CMPT/ARP/CPL read, so unknown mode
         * state never rejects a pop. RTN pops additionally observe
         * RTN; the root never issues the planned READ for them. */
        if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
        }
        if (ti_c54x_arch_state_read_sp(state, &sp_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16 &&
            ti_c54x_arch_state_read_rtn(state, &rtn_value) !=
                TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        /* The RTN value itself freezes at prepare; resolution only
         * gates its knownness here. */
        (void)rtn_value;
        address_result = ti_c54x_stack_pop_word16(sp_value);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS, &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            address_result.effect.operand.first_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY) {
        uint16_t x_ar_value = UINT16_C(0);
        uint16_t ar0_value = UINT16_C(0);
        uint16_t bk_value = UINT16_C(0);
        ti_c54x_indirect_dual_result dual_result;

        /* X-only reads observe no CMPT/ARP state: the selector stays
         * directly encoded and AR0/BK are read lazily for MOD3 only. */
        if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
            write_value_word != UINT16_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
        }
        if (ti_c54x_arch_state_read_ar(state, spec->dual.x_ar,
                                       &x_ar_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        if (spec->dual.x_mode == UINT8_C(3)) {
            if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &ar0_value) !=
                    TI_C54X_ARCH_STATE_OK ||
                ti_c54x_arch_state_read_bk(state, &bk_value) !=
                    TI_C54X_ARCH_STATE_OK) {
                return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
            }
        }
        dual_result = ti_c54x_indirect_dual_word16(
            spec->dual.x_ar, x_ar_value, spec->dual.x_ar, x_ar_value,
            (ti_c54x_indirect_dual_mode)spec->dual.x_mode,
            (ti_c54x_indirect_dual_mode)spec->dual.x_mode, ar0_value,
            bk_value, TI_C54X_ADDRESS_ACCESS_READ);
        if (dual_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        /* Only the X side is issued: validate its effect at the phase
         * selected by the actual planner result. The self-aliased Y side
         * carries no independent update by planner contract and is
         * discarded here, so exactly one read commits exactly one update. */
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &dual_result.x, dual_result.x.effect.update.phase,
            &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            dual_result.x.effect.operand.first_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = dual_result.x;
        out->address_commit = address_commit;
        out->request = request_result.request;
        out->ar0_shadow = ar0_value;
        out->bk_shadow = bk_value;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
        spec->kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
        spec->kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
        spec->kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
        uint16_t sp_value = UINT16_C(0);

        /* Pushes observe only SP: no CMPT/ARP/CPL read, so unknown mode
         * state never rejects a push. */
        if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
        }
        if (ti_c54x_arch_state_read_sp(state, &sp_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        address_result = ti_c54x_stack_push_word16(sp_value);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS, &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            address_result.effect.operand.first_word, write_value_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16) {
        ti_c54x_direct_page0_dma7 dma;

        /* Page-0 direct observes no state: no CMPT/ARP/CPL/DP read,
         * so unknown mode state never rejects a direct access. */
        dma.bits = spec->direct_dma7;
        address_result = ti_c54x_direct_page0_word(dma);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            TI_C54X_ADDRESS_UPDATE_PHASE_NONE, &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = access ==
                                 TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? ti_c54x_bus_word16_request_read(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word)
                             : ti_c54x_bus_word16_request_write(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word,
                                   write_value_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16) {
        uint16_t ar0_value = UINT16_C(0);

        /* Reverse-carry dependencies are deliberately ordered. The exact
         * spec shape above is complete before any architectural read. */
        if (ti_c54x_arch_state_read_cmpt(state, &cmpt) !=
                TI_C54X_ARCH_STATE_OK ||
            cmpt != UINT8_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE;
        }
        if (ti_c54x_arch_state_read_arp(state, &arp) !=
                TI_C54X_ARCH_STATE_OK ||
            arp != UINT8_C(0)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE;
        }
        if (ti_c54x_arch_state_read_ar(state, spec->arf, &ar_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &ar0_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }

        address_access = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? TI_C54X_ADDRESS_ACCESS_READ
                             : TI_C54X_ADDRESS_ACCESS_WRITE;
        address_result = ti_c54x_indirect_reverse_carry_word16(
            spec->arf, ar_value, ar0_value, spec->reverse_carry_mode,
            address_access);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS, &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? ti_c54x_bus_word16_request_read(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word)
                             : ti_c54x_bus_word16_request_write(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word,
                                   write_value_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    /* COMMON_TI dependency order is architectural and deliberate. */
    if (ti_c54x_arch_state_read_cmpt(state, &cmpt) !=
            TI_C54X_ARCH_STATE_OK ||
        cmpt != UINT8_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE;
    }
    if (ti_c54x_arch_state_read_arp(state, &arp) !=
            TI_C54X_ARCH_STATE_OK ||
        arp != UINT8_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE;
    }
    if (ti_c54x_arch_state_read_ar(state, spec->arf, &ar_value) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16) {
        uint16_t bk_value = UINT16_C(0);

        /* Circular forms consume BK only after CMPT, ARP, and selected AR. */
        if (ti_c54x_arch_state_read_bk(state, &bk_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        address_access = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? TI_C54X_ADDRESS_ACCESS_READ
                             : TI_C54X_ADDRESS_ACCESS_WRITE;
        address_result = ti_c54x_indirect_circular_word16(
            spec->arf, ar_value, bk_value, spec->circular_mode,
            address_access);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            bk_value == UINT16_C(0)
                ? TI_C54X_ADDRESS_UPDATE_PHASE_NONE
                : TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
            &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? ti_c54x_bus_word16_request_read(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word)
                             : ti_c54x_bus_word16_request_write(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word,
                                   write_value_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16) {
        uint16_t ar0_value = UINT16_C(0);

        /* Indexed forms consume selected ARx first, then the AR0 index. */
        if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &ar0_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        address_access = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? TI_C54X_ADDRESS_ACCESS_READ
                             : TI_C54X_ADDRESS_ACCESS_WRITE;
        address_result = ti_c54x_indirect_indexed_word(
            spec->arf, ar_value, ar0_value, spec->indexed_mode,
            address_access);
        if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result,
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS, &address_commit);
        if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
        }
        request_result = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             ? ti_c54x_bus_word16_request_read(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word)
                             : ti_c54x_bus_word16_request_write(
                                   TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                   address_result.effect.operand.first_word,
                                   write_value_word);
        if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
        }
        out->address_result = address_result;
        out->address_commit = address_commit;
        out->request = request_result.request;
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
    }

    address_access = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                         ? TI_C54X_ADDRESS_ACCESS_READ
                         : TI_C54X_ADDRESS_ACCESS_WRITE;
    address_result = ti_c54x_indirect_single_word(
        spec->arf, ar_value, spec->mode, address_access);
    if (address_result.status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }
    /* The planner is authoritative for operand/update shape.  Resolve the
     * phase from the selected standard-indirect mode so MOD0's NONE update
     * and MOD3's PRE update are validated without any mode-specific parsing
     * of the returned raw effect. */
    {
        const ti_c54x_address_update_phase requested_phase =
            spec->mode == TI_C54X_INDIRECT_SINGLE_MOD0
                ? TI_C54X_ADDRESS_UPDATE_PHASE_NONE
                : spec->mode == TI_C54X_INDIRECT_SINGLE_MOD3
                      ? TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS
                      : TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
        address_status = ti_c54x_sequencer_validate_address_effect(
            state, &address_result, requested_phase, &address_commit);
    }
    if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    request_result = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                         ? ti_c54x_bus_word16_request_read(
                               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                               address_result.effect.operand.first_word)
                         : ti_c54x_bus_word16_request_write(
                               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                               address_result.effect.operand.first_word,
                               write_value_word);
    if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }
    out->address_result = address_result;
    out->address_commit = address_commit;
    out->request = request_result.request;
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}
