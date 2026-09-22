/*
 * Module contract
 *
 * Owns: resolution of one copied stack-push WORD16 spec; see
 * address_resolution_stack_push.h.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle,
 * timing, observers, diagnostics, allocation, or callbacks.
 */

#include "address_resolution_stack_push.h"

#include <stddef.h>
#include <stdint.h>

#include "address/stack_push.h"

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_stack_push_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_memory_address_resolution *out)
{
    ti_c54x_address_result address_result;
    ti_c54x_sequencer_address_commit address_commit;
    ti_c54x_address_apply_status address_status;
    ti_c54x_bus_word16_request_result request_result;
    uint16_t sp_value = UINT16_C(0);

    if (state == NULL || spec == NULL || out == NULL) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT;
    }
    if (!ti_c54x_op_memory_address_kind_is_bundle_push(spec->kind) &&
        !ti_c54x_op_memory_address_kind_is_accumulator_push(spec->kind)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
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
    if (ti_c54x_op_memory_address_kind_is_far_push(spec->kind)) {
        /* Far double-pushes issue the staged return address to
         * old SP - 1 (the request below, already validated in
         * its native single-push shape) and the frozen XPC to
         * old SP - 2 (planned by the far-push owner), so the
         * single published SP update steps once more to old
         * SP - 2 with the established mod-65536 SP wrap. The
         * root candidate check re-pins the stepped update and
         * commit against the frozen SP snapshot. */
        address_result.effect.update.value_after = (uint16_t)(
            (uint32_t)address_result.effect.update.value_after -
            UINT32_C(1));
        address_commit.value_after =
            address_result.effect.update.value_after;
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
