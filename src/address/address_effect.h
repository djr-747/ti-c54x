#ifndef TI_C54X_ADDRESS_EFFECT_H
#define TI_C54X_ADDRESS_EFFECT_H

/*
 * Module contract
 *
 * Owns: the value types used to describe a checked effective-address result
 * and at most one auxiliary-register update.
 * Does not own: CPU/register storage, status-register policy, bus transfers,
 * opcode decoding, instruction sequencing, memory mapping, or diagnostics.
 * Results are returned by value. Callers own every result object and may keep
 * it after the planning call returns; no field borrows storage from the input.
 * Every non-OK result has an all-zero effect.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_address_status {
    TI_C54X_ADDRESS_STATUS_OK = 0,
    TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS = 2,
    TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY = 3
} ti_c54x_address_status;

typedef enum ti_c54x_address_access {
    TI_C54X_ADDRESS_ACCESS_READ = 0,
    TI_C54X_ADDRESS_ACCESS_WRITE = 1
} ti_c54x_address_access;

typedef enum ti_c54x_address_update_target {
    TI_C54X_ADDRESS_UPDATE_NONE = 0,
    TI_C54X_ADDRESS_UPDATE_AR = 1
} ti_c54x_address_update_target;

typedef enum ti_c54x_address_update_phase {
    TI_C54X_ADDRESS_UPDATE_PHASE_NONE = 0,
    TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS = 1,
    TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS = 2
} ti_c54x_address_update_phase;

typedef struct ti_c54x_address_update {
    ti_c54x_address_update_target target;
    ti_c54x_address_update_phase phase;
    uint8_t ar_index;
    uint16_t value_before;
    uint16_t value_after;
} ti_c54x_address_update;

typedef struct ti_c54x_address_effect {
    ti_c54x_data_word_address effective_address_word;
    uint8_t selected_ar_index;
    ti_c54x_address_update update;
} ti_c54x_address_effect;

typedef struct ti_c54x_address_result {
    ti_c54x_address_status status;
    ti_c54x_address_effect effect;
} ti_c54x_address_result;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_EFFECT_H */
