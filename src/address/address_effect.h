#ifndef TI_C54X_ADDRESS_EFFECT_H
#define TI_C54X_ADDRESS_EFFECT_H

/*
 * Module contract
 *
 * Owns: the value types used to describe a checked data-operand effective
 * address (including its width and explicit word pair), its value-owned
 * address-source snapshot, and at most one auxiliary-register update.
 * Does not own: CPU/register storage, status-register validation, bus transfers,
 * opcode decoding, instruction sequencing, memory mapping, or diagnostics.
 * Results are returned by value. Callers own every result object and may keep
 * it after the planning call returns; no field borrows storage from the input.
 * Every non-OK result has an all-zero effect.
 */

#include <stdint.h>

#include <ti_c54x/address.h>

#include "data_operand.h"

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

/*
 * State preconditions carried by an effect.  This is intentionally a private
 * execution seam rather than a public CPU policy: a producer tags the exact
 * architectural mode state its effect requires, and the sequencer validates
 * that tag before consulting the captured address source.
 */
typedef enum ti_c54x_address_state_policy {
    TI_C54X_ADDRESS_STATE_POLICY_NONE = 0,
    TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0 = 1,
    TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0 = 2,
    TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1 = 3
} ti_c54x_address_state_policy;

typedef enum ti_c54x_address_source_kind {
    TI_C54X_ADDRESS_SOURCE_NONE = 0,
    TI_C54X_ADDRESS_SOURCE_AR = 1,
    TI_C54X_ADDRESS_SOURCE_DIRECT_DP = 2,
    TI_C54X_ADDRESS_SOURCE_DIRECT_SP = 3,
    TI_C54X_ADDRESS_SOURCE_AR_INDEXED = 4,
    TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET = 5
} ti_c54x_address_source_kind;

typedef struct ti_c54x_address_source_ar {
    uint8_t index;
    uint16_t value_before;
} ti_c54x_address_source_ar;

/* Snapshot for AR0-indexed forms.  `index` is the selected ARx (AR1..AR7);
 * `value_before` is that register's old value and `ar0_value_before` is the
 * copied AR0 index value used for the one post-access update. */
typedef struct ti_c54x_address_source_ar_indexed {
    uint8_t index;
    uint16_t value_before;
    uint16_t ar0_value_before;
} ti_c54x_address_source_ar_indexed;

/* Snapshot for CMPT=0 signed long-offset MOD13 forms.  `modification` is
 * retained as the raw architectural modifier field (13 for MOD13), while
 * `long_offset_word` is the copied 16-bit instruction extension. */
typedef struct ti_c54x_address_source_ar_long_offset {
    uint8_t index;
    uint8_t modification;
    uint16_t value_before;
    uint16_t long_offset_word;
} ti_c54x_address_source_ar_long_offset;

typedef struct ti_c54x_address_source_direct_dp {
    uint16_t dp9;
    uint8_t dma7;
} ti_c54x_address_source_direct_dp;

typedef struct ti_c54x_address_source_direct_sp {
    uint16_t sp16;
    uint8_t dma7;
} ti_c54x_address_source_direct_sp;

/* The active union arm is selected by kind.  All fields are copied into the
 * effect; no source pointer or caller-owned storage is retained. */
typedef struct ti_c54x_address_source {
    ti_c54x_address_source_kind kind;
    union {
        ti_c54x_address_source_ar ar;
        ti_c54x_address_source_ar_indexed ar_indexed;
        ti_c54x_address_source_ar_long_offset ar_long_offset;
        ti_c54x_address_source_direct_dp direct_dp;
        ti_c54x_address_source_direct_sp direct_sp;
    } value;
} ti_c54x_address_source;

typedef struct ti_c54x_address_update {
    ti_c54x_address_update_target target;
    ti_c54x_address_update_phase phase;
    uint8_t ar_index;
    uint16_t value_before;
    uint16_t value_after;
} ti_c54x_address_update;

typedef struct ti_c54x_address_effect {
    ti_c54x_data_operand operand;
    ti_c54x_address_source source;
    ti_c54x_address_state_policy state_policy;
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
