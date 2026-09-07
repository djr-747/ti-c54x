#ifndef TI_C54X_ISA_OPS_DISPATCH_TRUSTED_SEMANTICS_H
#define TI_C54X_ISA_OPS_DISPATCH_TRUSTED_SEMANTICS_H

/*
 * Module contract
 *
 * Owns: declarations for catalog-token trusted semantic leaves. Does not own:
 * catalog metadata, packet preparation, checked dispatch, CPU orchestration,
 * effect commit, buses, timing, diagnostics, or object lifetimes. Every leaf
 * consumes borrowed constructor-validated inputs for one call only.
 */

#include "trusted_handler.h"

#define TI_C54X_OPCODE_EXACT(symbol, handler, first_word, program_words, \
                             operand_layout, repeatability, input_resources) \
    ti_c54x_op_status ti_c54x_trusted_op_##handler( \
        const ti_c54x_decoded_instruction *instruction, \
        const ti_c54x_trusted_operand_values *operands, \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
#define TI_C54X_OPCODE_MASKED(symbol, handler, first_word_value, \
                              first_word_mask, program_words, operand_layout, \
                              repeatability, input_resources) \
    ti_c54x_op_status ti_c54x_trusted_op_##handler( \
        const ti_c54x_decoded_instruction *instruction, \
        const ti_c54x_trusted_operand_values *operands, \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
#include "isa/catalog/opcode_catalog.def"
#undef TI_C54X_OPCODE_EXACT
#undef TI_C54X_OPCODE_MASKED

#endif /* TI_C54X_ISA_OPS_DISPATCH_TRUSTED_SEMANTICS_H */
