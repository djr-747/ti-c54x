#ifndef TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H
#define TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H

/*
 * Module contract
 *
 * Owns: the value-only generic WORD16 data-memory effect carrier, including
 * its MOD0--MOD3 single-indirect, AR0-indexed MOD5/MOD6, circular
 * MOD8/MOD10, ARF-selected reverse-carry MOD4/MOD7, and CMPT=0
 * MOD13 LONG32 and dual-indirect MOD0/MOD1/MOD2/MOD3 WORD16 address
 * specifications, the mode-independent STACK_PUSH/STACK_POP WORD16
 * address kinds, and static completion callback shapes.
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
    TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16 = 4,
    TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32 = 5,
    TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16 = 6,
    /* Single X-side read through the dual planner with Y self-aliased;
     * only the X read is issued and only the X update commits. */
    TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY = 7,
    /* Mode-independent stack push: the TOS DATA WORD16 lives at old SP - 1
     * with a PRE_ACCESS SP update. The WRITE payload carries the staged
     * return address; the access is WRITE-only. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 = 8,
    /* Mode-independent stack pop: the TOS DATA WORD16 lives at old SP
     * with a POST_ACCESS SP update. The access is READ-only with null
     * completion; the pop commit consumes the READ observation
     * directly instead of the ordinary accumulator/status staging. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 = 9
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
    union {
        struct {
            /* Used only by SINGLE_INDIRECT; must remain MOD0 for other kinds. */
            ti_c54x_indirect_single_mode mode;
            /* Used only by INDEXED_AR0; must remain zero/default for other kinds. */
            ti_c54x_indirect_indexed_mode indexed_mode;
            /* Used only by CIRCULAR; must remain zero/default for other kinds. */
            ti_c54x_indirect_circular_mode circular_mode;
            /* Used only by REVERSE_CARRY; must remain zero/default for other kinds. */
            ti_c54x_indirect_reverse_carry_mode reverse_carry_mode;
            /* Used only by LONG_OFFSET_MOD13_LONG32; copied verbatim as `lk`. */
            uint16_t long_offset_word;
            uint8_t arf;
        };
        struct {
            /* Dual-indirect forms carry two MOD0/MOD1/MOD2 AR selectors. */
            uint8_t x_mode;
            uint8_t x_ar;
            uint8_t y_mode;
            uint8_t y_ar;
            /* Reserved words keep the arm fixed-size and make the checked
             * hot-path validation four scalar comparisons rather than
             * sixteen byte comparisons. */
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
            uint32_t reserved_word3;
        } dual;
    };
} ti_c54x_op_memory_address_spec;

typedef ti_c54x_op_status (*ti_c54x_op_memory_complete_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

typedef ti_c54x_op_status (*ti_c54x_op_memory_complete_long32_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t first_msw,
    uint16_t second_lsw,
    ti_c54x_op_effect *next_effect);

typedef ti_c54x_op_status (*ti_c54x_op_memory_complete_dual_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t x_word,
    uint16_t y_word,
    ti_c54x_op_effect *next_effect);

typedef struct ti_c54x_op_memory_effect {
    ti_c54x_op_memory_address_spec address_spec;
    ti_c54x_bus_word16_request_access access;
    union {
        struct {
            /* Ordinary WRITE payload or RMW operand; ordinary READ requires
             * zero. The final byte is reserved and must remain zero. */
            uint16_t value_word;
            uint8_t rmw_operation;
            uint8_t transfer_reserved;
        };
        struct {
            /* Copied MSW/LSW payload for a LONG32 WRITE or READ result. */
            uint16_t first_value_msw;
            uint16_t second_value_lsw;
        };
    };
    union {
        ti_c54x_op_memory_complete_fn complete;
        ti_c54x_op_memory_complete_long32_fn complete_long32;
        ti_c54x_op_memory_complete_dual_fn complete_dual;
    };
} ti_c54x_op_memory_effect;

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24,
               "WORD16 memory address ABI changed on LP64");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 40,
               "WORD16 memory effect ABI changed on LP64");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24,
               "WORD16 memory address ABI changed on ILP32");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 36,
               "WORD16 memory effect ABI changed on ILP32");
#endif

bool ti_c54x_op_memory_effect_is_valid(
    const ti_c54x_op_memory_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H */
