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
 * address kinds with their delayed-branch counterparts (identical
 * stack planning; only the root PC commit timing differs), and
 * static completion callback shapes.
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
    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 = 9,
    /* Delayed-branch stack push: identical TOS planning to the
     * immediate push (old SP - 1 with a PRE_ACCESS SP update,
     * WRITE-only), but the root commits the sequential PC successor
     * and reports the frozen pmad as the deferred branch target. The
     * staged return address is PC+4 per SPRU172C §4 CALL[D]. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 = 10,
    /* Delayed-branch stack pop: identical TOS planning to the
     * immediate pop (old SP with a POST_ACCESS SP update,
     * READ-only with null completion), but the root commits the
     * sequential PC successor and reports the TOS READ observation
     * as the deferred branch target. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 = 11,
    /* Fast-return register pop: identical SP planning to the
     * immediate pop (old SP with a POST_ACCESS SP update,
     * READ-only with a zero staged value), but the root issues no
     * bus traffic. The PC target is the RTN value the root freezes
     * at prepare; a non-null completion additionally stages one
     * follow-on effect the root commits before the pop
     * architecture commit (the pop-completion seam). */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16 = 12,
    /* DP-independent page-0 direct WORD16: the 7-bit direct DATA
     * address rides `direct_dma7` (0x00-0x7F; 0x00-0x1F route to
     * the MMR file, 0x20-0x7F reach DP0 endpoint RAM). No AR,
     * CMPT, CPL, or DP dependence and no post-update; the nine
     * MSBs of the effective address clear regardless of DP. */
    TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16 = 13,
    /* DATA+PROGRAM READ pair (MACP): the Smem leg is a direct
     * page-0 or standard-indirect MOD0--MOD2 DATA read selected by
     * `pair_indirect` (MOD3 reads stay carrier-invalid by the
     * single-leg write-only rule; exotic modes above MOD3 never
     * reach the carrier), and the PROGRAM leg is an absolute
     * 16-bit program-word coefficient read with no post-update.
     * Both values publish through the shared `complete_dual`
     * shape with the Smem word as `x` and the coefficient as
     * `y`; read-phase T load takes the `x` word. */
    TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 = 14
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
            /* Used only by DIRECT_WORD16; 7-bit direct DATA address. Must
             * remain zero for other kinds. Appended last so existing field
             * offsets and the ABI sizes are unchanged. */
            uint8_t direct_dma7;
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
        struct {
            /* DATA+PROGRAM pair: the Smem DATA leg selector. Zero
             * selects the direct page-0 leg (`smem_dma7`); one
             * selects the standard-indirect leg (`smem_mode` with
             * `smem_arf`). */
            uint8_t indirect;
            /* Indirect-leg mode MOD0/MOD1/MOD2; MOD3 stays
             * carrier-invalid. Must remain zero for the direct leg. */
            uint8_t smem_mode;
            /* Indirect-leg AR selector. Must remain zero for the
             * direct leg. */
            uint8_t smem_arf;
            /* Direct-leg 7-bit DATA address. Must remain zero for
             * the indirect leg. */
            uint8_t smem_dma7;
            /* Absolute PROGRAM-leg word address (the coefficient). */
            uint16_t program_pmad;
            /* Reserved words keep the arm fixed-size and make the
             * checked hot-path validation four scalar comparisons. */
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } pair;
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
    /* Read-phase T load (MPYA): when true on a WORD16 READ, the root
     * loads T with the read value word after the read-value
     * completion commits. Only READ access with no RMW transform may
     * set this; all other carriers leave it false. */
    bool load_t_with_read;
    /* Smem delay write-back (MACD): when true on a DATA+PROGRAM READ
     * pair, the pair root writes the Smem (x) word to DATA
     * Smem-address-plus-one (16-bit wraparound) through the endpoint
     * after both reads complete and before the post-access commit.
     * Only the DATA+PROGRAM pair kind may set this; all other
     * carriers leave it false. Shares the existing padding: the
     * 48-byte effect ABI is unchanged. */
    bool write_smem_delay;
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
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 48,
               "WORD16 memory effect ABI changed on LP64");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24,
               "WORD16 memory address ABI changed on ILP32");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 40,
               "WORD16 memory effect ABI changed on ILP32");
#endif

bool ti_c54x_op_memory_effect_is_valid(
    const ti_c54x_op_memory_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MEMORY_MEMORY_EFFECT_H */
