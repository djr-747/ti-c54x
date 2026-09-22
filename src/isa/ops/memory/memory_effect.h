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
 * stack planning; only the root PC commit timing differs), the
 * accumulator-targeted STACK_PUSH_ACCUMULATOR WORD16 kinds with
 * their delayed counterpart (identical TOS planning; the PC target
 * rides the frozen input snapshot instead of the bundle), the
 * far-call STACK_PUSH_FAR WORD16 double-push kinds, the single
 * ownership point for stack-push kind families, and static
 * completion callback shapes.
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
    TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 = 14,
    /* Accumulator-targeted stack push (CALA): identical TOS planning
     * to the immediate push (old SP - 1 with a PRE_ACCESS SP update,
     * WRITE-only with the staged return address), but the PC target
     * is the low 16 bits of the selected accumulator, not the
     * bundle extension word. The accumulator select rides `arf`
     * (0 for A, 1 for B); all other first-arm fields stay zero.
     * The root reads the target from the frozen input snapshot
     * gated on accumulator knownness, so no frozen field grows. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 = 15,
    /* Delayed accumulator-targeted stack push (CALAD): identical TOS
     * planning to the accumulator push, but the root commits the
     * sequential PC successor and reports the accumulator target as
     * the deferred branch target. The staged return address is PC+3
     * for the one-word form per SPRU172C section 4 CALA[D]. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16 = 16,
    /* DP-relative direct WORD16: the effective DATA address is
     * (DP << 7) | dma with the frozen 9-bit DP page in
     * `long_offset_word` and the 7-bit direct offset in `direct_dma7`.
     * The shared fields are reinterpreted per kind (no arm growth: the
     * 24-byte address ABI is pinned); all other first-arm fields stay
     * zero. No AR, CMPT, CPL, or post-update; DP unknownness is gated
     * at the leaf through STATUS_DP. */
    TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16 = 17,
    /* Port I/O WORD16: the 16-bit absolute I/O port address rides
     * `port_address` with the DATA-leg Smem selectors below. The
     * effect `access` denotes the DATA-leg direction and the IO leg
     * is always inverse (DATA WRITE plans an IO READ for PORTR;
     * DATA READ plans an IO WRITE template for PORTW whose zero
     * value the reducer replaces with the observed DATA word before
     * issue). No AR, CMPT, CPL, or DP dependence and no post-update;
     * Smem unknownness is gated at the leaf. */
    TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16 = 18,
    /* Stack-push pair WORD16 (PSHD): the Smem DATA leg is read
     * first and the observed word is written to the TOS DATA
     * WORD16 at old SP - 1 with a PRE_ACCESS SP update. The
     * effect `access` is WRITE (the stack-leg direction). Direct
     * legs carry no AR, CMPT, CPL, or DP dependence; indirect
     * legs read AR with CMPT=0/ARP=0. No post-update; Smem and
     * SP unknownness are gated at the leaf and root. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 = 19,
    /* Stack-pop pair WORD16 (POPD): the TOS DATA WORD16 at old
     * SP is read first with a POST_ACCESS SP update and the
     * observed word is written to the Smem DATA leg. The effect
     * `access` is READ (the stack-leg direction). Direct legs
     * carry no AR, CMPT, CPL, or DP dependence; indirect legs
     * read AR with CMPT=0/ARP=0. No post-update; Smem and SP
     * unknownness are gated at the leaf and root. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16 = 20,
    /* Single-word Lmem LONG32 (double-precision family): the 32-bit
     * operand is addressed by one I + 7-bit cookie word. The direct
     * leg reads MSW/LSW at the page-0 dma7 address (the DP slot
     * stays reserved zero); the indirect leg reads at the
     * standard-indirect MOD0--MOD3 AR address with the landed
     * two-word AR deltas (MOD3 stays write-only). No post-update
     * beyond the indirect AR delta; DP/AR unknownness is gated at
     * the leaf and root. */
    TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE = 21,
    /* Absolute DATA WORD16 (MMR moves): the 16-bit absolute DATA
     * address rides `address` (the second program word). No AR,
     * CMPT, CPL, or DP dependence and no post-update; Smem
     * unknownness is gated at the leaf and root. */
    TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16 = 22,
    /* Smem-to-absolute DATA pair WORD16 (MVDK): the Smem DATA leg
     * is read first and the observed word is written to the
     * absolute DATA address riding `dmad` (the second program
     * word). The effect `access` is WRITE (the completing-leg
     * direction, PSHD convention). Direct legs carry the
     * DP-relative offset with no AR dependence; indirect legs
     * read AR with CMPT=0/ARP=0 and admit MOD0--MOD3 (the
     * resolver governs MOD validity). No post-update beyond the
     * indirect AR delta; DP/AR unknownness is gated at the leaf
     * and root. MVKD reuses this carrier with access READ. */
    TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16 = 23,
    /* Smem-to-program DATA pair WORD16 (MVPD/MVDP): the Smem
     * DATA leg and the absolute PROGRAM leg move one word with
     * the PROGRAM leg inverse to the effect `access` (PORT-style:
     * the access names the Smem-leg direction, so MVPD writes
     * Smem while MVDP reads Smem). Direct legs carry the
     * DP-relative offset with no AR dependence; indirect legs
     * read AR with CMPT=0/ARP=0 and admit MOD0--MOD3 (the
     * resolver governs MOD validity). No post-update beyond the
     * indirect AR delta; DP/AR unknownness is gated at the leaf
     * and root. */
    TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16 = 24,
    /* Smem-to-accumulator-addressed-program DATA pair WORD16
     * (READA/WRITA): the Smem DATA leg and the PROGRAM leg move
     * one word with the PROGRAM leg inverse to the effect
     * `access` (PORT-style). The PROGRAM address resolves from
     * accumulator A (PAR <- A, low 16 bits) at prepare time;
     * the arm stages the Smem selectors only. Direct and
     * indirect legs mirror the immediate-pmad pair;
     * DP/AR/A unknownness is gated at the leaf and root. */
    TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16 = 25,
    /* Smem-absolute-to-program DATA pair WORD16 (MVDP with an
     * absolute Smem): the 16-bit absolute DATA leg and the
     * absolute 16-bit PROGRAM leg move one word with the
     * PROGRAM leg inverse to the effect `access` (MVDP reads
     * Smem). The staged value stays zero (the second-leg
     * value is a bus observation the reducer replaces before
     * issue). No RMW, no T load, no delay, and no leaf
     * completion: the pair root owns the chaining
     * internally. */
    TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16 = 26,
    /* Absolute-Smem LONG32 store WORD32 (DST with an absolute
     * Smem): the 32-bit accumulator value stores as MSW/LSW
     * at the 16-bit absolute DATA address and the word
     * above it. The halves ride `first_value_msw` and
     * `second_value_lsw` (MOD13 LONG32 convention) with
     * WRITE access and no leaf completion: the long32 root
     * owns the chaining internally. */
    TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16 = 27,
    /* Atomic absolute WORD16 (ADDM with an absolute Smem):
     * the 16-bit absolute DATA address with a staged
     * immediate and an RMW transform (ADD with OVM/SXM
     * modes for ADDM). The read leg observes the cell and
     * the engine writes the transform back with no leaf
     * completion. */
    TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16 = 28,
    /* Absolute-Smem LONG32 load WORD32 (DLD with an absolute
     * Smem): the 32-bit long word reads as MSW/LSW from
     * the 16-bit absolute DATA address and the word above
     * it. The staged halves stay zero (MOD13 LONG32 READ
     * convention) with READ access and a non-null leaf
     * long32 completion: the long32 root owns the chaining
     * internally. */
    TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_LOAD_WORD16 = 29,
    /* Interrupt-take stack push (NMI): identical TOS planning to
     * the immediate push (old SP - 1 with a PRE_ACCESS SP update,
     * WRITE-only with the staged return address), but the staged
     * address is the parked live PC (not a sequential successor),
     * the PC target is the frozen device-owned vector (not a
     * bundle word), no RTN load occurs (takes are not calls), and
     * a non-null completion stages the INTM set. Only the take
     * root producer freezes this kind; no catalog row uses it. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16 = 30,
    /* Dual-indirect move pair WORD16 (MVDD): the shared `dual`
     * arm carries the X/Y MOD0--MOD3 modes and AR2..AR5
     * selectors. The X leg reads, the Y leg writes the observed
     * X word, and both post-access AR updates commit; the effect
     * `access` is WRITE (the completing-leg direction) with a
     * null leaf completion. The move root owns the chaining
     * internally. */
    TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16 = 31,
    /* Trap software-interrupt stack push: identical TOS planning
     * to the immediate push (old SP - 1 with a PRE_ACCESS SP
     * update, WRITE-only with the staged return address), but the
     * staged address is the sequential successor PC + 1 (not the
     * parked live PC of a take), the PC target is the frozen
     * K vector (IPTR-derived, not a bundle word), no RTN load
     * occurs (traps are not calls), and the leaf completion is
     * null (no INTM/IFR mutation). Only the op_trap leaf stages
     * this kind; the root freezes the successor and vector. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16 = 32,
    /* Dual-data-plus-program READ triple WORD16 (FIRS): the
     * dual DATA legs carry the X/Y MOD0--MOD3 modes and
     * AR2..AR5 selectors exactly as the dual arm, and the
     * PROGRAM leg is an absolute 16-bit program-word
     * coefficient read with no post-update. All three values
     * publish through the `complete_triple` shape with the X
     * word, the Y word, and the coefficient. No T load. */
    TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16 = 33,
    /* Intr software-interrupt stack push: identical TOS planning
     * and K-vector freeze to the trap push, but the leaf
     * completion is the INTM-setting take completion (software
     * interrupts globally disable maskable interrupts) and the
     * CPU retirement tail additionally clears the K-indexed IFR
     * bit. Only the op_intr leaf stages this kind. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16 = 34,
    /* Far-call stack double-push (FCALL): identical first-TOS
     * planning to the immediate push (old SP - 1 with a
     * PRE_ACCESS SP update, WRITE-only with the staged return
     * address PC+2), but the root issues a second WRITE carrying
     * the frozen live XPC to old SP - 2 before publication, so
     * the published SP update is old SP - 2. The PC target is the
     * frozen bundle pmad with the staged page committed to XPC;
     * the frozen staged return address additionally loads RTN
     * atomically with the push (calls load RTN with the address
     * they push). Only the op_fcall_pmad leaf stages this kind. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_WORD16 = 35,
    /* Delayed far-call stack double-push (FCALLD): identical
     * double-write planning to the far push with the staged
     * return address PC+4 per SPRU172C section 4 FCALL[D], but
     * the root commits the sequential PC successor and reports
     * the frozen pmad as the deferred branch target. Only the
     * op_fcalld leaf stages this kind. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_DELAYED_WORD16 = 36,
    /* Far-accumulator-call stack double-push (FCALA): identical
     * double-write planning to the far push with the staged
     * return address PC+1, but the PC target is the low 16 bits
     * of the selected accumulator (not the bundle extension
     * word) with the staged page from accumulator bits 22-16.
     * The accumulator select rides `arf` (0 for A, 1 for B); all
     * other first-arm fields stay zero. The root reads the
     * target and page from the frozen input snapshot gated on
     * accumulator knownness, so no frozen field grows. Only the
     * op_fcala leaf stages this kind. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_WORD16 = 37,
    /* Delayed far-accumulator-call stack double-push (FCALAD):
     * identical double-write planning to the far accumulator
     * push with the staged return address PC+3 per SPRU172C
     * section 4 FCALA[D], but the root commits the sequential PC
     * successor and reports the accumulator target as the
     * deferred branch target. Only the op_fcalad leaf stages
     * this kind. */
    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_DELAYED_WORD16 = 38
} ti_c54x_op_memory_address_kind;

/* Single ownership point for stack-push kind families: every
 * carrier, planner, validator, and root arm classifies through
 * these instead of hand-rolled enumerator chains, so a new push
 * kind joins each family in exactly one place. The take kind is
 * excluded: takes route through pair dispatch, never through the
 * push-prepare family below. */
static inline bool
ti_c54x_op_memory_address_kind_is_stack_push(
    ti_c54x_op_memory_address_kind kind)
{
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_DELAYED_WORD16;
}

/* Bundle push family (immediate, delayed, take, trap, intr,
 * and far): the first TOS lives at old SP - 1 with all-zero
 * first-arm fields (`arf` stays zero). The take kind shares this
 * planning shape but routes through pair dispatch, never through
 * push-prepare. */
static inline bool
ti_c54x_op_memory_address_kind_is_bundle_push(
    ti_c54x_op_memory_address_kind kind)
{
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_DELAYED_WORD16;
}

/* Accumulator push family (immediate, delayed, and far): the
 * first TOS lives at old SP - 1 with the accumulator select
 * riding `arf` (0 for A, 1 for B). */
static inline bool
ti_c54x_op_memory_address_kind_is_accumulator_push(
    ti_c54x_op_memory_address_kind kind)
{
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_DELAYED_WORD16;
}

/* Far-call double-push family (FCALL[D]/FCALA[D]): the root
 * issues a second WRITE carrying the frozen live XPC to old
 * SP - 2 and publishes old SP - 2. */
static inline bool
ti_c54x_op_memory_address_kind_is_far_push(
    ti_c54x_op_memory_address_kind kind)
{
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_DELAYED_WORD16;
}

/* Delayed-branch push family: the root commits the sequential
 * PC successor and reports the frozen target as the deferred
 * branch. Covers the bundle, accumulator, and far delayed
 * kinds. */
static inline bool
ti_c54x_op_memory_address_kind_is_delayed_push(
    ti_c54x_op_memory_address_kind kind)
{
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_DELAYED_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_DELAYED_WORD16;
}

/* Read-phase MMR sidecar direction for the absolute DATA arm.
 * The direction follows the leg: STORE_WITH_READ publishes the
 * DATA read word to the MMR destination (MVDM); LOAD_WITH_WRITE
 * snapshots the MMR source into the DATA write word (MVMD).
 * NONE carries no MMR leg. Values are dense from zero; unknown
 * enumerators never validate. */
typedef enum ti_c54x_op_memory_mmr_sidecar_kind {
    TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE = 0,
    TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ = 1,
    TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE = 2
} ti_c54x_op_memory_mmr_sidecar_kind;

/*
 * A WORD16 memory read may optionally feed one value-owned transform before
 * its paired write.  The carrier stores the operation in one byte of the
 * existing value/callback alignment padding; no callback or context is
 * retained here.
 *
 * OVERWRITE ignores the read word and writes value_word; WRITEBACK
 * ignores value_word and rewrites the read word.  Neither stages flags.
 *
 * ADD stages value_word as the 16-bit addend.  The OVM/SXM modes live in
 * the frozen input snapshot the sequencer already copies, so they travel
 * outside this carrier; the sequencer stages the resulting carry and
 * overflow beside the transformed write word and commits C/OVA from
 * those staged flags.
 */
typedef enum ti_c54x_op_memory_rmw_operation {
    TI_C54X_OP_MEMORY_RMW_OPERATION_NONE = 0,
    TI_C54X_OP_MEMORY_RMW_OPERATION_AND = 1,
    TI_C54X_OP_MEMORY_RMW_OPERATION_OR = 2,
    TI_C54X_OP_MEMORY_RMW_OPERATION_XOR = 3,
    TI_C54X_OP_MEMORY_RMW_OPERATION_ADD = 4,
    TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE = 5,
    TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK = 6
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
            /* Used only by LONG_OFFSET_MOD13_LONG32, copied verbatim as
             * `lk`, and by DIRECT_DP_WORD16, carrying the frozen 9-bit
             * DP page (0..511). Must remain zero for other kinds. */
            uint16_t long_offset_word;
            uint8_t arf;
            /* Used only by DIRECT_WORD16, carrying the 7-bit direct DATA
             * address, and by DIRECT_DP_WORD16, carrying the 7-bit
             * direct offset. Must remain zero for other kinds. Appended
             * last so existing field offsets and the ABI sizes are
             * unchanged. */
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
            /* Dual-data-plus-program triple: the dual DATA legs
             * carry the X/Y modes and AR selectors exactly as
             * the dual arm; the PROGRAM leg carries the absolute
             * 16-bit program-word coefficient address (the second
             * program word) with no post-update. Same footprint
             * as the dual arm: the address ABI is unchanged. */
            uint8_t x_mode;
            uint8_t x_ar;
            uint8_t y_mode;
            uint8_t y_ar;
            uint16_t program_pmad;
            uint16_t reserved_half;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
            uint32_t reserved_word3;
        } dual_program;
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
        struct {
            /* PORT pair: the Smem DATA leg selector. Zero selects
             * the direct-DP leg (`smem_dma7`); one selects the
             * standard-indirect leg (`smem_mode` with `smem_arf`). */
            uint8_t indirect;
            /* Indirect-leg mode MOD0 through MOD10. Must remain zero
             * for the direct leg. */
            uint8_t smem_mode;
            /* Indirect-leg AR selector. Must remain zero for the
             * direct leg. */
            uint8_t smem_arf;
            /* Direct-leg 7-bit DATA offset. Must remain zero for
             * the indirect leg. */
            uint8_t smem_dma7;
            /* Absolute 16-bit I/O port address (0-65535). */
            uint16_t port_address;
            /* Reserved words keep the arm fixed-size and make the
             * checked hot-path validation scalar comparisons. */
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } port;
        struct {
            /* Stack-push/pop pair: the Smem DATA leg selector. Zero
             * selects the direct page-0 leg (`smem_dma7`); one
             * selects the standard-indirect leg (`smem_mode` with
             * `smem_arf`). */
            uint8_t indirect;
            /* Indirect-leg mode MOD0--MOD2 for DATA-leg reads
             * (MOD3 is write-only), MOD0--MOD3 for DATA-leg
             * writes. Must remain zero for the direct leg. */
            uint8_t smem_mode;
            /* Indirect-leg AR selector. Must remain zero for the
             * direct leg. */
            uint8_t smem_arf;
            /* Direct-leg 7-bit DATA address. Must remain zero for
             * the indirect leg. */
            uint8_t smem_dma7;
            /* Reserved words keep the arm fixed-size and make the
             * checked hot-path validation scalar comparisons. */
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } stack_pair;
        struct {
            /* Smem-to-absolute pair: the Smem DATA leg selector.
             * Zero selects the DP-relative direct leg
             * (`smem_dma7`); one selects the standard-indirect
             * leg (`smem_mode` with `smem_arf`). */
            uint8_t indirect;
            /* Indirect-leg mode MOD0--MOD3. Must remain zero for
             * the direct leg. */
            uint8_t smem_mode;
            /* Indirect-leg AR selector. Must remain zero for the
             * direct leg. */
            uint8_t smem_arf;
            /* Direct-leg 7-bit DATA offset. Must remain zero for
             * the indirect leg. */
            uint8_t smem_dma7;
            /* Absolute 16-bit DATA destination (the second
             * program word). */
            uint16_t dmad;
            /* Reserved words keep the arm fixed-size and make the
             * checked hot-path validation scalar comparisons. */
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } smem_absolute;
        struct {
            /* Smem-to-program pair: same Smem DATA leg shape as
             * `smem_absolute`; only the second word differs (the
             * absolute 16-bit PROGRAM address). */
            uint8_t indirect;
            uint8_t smem_mode;
            uint8_t smem_arf;
            uint8_t smem_dma7;
            uint16_t pmad;
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } smem_program;
        struct {
            /* Smem-absolute-to-program pair: the 16-bit
             * absolute DATA address and the absolute 16-bit
             * PROGRAM address. Same footprint as the pair arms:
             * the address ABI is unchanged. */
            uint16_t absolute;
            uint16_t pmad;
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } smem_absolute_program;
        struct {
            /* Absolute-Smem LONG32 store: the 16-bit absolute
             * DATA address (MSW lands here, LSW above). Same
             * footprint as the pair arms: the address ABI is
             * unchanged. */
            uint16_t absolute;
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } long32_absolute;
        struct {
            /* Atomic absolute WORD16: the 16-bit absolute
             * DATA address. Same footprint as the pair arms:
             * the address ABI is unchanged. */
            uint16_t absolute;
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } atomic_absolute;
        struct {
            /* Smem-to-accumulator-addressed-program pair: same
             * Smem DATA leg shape as `smem_program` but the
             * PROGRAM address resolves from accumulator A, so
             * the immediate-`pmad` slot stays reserved. */
            uint8_t indirect;
            uint8_t smem_mode;
            uint8_t smem_arf;
            uint8_t smem_dma7;
            uint16_t reserved_pmad;
            uint16_t reserved_half;
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } smem_program_a;
        struct {
            /* Absolute 16-bit DATA address (the second program
             * word). Same footprint as the pair arms: the address
             * ABI is unchanged. */
            uint16_t address;
            /* Read-phase MMR sidecar: direction plus the 5-bit
             * MMR number. Reuses the reserved half with no
             * footprint change; NONE requires a zero number. */
            uint8_t mmr_sidecar_kind;
            uint8_t mmr_sidecar_number;
            /* Reserved words keep the arm fixed-size and make the
             * checked hot-path validation scalar comparisons. */
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
            uint32_t reserved_word3;
        } absolute_data;
        struct {
            /* Single-word Lmem LONG32 leg selector. Zero selects
             * the direct-DP leg (`dma7` with `dp_page`); one
             * selects the standard-indirect leg (`mode` with
             * `arf`). Same footprint as the pair arms: the
             * address ABI is unchanged. */
            uint8_t indirect;
            /* Indirect-leg mode MOD0--MOD3. Must remain zero for
             * the direct leg. */
            uint8_t mode;
            /* Indirect-leg AR selector (AR0--AR7). Must remain
             * zero for the direct leg. */
            uint8_t arf;
            /* Direct-leg 7-bit page-0 DATA offset. Must remain
             * zero for the indirect leg. */
            uint8_t dma7;
            /* Reserved: single-word directs are page-0 (S7/STM
             * precedent), so this DP slot stays zero on both
             * legs. Must remain zero. */
            uint16_t dp_page;
            /* Reserved words keep the arm fixed-size and make the
             * checked hot-path validation scalar comparisons. */
            uint32_t reserved_word0;
            uint32_t reserved_word1;
            uint32_t reserved_word2;
        } long32_single;
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

typedef ti_c54x_op_status (*ti_c54x_op_memory_complete_triple_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t x_word,
    uint16_t y_word,
    uint16_t third_word,
    ti_c54x_op_effect *next_effect);

typedef struct ti_c54x_op_memory_effect {
    ti_c54x_op_memory_address_spec address_spec;
    ti_c54x_bus_word16_request_access access;
    /* Read-phase T load (MPYA): when true on a WORD16 READ, the root
     * loads T with the read value word after the read-value
     * completion commits. Only READ access with no RMW transform may
     * set this; all other carriers leave it false. */
    bool load_t_with_read;
    /* Smem delay write-back (MACD/LTD/DELAY): when true on a
     * DATA+PROGRAM READ pair, the pair root writes the Smem (x) word
     * to DATA Smem-address-plus-one (16-bit wraparound) through the
     * endpoint after both reads complete and before the post-access
     * commit. When true on a single-word DIRECT or SINGLE_INDIRECT
     * DATA READ, the WORD16 root writes the read word to DATA
     * Smem-address-plus-one after the read completes and before the
     * architecture commit. Only those three kinds may set this; all
     * other carriers leave it false. Shares the existing padding:
     * the 48-byte effect ABI is unchanged. */
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
        ti_c54x_op_memory_complete_triple_fn complete_triple;
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
