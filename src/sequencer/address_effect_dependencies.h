/*
 * Module contract
 *
 * Owns: ordered, read-only architectural mode and source snapshot checks for
 * an already intrinsically valid borrowed address effect. It reads only the
 * state accessors required by the effect's tagged policy/source and never
 * mutates state.
 * Does not own: effect shape or arithmetic validation, phase/relation policy,
 * commit description construction, architectural writes, planning, decode,
 * CPU lifecycle, buses, memory, timing, diagnostics, or public ABI. State
 * and effect are borrowed for the duration of the call and are not retained.
 */

#ifndef TI_C54X_SEQUENCER_ADDRESS_EFFECT_DEPENDENCIES_H
#define TI_C54X_SEQUENCER_ADDRESS_EFFECT_DEPENDENCIES_H

#include "address_effect_apply.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Check mode and captured-source dependencies. Callers should pass an effect
 * accepted by the intrinsic validator; null inputs are rejected defensively.
 */
ti_c54x_address_apply_status
ti_c54x_sequencer_validate_address_effect_dependencies(
    const ti_c54x_arch_state *state,
    const ti_c54x_address_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_ADDRESS_EFFECT_DEPENDENCIES_H */
