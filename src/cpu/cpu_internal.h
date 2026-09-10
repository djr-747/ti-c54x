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
    bool execution_active;
};

#endif
