/*
 * Module contract
 *
 * Owns: executable TI proof for the CLKMD peripheral cell
 * (P3-S68, SPRU131 Figure 8-4, Tables 8-18/8-19): reset
 * leaves the cell UNKNOWN (pin-dependent reset, no value
 * adopted), the first write makes it known, every 15-bit
 * configuration round-trips exactly, PLLSTATUS bit 0 masks
 * to zero, unknown reads preserve the output, and NULL
 * rejects.
 * Does not own: address mapping, MMR routing, CPU state
 * construction, bus transfers, opcode semantics, or
 * test-harness behavior.
 */

#include "../../../src/devices/clock_mode_register.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_reset_leaves_unknown(void)
{
    ti_c54x_clkmd_state cell;
    uint16_t value = UINT16_C(0xA5A5);

    memset(&cell, 0xFF, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_reset(&cell) ==
                       TI_C54X_CLKMD_OK);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_read(&cell, &value) ==
                       TI_C54X_CLKMD_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xA5A5));
}

static void
test_first_write_makes_known(void)
{
    ti_c54x_clkmd_state cell;
    uint16_t value = UINT16_C(0);

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_reset(&cell) ==
                       TI_C54X_CLKMD_OK);
    /* The loader1 PLL-enable word, status bit clear. */
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_write(&cell, UINT16_C(0x3146)) ==
                       TI_C54X_CLKMD_OK);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_read(&cell, &value) ==
                       TI_C54X_CLKMD_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x3146));
}

static void
test_writable_bits_round_trip(void)
{
    ti_c54x_clkmd_state cell;
    uint32_t raw_value;

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_reset(&cell) ==
                       TI_C54X_CLKMD_OK);
    for (raw_value = UINT32_C(0); raw_value <= UINT32_C(0xFFFE);
         raw_value += UINT32_C(2)) {
        const uint16_t expected = (uint16_t)raw_value;
        uint16_t actual = UINT16_C(0xA5A5);

        TI_C54X_TEST_CHECK(ti_c54x_clkmd_write(&cell, expected) ==
                           TI_C54X_CLKMD_OK);
        TI_C54X_TEST_CHECK(ti_c54x_clkmd_read(&cell, &actual) ==
                           TI_C54X_CLKMD_OK);
        TI_C54X_TEST_CHECK_U64(actual, expected);
    }
}

static void
test_status_bit_masks_to_zero(void)
{
    ti_c54x_clkmd_state cell;
    uint16_t value = UINT16_C(0);

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_reset(&cell) ==
                       TI_C54X_CLKMD_OK);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_write(&cell, UINT16_C(0xFFFF)) ==
                       TI_C54X_CLKMD_OK);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_read(&cell, &value) ==
                       TI_C54X_CLKMD_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0xFFFE));
}

static void
test_null_rejects(void)
{
    ti_c54x_clkmd_state cell;
    uint16_t value = UINT16_C(0);

    memset(&cell, 0, sizeof(cell));
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_reset(NULL) ==
                       TI_C54X_CLKMD_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_read(NULL, &value) ==
                       TI_C54X_CLKMD_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_read(&cell, NULL) ==
                       TI_C54X_CLKMD_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_clkmd_write(NULL, UINT16_C(0)) ==
                       TI_C54X_CLKMD_INVALID_ARGUMENT);
}

int
main(void)
{
    test_reset_leaves_unknown();
    test_first_write_makes_known();
    test_writable_bits_round_trip();
    test_status_bit_masks_to_zero();
    test_null_rejects();
    return 0;
}
