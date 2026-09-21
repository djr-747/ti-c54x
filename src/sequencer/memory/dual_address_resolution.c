/*
 * Module contract
 *
 * Owns: ordered, non-mutating resolution of one W51A dual-indirect
 * MOD0/MOD1/MOD2 WORD16 DATA READ pair plus the MVDD move-pair
 * variant (X READ, Y WRITE template). It snapshots X/Y AR2..AR5 values from
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
#include "state/block_size_register.h"

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

static int
dual_mode_is_valid(uint8_t mode)
{
    return mode <= UINT8_C(3);
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
    uint16_t ar0_value = UINT16_C(0);
    uint16_t bk_value = UINT16_C(0);
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
        !dual_mode_is_valid(spec->dual.x_mode) ||
        !dual_mode_is_valid(spec->dual.y_mode) ||
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

    /* AR0/BK are read lazily: pairs without a MOD3 side never observe BK,
     * and no CMPT/ARP state is observed on any path. */
    if (spec->dual.x_mode == UINT8_C(3) || spec->dual.y_mode == UINT8_C(3)) {
        if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &ar0_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        if (ti_c54x_arch_state_read_bk(state, &bk_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
    }

    dual_result = ti_c54x_indirect_dual_word16(
        spec->dual.x_ar, x_ar_value, spec->dual.y_ar, y_ar_value,
        (ti_c54x_indirect_dual_mode)spec->dual.x_mode,
        (ti_c54x_indirect_dual_mode)spec->dual.y_mode, ar0_value, bk_value,
        TI_C54X_ADDRESS_ACCESS_READ);
    if (dual_result.status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    /* Validate each effect at the phase selected by its actual planner
     * result. MOD0 has no update; MOD1/MOD2/MOD3 updates are post-access
     * (MOD3 is a no-update plan only for the BK=0/AR0=0 case). The
     * dependency validator observes no CMPT/ARP state because all admitted
     * dual modes carry STATE_POLICY_NONE. */
    x_validation = ti_c54x_sequencer_validate_address_effect(
        state, &dual_result.x, dual_result.x.effect.update.phase, &x_commit);
    if (x_validation != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }
    y_validation = ti_c54x_sequencer_validate_address_effect(
        state, &dual_result.y, dual_result.y.effect.update.phase, &y_commit);
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
    out->ar0_value = ar0_value;
    out->bk_value = bk_value;
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_dual_move_pair(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_dual_memory_address_resolution *out)
{
    uint16_t x_ar_value = UINT16_C(0);
    uint16_t y_ar_value = UINT16_C(0);
    uint16_t ar0_value = UINT16_C(0);
    uint16_t bk_value = UINT16_C(0);
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

    if (spec->kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16 ||
        !dual_mode_is_valid(spec->dual.x_mode) ||
        !dual_mode_is_valid(spec->dual.y_mode) ||
        !dual_ar_index_is_valid(spec->dual.x_ar) ||
        !dual_ar_index_is_valid(spec->dual.y_ar) ||
        !dual_reserved_words_are_zero(spec)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
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

    /* AR0/BK are read lazily: pairs without a MOD3 side never observe BK,
     * and no CMPT/ARP state is observed on any path. */
    if (spec->dual.x_mode == UINT8_C(3) || spec->dual.y_mode == UINT8_C(3)) {
        if (ti_c54x_arch_state_read_ar(state, UINT8_C(0), &ar0_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
        if (ti_c54x_arch_state_read_bk(state, &bk_value) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE;
        }
    }

    /* Effective addresses are access-independent: plan both sides as
     * reads, then build the Y WRITE template from the Y address. */
    dual_result = ti_c54x_indirect_dual_word16(
        spec->dual.x_ar, x_ar_value, spec->dual.y_ar, y_ar_value,
        (ti_c54x_indirect_dual_mode)spec->dual.x_mode,
        (ti_c54x_indirect_dual_mode)spec->dual.y_mode, ar0_value, bk_value,
        TI_C54X_ADDRESS_ACCESS_READ);
    if (dual_result.status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    x_validation = ti_c54x_sequencer_validate_address_effect(
        state, &dual_result.x, dual_result.x.effect.update.phase, &x_commit);
    if (x_validation != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }
    y_validation = ti_c54x_sequencer_validate_address_effect(
        state, &dual_result.y, dual_result.y.effect.update.phase, &y_commit);
    if (y_validation != TI_C54X_ADDRESS_APPLY_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    x_request = ti_c54x_bus_word16_request_read(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        dual_result.x.effect.operand.first_word);
    if (x_request.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }
    /* Zero-staged Y WRITE template; the move root replaces the value
     * with the X observation before issue. */
    y_request = ti_c54x_bus_word16_request_write(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        dual_result.y.effect.operand.first_word, UINT16_C(0));
    if (y_request.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED;
    }

    out->x_address_result = dual_result.x;
    out->y_address_result = dual_result.y;
    out->x_address_commit = x_commit;
    out->y_address_commit = y_commit;
    out->x_request = x_request.request;
    out->y_request = y_request.request;
    out->ar0_value = ar0_value;
    out->bk_value = bk_value;
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}
