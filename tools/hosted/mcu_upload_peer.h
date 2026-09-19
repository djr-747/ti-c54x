/*
 * Module contract
 *
 * Owns: the hosted MCU upload peer's public identity: oracle-observed
 * DSP data addresses (UPLOADHEADER 0x87B-0x881, MBOX0 0x87F, MBOX1
 * 0x880, 512-word buffers A 0x900-0xAFF and B 0xB00-0xCFF),
 * the 1 = MCU owns / 0 = buffer ready polarity, the 64-block
 * ceiling, and the peer state and status vocabulary. Borrowed
 * DATA RAM and flash words are used for one call only under
 * caller-owned lifetimes; the peer allocates nothing and
 * performs no I/O.
 * Does not own: core implementation, ISA semantics, buses, CPU
 * state, file loading, CLI parsing, timing, observers,
 * diagnostics, allocation, logging, or environment.
 */

#ifndef TI_C54X_HOSTED_MCU_UPLOAD_PEER_H
#define TI_C54X_HOSTED_MCU_UPLOAD_PEER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* Canonical header cells (DSP data words). */
    TI_C54X_MCU_UPLOAD_HEADER_BASE = 0x87B,
    TI_C54X_MCU_UPLOAD_HEADER_WORDS = 7,
    TI_C54X_MCU_UPLOAD_MBOX0 = 0x87F,
    TI_C54X_MCU_UPLOAD_MBOX1 = 0x880,
    /* Buffer geometry (DSP data words). */
    TI_C54X_MCU_UPLOAD_BUFFER_WORDS = 512,
    TI_C54X_MCU_UPLOAD_BUFFER_A_BASE = 0x900,
    TI_C54X_MCU_UPLOAD_BUFFER_B_BASE = 0xB00,
    /* Highest DSP data word the peer ever touches. */
    TI_C54X_MCU_UPLOAD_WINDOW_TOP = 0xD00,
    /* Oracle upload ceiling: 64 buffers of 512 words. */
    TI_C54X_MCU_UPLOAD_MAX_BLOCKS = 64,
    /* Canonical header immediates. */
    TI_C54X_MCU_UPLOAD_DEST_A = 0x100,
    TI_C54X_MCU_UPLOAD_DEST_B = 0x300,
    TI_C54X_MCU_UPLOAD_LEN_WORDS = 512,
    /* Flash geometry (bus bytes): the staged image opens at bus
     * 0x200000, block 0 opens at 0x200040 and blocks stride
     * 0x4000. The runner converts to word indexes. */
    TI_C54X_MCU_UPLOAD_FLASH_BUS_BASE = 0x200000U,
    TI_C54X_MCU_UPLOAD_FLASH_BLOCK0_BUS = 0x200040U,
    TI_C54X_MCU_UPLOAD_FLASH_STRIDE_BYTES = 0x4000U
};

typedef enum ti_c54x_mcu_upload_peer_status {
    TI_C54X_MCU_UPLOAD_PEER_OK = 0,
    TI_C54X_MCU_UPLOAD_PEER_INVALID_ARGUMENT = 1,
    TI_C54X_MCU_UPLOAD_PEER_WINDOW_TOO_SMALL = 2,
    TI_C54X_MCU_UPLOAD_PEER_FLASH_TOO_SMALL = 3
} ti_c54x_mcu_upload_peer_status;

typedef struct ti_c54x_mcu_upload_peer_state {
    uint16_t *data_words;
    uint32_t data_base_word;
    uint32_t data_word_count;
    const uint16_t *flash_words;
    uint32_t flash_word_count;
    uint32_t flash_block_base_word;
    uint32_t flash_block_stride_words;
    uint32_t blocks;
    uint32_t fills_done;
    uint32_t fills_a;
    uint32_t fills_b;
    bool armed;
} ti_c54x_mcu_upload_peer_state;

/* Publish the canonical header, fill buffer A from flash block 0,
 * and hand buffer A to the DSP (MBOX0 0, MBOX1 1). The DATA window
 * must cover 0x87B-0xCFF and flash must cover every configured
 * block; blocks is clamped to 1-64. Failed init leaves the peer
 * disarmed and the DATA window untouched. */
ti_c54x_mcu_upload_peer_status
ti_c54x_mcu_upload_peer_init(
    ti_c54x_mcu_upload_peer_state *peer, uint16_t *data_words,
    uint32_t data_base_word, uint32_t data_word_count,
    const uint16_t *flash_words, uint32_t flash_word_count,
    uint32_t flash_block_base_word, uint32_t flash_block_stride_words,
    uint32_t blocks);

/* Answer one completed DSP DATA write. A write of 1 to a mailbox
 * cell is a buffer ACK: while fills remain the other buffer is
 * refilled from the next flash block and its mailbox cleared to
 * 0, otherwise the peer holds (mailboxes unchanged). All other
 * traffic is ignored. Never touches words outside the peer
 * window. */
void
ti_c54x_mcu_upload_peer_notify_data_write(
    ti_c54x_mcu_upload_peer_state *peer, uint32_t address_word,
    uint16_t value_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_HOSTED_MCU_UPLOAD_PEER_H */
