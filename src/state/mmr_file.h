#ifndef TI_C54X_STATE_MMR_FILE_H
#define TI_C54X_STATE_MMR_FILE_H

/*
 * Module contract
 *
 * Owns: the private MMR file cells for the five Table 3-3 registers
 * with no architectural owner yet (IMR, TRN, BRC, RSA, REA):
 * instance-owned 16-bit storage plus knownness with checked
 * read/write access. BRC/RSA/REA storage migrates to the repeat
 * owner when S2 lands; until then the file is their truth.
 * Does not own: owner-backed registers, address mapping, request
 * routing, opcode semantics, CPU lifecycle, or timing. Callers own
 * the file struct and its lifetime.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_mmr_file_status {
    TI_C54X_MMR_FILE_OK = 0,
    TI_C54X_MMR_FILE_UNKNOWN = 1,
    TI_C54X_MMR_FILE_INVALID_ARGUMENT = 2
} ti_c54x_mmr_file_status;

typedef enum ti_c54x_mmr_file_known {
    TI_C54X_MMR_FILE_KNOWN_IMR = 1U << 0,
    TI_C54X_MMR_FILE_KNOWN_TRN = 1U << 1,
    TI_C54X_MMR_FILE_KNOWN_BRC = 1U << 2,
    TI_C54X_MMR_FILE_KNOWN_RSA = 1U << 3,
    TI_C54X_MMR_FILE_KNOWN_REA = 1U << 4
} ti_c54x_mmr_file_known;

typedef struct ti_c54x_mmr_file_state {
    uint16_t imr;
    uint16_t trn;
    uint16_t brc;
    uint16_t rsa;
    uint16_t rea;
    uint8_t known_mask;
} ti_c54x_mmr_file_state;

/* Zero storage and knownness. NULL rejects. */
ti_c54x_mmr_file_status
ti_c54x_mmr_file_reset(ti_c54x_mmr_file_state *file);

/* Checked cell access. Reads require the cell knownness bit and
 * preserve *value on UNKNOWN or INVALID_ARGUMENT; writes accept
 * any 16-bit value and mark the cell known. */
ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_imr(const ti_c54x_mmr_file_state *file,
                          uint16_t *value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_imr(ti_c54x_mmr_file_state *file,
                           uint16_t value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_trn(const ti_c54x_mmr_file_state *file,
                          uint16_t *value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_trn(ti_c54x_mmr_file_state *file,
                           uint16_t value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_brc(const ti_c54x_mmr_file_state *file,
                          uint16_t *value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_brc(ti_c54x_mmr_file_state *file,
                           uint16_t value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_rsa(const ti_c54x_mmr_file_state *file,
                          uint16_t *value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_rsa(ti_c54x_mmr_file_state *file,
                           uint16_t value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_rea(const ti_c54x_mmr_file_state *file,
                          uint16_t *value);
ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_rea(ti_c54x_mmr_file_state *file,
                           uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_STATE_MMR_FILE_H */
