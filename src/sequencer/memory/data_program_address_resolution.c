/*
 * Module contract
 *
 * Owns: ordered, non-mutating resolution of one DATA+PROGRAM WORD16
 * READ pair. It snapshots the Smem DATA leg through the generic
 * single-leg resolver (direct page-0 or standard-indirect
 * MOD0/MOD1/MOD2 with the resolver-owned commit) and pairs it with
 * an absolute PROGRAM-leg coefficient read carrying a zeroed
 * no-update commit. The PROGRAM leg observes no architectural
 * state; only the Smem leg can reject on unknown sources.
 * Does not own: CMPT/ARP policy, opcode semantics, bus callbacks,
 * memory, architectural mutation, instruction sequencing, timing,
 * lifecycle, diagnostics, or device/MMR behavior.
 */

#include "data_program_address_resolution.h"

#include <stddef.h>
#include <stdint.h>

static ti_c54x_memory_address_resolution_status
invalid_output(ti_c54x_dual_memory_address_snapshot *out,
               ti_c54x_memory_address_resolution_status status)
{
    if (out != NULL) {
        *out = (ti_c54x_dual_memory_address_snapshot){0};
    }
    return status;
}

static bool
pair_spec_is_valid(const ti_c54x_op_memory_address_spec *spec)
{
    if (spec->kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 ||
        spec->pair.indirect > UINT8_C(1) ||
        spec->pair.smem_arf > UINT8_C(7) ||
        spec->pair.smem_dma7 > UINT8_C(0x7f) ||
        spec->pair.reserved_half != UINT16_C(0) ||
        spec->pair.reserved_word0 != UINT32_C(0) ||
        spec->pair.reserved_word1 != UINT32_C(0) ||
        spec->pair.reserved_word2 != UINT32_C(0)) {
        return false;
    }
    if (spec->pair.indirect == UINT8_C(0)) {
        return spec->pair.smem_mode == UINT8_C(0) &&
               spec->pair.smem_arf == UINT8_C(0);
    }
    return spec->pair.smem_mode <= UINT8_C(2) &&
           spec->pair.smem_dma7 == UINT8_C(0);
}

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_data_program_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_dual_memory_address_snapshot *out)
{
    ti_c54x_op_memory_address_spec smem_spec = {0};
    ti_c54x_memory_address_resolution smem_resolution = {0};
    ti_c54x_dual_memory_address_snapshot snapshot = {0};

    if (state == NULL || spec == NULL || out == NULL) {
        return invalid_output(
            out, TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
    }
    *out = (ti_c54x_dual_memory_address_snapshot){0};

    if (!pair_spec_is_valid(spec)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC;
    }
    if (access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        write_value_word != UINT16_C(0)) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS;
    }

    /* Stage the Smem DATA leg through the generic single-leg
     * resolver so direct routing, MOD0--MOD2 planning, and commit
     * staging match the single-read path exactly. */
    if (spec->pair.indirect == UINT8_C(0)) {
        smem_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        smem_spec.direct_dma7 = spec->pair.smem_dma7;
    } else {
        smem_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
        smem_spec.mode = (ti_c54x_indirect_single_mode)spec->pair.smem_mode;
        smem_spec.arf = spec->pair.smem_arf;
    }
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &smem_spec, access, UINT16_C(0),
            &smem_resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED;
    }

    /* The PROGRAM leg is absolute: the 16-bit pmad names the
     * coefficient word with no state dependence and no
     * post-update. A PROGRAM-space request accepts any word
     * address, so this construction is infallible. */
    snapshot.x_request = smem_resolution.request;
    snapshot.y_request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    snapshot.y_request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    snapshot.y_request.address_word = (uint32_t)spec->pair.program_pmad;
    snapshot.y_request.write_value_word = UINT16_C(0);
    snapshot.x_commit = smem_resolution.address_commit;
    snapshot.y_commit = (ti_c54x_sequencer_address_commit){0};
    snapshot.ar0_shadow = smem_resolution.ar0_shadow;
    snapshot.bk_shadow = smem_resolution.bk_shadow;
    if (!ti_c54x_bus_word16_request_is_valid(&snapshot.x_request) ||
        !ti_c54x_bus_word16_request_is_valid(&snapshot.y_request)) {
        return invalid_output(
            out, TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
    }
    *out = snapshot;
    return TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK;
}
