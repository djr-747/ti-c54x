/*
 * Module contract
 *
 * Owns: the hosted MCU upload peer's hosted-side pump: canonical
 * header publication, first-fill of buffer A from flash block 0,
 * and ACK-driven ping-pong refills from successive flash blocks
 * with a configured block-count bound. All DSP data addresses,
 * polarity, and geometry come from the public peer header;
 * observed behavior matches the boot-sequence oracle (header
 * destA/destB/remaining/MBOX/len, 512-word buffers, 1 = MCU
 * owns / 0 = ready, DSP ACKs with 1). Borrowed DATA RAM and
 * flash words are used for one call only under caller-owned
 * lifetimes; the peer allocates nothing and performs no I/O.
 * Does not own: core implementation, ISA semantics, buses, CPU
 * state, file loading, CLI parsing, timing, observers,
 * diagnostics, allocation, logging, or environment.
 */

#include "mcu_upload_peer.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static bool
peer_window_covers(const ti_c54x_mcu_upload_peer_state *peer,
                   uint32_t address_word, uint32_t word_count)
{
    uint64_t top;

    if (peer->data_words == NULL || word_count == UINT32_C(0)) {
        return false;
    }
    if (address_word < peer->data_base_word) {
        return false;
    }
    top = (uint64_t)(address_word - peer->data_base_word) +
          (uint64_t)word_count;
    return top <= (uint64_t)peer->data_word_count;
}

static void
peer_store(ti_c54x_mcu_upload_peer_state *peer, uint32_t address_word,
           uint16_t value_word)
{
    peer->data_words[(size_t)(address_word - peer->data_base_word)] =
        value_word;
}

static void
peer_fill_buffer(ti_c54x_mcu_upload_peer_state *peer,
                 uint32_t buffer_base_word, uint32_t block_index)
{
    uint32_t block_base_word =
        peer->flash_block_base_word + block_index *
                                          peer->flash_block_stride_words;

    (void)memcpy(&peer->data_words[(size_t)(buffer_base_word -
                                            peer->data_base_word)],
                 &peer->flash_words[(size_t)block_base_word],
                 (size_t)TI_C54X_MCU_UPLOAD_BUFFER_WORDS *
                     sizeof(uint16_t));
}

ti_c54x_mcu_upload_peer_status
ti_c54x_mcu_upload_peer_init(
    ti_c54x_mcu_upload_peer_state *peer, uint16_t *data_words,
    uint32_t data_base_word, uint32_t data_word_count,
    const uint16_t *flash_words, uint32_t flash_word_count,
    uint32_t flash_block_base_word, uint32_t flash_block_stride_words,
    uint32_t blocks)
{
    uint64_t remaining_words;
    uint64_t flash_need;

    if (peer == NULL || data_words == NULL || flash_words == NULL ||
        blocks == UINT32_C(0) ||
        blocks > (uint32_t)TI_C54X_MCU_UPLOAD_MAX_BLOCKS) {
        return TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT;
    }
    memset(peer, 0, sizeof(*peer));
    peer->data_words = data_words;
    peer->data_base_word = data_base_word;
    peer->data_word_count = data_word_count;
    peer->flash_words = flash_words;
    peer->flash_word_count = flash_word_count;
    peer->flash_block_base_word = flash_block_base_word;
    peer->flash_block_stride_words = flash_block_stride_words;
    peer->blocks = blocks;
    if (!peer_window_covers(peer,
                            (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_BASE,
                            (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_WORDS) ||
        !peer_window_covers(
            peer, (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_A_BASE,
            (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_WORDS) ||
        !peer_window_covers(
            peer, (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_B_BASE,
            (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_WORDS)) {
        peer->armed = false;
        return TI_C54X_MCU_UPLOAD_PEER_WINDOW_TOO_SMALL;
    }
    flash_need = (uint64_t)flash_block_base_word +
                 (uint64_t)(blocks - UINT32_C(1)) *
                     (uint64_t)flash_block_stride_words +
                 (uint64_t)TI_C54X_MCU_UPLOAD_BUFFER_WORDS;
    if (flash_need > (uint64_t)flash_word_count) {
        peer->armed = false;
        return TI_C54X_MCU_UPLOAD_PEER_FLASH_TOO_SMALL;
    }
    remaining_words =
        (uint64_t)blocks * (uint64_t)TI_C54X_MCU_UPLOAD_BUFFER_WORDS;
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_BASE,
               UINT16_C(0x0100));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_BASE + UINT32_C(1),
               UINT16_C(0x0300));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_BASE + UINT32_C(2),
               (uint16_t)(remaining_words >> 16));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_BASE + UINT32_C(3),
               (uint16_t)(remaining_words & UINT64_C(0xFFFF)));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_MBOX0, UINT16_C(1));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_MBOX1, UINT16_C(1));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_HEADER_BASE + UINT32_C(6),
               UINT16_C(0x0200));
    peer_fill_buffer(peer, (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_A_BASE,
                     UINT32_C(0));
    peer_store(peer, (uint32_t)TI_C54X_MCU_UPLOAD_MBOX0, UINT16_C(0));
    peer->fills_done = UINT32_C(1);
    peer->fills_a = UINT32_C(1);
    peer->armed = true;
    return TI_C54X_MCU_UPLOAD_PEER_OK;
}

void
ti_c54x_mcu_upload_peer_notify_data_write(
    ti_c54x_mcu_upload_peer_state *peer, uint32_t address_word,
    uint16_t value_word)
{
    uint32_t refill_base_word;
    uint32_t refill_mbox_word;

    if (peer == NULL || !peer->armed || value_word != UINT16_C(1)) {
        return;
    }
    if (address_word == (uint32_t)TI_C54X_MCU_UPLOAD_MBOX0) {
        refill_base_word = (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_B_BASE;
        refill_mbox_word = (uint32_t)TI_C54X_MCU_UPLOAD_MBOX1;
    } else if (address_word == (uint32_t)TI_C54X_MCU_UPLOAD_MBOX1) {
        refill_base_word = (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_A_BASE;
        refill_mbox_word = (uint32_t)TI_C54X_MCU_UPLOAD_MBOX0;
    } else {
        return;
    }
    /* Bounded hold: past the block limit the mailboxes keep
     * whatever the DSP wrote and no flash is touched. */
    if (peer->fills_done >= peer->blocks) {
        return;
    }
    peer_fill_buffer(peer, refill_base_word, peer->fills_done);
    peer_store(peer, refill_mbox_word, UINT16_C(0));
    peer->fills_done++;
    if (refill_base_word ==
        (uint32_t)TI_C54X_MCU_UPLOAD_BUFFER_A_BASE) {
        peer->fills_a++;
    } else {
        peer->fills_b++;
    }
}
