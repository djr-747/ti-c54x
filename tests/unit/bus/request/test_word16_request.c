/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof of the private value-owned
 * WORD16 bus-request constructors and validator.
 * Does not own: bus callbacks, read results, byte order, transaction
 * lifecycle, sequencer phases, memory, CPU state, opcode effects, timing,
 * arbitration, diagnostics, or test discovery.
 */

#include "../../../../src/bus/request/word16_request.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void
check_invalid_request(const ti_c54x_bus_word16_request_result *result,
                      ti_c54x_bus_word16_request_status status)
{
    TI_C54X_TEST_CHECK(result->status == status);
    TI_C54X_TEST_CHECK(result->request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID);
    TI_C54X_TEST_CHECK(result->request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID);
    TI_C54X_TEST_CHECK(result->request.address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(result->request.write_value_word == UINT16_C(0));
}

static void
check_same_request_fields(const ti_c54x_bus_word16_request *actual,
                          const ti_c54x_bus_word16_request *expected)
{
    TI_C54X_TEST_CHECK(actual->space == expected->space);
    TI_C54X_TEST_CHECK(actual->access == expected->access);
    TI_C54X_TEST_CHECK(actual->address_word == expected->address_word);
    TI_C54X_TEST_CHECK(actual->write_value_word == expected->write_value_word);
}

static void
test_valid_read_requests_cover_all_spaces(void)
{
    const ti_c54x_bus_word16_request_space spaces[] = {
        TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_IO};
    const uint32_t addresses[] = {UINT32_C(0), UINT32_C(0xffff),
                                  UINT32_MAX};
    size_t space_index;
    size_t address_index;

    for (space_index = 0U; space_index < sizeof(spaces) / sizeof(spaces[0]);
         ++space_index) {
        const size_t address_count =
            spaces[space_index] ==
                    TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM
                ? sizeof(addresses) / sizeof(addresses[0])
                : 2U;

        for (address_index = 0U; address_index < address_count;
             ++address_index) {
            const ti_c54x_bus_word16_request_result result =
                ti_c54x_bus_word16_request_read(spaces[space_index],
                                                addresses[address_index]);

            TI_C54X_TEST_CHECK(result.status ==
                               TI_C54X_BUS_WORD16_REQUEST_STATUS_OK);
            TI_C54X_TEST_CHECK(result.request.space == spaces[space_index]);
            TI_C54X_TEST_CHECK(result.request.access ==
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
            TI_C54X_TEST_CHECK(result.request.address_word ==
                               addresses[address_index]);
            TI_C54X_TEST_CHECK(result.request.write_value_word == UINT16_C(0));
            TI_C54X_TEST_CHECK(
                ti_c54x_bus_word16_request_is_valid(&result.request));
        }
    }
}

static void
test_valid_write_requests_cover_all_spaces_and_values(void)
{
    const ti_c54x_bus_word16_request_space spaces[] = {
        TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_IO};
    const uint32_t addresses[] = {UINT32_C(0), UINT32_C(0xffff),
                                  UINT32_MAX};
    const uint16_t values[] = {UINT16_C(0), UINT16_C(0x1234), UINT16_MAX};
    size_t space_index;
    size_t address_index;
    size_t value_index;

    for (space_index = 0U; space_index < sizeof(spaces) / sizeof(spaces[0]);
         ++space_index) {
        const size_t address_count =
            spaces[space_index] ==
                    TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM
                ? sizeof(addresses) / sizeof(addresses[0])
                : 2U;

        for (address_index = 0U; address_index < address_count;
             ++address_index) {
            for (value_index = 0U;
                 value_index < sizeof(values) / sizeof(values[0]);
                 ++value_index) {
                const ti_c54x_bus_word16_request_result result =
                    ti_c54x_bus_word16_request_write(
                        spaces[space_index], addresses[address_index],
                        values[value_index]);

                TI_C54X_TEST_CHECK(result.status ==
                                   TI_C54X_BUS_WORD16_REQUEST_STATUS_OK);
                TI_C54X_TEST_CHECK(result.request.space == spaces[space_index]);
                TI_C54X_TEST_CHECK(result.request.access ==
                                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
                TI_C54X_TEST_CHECK(result.request.address_word ==
                                   addresses[address_index]);
                TI_C54X_TEST_CHECK(result.request.write_value_word ==
                                   values[value_index]);
                TI_C54X_TEST_CHECK(
                    ti_c54x_bus_word16_request_is_valid(&result.request));
            }
        }
    }
}

static void
test_data_and_io_reject_wide_addresses_before_narrowing(void)
{
    const ti_c54x_bus_word16_request_space spaces[] = {
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_IO};
    const uint32_t addresses[] = {UINT32_C(0x10000), UINT32_MAX};
    size_t space_index;
    size_t address_index;

    for (space_index = 0U; space_index < sizeof(spaces) / sizeof(spaces[0]);
         ++space_index) {
        for (address_index = 0U;
             address_index < sizeof(addresses) / sizeof(addresses[0]);
             ++address_index) {
            const ti_c54x_bus_word16_request_result read_result =
                ti_c54x_bus_word16_request_read(spaces[space_index],
                                                addresses[address_index]);
            const ti_c54x_bus_word16_request_result write_result =
                ti_c54x_bus_word16_request_write(spaces[space_index],
                                                 addresses[address_index],
                                                 UINT16_C(0xbeef));

            check_invalid_request(
                &read_result,
                TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_ADDRESS);
            check_invalid_request(
                &write_result,
                TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_ADDRESS);
        }
    }
}

static void
test_invalid_space_wins_status_precedence_and_zeroes_request(void)
{
    const ti_c54x_bus_word16_request_space invalid_spaces[] = {
        TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID,
        (ti_c54x_bus_word16_request_space)UINT8_C(0x7f)};
    size_t index;

    for (index = 0U;
         index < sizeof(invalid_spaces) / sizeof(invalid_spaces[0]); ++index) {
        const ti_c54x_bus_word16_request_result read_result =
            ti_c54x_bus_word16_request_read(invalid_spaces[index], UINT32_MAX);
        const ti_c54x_bus_word16_request_result write_result =
            ti_c54x_bus_word16_request_write(invalid_spaces[index], UINT32_MAX,
                                             UINT16_C(0xbeef));

        check_invalid_request(
            &read_result, TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_SPACE);
        check_invalid_request(
            &write_result, TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_SPACE);
    }
}

static void
test_validator_rejects_null_zero_forged_range_and_noncanonical_values(void)
{
    ti_c54x_bus_word16_request request = {0};

    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(NULL));
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(&request));

    request.space = (ti_c54x_bus_word16_request_space)UINT8_C(0x7f);
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(&request));

    request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    request.access = (ti_c54x_bus_word16_request_access)UINT8_C(0x7f);
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(&request));

    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    request.address_word = UINT32_C(0x10000);
    request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(&request));

    request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_IO;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(&request));

    request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    request.address_word = UINT32_C(0x1234);
    request.write_value_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_request_is_valid(&request));
}

static void
test_validator_does_not_mutate_borrowed_requests(void)
{
    const ti_c54x_bus_word16_request_result valid_result =
        ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM, UINT32_MAX,
            UINT16_C(0xbeef));
    ti_c54x_bus_word16_request valid_request = valid_result.request;
    ti_c54x_bus_word16_request invalid_request = valid_request;
    const ti_c54x_bus_word16_request valid_before = valid_request;
    ti_c54x_bus_word16_request invalid_before;

    invalid_request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    invalid_before = invalid_request;

    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_request_is_valid(&valid_request));
    check_same_request_fields(&valid_request, &valid_before);

    TI_C54X_TEST_CHECK(
        !ti_c54x_bus_word16_request_is_valid(&invalid_request));
    check_same_request_fields(&invalid_request, &invalid_before);
}

int
main(void)
{
    test_valid_read_requests_cover_all_spaces();
    test_valid_write_requests_cover_all_spaces_and_values();
    test_data_and_io_reject_wide_addresses_before_narrowing();
    test_invalid_space_wins_status_precedence_and_zeroes_request();
    test_validator_rejects_null_zero_forged_range_and_noncanonical_values();
    test_validator_does_not_mutate_borrowed_requests();
    return 0;
}
