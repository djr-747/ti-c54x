#ifndef TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H
#define TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H

/*
 * Module contract
 *
 * Owns: the value-only generic WORD16 data-memory effect carrier, including
 * its fixed MOD2 single-indirect address specification and static completion
 * callback shape.
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
#include "address/indirect_single.h"
#include "bus/request/word16_request.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_op_effect ti_c54x_op_effect;

typedef enum ti_c54x_op_memory_address_kind {
    TI_C54X_OP_MEMORY_ADDRESS_INVALID = 0,
    TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16 = 1
} ti_c54x_op_memory_address_kind;

typedef struct ti_c54x_op_memory_address_spec {
    ti_c54x_op_memory_address_kind kind;
    ti_c54x_indirect_single_mode mode;
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
    uint16_t write_value_word;
    ti_c54x_op_memory_complete_fn complete;
} ti_c54x_op_memory_effect;

bool ti_c54x_op_memory_effect_is_valid(
    const ti_c54x_op_memory_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H */
