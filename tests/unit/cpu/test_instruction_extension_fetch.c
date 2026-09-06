/*
 * Module contract
 *
 * Owns: project-contract proof for the CPU-owned transactional collection of
 * one supplied program word plus at most one extension word.
 * Does not own: first-word fetch orchestration, CPU state, program-counter
 * updates, instruction decoding, timing, observers, diagnostics, or opcode
 * semantics.
 */

#include "../../../src/cpu/instruction_fetch.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct extension_bus_fixture {
    ti_c54x_bus_status status;
    uint16_t value_word;
    uint32_t addresses[2];
    unsigned calls;
    bool scribble_on_failure;
} extension_bus_fixture;

static ti_c54x_bus_status
read_extension(void *context,
               ti_c54x_program_word_address address_word,
               uint16_t *value_word)
{
    extension_bus_fixture *fixture = (extension_bus_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->calls < 2U);
    fixture->addresses[fixture->calls] = address_word;
    fixture->calls++;
    *value_word = fixture->value_word;
    if (fixture->status != TI_C54X_BUS_OK && fixture->scribble_on_failure) {
        *value_word = UINT16_C(0xDEAD);
    }
    return fixture->status;
}

static void
check_zero_bundle(const ti_c54x_instruction_word_bundle *bundle)
{
    TI_C54X_TEST_CHECK(bundle->program_words[0] == UINT16_C(0));
    TI_C54X_TEST_CHECK(bundle->program_words[1] == UINT16_C(0));
    TI_C54X_TEST_CHECK(bundle->program_word_count == UINT8_C(0));
}

static void
test_count_one_publishes_supplied_word_without_callback(void)
{
    const ti_c54x_bus bus = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    ti_c54x_instruction_extension_fetch_result result;

    result = ti_c54x_instruction_extension_fetch(
        &bus, UINT32_C(0x10042), UINT16_C(0x1234), UINT8_C(1));
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(result.bundle.program_words[0] == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(result.bundle.program_words[1] == UINT16_C(0));
}

static void
test_count_two_reads_exact_low_and_high_successors(void)
{
    const uint32_t first_addresses[] = {
        UINT32_C(0x0042), UINT32_C(0x10042)};
    size_t index;

    for (index = 0U;
         index < sizeof(first_addresses) / sizeof(first_addresses[0]);
         ++index) {
        extension_bus_fixture fixture = {
            TI_C54X_BUS_OK, UINT16_C(0xBEEF), {0U, 0U}, 0U, false};
        const ti_c54x_bus bus = {
            &fixture, read_extension, NULL, NULL, NULL, NULL, NULL};
        ti_c54x_instruction_extension_fetch_result result;

        result = ti_c54x_instruction_extension_fetch(
            &bus, first_addresses[index], UINT16_C(0xCAFE), UINT8_C(2));
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.bundle.program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(result.bundle.program_words[0] == UINT16_C(0xCAFE));
        TI_C54X_TEST_CHECK(result.bundle.program_words[1] == UINT16_C(0xBEEF));
        TI_C54X_TEST_CHECK(fixture.calls == 1U);
        TI_C54X_TEST_CHECK(
            fixture.addresses[0] == first_addresses[index] + UINT32_C(1));
    }
}

static void
test_count_two_rejects_address_overflow_before_io(void)
{
    extension_bus_fixture fixture = {TI_C54X_BUS_OK, UINT16_C(0xBEEF),
                                      {0U, 0U}, 0U, false};
    const ti_c54x_bus bus = {&fixture, read_extension, NULL, NULL, NULL, NULL,
                             NULL};
    ti_c54x_instruction_extension_fetch_result result;

    result = ti_c54x_instruction_extension_fetch(
        &bus, UINT32_MAX, UINT16_C(0xCAFE), UINT8_C(2));
    TI_C54X_TEST_CHECK(
        result.status ==
        TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_ADDRESS_OVERFLOW);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    check_zero_bundle(&result.bundle);
    TI_C54X_TEST_CHECK(fixture.calls == 0U);
}

static void
test_invalid_counts_and_null_bus_do_not_io(void)
{
    extension_bus_fixture fixture = {TI_C54X_BUS_OK, UINT16_C(0xBEEF),
                                      {0U, 0U}, 0U, false};
    const ti_c54x_bus bus = {&fixture, read_extension, NULL, NULL, NULL, NULL,
                             NULL};
    const uint8_t invalid_counts[] = {0U, 3U, UINT8_MAX};
    size_t index;

    for (index = 0U;
         index < sizeof(invalid_counts) / sizeof(invalid_counts[0]);
         ++index) {
        ti_c54x_instruction_extension_fetch_result result =
            ti_c54x_instruction_extension_fetch(
                &bus, UINT32_C(0x10042), UINT16_C(0xCAFE),
                invalid_counts[index]);
        TI_C54X_TEST_CHECK(
            result.status ==
            TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        check_zero_bundle(&result.bundle);
    }
    TI_C54X_TEST_CHECK(fixture.calls == 0U);

    {
        ti_c54x_instruction_extension_fetch_result result =
            ti_c54x_instruction_extension_fetch(
                NULL, UINT32_C(0x10042), UINT16_C(0xCAFE), UINT8_C(1));
        TI_C54X_TEST_CHECK(
            result.status ==
            TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        check_zero_bundle(&result.bundle);
    }
}

static void
test_missing_callback_is_bus_fault_and_failure_is_zero(void)
{
    const ti_c54x_bus bus = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    ti_c54x_instruction_extension_fetch_result result =
        ti_c54x_instruction_extension_fetch(
            &bus, UINT32_C(0x10042), UINT16_C(0xCAFE), UINT8_C(2));

    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
    check_zero_bundle(&result.bundle);
}

static void
test_callback_statuses_are_preserved_without_retry_or_partial_publish(void)
{
    const ti_c54x_bus_status statuses[] = {TI_C54X_BUS_UNMAPPED,
                                            TI_C54X_BUS_FAULT,
                                            TI_C54X_BUS_NO_CALLBACK};
    size_t index;

    for (index = 0U; index < sizeof(statuses) / sizeof(statuses[0]); ++index) {
        extension_bus_fixture fixture = {statuses[index], UINT16_C(0xBEEF),
                                          {0U, 0U}, 0U, true};
        const ti_c54x_bus bus = {&fixture, read_extension, NULL, NULL, NULL,
                                 NULL, NULL};
        ti_c54x_instruction_extension_fetch_result result =
            ti_c54x_instruction_extension_fetch(
                &bus, UINT32_C(0x10042), UINT16_C(0xCAFE), UINT8_C(2));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == statuses[index]);
        check_zero_bundle(&result.bundle);
        TI_C54X_TEST_CHECK(fixture.calls == 1U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0x10043));
    }
}

int
main(void)
{
    test_count_one_publishes_supplied_word_without_callback();
    test_count_two_reads_exact_low_and_high_successors();
    test_count_two_rejects_address_overflow_before_io();
    test_invalid_counts_and_null_bus_do_not_io();
    test_missing_callback_is_bus_fault_and_failure_is_zero();
    test_callback_statuses_are_preserved_without_retry_or_partial_publish();
    return 0;
}
