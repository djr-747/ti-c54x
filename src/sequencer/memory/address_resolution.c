/*
 * Module contract
 *
 * Owns: ordered, non-mutating resolution of a generic CMPT=0 W16 data
 * address (single-indirect MOD0--MOD3 or AR0-indexed MOD5/MOD6) into the
 * existing checked address planner and request constructor.
 * Does not own: opcode semantics, bus callbacks, physical memory, CPU state
 * mutation, timing, lifecycle, or diagnostics.
 */

#include "address_resolution.h"

#include <stddef.h>

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
        if (spec->mode < TI_C54X_INDIRECT_SINGLE_MOD0 ||
            spec->mode > TI_C54X_INDIRECT_SINGLE_MOD3 ||
            spec->arf > UINT8_C(7)) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (spec->kind == TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16) {
        if (spec->mode != TI_C54X_INDIRECT_SINGLE_MOD0 ||
            (spec->indexed_mode != TI_C54X_INDIRECT_INDEXED_MOD5 &&
             spec->indexed_mode != TI_C54X_INDIRECT_INDEXED_MOD6) ||
            spec->arf == UINT8_C(0) || spec->arf > UINT8_C(7)) {
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
