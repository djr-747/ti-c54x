/* Module contract
 * Owns: the minimal consumer's host bus, storage, and result presentation.
 * Does not own: core architectural state or instruction execution semantics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ti_c54x/cpu.h>

typedef struct example_memory {
    uint16_t program_word;
} example_memory;

static ti_c54x_bus_status
read_program_word(void *context,
                  ti_c54x_program_word_address address_word,
                  uint16_t *value_word)
{
    example_memory *memory = (example_memory *)context;

    if (address_word != 0U || value_word == NULL) {
        return TI_C54X_BUS_UNMAPPED;
    }
    *value_word = memory->program_word;
    return TI_C54X_BUS_OK;
}

int
main(void)
{
    example_memory memory = {0U};
    ti_c54x_bus bus = {
        .context = &memory,
        .program_read_word = read_program_word,
    };
    ti_c54x_config config = {
        .reset_pc_word = 0U,
        .trace = {0U, NULL, NULL},
    };
    ti_c54x_run_result result;
    void *storage;
    ti_c54x_cpu *cpu = NULL;

    storage = malloc(ti_c54x_cpu_size());
    if (storage == NULL) {
        return EXIT_FAILURE;
    }
    if (ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus, &cpu) !=
        TI_C54X_ERROR_OK) {
        free(storage);
        return EXIT_FAILURE;
    }

    result = ti_c54x_cpu_step(cpu);
    printf("stop reason: %d, fetched: %s\n",
           (int)result.reason,
           result.opcode_valid ? "yes" : "no");

    ti_c54x_cpu_deinit(cpu);
    free(storage);
    return EXIT_SUCCESS;
}
