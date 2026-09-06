#ifndef TI_C54X_SEQUENCER_ADDRESS_EFFECT_APPLY_H
#define TI_C54X_SEQUENCER_ADDRESS_EFFECT_APPLY_H

/*
 * Module contract
 *
 * Owns: the callable private entry point and final transactional application
 * of one value-owned indirect address effect to caller-owned architectural
 * state. Ordered validation, dependency reads, and the private commit
 * descriptor are coordinated by address_effect_validate.c through separate
 * intrinsic and dependency owners; this module owns only the final sole-AR
 * write after validation succeeds. Canonical
 * data-operand shape and the value-owned source snapshot are intrinsic
 * producer contracts validated before any phase relation or state access.
 * MOD13 long-offset and MOD4/MOD7 reverse-carry effects invoke their
 * address-owned intrinsic validators before this phase/mode/source sequence.
 * Circular MOD8/MOD10 WORD16 effects invoke their address-owned intrinsic
 * validator before this phase/mode/source sequence; positive-BK effects read
 * the selected AR followed by BK and never write BK, while BK=0 effects carry
 * the canonical NONE update and remain non-mutating.
 * Does not own: address planning, opcode decoding, CPU lifecycle, buses,
 * memory, timing, or diagnostics. It owns validation of the private state
 * precondition carried by an address effect, but not the storage or layout of
 * ARP/CMPT fields and not the public CPU ABI. For a tagged effect, all
 * required mode fields are read through state accessors and must be known
 * before their value constraints or a tagged source is consulted; indexed
 * AR0 effects read the selected AR and then AR0 in that order. MOD13 effects
 * read only the selected AR; their copied offset word is not architectural
 * state. Reverse-carry effects read the selected AR followed by copied AR0
 * and never write AR0. Policy failures never repair or partially mutate
 * state.
 * The state pointer is caller-owned and must remain valid for the duration of
 * the call. The address result pointer is borrowed for the duration of the
 * call and is never retained or modified. A rejected call leaves the complete
 * state object byte-for-byte unchanged; a successful NONE effect does not
 * mutate it. Direct effects read only CPL and their tagged DP or SP through
 * accessors; they never read an AR or write any source register as part of
 * application.
 */

#include "address/address_effect.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_address_apply_status {
    TI_C54X_ADDRESS_APPLY_OK = 0,
    TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT = 1,
    TI_C54X_ADDRESS_APPLY_REJECTED_RESULT = 2,
    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT = 3,
    TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH = 4,
    TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE = 5,
    TI_C54X_ADDRESS_APPLY_STALE_SOURCE = 6,
    TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE = 7,
    TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION = 8
} ti_c54x_address_apply_status;

/*
 * Validate and apply one planner result at its recorded PRE_ACCESS,
 * POST_ACCESS, or NONE phase. The result is borrowed and must have a stable
 * lifetime until the call returns. The state remains caller-owned. Source kind,
 * source-arm domains, operand/source relationships, update shape, and the
 * exact policy-to-source mapping are checked before state access. Required
 * mode fields are known before their value constraints; only then is the
 * tagged source read for unknown/stale comparison. No arithmetic is inferred
 * for AR updates: value_before/value_after are checked against the effective
 * address descriptor's first word and the source snapshot. Indexed AR0 effects
 * require a WORD16 old-target operand and POST_ACCESS update of the selected AR
 * only; their copied AR0 snapshot is read but never written. MOD13 effects
 * require a validator-approved WORD16/LONG32 operand and PRE_ACCESS selected-AR
 * update; the raw offset is immutable input and is never stale-checked.
 * Reverse-carry effects require a validator-approved WORD16 old-address
 * operand and one POST_ACCESS selected-AR update; their copied AR0 value is
 * read after the selected AR and is never written. Circular effects require
 * a validator-approved WORD16 old-address operand; positive-BK effects carry
 * one POST_ACCESS selected-AR update, while BK=0 effects carry no update.
 * Their copied BK value is read after the selected AR and is never written.
 * Direct DP/SP have no
 * update and never touch an AR.
 */
ti_c54x_address_apply_status ti_c54x_sequencer_apply_address_result(
    ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested_phase);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_ADDRESS_EFFECT_APPLY_H */
