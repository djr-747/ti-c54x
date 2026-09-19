#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRANSACTION_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRANSACTION_H

/*
 * Module contract
 *
 * Owns: one persistent, allocation-free root transaction for a generic data
 * memory instruction, including copied decode/input/effect candidates, the
 * composed bus reducer, staged address/PC/counter values, and resumable
 * endpoint orchestration.
 * Does not own: CPU storage/lifecycle, opcode identity or dispatch, physical
 * memory, endpoint context, timing, observers, or diagnostics. Inputs are
 * borrowed for prepare/advance calls; the candidate owns copied values and
 * retains no caller pointer. Endpoint context remains host-owned.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "bus/transaction/word16_transaction.h"
#include "sequencer/instruction/data_program_word16_read_transaction.h"
#include "sequencer/instruction/dual_word16_read_transaction.h"
#include "sequencer/instruction/long32_pair_transaction.h"
#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "sequencer/memory/address_resolution.h"
#include "sequencer/memory/dual_address_resolution.h"
#include "sequencer/memory/port_access_plan.h"
#include "sequencer/instruction/memory_instruction_stack_pair_candidate_validate.h"
#include "sequencer/instruction/memory_instruction_smem_absolute_pair_candidate_validate.h"
#include "sequencer/instruction/memory_instruction_smem_program_pair_candidate_validate.h"
#include "sequencer/program_pc_bridge.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_memory_instruction_status {
    TI_C54X_MEMORY_INSTRUCTION_STATUS_OK = 0,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT = 1,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED = 2,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT = 3,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED = 4,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT = 5,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE = 6,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION = 7,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW = 8,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED = 9,
    TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED = 10
} ti_c54x_memory_instruction_status;

typedef enum ti_c54x_memory_instruction_phase {
    TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE = 0,
    TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED = 1,
    TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING = 2,
    TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE = 3,
    TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT = 4
} ti_c54x_memory_instruction_phase;

typedef enum ti_c54x_memory_instruction_child_kind {
    TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16 = 1,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32 = 2,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ = 3,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_DATA_PROGRAM_WORD16_READ = 4,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_PORT_WORD16_PAIR = 5,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_STACK_PAIR_WORD16 = 6,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_ABSOLUTE_PAIR_WORD16 = 7,
    TI_C54X_MEMORY_INSTRUCTION_CHILD_SMEM_PROGRAM_PAIR_WORD16 = 8
} ti_c54x_memory_instruction_child_kind;

typedef struct ti_c54x_memory_instruction_candidate {
    ti_c54x_decoded_instruction instruction;
    ti_c54x_op_input_snapshot input;
    ti_c54x_op_effect effect;
    union {
        ti_c54x_memory_address_resolution address_resolution;
        ti_c54x_dual_memory_address_snapshot dual_address_snapshot;
        ti_c54x_port_address_snapshot port_address_snapshot;
        ti_c54x_stack_pair_address_snapshot stack_pair_address_snapshot;
        ti_c54x_smem_absolute_pair_address_snapshot
            smem_absolute_address_snapshot;
        ti_c54x_smem_program_pair_address_snapshot
            smem_program_address_snapshot;
        ti_c54x_smem_program_pair_address_snapshot
            smem_program_a_address_snapshot;
    };
    ti_c54x_program_pc_bridge_effect pc_effect;
    uint64_t instructions_attempted_after;
    uint64_t instructions_retired_after;
} ti_c54x_memory_instruction_candidate;

/* The immediate-pmad and accumulator-addressed program pairs
 * share one snapshot payload type in sibling union members;
 * select the stored member by address kind. */
static inline const ti_c54x_smem_program_pair_address_snapshot *
ti_c54x_memory_instruction_smem_program_pair_snapshot_for_kind(
    const ti_c54x_memory_instruction_candidate *candidate)
{
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16) {
        return &candidate->smem_program_a_address_snapshot;
    }
    return &candidate->smem_program_address_snapshot;
}

static inline ti_c54x_smem_program_pair_address_snapshot *
ti_c54x_memory_instruction_smem_program_pair_mutable_snapshot_for_kind(
    ti_c54x_memory_instruction_candidate *candidate)
{
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16) {
        return &candidate->smem_program_a_address_snapshot;
    }
    return &candidate->smem_program_address_snapshot;
}

typedef struct ti_c54x_memory_instruction_result {
    ti_c54x_memory_instruction_status status;
    ti_c54x_bus_status bus_status;
    uint64_t transaction_id;
    uint64_t instructions_attempted;
    uint64_t instructions_retired;
    /* Delayed-branch report: armed only by a retiring delayed stack
     * push/pop, carrying the 16-bit deferred PC target (frozen pmad
     * for pushes, TOS READ observation for pops). The CPU loop arms
     * its two-word countdown from this report. */
    bool deferred_branch_armed;
    uint16_t deferred_branch_pc_word;
} ti_c54x_memory_instruction_result;

typedef struct ti_c54x_memory_instruction_transaction {
    ti_c54x_memory_instruction_phase phase;
    union {
        ti_c54x_bus_word16_transaction bus_transaction;
        ti_c54x_long32_pair_transaction long32_bus_transaction;
        ti_c54x_dual_word16_read_transaction dual_bus_transaction;
        ti_c54x_data_program_word16_read_transaction
            data_program_bus_transaction;
    };
    uint64_t last_transaction_id;
    /* The transformed WORD16 is staged between the READ and paired WRITE;
     * it is never recomputed while the WRITE is pending. */
    union {
        uint16_t rmw_write_value_word;
        /* Single-word Smem delay write-back: stages the READ
         * observation between READ READY and the delay WRITE
         * issue/commit. Shares storage with the RMW slot; the
         * carrier forbids RMW together with delay, so the two
         * stages never overlap. */
        uint16_t single_delay_read_value_word;
        /* LONG32 uses the otherwise RMW-only slot for the frozen MSW. */
        uint16_t long32_first_value_msw;
        /* Dual MOD0 uses it as an immutable X-address shadow. */
        uint16_t dual_x_address_word;
        /* The DATA+PROGRAM pair uses it as an immutable Smem DATA
         * address shadow. */
        uint16_t data_program_smem_address_word;
    };
    /* Immutable copy of the RMW operator, or packed dual X/Y AR selectors.
     * The copied candidate remains externally inspectable, so a pending
     * resume must reject edits that would otherwise remain structurally
     * valid. */
    uint8_t candidate_rmw_operation;
    /* Child discriminator occupies the existing scalar padding before the
     * candidate's alignment on LP64 and ILP32. */
    uint8_t child_kind;
    /* RMW-ADD staged flags: frozen beside rmw_write_value_word when the
     * ADD reducer runs at advance time; meaningful only while
     * candidate_rmw_operation holds ADD. They occupy the remaining
     * scalar padding, so the root size is unchanged on LP64/ILP32. */
    bool rmw_add_carry;
    bool rmw_add_overflow;
    union {
        uint16_t candidate_rmw_value_word;
        /* LONG32's second payload is frozen in the same scalar slot. */
        uint16_t long32_second_value_lsw;
        /* Dual MOD0 uses it as an immutable Y-address shadow. */
        uint16_t dual_y_address_word;
        /* The DATA+PROGRAM pair uses it as an immutable PROGRAM
         * address (pmad) shadow. */
        uint16_t data_program_pmad_word;
    };
    ti_c54x_memory_instruction_candidate candidate;
} ti_c54x_memory_instruction_transaction;

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint64_t instructions_attempted_before,
    uint64_t instructions_retired_before);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

/* Clear an idle or terminal root while preserving the ID sequence. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRANSACTION_H */
