#ifndef TI_C54X_SEQUENCER_EFFECT_TRANSACTION_H
#define TI_C54X_SEQUENCER_EFFECT_TRANSACTION_H

/*
 * Module contract
 *
 * Owns: a value-owned, route-tagged candidate transaction for the currently
 * supported opcode effects. Staging borrows live state only for the call and
 * never retains pointers; commit borrows destination state only for the call.
 * The candidate contains only the state selected by its route. Both APIs are
 * deterministic and leave their caller-owned output unchanged on failure.
 * Does not own: opcode semantics, decode, program-counter effects, CPU
 * lifecycle, repeat/interrupt policy, memory, buses, timing, diagnostics,
 * allocation, observers, or public persistence state.
 */

#include "../isa/ops/op_effect.h"
#include "../state/accumulator/accumulator.h"
#include "../state/mmr_file.h"
#include "../state/status/registers.h"
#include "../state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_effect_transaction_status {
    TI_C54X_EFFECT_TRANSACTION_OK = 0,
    TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT = 1,
    TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED = 2,
    TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT = 3
} ti_c54x_effect_transaction_status;

typedef enum ti_c54x_effect_transaction_target {
    TI_C54X_EFFECT_TRANSACTION_NONE = 0,
    TI_C54X_EFFECT_TRANSACTION_STATUS_FIELD = 1,
    TI_C54X_EFFECT_TRANSACTION_ACCUMULATOR = 2,
    TI_C54X_EFFECT_TRANSACTION_T_REGISTER = 3,
    TI_C54X_EFFECT_TRANSACTION_STATUS_BIT = 4,
    TI_C54X_EFFECT_TRANSACTION_TRN_SHIFT = 5
} ti_c54x_effect_transaction_target;

typedef struct ti_c54x_effect_transaction_trn_shift_candidate {
    ti_c54x_mmr_file_state mmr_file;
    ti_c54x_status_register_state status;
} ti_c54x_effect_transaction_trn_shift_candidate;

typedef struct ti_c54x_effect_transaction_accumulator_candidate {
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
} ti_c54x_effect_transaction_accumulator_candidate;

typedef union ti_c54x_effect_transaction_candidate {
    ti_c54x_status_register_state status_field;
    ti_c54x_status_register_state status_bit;
    ti_c54x_effect_transaction_accumulator_candidate accumulator;
    ti_c54x_t_register_state t_register;
    ti_c54x_effect_transaction_trn_shift_candidate trn_shift;
} ti_c54x_effect_transaction_candidate;

typedef struct ti_c54x_effect_transaction {
    ti_c54x_effect_transaction_target target;
    ti_c54x_effect_transaction_candidate candidate;
} ti_c54x_effect_transaction;

/*
 * Borrowed inputs are read only for this call. On success, out receives a
 * fully self-contained candidate. On failure, *out is byte-identical.
 */
ti_c54x_effect_transaction_status ti_c54x_effect_transaction_stage(
    const ti_c54x_op_effect *effect,
    const ti_c54x_status_register_state *status,
    const ti_c54x_accumulator_state *accumulators,
    const ti_c54x_t_register_state *t_register,
    const ti_c54x_mmr_file_state *mmr_file,
    ti_c54x_effect_transaction *out);

/*
 * Destination pointers are borrowed for this call and never retained. All
 * route-required destinations are checked before any selected state is copied;
 * failures therefore leave every destination byte-identical.
 */
ti_c54x_effect_transaction_status ti_c54x_effect_transaction_commit(
    const ti_c54x_effect_transaction *transaction,
    ti_c54x_status_register_state *status,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_mmr_file_state *mmr_file);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_EFFECT_TRANSACTION_H */
