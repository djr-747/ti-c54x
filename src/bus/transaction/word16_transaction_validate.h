#ifndef TI_C54X_BUS_TRANSACTION_WORD16_TRANSACTION_VALIDATE_H
#define TI_C54X_BUS_TRANSACTION_WORD16_TRANSACTION_VALIDATE_H

/*
 * Module contract
 *
 * Owns: read-only shape and payload validation for the private WORD16
 * transaction reducer.
 * Does not own: lifecycle mutation, issue/completion transitions, callbacks,
 * memory, arbitration, timing, CPU state, diagnostics, or public ABI.
 */

#include <stdbool.h>

#include "word16_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_bus_word16_transaction_state_is_valid(
    ti_c54x_bus_word16_transaction_state state);

bool ti_c54x_bus_word16_transaction_is_canonical(
    const ti_c54x_bus_word16_transaction *transaction);

bool ti_c54x_bus_word16_transaction_request_is_valid(
    const ti_c54x_bus_word16_request *request);

bool ti_c54x_bus_word16_observation_is_valid(
    const ti_c54x_bus_word16_observation *observation);

bool ti_c54x_bus_word16_transaction_fault_status_is_valid(
    ti_c54x_bus_status status);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_TRANSACTION_WORD16_TRANSACTION_VALIDATE_H */
