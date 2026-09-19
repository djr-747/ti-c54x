/*
 * Module contract
 *
 * Owns: structural validation of the generic SINGLE_INDIRECT MOD0--MOD3,
 * INDEXED_AR0 MOD5/MOD6, CIRCULAR MOD8/MOD10, ARF-selected
 * REVERSE_CARRY MOD4/MOD7 WORD16, MOD13 LONG32, X-only
 * dual-indirect WORD16, DATA+PROGRAM pair WORD16, and
 * mode-independent STACK_PUSH/STACK_POP WORD16 memory effect carriers,
 * plus the single-word DIRECT/SINGLE_INDIRECT delay write-back READ
 * shape (optional completion; WRITE/RMW access stays invalid).
 * The single-word Lmem LONG32, absolute DATA, and dual-indirect
 * MOD0/MOD1/MOD2 arms delegate to their own modules. Dual MOD3
 * remains deferred with the AR0/BK indexed/circular dependency.
 * Does not own: address state, bus execution, memory visibility, opcode
 * identity, timing, or diagnostics.
 */

#include "memory_effect.h"

#include "memory_effect_absolute_data.h"
#include "memory_effect_dual.h"
#include "memory_effect_smem_absolute_pair.h"
#include "memory_effect_smem_program_pair.h"
#include "memory_effect_long32_single.h"

#include <stddef.h>

static bool
ti_c54x_op_memory_rmw_operation_is_valid(uint8_t operation)
{
    return operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_AND ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_OR ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_XOR ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK;
}

/* Single-word Smem delay write-back (LTD/DELAY): a plain DATA
 * READ with no RMW and a zero staged value. The completion is
 * optional: a T-register-write completion is the LTD shape while a
 * null completion is the DELAY shape whose delay WRITE is the only
 * publication. */
static bool
ti_c54x_op_memory_effect_single_delay_shape_is_valid(
    const ti_c54x_op_memory_effect *effect)
{
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           effect->value_word == UINT16_C(0) &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->transfer_reserved == UINT8_C(0);
}

static bool
ti_c54x_op_memory_effect_access_shape_is_valid(
    const ti_c54x_op_memory_effect *effect)
{
    if (!ti_c54x_op_memory_rmw_operation_is_valid(effect->rmw_operation)) {
        return false;
    }
    if (effect->transfer_reserved != UINT8_C(0)) {
        return false;
    }
    if (effect->load_t_with_read &&
        (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
         effect->rmw_operation !=
             (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE)) {
        return false;
    }
    if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        if (effect->rmw_operation ==
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE) {
            return effect->value_word == UINT16_C(0) &&
                   effect->complete != NULL;
        }
        return effect->complete == NULL;
    }
    /* A WRITE with no RMW transform carries the staged value word
     * directly; a non-null completion additionally stages at most
     * one ordinary follow-on effect after the write resolves,
     * receiving the staged value word (CMPS TRN/TC precedent). */
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
}

static bool
ti_c54x_op_memory_data_program_address_is_valid(
    const ti_c54x_op_memory_address_spec *address_spec)
{
    if (address_spec->pair.indirect > UINT8_C(1) ||
        address_spec->pair.smem_arf > UINT8_C(7) ||
        address_spec->pair.smem_dma7 > UINT8_C(0x7f) ||
        address_spec->pair.reserved_half != UINT16_C(0) ||
        address_spec->pair.reserved_word0 != UINT32_C(0) ||
        address_spec->pair.reserved_word1 != UINT32_C(0) ||
        address_spec->pair.reserved_word2 != UINT32_C(0)) {
        return false;
    }
    if (address_spec->pair.indirect == UINT8_C(0)) {
        /* Direct leg: no AR selector and no indirect mode. */
        return address_spec->pair.smem_mode == UINT8_C(0) &&
               address_spec->pair.smem_arf == UINT8_C(0);
    }
    /* Indirect leg: a MOD0/MOD1/MOD2 AR selector with no direct
     * address. MOD3 reads stay carrier-invalid by the single-leg
     * write-only rule enforced for SINGLE_INDIRECT above. */
    return address_spec->pair.smem_mode <= UINT8_C(2) &&
           address_spec->pair.smem_dma7 == UINT8_C(0);
}

static bool
ti_c54x_op_memory_effect_x_only_shape_is_valid(
    const ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_op_memory_address_spec *spec = &effect->address_spec;

    /* The X side carries an AR2..AR5 selector with any dual mode; Y
     * self-aliases X and the reserved words stay zero. The transfer is
     * an ordinary single WORD16 read with a value-owned completion. */
    if (spec->dual.x_mode > UINT8_C(3) || spec->dual.x_ar < UINT8_C(2) ||
        spec->dual.x_ar > UINT8_C(5) ||
        spec->dual.y_mode != spec->dual.x_mode ||
        spec->dual.y_ar != spec->dual.x_ar ||
        spec->dual.reserved_word0 != UINT32_C(0) ||
        spec->dual.reserved_word1 != UINT32_C(0) ||
        spec->dual.reserved_word2 != UINT32_C(0) ||
        spec->dual.reserved_word3 != UINT32_C(0)) {
        return false;
    }
    return ti_c54x_op_memory_effect_access_shape_is_valid(effect);
}

bool
ti_c54x_op_memory_effect_is_valid(const ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL) {
        return false;
    }
    /* The Smem delay write-back rides the DATA+PROGRAM pair
     * (MACD) and the single-word DIRECT/SINGLE_INDIRECT DATA reads
     * (LTD/DELAY); all other carriers leave it false. */
    if (effect->write_smem_delay &&
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 &&
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16 &&
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16) {
        return false;
    }

    switch (effect->address_spec.kind) {
    case TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16:
        if (effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.mode < TI_C54X_INDIRECT_SINGLE_MOD0 ||
            effect->address_spec.mode > TI_C54X_INDIRECT_SINGLE_MOD3 ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
            effect->address_spec.mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
            return false;
        }
        if (effect->write_smem_delay) {
            return ti_c54x_op_memory_effect_single_delay_shape_is_valid(
                effect);
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.indexed_mode <
                TI_C54X_INDIRECT_INDEXED_MOD5 ||
            effect->address_spec.indexed_mode >
                TI_C54X_INDIRECT_INDEXED_MOD6 ||
            effect->address_spec.arf == UINT8_C(0) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            (effect->address_spec.circular_mode !=
                 TI_C54X_INDIRECT_CIRCULAR_MOD8 &&
             effect->address_spec.circular_mode !=
                 TI_C54X_INDIRECT_CIRCULAR_MOD10) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            (effect->address_spec.reverse_carry_mode !=
                 TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 &&
             effect->address_spec.reverse_carry_mode !=
                 TI_C54X_INDIRECT_REVERSE_CARRY_MOD7) ||
            effect->address_spec.arf == UINT8_C(0) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            return effect->first_value_msw == UINT16_C(0) &&
                   effect->second_value_lsw == UINT16_C(0) &&
                   effect->complete_long32 != NULL;
        }
        return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
               effect->complete_long32 == NULL;

    case TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE:
        return ti_c54x_op_memory_effect_long32_single_shape_is_valid(
            effect);

    case TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16:
        return ti_c54x_op_memory_effect_absolute_data_shape_is_valid(
            effect);

    case TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16:
        return ti_c54x_op_memory_dual_address_is_valid(
                   &effect->address_spec) &&
               ti_c54x_op_memory_effect_dual_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY:
        return ti_c54x_op_memory_effect_x_only_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16:
    case TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16:
        /* All-zero spec with no AR selector; WRITE-only with no RMW and no
         * completion. The staged value rides the TOS WRITE verbatim.
         * The delayed kind shares this shape exactly: only the root PC
         * commit timing differs. */
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf != UINT8_C(0)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect) &&
               effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;

    case TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16:
    case TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16:
        /* Accumulator-targeted push: all first-arm fields zero except
         * `arf`, which carries the accumulator select (0 for A, 1 for
         * B); WRITE-only with no RMW and no completion, sharing the
         * access shape of the immediate pushes. The delayed kind
         * shares this shape exactly: only the root PC commit timing
         * differs. */
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.long_offset_word != UINT16_C(0) ||
            effect->address_spec.direct_dma7 != UINT8_C(0) ||
            effect->address_spec.arf > UINT8_C(1)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect) &&
               effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;

    case TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16:
    case TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16:
        /* All-zero spec with no AR selector; READ-only with a zero staged
         * value and no RMW. Pops consume the READ observation in the pop
         * commit, so a null completion is exact; a non-null completion
         * additionally stages one follow-on effect the root commits
         * before the pop architecture commit (the pop-completion seam).
         * The delayed kind shares this shape exactly. */
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf != UINT8_C(0)) {
            return false;
        }
        return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
               effect->value_word == UINT16_C(0) &&
               effect->rmw_operation ==
                   (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
               effect->transfer_reserved == UINT8_C(0);

    case TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16:
        /* Same all-zero spec and READ-only/no-RMW shape as the immediate
         * pops, but the staged value is the frozen RTN return address,
         * not the zero of a handler-staged effect: prepare overwrites
         * the handler's zero with the live RTN value before the
         * candidate shape check, and the resume liveness guard pins
         * the frozen value against the live register. The commit
         * consumes the frozen value with no bus observation. */
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf != UINT8_C(0)) {
            return false;
        }
        return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
               effect->rmw_operation ==
                   (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
               effect->transfer_reserved == UINT8_C(0);

    case TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16:
        /* DP-independent page-0 direct: all mode selectors zero,
         * no AR selector, no long offset; the 7-bit address rides
         * `direct_dma7`. READ and WRITE share the access shape;
         * RMW staging follows the generic access-shape rule. */
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf != UINT8_C(0) ||
            effect->address_spec.long_offset_word != UINT16_C(0) ||
            effect->address_spec.direct_dma7 > UINT8_C(0x7f)) {
            return false;
        }
        if (effect->write_smem_delay) {
            return ti_c54x_op_memory_effect_single_delay_shape_is_valid(
                effect);
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16:
        /* DP-relative direct: all mode selectors zero, no AR selector;
         * the 9-bit DP page rides `long_offset_word` with the 7-bit
         * offset in `direct_dma7` (per-kind reinterpretation; the
         * DIRECT arm above still requires the long word to be zero).
         * READ and WRITE share the access shape. */
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf != UINT8_C(0) ||
            effect->address_spec.long_offset_word > UINT16_C(0x1FF) ||
            effect->address_spec.direct_dma7 > UINT8_C(0x7F)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16:
        return ti_c54x_op_memory_data_program_address_is_valid(
                   &effect->address_spec) &&
               ti_c54x_op_memory_effect_dual_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16:
        /* Port I/O pair: the DATA-leg direction rides `access` with
         * the IO leg always inverse; the staged value is zero in both
         * directions (the second-leg value is a bus observation, and
         * the reducer replaces the WRITE template before issue). No
         * RMW and no leaf completion: the reducer owns the chaining
         * internally. Arm shape mirrors the IO-leg planner. */
        if (effect->address_spec.port.indirect > UINT8_C(1)) {
            return false;
        }
        if (effect->address_spec.port.indirect == UINT8_C(0)) {
            if (effect->address_spec.port.smem_mode != UINT8_C(0) ||
                effect->address_spec.port.smem_arf != UINT8_C(0) ||
                effect->address_spec.port.smem_dma7 > UINT8_C(0x7F)) {
                return false;
            }
        } else {
            if (effect->address_spec.port.smem_dma7 != UINT8_C(0) ||
                effect->address_spec.port.smem_arf > UINT8_C(7) ||
                effect->address_spec.port.smem_mode > UINT8_C(10)) {
                return false;
            }
        }
        if (effect->address_spec.port.reserved_half != UINT16_C(0) ||
            effect->address_spec.port.reserved_word0 != UINT32_C(0) ||
            effect->address_spec.port.reserved_word1 != UINT32_C(0) ||
            effect->address_spec.port.reserved_word2 != UINT32_C(0)) {
            return false;
        }
        return (effect->access ==
                    TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
                effect->access ==
                    TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) &&
               effect->value_word == UINT16_C(0) &&
               effect->rmw_operation ==
                   (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
               effect->complete == NULL &&
               effect->transfer_reserved == UINT8_C(0);

    case TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16:
    case TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16:
        /* Stack push/pop pair: the Smem DATA leg is read or written
         * with the TOS stack leg on the other side; the staged
         * value is zero in both directions (the second-leg value
         * is a bus observation, and the reducer replaces the WRITE
         * template before issue). No RMW and no leaf completion:
         * the reducer owns the chaining internally. Direct legs
         * carry the page-0 dma; indirect legs carry the MOD/ARF
         * selector with no direct address (DATA_PROGRAM pair
         * precedent). The DATA-leg direction gates the mode: the
         * PSHD DATA leg reads (MOD0--MOD2, MOD3 is write-only),
         * the POPD DATA leg writes (MOD0--MOD3). */
        if (effect->address_spec.stack_pair.indirect > UINT8_C(1) ||
            effect->address_spec.stack_pair.reserved_half !=
                UINT16_C(0) ||
            effect->address_spec.stack_pair.reserved_word0 !=
                UINT32_C(0) ||
            effect->address_spec.stack_pair.reserved_word1 !=
                UINT32_C(0) ||
            effect->address_spec.stack_pair.reserved_word2 !=
                UINT32_C(0)) {
            return false;
        }
        if (effect->address_spec.stack_pair.indirect == UINT8_C(0)) {
            if (effect->address_spec.stack_pair.smem_mode !=
                    UINT8_C(0) ||
                effect->address_spec.stack_pair.smem_arf !=
                    UINT8_C(0) ||
                effect->address_spec.stack_pair.smem_dma7 >
                    UINT8_C(0x7F)) {
                return false;
            }
        } else {
            const uint8_t mode_limit =
                (effect->address_spec.kind ==
                         TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16)
                    ? UINT8_C(2)
                    : UINT8_C(3);
            if (effect->address_spec.stack_pair.smem_dma7 !=
                    UINT8_C(0) ||
                effect->address_spec.stack_pair.smem_arf >
                    UINT8_C(7) ||
                effect->address_spec.stack_pair.smem_mode >
                    mode_limit) {
                return false;
            }
        }
        if (effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16) {
            if (effect->access !=
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
                return false;
            }
        } else if (effect->access !=
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            return false;
        }
        return effect->value_word == UINT16_C(0) &&
               effect->rmw_operation ==
                   (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
               effect->complete == NULL &&
               effect->transfer_reserved == UINT8_C(0);

    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16:
        return ti_c54x_op_memory_effect_smem_absolute_pair_shape_is_valid(
            effect);
    case TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16:
        return ti_c54x_op_memory_effect_smem_program_pair_shape_is_valid(
            effect);

    case TI_C54X_OP_MEMORY_ADDRESS_INVALID:
    default:
        return false;
    }
}
