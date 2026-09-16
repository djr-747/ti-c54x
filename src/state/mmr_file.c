/*
 * Module contract
 *
 * Owns: checked storage and knownness for the five MMR file cells
 * with no architectural owner yet (IMR, TRN, BRC, RSA, REA).
 * Does not own: owner-backed registers, address mapping, request
 * routing, opcode semantics, CPU lifecycle, or timing.
 */

#include "mmr_file.h"

#include <stddef.h>
#include <string.h>

typedef enum mmr_cell {
    MMR_CELL_IMR = 0,
    MMR_CELL_TRN = 1,
    MMR_CELL_BRC = 2,
    MMR_CELL_RSA = 3,
    MMR_CELL_REA = 4
} mmr_cell;

static uint16_t cell_value(const ti_c54x_mmr_file_state *file,
                           mmr_cell cell)
{
    switch (cell) {
    case MMR_CELL_IMR:
        return file->imr;
    case MMR_CELL_TRN:
        return file->trn;
    case MMR_CELL_BRC:
        return file->brc;
    case MMR_CELL_RSA:
        return file->rsa;
    case MMR_CELL_REA:
    default:
        return file->rea;
    }
}

static void cell_store(ti_c54x_mmr_file_state *file, mmr_cell cell,
                       uint16_t value)
{
    switch (cell) {
    case MMR_CELL_IMR:
        file->imr = value;
        break;
    case MMR_CELL_TRN:
        file->trn = value;
        break;
    case MMR_CELL_BRC:
        file->brc = value;
        break;
    case MMR_CELL_RSA:
        file->rsa = value;
        break;
    case MMR_CELL_REA:
    default:
        file->rea = value;
        break;
    }
}

static uint8_t cell_known_bit(mmr_cell cell)
{
    switch (cell) {
    case MMR_CELL_IMR:
        return (uint8_t)TI_C54X_MMR_FILE_KNOWN_IMR;
    case MMR_CELL_TRN:
        return (uint8_t)TI_C54X_MMR_FILE_KNOWN_TRN;
    case MMR_CELL_BRC:
        return (uint8_t)TI_C54X_MMR_FILE_KNOWN_BRC;
    case MMR_CELL_RSA:
        return (uint8_t)TI_C54X_MMR_FILE_KNOWN_RSA;
    case MMR_CELL_REA:
    default:
        return (uint8_t)TI_C54X_MMR_FILE_KNOWN_REA;
    }
}

static ti_c54x_mmr_file_status cell_read(
    const ti_c54x_mmr_file_state *file, mmr_cell cell, uint16_t *value)
{
    if (file == NULL || value == NULL) {
        return TI_C54X_MMR_FILE_INVALID_ARGUMENT;
    }
    if ((file->known_mask & cell_known_bit(cell)) == 0U) {
        return TI_C54X_MMR_FILE_UNKNOWN;
    }
    *value = cell_value(file, cell);
    return TI_C54X_MMR_FILE_OK;
}

static ti_c54x_mmr_file_status cell_write(ti_c54x_mmr_file_state *file,
                                          mmr_cell cell, uint16_t value)
{
    if (file == NULL) {
        return TI_C54X_MMR_FILE_INVALID_ARGUMENT;
    }
    cell_store(file, cell, value);
    file->known_mask =
        (uint8_t)(file->known_mask | cell_known_bit(cell));
    return TI_C54X_MMR_FILE_OK;
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_reset(ti_c54x_mmr_file_state *file)
{
    if (file == NULL) {
        return TI_C54X_MMR_FILE_INVALID_ARGUMENT;
    }
    memset(file, 0, sizeof(*file));
    return TI_C54X_MMR_FILE_OK;
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_imr(const ti_c54x_mmr_file_state *file,
                          uint16_t *value)
{
    return cell_read(file, MMR_CELL_IMR, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_imr(ti_c54x_mmr_file_state *file, uint16_t value)
{
    return cell_write(file, MMR_CELL_IMR, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_trn(const ti_c54x_mmr_file_state *file,
                          uint16_t *value)
{
    return cell_read(file, MMR_CELL_TRN, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_trn(ti_c54x_mmr_file_state *file, uint16_t value)
{
    return cell_write(file, MMR_CELL_TRN, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_brc(const ti_c54x_mmr_file_state *file,
                          uint16_t *value)
{
    return cell_read(file, MMR_CELL_BRC, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_brc(ti_c54x_mmr_file_state *file, uint16_t value)
{
    return cell_write(file, MMR_CELL_BRC, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_rsa(const ti_c54x_mmr_file_state *file,
                          uint16_t *value)
{
    return cell_read(file, MMR_CELL_RSA, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_rsa(ti_c54x_mmr_file_state *file, uint16_t value)
{
    return cell_write(file, MMR_CELL_RSA, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_read_rea(const ti_c54x_mmr_file_state *file,
                          uint16_t *value)
{
    return cell_read(file, MMR_CELL_REA, value);
}

ti_c54x_mmr_file_status
ti_c54x_mmr_file_write_rea(ti_c54x_mmr_file_state *file, uint16_t value)
{
    return cell_write(file, MMR_CELL_REA, value);
}
