/*
 * Module contract
 *
 * Owns: ordered, non-mutating resolution of one W50A dual-indirect
 * MOD0/MOD0 WORD16 DATA READ pair. It snapshots X/Y AR2..AR5 values from
 * the supplied state, delegates effective-address construction to the
 * address planner, validates each intrinsic effect, and constructs both
 * canonical bus requests before publishing the pair.
 * Does not own: CMPT/ARP policy, opcode semantics, bus callbacks, memory,
 * architectural mutation, instruction sequencing, timing, lifecycle,
 * diagnostics, or device/MMR behavior.
 */

#include "dual_address_resolution.h"

#include <stddef.h>
#include <stdint.h>

#include "address/indirect_dual.h"

static ti_c54x_memory_address_resolution_status
invalid_output(ti_c54x_dual_memory_address_resolution *out,
               ti_c54x_memory_address_resolution_status status)
{
    if (out != NULL) {
        *out = (ti_c54x_dual_memory_address_resolution){0};
    }
    return status;
}

static int
dual_reserved_words_are_zero(const ti_c54x_op_memory_address_spec *spec)
{
    return spec->dual.reserved_word0 == UINT32_C(0) &&
           spec->dual.reserved_word1 == UINT32_C(0) &&
           spec->dual.reserved_word2 == UINT32_C(0) &&
           spec->dual.reserved_word3 == UINT32_C(0);
}

static int
dual_ar_index_is_valid(uint8_t index)
{
    return index >= UINT8_C(2) && index <= UINT8_C(5);
}

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_dual_memory_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_dual_memory_address_resolution *out)
{
    uint16_t x_ar_value = UINT16_C(0);
    uint16_t y_ar_value = UINT16_C(0);
    ti_c54x_indirect_dual_result dual_result;
    ti_c54x_sequencer_address_commit x_commit;
    ti_c54x_sequencer_address_commit y_commit;
    ti_c54x_bus_word16_request_result x_request;
    ti_c54x_bus_word16_request_result y_request;
    ti_c54x_address_apply_status x_validation;
    ti_c54x_address_apply_status y_validation;

    if (state == NULL || spec == NULL || out == NULL) {
        return invalid_output(
            out, TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
    }
    *out = (ti_c54x_dual_memory_address_resolution){0};

    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16 ||
        spec->dual.x_mode != UINT8_C(0) ||
        spec->dual.y_mode != UINT8_C(0) ||
        !dual_ar_index_is_valid(spec->dual.x_ar) ||
        !dual_ar_index_is_valid(spec->dual.y_ar) ||
        !dual_reserved_words_are_zero(spec)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        write_value_word != UINT16_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
    }

    /* X/Y are independent frozen reads. An alias is intentionally read twice
     * and cross-checked so a future state backend cannot publish a torn pair. */
    if (ti_c54x_arch_state_read_ar(state, spec->dual.x_ar, &x_ar_value) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
    }
    if (ti_c54x_arch_state_read_ar(state, spec->dual.y_ar, &y_ar_value) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
    }
    if (spec->dual.x_ar == spec->dual.y_ar && x_ar_value != y_ar_value) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    dual_result = ti_c54x_indirect_dual_word16(
        spec->dual.x_ar, x_ar_value, spec->dual.y_ar, y_ar_value,
        TI_C54X_INDIRECT_DUAL_MOD0, TI_C54X_INDIRECT_DUAL_MOD0,
        TI_C54X_ADDRESS_ACCESS_READ);
    if (dual_result.status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    /* MOD0/MOD0 carries STATE_POLICY_NONE by design. Intrinsic validation
     * still proves operand/source/update shape for each independent result. */
    x_validation = ti_c54x_sequencer_validate_address_effect(
        state, &dual_result.x, TI_C54X_ADDRESS_UPDATE_PHASE_NONE, &x_commit);
    if (x_validation != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }
    y_validation = ti_c54x_sequencer_validate_address_effect(
        state, &dual_result.y, TI_C54X_ADDRESS_UPDATE_PHASE_NONE, &y_commit);
    if (y_validation != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    x_request = ti_c54x_bus_word16_request_read(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        dual_result.x.effect.operand.first_word);
    if (x_request.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }
    y_request = ti_c54x_bus_word16_request_read(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        dual_result.y.effect.operand.first_word);
    if (y_request.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }

    out->x_address_result = dual_result.x;
    out->y_address_result = dual_result.y;
    out->x_address_commit = x_commit;
    out->y_address_commit = y_commit;
    out->x_request = x_request.request;
    out->y_request = y_request.request;
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}
