/* Module contract
 *
 * Owns: resolution of one copied MOD13 long-offset LONG32
 * spec; see address_resolution_long32_mod13.h.
 * Does not own: other address kinds, endpoint traffic, the
 * instruction commit root, opcode semantics, CPU lifecycle,
 * timing, observers, diagnostics, allocation, or callbacks.
 */

#ifndef TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_LONG32_MOD13_H
#define TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_LONG32_MOD13_H

#include "address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_memory_address_resolution_status
ti_c54x_sequencer_resolve_long32_mod13_address(
    const ti_c54x_arch_state *state,
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access access,
    uint16_t write_value_word,
    ti_c54x_memory_address_resolution *out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_MEMORY_ADDRESS_RESOLUTION_LONG32_MOD13_H */
