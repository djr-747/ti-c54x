#ifndef TI_C54X_ISA_OPS_STORE_OP_STORE_INDEXED_COMMON_H
#define TI_C54X_ISA_OPS_STORE_OP_STORE_INDEXED_COMMON_H

/*
 * Module contract
 *
 * Owns: private shared validation and MEMORY-effect staging for the
 * standard-indirect AR0-indexed MOD5/MOD6 WORD16 store leaves. It validates
 * copied catalog/decode metadata and typed MOD4 operands, then stages a
 * transaction-ready indexed write. It retains no caller-owned pointer.
 * Does not own: opcode-family payload selection, opcode catalog generation,
 * decode tables, address resolution, buses, CPU state, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 */

#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_store_indexed_operands {
    uint8_t source;
    uint8_t arf;
    uint16_t extension;
    uint8_t mode;
} ti_c54x_store_indexed_operands;

ti_c54x_op_status ti_c54x_store_validate_indexed_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t expected_mode,
    ti_c54x_store_indexed_operands *operands);

ti_c54x_op_status ti_c54x_store_validate_indexed_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t expected_mode,
    ti_c54x_store_indexed_operands *operands);

void ti_c54x_store_stage_indexed_write(
    uint8_t arf, uint8_t indexed_mode, uint16_t value,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_STORE_INDEXED_COMMON_H */
