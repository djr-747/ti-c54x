/* Module contract
 * Owns: executable RED/GREEN proof for the hosted port peer
 * (P3-S86, first hardware-port access at 0xF57: PORTW writes
 * *(0x0008) to port 0). Covers init arming, write journal
 * order, sticky reads, unknown-port zero, journal overflow,
 * and null rejection. The peer implementation is compiled
 * in through its hosted source file; no core state, buses,
 * files, or environment are touched.
 * Does not own: core implementation, ISA semantics, the runner
 * CLI, hardware port truth, timing, or the production peer
 * source.
 */

#include "../../../tools/hosted/port_peer.c"

#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_init_arms_empty(void)
{
    ti_c54x_port_peer_state peer;

    TI_C54X_TEST_CHECK(ti_c54x_port_peer_init(NULL) ==
                       TI_C54X_PORT_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_init(&peer) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(peer.writes == UINT32_C(0));
    TI_C54X_TEST_CHECK(peer.reads == UINT32_C(0));
    TI_C54X_TEST_CHECK(peer.journaled == UINT32_C(0));
    TI_C54X_TEST_CHECK(peer.overflow == false);
}

static void
test_writes_journal_in_order(void)
{
    ti_c54x_port_peer_state peer;

    TI_C54X_TEST_CHECK(ti_c54x_port_peer_init(&peer) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_write(
                           &peer, UINT16_C(0x0000),
                           UINT16_C(0x1234)) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_write(
                           &peer, UINT16_C(0x0005),
                           UINT16_C(0x00FF)) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(peer.writes == UINT32_C(2));
    TI_C54X_TEST_CHECK(peer.journaled == UINT32_C(2));
    TI_C54X_TEST_CHECK(peer.journal[0].port == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(peer.journal[0].value == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(peer.journal[1].port == UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(peer.journal[1].value == UINT16_C(0x00FF));
    TI_C54X_TEST_CHECK(peer.overflow == false);
}

static void
test_reads_are_sticky_else_zero(void)
{
    ti_c54x_port_peer_state peer;
    uint16_t value = UINT16_C(0xFFFF);

    TI_C54X_TEST_CHECK(ti_c54x_port_peer_init(&peer) ==
                       TI_C54X_PORT_PEER_OK);
    /* Unknown port reads zero (staged simplification). */
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_read(
                           &peer, UINT16_C(0x0003), &value) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(value == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_write(
                           &peer, UINT16_C(0x0003),
                           UINT16_C(0x00AB)) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_write(
                           &peer, UINT16_C(0x0003),
                           UINT16_C(0x00CD)) ==
                       TI_C54X_PORT_PEER_OK);
    /* Last write to the port wins. */
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_read(
                           &peer, UINT16_C(0x0003), &value) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(value == UINT16_C(0x00CD));
    TI_C54X_TEST_CHECK(peer.reads == UINT32_C(2));
}

static void
test_overflow_counts_and_flags(void)
{
    ti_c54x_port_peer_state peer;
    uint32_t i;

    TI_C54X_TEST_CHECK(ti_c54x_port_peer_init(&peer) ==
                       TI_C54X_PORT_PEER_OK);
    for (i = UINT32_C(0);
         i < (uint32_t)TI_C54X_PORT_PEER_JOURNAL_WORDS + UINT32_C(1);
         i++) {
        TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_write(
                               &peer, (uint16_t)i,
                               UINT16_C(0x0001)) ==
                           TI_C54X_PORT_PEER_OK);
    }
    TI_C54X_TEST_CHECK(peer.writes ==
                       (uint32_t)TI_C54X_PORT_PEER_JOURNAL_WORDS +
                           UINT32_C(1));
    TI_C54X_TEST_CHECK(peer.journaled ==
                       (uint32_t)TI_C54X_PORT_PEER_JOURNAL_WORDS);
    TI_C54X_TEST_CHECK(peer.overflow == true);
}

static void
test_null_arguments_reject(void)
{
    ti_c54x_port_peer_state peer;
    uint16_t value = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_port_peer_init(&peer) ==
                       TI_C54X_PORT_PEER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_write(
                           NULL, UINT16_C(0), UINT16_C(0)) ==
                       TI_C54X_PORT_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_read(
                           NULL, UINT16_C(0), &value) ==
                       TI_C54X_PORT_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_port_peer_notify_read(
                           &peer, UINT16_C(0), NULL) ==
                       TI_C54X_PORT_PEER_INVALID_ARGUMENT);
    /* Rejected calls leave the counters untouched. */
    TI_C54X_TEST_CHECK(peer.writes == UINT32_C(0));
    TI_C54X_TEST_CHECK(peer.reads == UINT32_C(0));
}

int
main(void)
{
    test_init_arms_empty();
    test_writes_journal_in_order();
    test_reads_are_sticky_else_zero();
    test_overflow_counts_and_flags();
    test_null_arguments_reject();
    return 0;
}
