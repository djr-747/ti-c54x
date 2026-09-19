/*
 * Module contract
 *
 * Owns: structural and live-state validation of one copied
 * atomic absolute WORD16 resolution against its memory
 * effect; see memory_candidate_atomic_absolute.h.
 * Does not own: other address kinds, transaction lifecycle,
 * endpoint I/O, architectural mutation, effect completion,
 * opcode dispatch, timing, observers, diagnostics, allocation,
 * or callbacks.
 */

#include "memory_candidate_atomic_absolute.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"

static bool
absolute_echo_matches(const ti_c54x_address_effect *address_effect,
                      uint16_t address)
{
    if (address_effect->source.kind !=
        TI_C54X_ADDRESS_SOURCE_ABSOLUTE) {
        return false;
    }
    if (address_effect->source.value.absolute.address != address) {
        return false;
    }
    return address_effect->operand.width ==
               TI_C54X_DATA_OPERAND_WIDTH_WORD16 &&
           address_effect->operand.first_word == (uint32_t)address;
}

bool
ti_c54x_memory_candidate_atomic_absolute_shape_is_valid(
    const ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_address_effect *address_effect;
    uint16_t address;

    if (resolution == NULL || effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16 ||
        effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        effect->rmw_operation !=
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD ||
        effect->complete != NULL ||
        resolution->address_result.status != TI_C54X_ADDRESS_STATUS_OK ||
        resolution->request.space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        resolution->request.access != effect->access ||
        resolution->request.address_word !=
            resolution->address_result.effect.operand.first_word ||
        !ti_c54x_bus_word16_request_is_valid(&resolution->request)) {
        return false;
    }
    /* The RMW engine issues its own transformed writeback, so
     * the resolution template carries no staged value: the
     * immediate rides the frozen effect the transform reads. */
    if (resolution->request.write_value_word != UINT16_C(0)) {
        return false;
    }
    address = effect->address_spec.atomic_absolute.absolute;
    address_effect = &resolution->address_result.effect;
    return address_effect->state_policy ==
               TI_C54X_ADDRESS_STATE_POLICY_NONE &&
           absolute_echo_matches(address_effect, address) &&
           resolution->request.address_word == (uint32_t)address;
}

bool
ti_c54x_memory_candidate_atomic_absolute_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;
    uint16_t address;

    if (candidate == NULL || state == NULL) {
        return false;
    }
    /* Absolute DATA observes no state, so neither the generic
     * CMPT=0/ARP=0 gate nor any live source echo applies. The
     * NONE policy, the frozen absolute echo, and the staged
     * ADD transform re-prove the shape; only the sequential
     * PC is live. */
    if (!ti_c54x_memory_candidate_atomic_absolute_shape_is_valid(
            &candidate->address_resolution,
            &candidate->effect.payload.memory)) {
        return false;
    }
    address =
        candidate->effect.payload.memory.address_spec.atomic_absolute
            .absolute;
    address_effect =
        &candidate->address_resolution.address_result.effect;
    return absolute_echo_matches(address_effect, address) &&
           ti_c54x_memory_candidate_pc_matches_live(
               &candidate->pc_effect, state);
}
