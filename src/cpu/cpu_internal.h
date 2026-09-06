#ifndef TI_C54X_INTERNAL_CPU_H
#define TI_C54X_INTERNAL_CPU_H

/* Module contract
 * Owns: the private representation used by src/cpu/cpu.c.
 * Does not own: the storage containing this object, callback contexts, or bus
 * memory referenced by the copied public descriptors.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/cpu.h>

#include "../debug/observer.h"
#include "../state/status/registers.h"
#include "../state/accumulator/accumulator.h"
#include "../state/temporary/t_register.h"

struct ti_c54x_cpu {
    ti_c54x_program_word_address pc_word;
    uint16_t last_fetched_word;
    bool last_fetch_valid;
    uint64_t instructions_attempted;
    uint64_t instructions_retired;
    ti_c54x_program_word_address reset_pc_word;
    ti_c54x_status_register_state status;
    ti_c54x_accumulator_state accumulators;
    ti_c54x_t_register_state t_register;
    ti_c54x_bus bus;
    ti_c54x_observer_state observer;
    bool execution_active;
};

#endif
