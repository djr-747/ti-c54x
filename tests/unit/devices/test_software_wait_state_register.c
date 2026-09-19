/*
 * Module contract
 *
 * Owns: executable TI proof for the SWWSR peripheral cell
 * (P3-S67, SPRU131 Tables 8-2/10-2, Figure 10-2): reset
 * adopts 0x7FFF as known, every 15-bit value round-trips
 * exactly, reserved bit 15 masks to zero, unknown reads
 * preserve the output, and NULL rejects.
 * Does not own: address mapping, MMR routing, CPU state
 * construction, bus transfers, opcode semantics, or
 * test-harness behavior.
 */

#include "../../../src/devices/software_wait_state_register.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_reset_adopts_documented_value_known(void)
{
    ti_c54x_swwsr_state cell;
    uint16_t value = UINT16_C(0);

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_reset(&cell) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(&cell, &value) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x7FFF));
}

static void
test_writable_bits_round_trip(void)
{
    ti_c54x_swwsr_state cell;
    uint32_t raw_value;

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_reset(&cell) ==
                       TI_C54X_SWWSR_OK);
    for (raw_value = UINT32_C(0); raw_value <= UINT32_C(0x7FFF);
         ++raw_value) {
        const uint16_t expected = (uint16_t)raw_value;
        uint16_t actual = UINT16_C(0xA5A5);

        TI_C54X_TEST_CHECK(ti_c54x_swwsr_write(&cell, expected) ==
                           TI_C54X_SWWSR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(&cell, &actual) ==
                           TI_C54X_SWWSR_OK);
        TI_C54X_TEST_CHECK_U64(actual, expected);
    }
}

static void
test_reserved_bit_masks_to_zero(void)
{
    ti_c54x_swwsr_state cell;
    uint16_t value = UINT16_C(0);

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_reset(&cell) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_write(&cell, UINT16_C(0xFFFF)) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(&cell, &value) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x7FFF));
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_write(&cell, UINT16_C(0x8000)) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(&cell, &value) ==
                       TI_C54X_SWWSR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x0000));
}

static void
test_unknown_reads_preserve_output(void)
{
    ti_c54x_swwsr_state cell;
    uint16_t value = UINT16_C(0xA5A5);

    /* No reset: the cell is unknown and the output is kept. */
    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(&cell, &value) ==
                       TI_C54X_SWWSR_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
}

static void
test_null_rejects(void)
{
    ti_c54x_swwsr_state cell;
    uint16_t value = UINT16_C(0);

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_reset(NULL) ==
                       TI_C54X_SWWSR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(NULL, &value) ==
                       TI_C54X_SWWSR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_read(&cell, NULL) ==
                       TI_C54X_SWWSR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_swwsr_write(NULL, UINT16_C(0)) ==
                       TI_C54X_SWWSR_INVALID_ARGUMENT);
}

int
main(void)
{
    test_reset_adopts_documented_value_known();
    test_writable_bits_round_trip();
    test_reserved_bit_masks_to_zero();
    test_unknown_reads_preserve_output();
    test_null_rejects();
    return 0;
}
