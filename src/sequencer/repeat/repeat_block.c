/*
 * Module contract
 *
 * Owns: the block-repeat ACTIVE-lifecycle decisions; see
 * repeat_block.h.
 * Does not own: catalog rows, decode, dispatch, MMR owner
 * storage, PC writes, CPU lifecycle, vectors, take paths, NMI
 * sources, timing, buses, observers, diagnostics, allocation,
 * or logging.
 */

#include "repeat_block.h"

#include <stddef.h>
#include <stdint.h>

#include "state/mmr_file.h"
#include "state/status/registers.h"

ti_c54x_repeat_block_status ti_c54x_repeat_block_start(
    ti_c54x_repeat_block_progress *progress,
    const ti_c54x_op_effect *effect, uint16_t rsa,
    ti_c54x_arch_state *state, bool delayed_branch_armed)
{
    uint16_t brc;

    if (progress == NULL || effect == NULL || state == NULL) {
        return TI_C54X_REPEAT_BLOCK_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_REPEAT_BLOCK_START) {
        return TI_C54X_REPEAT_BLOCK_INVALID_EFFECT;
    }
    /* Delay-slot starts and unknown BRC reject before any
     * mutation; a nested ACTIVE start falls through to the
     * overwrite below. */
    if (delayed_branch_armed) {
        return TI_C54X_REPEAT_BLOCK_INVALID_STATE;
    }
    if (ti_c54x_mmr_file_read_brc(&state->mmr_file, &brc) !=
        TI_C54X_MMR_FILE_OK) {
        return TI_C54X_REPEAT_BLOCK_UNKNOWN_BRC;
    }
    if (ti_c54x_mmr_file_write_rsa(&state->mmr_file, rsa) !=
            TI_C54X_MMR_FILE_OK ||
        ti_c54x_mmr_file_write_rea(
            &state->mmr_file, effect->payload.repeat_block_start.pmad) !=
            TI_C54X_MMR_FILE_OK ||
        ti_c54x_status_register_write_bit(
            &state->status, TI_C54X_STATUS_REGISTER_TARGET_ST1, UINT8_C(15),
            UINT8_C(1)) != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_REPEAT_BLOCK_INVALID_STATE;
    }
    progress->phase = TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE;
    return TI_C54X_REPEAT_BLOCK_OK;
}

ti_c54x_repeat_block_status ti_c54x_repeat_block_body_retired(
    ti_c54x_repeat_block_progress *progress,
    ti_c54x_arch_state *state, uint32_t retired_pc_word,
    bool fell_through, bool delay_armed,
    ti_c54x_repeat_block_retired_action *action, uint16_t *loop_pc_word)
{
    uint16_t rea;
    uint16_t rsa;
    uint16_t brc;

    if (progress == NULL || state == NULL || action == NULL ||
        loop_pc_word == NULL) {
        return TI_C54X_REPEAT_BLOCK_INVALID_ARGUMENT;
    }
    *action = TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE;
    if (progress->phase != TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE) {
        return TI_C54X_REPEAT_BLOCK_OK;
    }
    /* Delay-countdown retirements belong to the pending branch,
     * never to the loop. */
    if (delay_armed || !fell_through) {
        return TI_C54X_REPEAT_BLOCK_OK;
    }
    /* REA/BRC reads precede mutation so unknown cells reject
     * transactionally. */
    if (ti_c54x_mmr_file_read_rea(&state->mmr_file, &rea) !=
            TI_C54X_MMR_FILE_OK ||
        ti_c54x_mmr_file_read_brc(&state->mmr_file, &brc) !=
            TI_C54X_MMR_FILE_OK) {
        return TI_C54X_REPEAT_BLOCK_INVALID_STATE;
    }
    if (retired_pc_word != (uint32_t)rea) {
        return TI_C54X_REPEAT_BLOCK_OK;
    }
    if (brc == UINT16_C(0)) {
        if (ti_c54x_status_register_write_bit(
                &state->status, TI_C54X_STATUS_REGISTER_TARGET_ST1,
                UINT8_C(15),
                UINT8_C(0)) != TI_C54X_STATUS_REGISTER_OK) {
            return TI_C54X_REPEAT_BLOCK_INVALID_STATE;
        }
        progress->phase = TI_C54X_REPEAT_BLOCK_PHASE_IDLE;
        *action = TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_END;
        return TI_C54X_REPEAT_BLOCK_OK;
    }
    if (ti_c54x_mmr_file_read_rsa(&state->mmr_file, &rsa) !=
            TI_C54X_MMR_FILE_OK ||
        ti_c54x_mmr_file_write_brc(&state->mmr_file,
                                   (uint16_t)(brc - UINT16_C(1))) !=
            TI_C54X_MMR_FILE_OK) {
        return TI_C54X_REPEAT_BLOCK_INVALID_STATE;
    }
    *loop_pc_word = rsa;
    *action = TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_LOOP;
    return TI_C54X_REPEAT_BLOCK_OK;
}
