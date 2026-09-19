/*
 * Module contract
 *
 * Owns: resolution of one copied absolute-DATA WORD16 spec; see
 * address_resolution_absolute_data.h.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle,
 * timing, observers, diagnostics, allocation, or callbacks.
 */

#include "address_resolution_absolute_data.h"

#include <stddef.h>
#include <stdint.h>

#include "address/absolute.h"

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_absolute_data_address(
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
    uint8_t sidecar_kind;

    if (state == NULL || spec == NULL || out == NULL) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT;
    }
    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    if (spec->absolute_data.reserved_word0 != UINT32_C(0) ||
        spec->absolute_data.reserved_word1 != UINT32_C(0) ||
        spec->absolute_data.reserved_word2 != UINT32_C(0) ||
        spec->absolute_data.reserved_word3 != UINT32_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    sidecar_kind = spec->absolute_data.mmr_sidecar_kind;
    /* The kind/access pairing mirrors the effect gate: READ legs
     * run plain or with a store leg, while WRITE legs run with a
     * load leg (the resume path snapshots the MMR into the
     * request, so resolution takes the staged zero here) or plain
     * (the Smem-absolute pair stages a zero dmad template the
     * second-leg issue replaces with the Smem observation). */
    if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        if (sidecar_kind !=
                (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE &&
            sidecar_kind !=
                (uint8_t)
                    TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        if (sidecar_kind !=
                (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE &&
            sidecar_kind !=
                (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
        }
    } else {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
    }
    absolute.bits = spec->absolute_data.address;

    /* Absolute DATA carries the full address with no live state
     * dependence, so it plans through the dedicated ABSOLUTE
     * source instead of the DP planner: no CPL or DP register
     * is observed and unknown mode state never rejects it. */
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
    if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        request_result = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            absolute_result.effect.operand.first_word);
    } else {
        request_result = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            absolute_result.effect.operand.first_word,
            write_value_word);
    }
    if (request_result.status !=
        TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }
    out->address_result = absolute_result;
    out->address_commit = address_commit;
    out->request = request_result.request;
    out->ar0_shadow = UINT16_C(0);
    out->bk_shadow = UINT16_C(0);
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}
