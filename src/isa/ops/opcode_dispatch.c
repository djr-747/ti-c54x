/*
 * Module contract
 *
 * Owns: catalog-derived handler selection and transactional dispatch of a
 * decoded instruction.
 * Does not own: opcode inventory, decode, CPU/register state, status effects,
 * program-counter sequencing, timing, memory, buses, or diagnostics. No
 * default handler exists; unsupported identities and empty slots are errors.
 */

#include "opcode_dispatch.h"

#include <stddef.h>

#include "isa/catalog/opcode_catalog.h"
#include "op_nop.h"

#define TI_C54X_OPCODE_EXACT(name, handler, word, count) \
    [TI_C54X_OPCODE_ID_##name] = ti_c54x_op_##handler,
static const ti_c54x_op_handler ti_c54x_handlers[TI_C54X_OPCODE_ID_COUNT] = {
#include "../catalog/opcode_catalog.def"
};
#undef TI_C54X_OPCODE_EXACT

ti_c54x_op_status
ti_c54x_dispatch_decoded(const ti_c54x_decode_result *instruction,
                         ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;
    ti_c54x_op_handler handler;
    int id;
    ti_c54x_op_status status;

    if (instruction == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->status != TI_C54X_DECODE_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    id = (int)instruction->id;
    if (id <= (int)TI_C54X_OPCODE_ID_UNKNOWN ||
        id >= (int)TI_C54X_OPCODE_ID_COUNT) {
        return TI_C54X_OP_STATUS_UNSUPPORTED_OPCODE;
    }

    handler = ti_c54x_handlers[(size_t)id];
    if (handler == NULL) {
        return TI_C54X_OP_STATUS_UNSUPPORTED_OPCODE;
    }

    status = handler(instruction, &staged);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
