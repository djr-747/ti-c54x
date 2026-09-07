/* Module contract
 * Owns: transactional collection of catalog-declared accumulator and status C,
 * TC, or SXM resources into a copied opcode input snapshot.
 * Does not own: opcode semantics, decode, CPU lifecycle, effects, sequencing,
 * timing, buses, diagnostics, allocation, or global state.
 */

#include "collect.h"

#include <stddef.h>

static bool valid_accumulator_state(const ti_c54x_accumulator_state *state)
{
    const uint8_t known_mask =
        (uint8_t)(state->known_mask & (uint8_t)~UINT8_C(3));

    return known_mask == 0U && state->a_raw40 <= TI_C54X_U40_MASK &&
           state->b_raw40 <= TI_C54X_U40_MASK;
}

ti_c54x_input_collect_status ti_c54x_input_collect(
    ti_c54x_input_resource_mask resources,
    const ti_c54x_input_sources *sources,
    ti_c54x_op_input_snapshot *output)
{
    ti_c54x_op_input_snapshot staged = {0};

    if (output == NULL) {
        return TI_C54X_INPUT_COLLECT_INVALID_ARGUMENT;
    }
    if ((resources & (ti_c54x_input_resource_mask)
                    ~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        return TI_C54X_INPUT_COLLECT_INVALID_RESOURCE_MASK;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_ACCUMULATORS) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_accumulator_snapshot snapshot;
        ti_c54x_accumulator_status status;

        if (sources == NULL || sources->accumulators == NULL ||
            !valid_accumulator_state(sources->accumulators)) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        status = ti_c54x_accumulator_read_snapshot(sources->accumulators,
                                                    &snapshot);
        if (status != TI_C54X_ACCUMULATOR_OK &&
            status != TI_C54X_ACCUMULATOR_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.accumulator_a_raw40 = snapshot.a_raw40;
        staged.accumulator_b_raw40 = snapshot.b_raw40;
        staged.accumulator_a_known = snapshot.a_known;
        staged.accumulator_b_known = snapshot.b_known;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_STATUS_C) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_status_register_field_result c_result;

        if (sources == NULL || sources->status == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        c_result = ti_c54x_status_register_read_c(sources->status);
        if (c_result.status != TI_C54X_STATUS_REGISTER_OK &&
            c_result.status != TI_C54X_STATUS_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.status_c = (uint8_t)c_result.raw_value;
        staged.status_c_known = c_result.status == TI_C54X_STATUS_REGISTER_OK;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_STATUS_TC) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_status_register_field_result tc_result;

        if (sources == NULL || sources->status == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        tc_result = ti_c54x_status_register_read_tc(sources->status);
        if (tc_result.status != TI_C54X_STATUS_REGISTER_OK &&
            tc_result.status != TI_C54X_STATUS_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.status_tc = (uint8_t)tc_result.raw_value;
        staged.status_tc_known = tc_result.status == TI_C54X_STATUS_REGISTER_OK;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_STATUS_SXM) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_status_register_field_result sxm_result;

        if (sources == NULL || sources->status == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        sxm_result = ti_c54x_status_register_read_sxm(sources->status);
        if (sxm_result.status != TI_C54X_STATUS_REGISTER_OK &&
            sxm_result.status != TI_C54X_STATUS_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.status_sxm = (uint8_t)sxm_result.raw_value;
        staged.status_sxm_known =
            sxm_result.status == TI_C54X_STATUS_REGISTER_OK;
    }

    *output = staged;
    return TI_C54X_INPUT_COLLECT_OK;
}
