#ifndef TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_H
#define TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_H

/*
 * Module contract
 *
 * Owns: pure resolution of the generic W16 data-memory address descriptor
 * against caller-owned architectural state, including CMPT=0 single-indirect
 * MOD0--MOD3, AR0-indexed MOD5/MOD6, fixed-step circular MOD8/MOD10,
 * reverse-carry MOD4/MOD7, X-only dual-indirect MOD0--MOD3, and
 * mode-independent STACK_PUSH/STACK_POP forms, a value-owned address
 * result, prevalidated AR-or-SP commit, and DATA bus request.
 * The AR0/BK shadows are populated only by the X-only MOD3 path and stay
 * zero otherwise.
 * Does not own: opcode identity, memory visibility, endpoint callbacks,
 * architectural mutation, timing, CPU lifecycle, or diagnostics. State and
 * descriptor inputs are borrowed for one call; output owns no pointers and
 * is zeroed on rejection.
 */

#include "address/address_effect.h"
#include "isa/ops/memory/memory_effect.h"
#include "state/arch_state.h"
#include "sequencer/address_effect_validate.h"
#include "bus/request/word16_request.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_memory_address_resolution_status {
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK = 0,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT = 1,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC = 2,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS = 3,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE = 4,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE = 5,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED = 6,
    TI_C54X_MEMORY_ADDRESS_RESOLUTION_REQUEST_REJECTED = 7
} ti_c54x_memory_address_resolution_status;

typedef struct ti_c54x_memory_address_resolution {
    ti_c54x_address_result address_result;
    ti_c54x_sequencer_address_commit address_commit;
    ti_c54x_bus_word16_request request;
    /* Copied AR0/BK observed only by the X-only MOD3 path; zero otherwise. */
    uint16_t ar0_shadow;
    uint16_t bk_shadow;
} ti_c54x_memory_address_resolution;

/* Resolve one supported generic address and stage all fallible candidates. */
ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_memory_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_memory_address_resolution *out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_H */
