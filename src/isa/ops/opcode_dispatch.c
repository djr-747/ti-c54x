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

#define TI_C54X_OPCODE_EXACT(symbol, handler, first_word, program_words, \
                             operand_layout, repeatability) \
    [TI_C54X_OPCODE_ID_##symbol] = ti_c54x_op_##handler,
#define TI_C54X_OPCODE_MASKED(symbol, handler, first_word_value, \
                              first_word_mask, program_words, operand_layout, \
                              repeatability) \
    [TI_C54X_OPCODE_ID_##symbol] = ti_c54x_op_##handler,
static const ti_c54x_op_handler ti_c54x_handlers[TI_C54X_OPCODE_ID_COUNT] = {
#include "../catalog/opcode_catalog.def"
};
#undef TI_C54X_OPCODE_EXACT
#undef TI_C54X_OPCODE_MASKED

ti_c54x_op_status
ti_c54x_dispatch_decoded(const ti_c54x_decoded_instruction *instruction,
                         ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_handler handler;
    int id;
    ti_c54x_op_status status;

    if (instruction == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    id = (int)instruction->id;
    if (id <= (int)TI_C54X_OPCODE_ID_UNKNOWN ||
        id >= (int)TI_C54X_OPCODE_ID_COUNT) {
        return TI_C54X_OP_STATUS_UNSUPPORTED_OPCODE;
    }

    metadata = ti_c54x_opcode_metadata_for_id(instruction->id);
    if (metadata == NULL || metadata->id != instruction->id ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->bundle.program_word_count == UINT8_C(0) ||
        instruction->bundle.program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
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
