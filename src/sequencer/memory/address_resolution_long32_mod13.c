/*
 * Module contract
 *
 * Owns: resolution of one copied MOD13 long-offset LONG32
 * spec; see address_resolution_long32_mod13.h.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle,
 * timing, observers, diagnostics, allocation, or callbacks.
 */

#include "address_resolution_long32_mod13.h"

#include <stddef.h>
#include <stdint.h>

#include "address/indirect_long_offset.h"
#include "state/arch_state.h"

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_long32_mod13_address(
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
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT;
    }
    if (spec->kind !=
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    if (spec->mode != 0 || spec->indexed_mode != 0 ||
        spec->circular_mode != 0 || spec->reverse_carry_mode != 0 ||
        spec->arf > UINT8_C(7)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
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
