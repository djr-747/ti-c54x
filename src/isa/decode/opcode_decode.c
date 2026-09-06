/*
 * Module contract
 *
 * Owns: pure first-word decode result construction from the catalog's direct
 * lookup. Unknown words remain explicit and preserve the raw fetched value.
 * Does not own: bus/memory fetches, CPU state, program-counter sequencing,
 * opcode semantics, timing, observers, or diagnostics.
 */

#include "opcode_decode.h"

#include <stddef.h>

#include "../catalog/opcode_catalog_internal.h"

ti_c54x_decode_result
ti_c54x_decode_first_word(uint16_t first_word)
{
    const ti_c54x_opcode_id id =
        ti_c54x_opcode_catalog_lookup_first_word(first_word);
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    ti_c54x_decode_result result = {
        TI_C54X_DECODE_STATUS_UNKNOWN_WORD,
        TI_C54X_OPCODE_ID_UNKNOWN,
        first_word,
        UINT8_C(0)};

    if (metadata != NULL && id != TI_C54X_OPCODE_ID_UNKNOWN) {
        result.status = TI_C54X_DECODE_STATUS_OK;
        result.id = id;
        result.program_word_count = metadata->program_word_count;
    }
    return result;
}
