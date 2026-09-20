#ifndef TI_C54X_INTERNAL_CPU_H
#define TI_C54X_INTERNAL_CPU_H

/* Module contract
 * Owns: the private representation used by src/cpu/cpu.c.  The embedded
 * arch_state is the sole live owner of the program counter and status
 * registers; reset_pc_word is only the copied configured reset carrier.  The
 * embedded root transaction is the sole owner of resumable data-memory work
 * and its transaction-ID sequence.  The embedded delay countdown is the
 * sole owner of a pending delayed-branch target; it is pipeline state,
 * never architectural state.
 * Does not own: the storage containing this object, callback contexts, or bus
 * memory referenced by the copied public descriptors.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/cpu.h>

#include "../debug/observer.h"
#include "../sequencer/delayed_branch/delayed_branch.h"
#include "../sequencer/instruction/memory_instruction_transaction.h"
#include "../sequencer/interrupt/repeat_pending_request.h"
#include "../sequencer/repeat/repeat_block.h"
#include "../sequencer/repeat/repeat_progress.h"
#include "../state/arch_state.h"
#include "../state/accumulator/accumulator.h"
#include "../state/temporary/t_register.h"

struct ti_c54x_cpu {
    ti_c54x_arch_state arch_state;
    uint16_t last_fetched_word;
    bool last_fetch_valid;
    uint64_t instructions_attempted;
    uint64_t instructions_retired;
    ti_c54x_program_word_address reset_pc_word;
    ti_c54x_accumulator_state accumulators;
    ti_c54x_t_register_state t_register;
    ti_c54x_bus bus;
    ti_c54x_memory_instruction_transaction instruction_transaction;
    ti_c54x_observer_state observer;
    /* Pending delayed-branch countdown: armed by a retiring delayed
     * stack branch (W67B wires the adopt/advance loop), cleared by
     * init and reset. Pipeline state, not architectural state. */
    ti_c54x_sequencer_delayed_branch_state delayed_branch;
    /* Single-repeat pipeline state: ACTIVE progress plus one latched
     * maskable request (S2 wires the start/predecode loop) plus the
     * latched body-PC word re-driven on HOLD (S3b wires the
     * retirement tails), cleared by init and reset. Pipeline state,
     * not architectural state. */
    ti_c54x_repeat_progress repeat_progress;
    ti_c54x_repeat_pending_request repeat_pending;
    ti_c54x_program_word_address repeat_body_pc_word;
    /* Block-repeat pipeline state: ACTIVE phase plus the last
     * fetch address and decoded width the retirement tails use
     * for REA fall-through detection (S79 wires the tails
     * through the shared body-retired call), cleared by init
     * and reset. Pipeline state, not architectural state. */
    ti_c54x_repeat_block_progress repeat_block_progress;
    ti_c54x_program_word_address last_fetch_pc_word;
    uint8_t last_fetch_word_count;
    bool execution_active;
    /* Pending NMI take: latched by the public NMI request with its
     * device-owned vector address (S94 wires the predecode
     * arbitration and take issue), cleared only by a RETIRED take
     * commit, init, and reset. Pipeline state, not architectural
     * state: NMI has no IFR bit. */
    bool nmi_pending;
    uint16_t nmi_vector_word;
    /* Maskable routing table: one caller-selected entry address
     * per source plus a routed mask (S95). The S93 vectorless
     * latch sets IFR bits; the take additionally needs the
     * routed entry. Reset clears the table; init zeroes it. */
    uint16_t interrupt_vectors[16];
    uint16_t interrupt_vectors_known;
    /* Frozen take source for the IFR acknowledge: set at take
     * issue alongside the root prepare, consumed when the take
     * retires (immediate or resumed path). Maskable takes clear
     * exactly this IFR bit; NMI takes clear the pending edge. */
    uint8_t take_source_bit;
    bool take_source_is_maskable;
};

#endif
