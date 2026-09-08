#ifndef TI_C54X_SEQUENCER_INSTRUCTION_LONG32_PAIR_TRANSACTION_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_LONG32_PAIR_TRANSACTION_VALIDATE_H

/*
 * Module contract
 *
 * Owns: read-only validation and canonical WORD16 request construction for
 * the private LONG32 pair reducer.
 * Does not own: lifecycle transitions, endpoint callbacks, physical memory,
 * CPU state, publication, timing, observers, diagnostics, or global state.
 */

#include <stdbool.h>

#include "long32_pair_transaction.h"

bool ti_c54x_long32_pair_build_word16_request(
    const ti_c54x_long32_pair_request *request,
    bool second,
    ti_c54x_bus_word16_request *out_request);

bool ti_c54x_long32_pair_transaction_is_canonical(
    const ti_c54x_long32_pair_transaction *transaction);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_LONG32_PAIR_TRANSACTION_VALIDATE_H */
