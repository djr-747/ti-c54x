/*
 * Module contract
 *
 * Owns: resolution of one copied atomic absolute WORD16
 * spec; see address_resolution_atomic_absolute.h.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle,
 * timing, observers, diagnostics, allocation, or callbacks.
 */

#include "address_resolution_atomic_absolute.h"

#include <stddef.h>
#include <stdint.h>

#include "address/absolute.h"

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_atomic_absolute_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_memory_address_resolution *out)
{
    ti_c54x_absolute_address16 absolute;
    ti_c54x_address_result absolute_result;
    ti_c54x_sequencer_address_commit address_commit;
    ti_c54x_address_apply_status address_status;
    ti_c54x_bus_word16_request_result request_result;

    (void)write_value_word;

    if (state == NULL || spec == NULL || out == NULL) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT;
    }
    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    if (spec->atomic_absolute.reserved_half != UINT16_C(0) ||
        spec->atomic_absolute.reserved_word0 != UINT32_C(0) ||
        spec->atomic_absolute.reserved_word1 != UINT32_C(0) ||
        spec->atomic_absolute.reserved_word2 != UINT32_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    /* Atomics read the cell first: the RMW engine issues its
     * own transformed writeback, never a staged template
     * (the shape gate echoes the template separately). */
    if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
    }
    absolute.bits = spec->atomic_absolute.absolute;

    /* Absolute DATA carries the full address with no live state
     * dependence, so it plans through the dedicated ABSOLUTE
     * source: no CPL or DP register is observed and unknown
     * mode state never rejects it. */
    absolute_result = ti_c54x_absolute_word(absolute);
    if (absolute_result.status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }
    address_status = ti_c54x_sequencer_validate_address_effect(
        state, &absolute_result,
        TI_C54X_ADDRESS_UPDATE_PHASE_NONE, &address_commit);
    if (address_status != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }
    request_result = ti_c54x_bus_word16_request_read(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        absolute_result.effect.operand.first_word);
    if (request_result.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }
    out->address_result = absolute_result;
    out->address_commit = address_commit;
    out->request = request_result.request;
    out->ar0_shadow = UINT16_C(0);
    out->bk_shadow = UINT16_C(0);
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}
