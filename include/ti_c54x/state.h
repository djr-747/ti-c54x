#ifndef TI_C54X_STATE_H
#define TI_C54X_STATE_H

/* Module contract
 * Owns: the read-only observation value type.
 * Does not own: CPU storage, architectural state, or the destination supplied
 * to the inspection API.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/address.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A read-only architectural observation.  It intentionally grows as the
 * instruction set is implemented; callers must not use it as a raw snapshot
 * or persistence ABI. */
typedef struct ti_c54x_state {
    ti_c54x_program_word_address pc_word;
    uint16_t last_fetched_word;
    bool last_fetch_valid;
    /* These counters accumulate over the live CPU instance and reset on
     * ti_c54x_cpu_reset. */
    uint64_t instructions_attempted_total;
    uint64_t instructions_retired_total;
    uint16_t dp;
    bool dp_known;
    uint16_t asm_raw;
    bool asm_known;
    uint8_t arp;
    bool arp_known;
    uint64_t accumulator_a_raw40;
    bool accumulator_a_known;
    uint64_t accumulator_b_raw40;
    bool accumulator_b_known;
    uint8_t c;
    bool c_known;
    uint16_t ifr_value;
    bool ifr_known;
    uint8_t intm_value;
    bool intm_known;
    /* Pending NMI take latched by the public NMI request with its
     * device-owned vector address. NMI has no IFR bit, so this
     * pipeline flag (not architectural state) is the only
     * visibility until the take retires. */
    bool nmi_pending;
    uint16_t nmi_vector_word;
} ti_c54x_state;

#ifdef __cplusplus
}
#endif

#endif
