/* Module contract
 * Owns: executable RED/GREEN proof for the hosted MCU upload peer
 * (P3-S71, boot-sequence oracle sections 2-3: UPLOADHEADER at DSP
 * data 0x87B-0x881, MBOX0 0x87F, MBOX1 0x880, 512-word buffers A
 * 0x900-0xAFF and B 0xB00-0xCFF, polarity 1 = MCU owns / 0 =
 * buffer ready, DSP ACKs with 1, 64 blocks ping-pong 32+32).
 * Covers header init, first fill, ACK advance, flash slicing,
 * the 64-iteration 32+32 split, bounded hold, neighbor
 * preservation, and init rejection. The peer implementation is
 * compiled in through its hosted source file; no core state,
 * buses, files, or environment are touched.
 * Does not own: core implementation, ISA semantics, the runner
 * CLI, flash staging, timing, or the production peer source.
 */

#include "../../../tools/hosted/mcu_upload_peer.c"

#include "../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

#define HOST_DATA_BASE UINT32_C(0x0800)
#define HOST_WINDOW_WORDS UINT32_C(0x0600)

static uint16_t ram_window[0x0600];
static uint16_t flash_blocks[2048];

static void
fill_flash_pattern(void)
{
    /* Three strided 512-word blocks at base 10, stride 600:
     * block n opens with 0xA000+n and counts up. */
    for (uint32_t n = UINT32_C(0); n < UINT32_C(3); n++) {
        for (uint32_t i = UINT32_C(0); i < UINT32_C(512); i++) {
            flash_blocks[(size_t)(UINT32_C(10) + n * UINT32_C(600) + i)] =
                (uint16_t)(UINT16_C(0xA000) + (uint16_t)(n * UINT16_C(16)) +
                           (uint16_t)i);
        }
    }
}

static void
reset_ram(void)
{
    memset(ram_window, 0, sizeof(ram_window));
}

static uint16_t
ram_at(uint32_t address)
{
    return ram_window[(size_t)(address - HOST_DATA_BASE)];
}

static void
dsp_write(ti_c54x_mcu_upload_peer_state *peer, uint32_t address, uint16_t value)
{
    ram_window[(size_t)(address - HOST_DATA_BASE)] = value;
    ti_c54x_mcu_upload_peer_notify_data_write(peer, address, value);
}

static void
test_header_init_and_first_fill(void)
{
    ti_c54x_mcu_upload_peer_state peer;

    reset_ram();
    fill_flash_pattern();
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_OK);
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87B)) == UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87C)) == UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87D)) == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87E)) == UINT16_C(0x0600));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87F)) == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x880)) == UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x881)) == UINT16_C(0x0200));
    for (uint32_t i = UINT32_C(0); i < UINT32_C(512); i++) {
        TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0900) + i) ==
                           flash_blocks[(size_t)(UINT32_C(10) + i)]);
    }
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(1));
    TI_C54X_TEST_CHECK(peer.fills_a == UINT32_C(1));
    TI_C54X_TEST_CHECK(peer.fills_b == UINT32_C(0));
}

static void
test_ack_advance_pingpong(void)
{
    ti_c54x_mcu_upload_peer_state peer;

    reset_ram();
    fill_flash_pattern();
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_OK);
    dsp_write(&peer, UINT32_C(0x87F), UINT16_C(0x0001));
    for (uint32_t i = UINT32_C(0); i < UINT32_C(512); i++) {
        TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0B00) + i) ==
                           flash_blocks[(size_t)(UINT32_C(610) + i)]);
    }
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x880)) == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87F)) == UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(2));
    TI_C54X_TEST_CHECK(peer.fills_b == UINT32_C(1));
    dsp_write(&peer, UINT32_C(0x880), UINT16_C(0x0001));
    for (uint32_t i = UINT32_C(0); i < UINT32_C(512); i++) {
        TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0900) + i) ==
                           flash_blocks[(size_t)(UINT32_C(1210) + i)]);
    }
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87F)) == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(3));
    TI_C54X_TEST_CHECK(peer.fills_a == UINT32_C(2));
}

static void
test_bounded_hold(void)
{
    ti_c54x_mcu_upload_peer_state peer;
    uint16_t held_a;

    reset_ram();
    fill_flash_pattern();
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(2)) == TI_C54X_MCU_UPLOAD_PEER_OK);
    dsp_write(&peer, UINT32_C(0x87F), UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(2));
    held_a = ram_at(UINT32_C(0x0900));
    /* Block limit reached: the next ACK holds, MBOX stays 1
     * and buffer A keeps block 0. */
    dsp_write(&peer, UINT32_C(0x880), UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(2));
    TI_C54X_TEST_CHECK(peer.fills_a == UINT32_C(1));
    TI_C54X_TEST_CHECK(peer.fills_b == UINT32_C(1));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x880)) == UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0900)) == held_a);
    dsp_write(&peer, UINT32_C(0x87F), UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(2));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0B00)) ==
                       flash_blocks[(size_t)UINT32_C(610)]);
}

static void
test_neighbors_and_non_mbox_untouched(void)
{
    ti_c54x_mcu_upload_peer_state peer;

    reset_ram();
    fill_flash_pattern();
    ram_window[(size_t)(UINT32_C(0x87A) - HOST_DATA_BASE)] =
        UINT16_C(0x5A5A);
    ram_window[(size_t)(UINT32_C(0x882) - HOST_DATA_BASE)] =
        UINT16_C(0x5A5A);
    ram_window[(size_t)(UINT32_C(0x8FF) - HOST_DATA_BASE)] =
        UINT16_C(0x5A5A);
    ram_window[(size_t)(UINT32_C(0xD00) - HOST_DATA_BASE)] =
        UINT16_C(0x5A5A);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_OK);
    /* A data write of 1 inside buffer A is payload, not an ACK. */
    dsp_write(&peer, UINT32_C(0x0900), UINT16_C(0x0001));
    /* A zero write to a mailbox is not an ACK. */
    dsp_write(&peer, UINT32_C(0x87F), UINT16_C(0x0000));
    /* Out-of-window traffic never reaches the pump. */
    ti_c54x_mcu_upload_peer_notify_data_write(&peer, UINT32_C(0x2800),
                                      UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(1));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87A)) == UINT16_C(0x5A5A));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x882)) == UINT16_C(0x5A5A));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x8FF)) == UINT16_C(0x5A5A));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0xD00)) == UINT16_C(0x5A5A));
}

static void
test_init_rejects(void)
{
    ti_c54x_mcu_upload_peer_state peer;

    reset_ram();
    fill_flash_pattern();
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           NULL, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, NULL, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, NULL, UINT32_C(2048),
                           UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(0)) == TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(65)) == TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           UINT32_C(0x0400), flash_blocks,
                           UINT32_C(2048), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_WINDOW_TOO_SMALL);
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash_blocks,
                           UINT32_C(1200), UINT32_C(10), UINT32_C(600),
                           UINT32_C(3)) == TI_C54X_MCU_UPLOAD_PEER_FLASH_TOO_SMALL);
}

static uint16_t flash64[UINT32_C(64) * UINT32_C(512)];

static void
test_sixty_four_iterations_split_32_32(void)
{
    ti_c54x_mcu_upload_peer_state peer;

    for (uint32_t n = UINT32_C(0); n < UINT32_C(64); n++) {
        for (uint32_t i = UINT32_C(0); i < UINT32_C(512); i++) {
            flash64[(size_t)(n * UINT32_C(512) + i)] =
                (uint16_t)((uint16_t)(n * UINT16_C(64)) +
                           (uint16_t)(i & UINT16_C(0x3F)));
        }
    }
    reset_ram();
    TI_C54X_TEST_CHECK(ti_c54x_mcu_upload_peer_init(
                           &peer, ram_window, HOST_DATA_BASE,
                           HOST_WINDOW_WORDS, flash64,
                           UINT32_C(64) * UINT32_C(512), UINT32_C(0),
                           UINT32_C(512),
                           UINT32_C(64)) == TI_C54X_MCU_UPLOAD_PEER_OK);
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87D)) == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87E)) == UINT16_C(0x8000));
    for (uint32_t ack = UINT32_C(0); ack < UINT32_C(64); ack++) {
        dsp_write(&peer,
                  (ack % UINT32_C(2) == UINT32_C(0)) ? UINT32_C(0x87F)
                                                    : UINT32_C(0x880),
                  UINT16_C(0x0001));
    }
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(64));
    TI_C54X_TEST_CHECK(peer.fills_a == UINT32_C(32));
    TI_C54X_TEST_CHECK(peer.fills_b == UINT32_C(32));
    /* Last A fill is block 62, last B fill is block 63. */
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0900)) ==
                       flash64[(size_t)(UINT32_C(62) * UINT32_C(512))]);
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x0B00)) ==
                       flash64[(size_t)(UINT32_C(63) * UINT32_C(512))]);
    /* The 65th ACK holds: no 65th fill exists. */
    dsp_write(&peer, UINT32_C(0x87F), UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(peer.fills_done == UINT32_C(64));
    TI_C54X_TEST_CHECK(ram_at(UINT32_C(0x87F)) == UINT16_C(0x0001));
}

int
main(void)
{
    test_header_init_and_first_fill();
    test_ack_advance_pingpong();
    test_bounded_hold();
    test_neighbors_and_non_mbox_untouched();
    test_init_rejects();
    test_sixty_four_iterations_split_32_32();
    return 0;
}
