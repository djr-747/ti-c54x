#ifndef TI_C54X_STATE_TEMPORARY_T_REGISTER_H
#define TI_C54X_STATE_TEMPORARY_T_REGISTER_H

/*
 * Module contract
 *
 * Owns: the private 16-bit T temporary-register storage, its architecturally
 * known bit, and checked reset/read/write operations. The state pointer is
 * borrowed for the duration of each call; no pointer is retained. Read
 * outputs are caller-owned and rejected or unknown reads leave them unchanged.
 * Reset physically clears storage while marking the architectural value
 * unknown. Writes accept every uint16_t value and mark T known.
 * Does not own: MMR or data-memory routing, CPU lifecycle, opcode semantics,
 * effect sequencing, timing, diagnostics, persistence, or public ABI state.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_t_register_status {
    TI_C54X_T_REGISTER_OK = 0,
    TI_C54X_T_REGISTER_UNKNOWN = 1,
    TI_C54X_T_REGISTER_INVALID_ARGUMENT = 2
} ti_c54x_t_register_status;

typedef struct ti_c54x_t_register_state {
    uint16_t raw_value;
    bool known;
} ti_c54x_t_register_state;

ti_c54x_t_register_status
ti_c54x_t_register_reset(ti_c54x_t_register_state *state);

ti_c54x_t_register_status
ti_c54x_t_register_read(const ti_c54x_t_register_state *state,
                        uint16_t *value);

ti_c54x_t_register_status
ti_c54x_t_register_write(ti_c54x_t_register_state *state, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_TEMPORARY_T_REGISTER_H */
