/*
 * Module contract
 *
 * Owns: the CPU-owned, allocation-free extension-word fetch transaction and
 * its checked status mapping. Does not own: first-word fetches, CPU/state or
 * program-counter mutation, instruction decode, timing, memory, observers,
 * diagnostics, or opcode semantics.
 */

#include "instruction_fetch.h"

#include <stddef.h>

static ti_c54x_instruction_extension_fetch_result
ti_c54x_instruction_extension_fetch_failure(
    ti_c54x_instruction_extension_fetch_status status,
    ti_c54x_bus_status bus_status)
{
    ti_c54x_instruction_extension_fetch_result result = {0};

    result.status = status;
    result.bus_status = bus_status;
    return result;
}

ti_c54x_instruction_extension_fetch_result
ti_c54x_instruction_extension_fetch(const ti_c54x_bus *bus,
                                    uint32_t first_program_word_address,
                                    uint16_t first_program_word,
                                    uint8_t program_word_count)
{
    ti_c54x_instruction_extension_fetch_result result;
    uint16_t extension_word = UINT16_C(0);
    ti_c54x_bus_status bus_status;

    if (bus == NULL || program_word_count == UINT8_C(0) ||
        program_word_count > UINT8_C(2)) {
        return ti_c54x_instruction_extension_fetch_failure(
            TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_INVALID_ARGUMENT,
            TI_C54X_BUS_OK);
    }

    if (program_word_count == UINT8_C(2)) {
        if (first_program_word_address == UINT32_MAX) {
            return ti_c54x_instruction_extension_fetch_failure(
                TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_ADDRESS_OVERFLOW,
                TI_C54X_BUS_OK);
        }
        if (bus->program_read_word == NULL) {
            return ti_c54x_instruction_extension_fetch_failure(
                TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT,
                TI_C54X_BUS_NO_CALLBACK);
        }

        bus_status = bus->program_read_word(
            bus->context, first_program_word_address + UINT32_C(1),
            &extension_word);
        if (bus_status != TI_C54X_BUS_OK) {
            return ti_c54x_instruction_extension_fetch_failure(
                TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT,
                bus_status);
        }
    }

    result = (ti_c54x_instruction_extension_fetch_result){
        .status = TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK,
        .bus_status = TI_C54X_BUS_OK,
        .bundle = {
            .program_words = {first_program_word, extension_word},
            .program_word_count = program_word_count
        }
    };
    return result;
}
