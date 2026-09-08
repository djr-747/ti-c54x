#ifndef TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H
#define TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H

/*
 * Module contract
 *
 * Owns: the value-only generic WORD16 data-memory effect carrier, including
 * its MOD0--MOD3 single-indirect, AR0-indexed MOD5/MOD6, circular
 * MOD8/MOD10, and ARF-selected reverse-carry MOD4/MOD7 address
 * specifications and static completion callback shape.
 * Does not own: architectural state, address resolution, bus callbacks,
 * memory visibility, opcode identity, timing, or diagnostics. Every field is
 * copied by value; the callback is process-lifetime code and no context is
 * retained.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"
#include "address/indirect_circular.h"
#include "address/indirect_indexed.h"
#include "address/indirect_reverse_carry.h"
#include "address/indirect_single.h"
#include "bus/request/word16_request.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_op_effect ti_c54x_op_effect;

typedef enum ti_c54x_op_memory_address_kind {
    TI_C54X_OP_MEMORY_ADDRESS_INVALID = 0,
    TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16 = 1,
    TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16 = 2,
    TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16 = 3,
    TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16 = 4
} ti_c54x_op_memory_address_kind;

/*
 * A WORD16 memory read may optionally feed one value-owned transform before
 * its paired write.  The carrier stores the operation in one byte of the
 * existing value/callback alignment padding; no callback or context is
 * retained here.
 */
typedef enum ti_c54x_op_memory_rmw_operation {
    TI_C54X_OP_MEMORY_RMW_OPERATION_NONE = 0,
    TI_C54X_OP_MEMORY_RMW_OPERATION_AND = 1,
    TI_C54X_OP_MEMORY_RMW_OPERATION_OR = 2,
    TI_C54X_OP_MEMORY_RMW_OPERATION_XOR = 3
} ti_c54x_op_memory_rmw_operation;

typedef struct ti_c54x_op_memory_address_spec {
    ti_c54x_op_memory_address_kind kind;
    /* Used only by SINGLE_INDIRECT; must remain MOD0 for other kinds. */
    ti_c54x_indirect_single_mode mode;
    /* Used only by INDEXED_AR0; must remain zero/default for other kinds. */
    ti_c54x_indirect_indexed_mode indexed_mode;
    /* Used only by CIRCULAR; must remain zero/default for other kinds. */
    ti_c54x_indirect_circular_mode circular_mode;
    /* Used only by REVERSE_CARRY; must remain zero/default for other kinds. */
    ti_c54x_indirect_reverse_carry_mode reverse_carry_mode;
    uint8_t arf;
} ti_c54x_op_memory_address_spec;

typedef ti_c54x_op_status (*ti_c54x_op_memory_complete_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

typedef struct ti_c54x_op_memory_effect {
    ti_c54x_op_memory_address_spec address_spec;
    ti_c54x_bus_word16_request_access access;
    /* Ordinary WRITE payload or RMW operand; ordinary READ requires zero. */
    uint16_t value_word;
    /* Kept byte-sized so the carrier remains compact on existing ABIs. */
    uint8_t rmw_operation;
    ti_c54x_op_memory_complete_fn complete;
} ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_is_valid(
    const ti_c54x_op_memory_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H */
