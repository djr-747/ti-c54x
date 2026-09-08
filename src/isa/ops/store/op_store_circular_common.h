#ifndef TI_C54X_ISA_OPS_STORE_OP_STORE_CIRCULAR_COMMON_H
#define TI_C54X_ISA_OPS_STORE_OP_STORE_CIRCULAR_COMMON_H

/*
 * Module contract
 *
 * Owns: private checked/trusted validation and MEMORY-effect staging for
 * CMPT=0 circular MOD8/MOD10 WORD16 store wrappers. It validates copied
 * catalog/decode metadata and typed four-bit circular operands, admits AR0--AR7,
 * and retains no caller-owned pointer. Does not own: opcode-family payload
 * selection, opcode catalog generation, decode tables, address resolution,
 * buses, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include <stdint.h>

#include "address/indirect_circular.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_store_circular_operands {
    uint8_t source;
    uint8_t arf;
    uint16_t extension;
    ti_c54x_indirect_circular_mode mode;
} ti_c54x_store_circular_operands;

ti_c54x_op_status ti_c54x_store_validate_circular_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_store_circular_operands *operands);

ti_c54x_op_status ti_c54x_store_validate_circular_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_store_circular_operands *operands);

void ti_c54x_store_stage_circular_write(
    uint8_t arf, ti_c54x_indirect_circular_mode mode, uint16_t value,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_STORE_CIRCULAR_COMMON_H */
