/*
 * Module contract
 *
 * Owns: root routing and atomic publication for value-owned opcode effects.
 * It builds only the selected route's local candidate through its leaf and
 * publishes that member only after validation succeeds. It never retains
 * borrowed state pointers and never branches on opcode identity; routing is
 * solely by effect kind.
 * Does not own: opcode semantics, decode, PC effects, CPU lifecycle,
 * repeat/interrupt policy, memory, buses, timing, diagnostics, allocation,
 * observers, or public persistence state.
 */

#include "effect_transaction.h"

#include "accumulator/effect_apply.h"
#include "status/bit_effect_apply.h"
#include "status/field_effect_apply.h"
#include "temporary/effect_apply.h"

#include <stddef.h>
static ti_c54x_effect_transaction_status map_status_effect(
    ti_c54x_status_field_effect_status status)
{
    switch (status) {
    case TI_C54X_STATUS_FIELD_EFFECT_OK:
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_STATUS_FIELD_EFFECT_INVALID_ARGUMENT:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
    case TI_C54X_STATUS_FIELD_EFFECT_UNSUPPORTED:
        return TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED;
    case TI_C54X_STATUS_FIELD_EFFECT_INVALID_EFFECT:
    default:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT;
    }
}

static ti_c54x_effect_transaction_status map_status_bit_effect(
    ti_c54x_status_bit_effect_status status)
{
    switch (status) {
    case TI_C54X_STATUS_BIT_EFFECT_OK:
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_STATUS_BIT_EFFECT_INVALID_ARGUMENT:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
    case TI_C54X_STATUS_BIT_EFFECT_UNSUPPORTED:
        return TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED;
    case TI_C54X_STATUS_BIT_EFFECT_INVALID_EFFECT:
    default:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT;
    }
}

static ti_c54x_effect_transaction_status map_accumulator_effect(
    ti_c54x_accumulator_effect_status status)
{
    switch (status) {
    case TI_C54X_ACCUMULATOR_EFFECT_OK:
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
    case TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED:
        return TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED;
    case TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT:
    default:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT;
    }
}

static ti_c54x_effect_transaction_status map_t_effect(
    ti_c54x_t_register_effect_status status)
{
    switch (status) {
    case TI_C54X_T_REGISTER_EFFECT_OK:
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_T_REGISTER_EFFECT_INVALID_ARGUMENT:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
    case TI_C54X_T_REGISTER_EFFECT_UNSUPPORTED:
    default:
        return TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED;
    }
}

ti_c54x_effect_transaction_status ti_c54x_effect_transaction_stage(
    const ti_c54x_op_effect *effect,
    const ti_c54x_status_register_state *status,
    const ti_c54x_accumulator_state *accumulators,
    const ti_c54x_t_register_state *t_register,
    ti_c54x_effect_transaction *out)
{
    ti_c54x_effect_transaction_status result;

    if (effect == NULL || out == NULL) {
        return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
    }

    switch (effect->kind) {
    case TI_C54X_OP_EFFECT_NONE:
        out->target = TI_C54X_EFFECT_TRANSACTION_NONE;
        return TI_C54X_EFFECT_TRANSACTION_OK;

    case TI_C54X_OP_EFFECT_REPEAT_START:
        return TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED;

    case TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE:
    {
        ti_c54x_status_register_state candidate;

        if (status == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        candidate = *status;
        result = map_status_effect(ti_c54x_status_field_effect_apply(
            effect, &candidate));
        if (result != TI_C54X_EFFECT_TRANSACTION_OK) {
            return result;
        }
        out->candidate.status_field = candidate;
        out->target = TI_C54X_EFFECT_TRANSACTION_STATUS_FIELD;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    }

    case TI_C54X_OP_EFFECT_STATUS_BIT_WRITE:
    {
        ti_c54x_status_register_state candidate;

        if (status == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        candidate = *status;
        result = map_status_bit_effect(ti_c54x_status_bit_effect_apply(
            effect, &candidate));
        if (result != TI_C54X_EFFECT_TRANSACTION_OK) {
            return result;
        }
        out->candidate.status_bit = candidate;
        out->target = TI_C54X_EFFECT_TRANSACTION_STATUS_BIT;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    }

    case TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE:
    {
        ti_c54x_effect_transaction_accumulator_candidate candidate;

        if (status == NULL || accumulators == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        candidate.accumulators = *accumulators;
        candidate.status = *status;
        result = map_accumulator_effect(ti_c54x_accumulator_effect_apply(
            effect, &candidate.accumulators, &candidate.status));
        if (result != TI_C54X_EFFECT_TRANSACTION_OK) {
            return result;
        }
        out->candidate.accumulator = candidate;
        out->target = TI_C54X_EFFECT_TRANSACTION_ACCUMULATOR;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    }

    case TI_C54X_OP_EFFECT_T_REGISTER_WRITE:
    {
        ti_c54x_t_register_state candidate;

        if (t_register == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        candidate = *t_register;
        result = map_t_effect(ti_c54x_t_register_effect_apply(
            effect, &candidate));
        if (result != TI_C54X_EFFECT_TRANSACTION_OK) {
            return result;
        }
        out->candidate.t_register = candidate;
        out->target = TI_C54X_EFFECT_TRANSACTION_T_REGISTER;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    }

    default:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT;
    }
}

ti_c54x_effect_transaction_status ti_c54x_effect_transaction_commit(
    const ti_c54x_effect_transaction *transaction,
    ti_c54x_status_register_state *status,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register)
{
    if (transaction == NULL) {
        return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
    }

    /* Validate the complete route before the first destination assignment. */
    switch (transaction->target) {
    case TI_C54X_EFFECT_TRANSACTION_NONE:
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_EFFECT_TRANSACTION_STATUS_FIELD:
        if (status == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        *status = transaction->candidate.status_field;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_EFFECT_TRANSACTION_STATUS_BIT:
        if (status == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        *status = transaction->candidate.status_bit;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_EFFECT_TRANSACTION_ACCUMULATOR:
        if (status == NULL || accumulators == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        *accumulators = transaction->candidate.accumulator.accumulators;
        *status = transaction->candidate.accumulator.status;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    case TI_C54X_EFFECT_TRANSACTION_T_REGISTER:
        if (t_register == NULL) {
            return TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT;
        }
        *t_register = transaction->candidate.t_register;
        return TI_C54X_EFFECT_TRANSACTION_OK;
    default:
        return TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT;
    }
}
