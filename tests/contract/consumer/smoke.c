/* Module contract
 * Owns: a downstream-package compile/link/lifetime smoke test.
 * Does not own: the library implementation, CPU storage, bus memory, or
 * callback state after the caller-owned objects leave main.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

static ti_c54x_bus_status
read_program(void *context,
             ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    uint16_t *word = (uint16_t *)context;

    (void)address_word;
    *value_word = *word;
    return TI_C54X_BUS_OK;
}

int
main(void)
{
    uint16_t program_word = UINT16_C(0xFFFF);
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {0};
    ti_c54x_state state = {0};
    ti_c54x_cpu *cpu = NULL;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_run_result result;

    if (storage == NULL) {
        return 1;
    }
    bus.context = &program_word;
    bus.program_read_word = read_program;
    if (ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus, &cpu) !=
        TI_C54X_ERROR_OK) {
        free(storage);
        return 2;
    }
    result = ti_c54x_cpu_step(cpu);
    if (result.reason != TI_C54X_STOP_UNIMPLEMENTED_OPCODE ||
        ti_c54x_cpu_inspect(cpu, &state) != TI_C54X_ERROR_OK) {
        ti_c54x_cpu_deinit(cpu);
        free(storage);
        return 3;
    }
    ti_c54x_cpu_deinit(cpu);
    free(storage);
    return 0;
}
