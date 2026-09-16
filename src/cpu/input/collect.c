/* Module contract
 * Owns: transactional collection of catalog-declared accumulator, auxiliary,
 * T, stack-pointer, return-register, and status C, TC, SXM, SST, or OVM
 * resources into a copied opcode input snapshot.
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

    if ((resources & TI_C54X_INPUT_RESOURCES_STATUS_SST) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_status_register_field_result sst_result;

        if (sources == NULL || sources->status == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        sst_result = ti_c54x_status_register_read_sst(sources->status);
        if (sst_result.status != TI_C54X_STATUS_REGISTER_OK &&
            sst_result.status != TI_C54X_STATUS_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.status_sst = (uint8_t)sst_result.raw_value;
        staged.status_sst_known =
            sst_result.status == TI_C54X_STATUS_REGISTER_OK;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_STATUS_OVM) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_status_register_field_result ovm_result;

        if (sources == NULL || sources->status == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        ovm_result = ti_c54x_status_register_read_ovm(sources->status);
        if (ovm_result.status != TI_C54X_STATUS_REGISTER_OK &&
            ovm_result.status != TI_C54X_STATUS_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.status_ovm = (uint8_t)ovm_result.raw_value;
        staged.status_ovm_known =
            ovm_result.status == TI_C54X_STATUS_REGISTER_OK;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_STATUS_ASM) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        ti_c54x_status_register_field_result asm_result;

        if (sources == NULL || sources->status == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        asm_result = ti_c54x_status_register_read_asm(sources->status);
        if (asm_result.status != TI_C54X_STATUS_REGISTER_OK &&
            asm_result.status != TI_C54X_STATUS_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        staged.status_asm = (uint8_t)asm_result.raw_value;
        staged.status_asm_known =
            asm_result.status == TI_C54X_STATUS_REGISTER_OK;
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_T_REGISTER) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        uint16_t raw_value = 0U;
        const ti_c54x_t_register_status status =
            sources == NULL || sources->t_register == NULL
                ? TI_C54X_T_REGISTER_INVALID_ARGUMENT
                : ti_c54x_t_register_read(sources->t_register, &raw_value);

        if (status == TI_C54X_T_REGISTER_OK) {
            staged.t_register_raw16 = raw_value;
            staged.t_register_known = true;
        } else if (status != TI_C54X_T_REGISTER_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        uint8_t index;

        if (sources == NULL || sources->arch_state == NULL) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
        for (index = 0U; index < UINT8_C(8); ++index) {
            uint16_t raw_value = 0U;
            const ti_c54x_arch_state_status status =
                ti_c54x_arch_state_read_ar(sources->arch_state, index,
                                            &raw_value);

            if (status == TI_C54X_ARCH_STATE_OK) {
                staged.auxiliary_registers_raw16[index] = raw_value;
                staged.auxiliary_registers_known_mask = (uint8_t)(
                    staged.auxiliary_registers_known_mask |
                    (uint8_t)(UINT8_C(1) << index));
            } else if (status != TI_C54X_ARCH_STATE_UNKNOWN) {
                return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
            }
        }
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_STACK_POINTER) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        uint16_t raw_value = 0U;
        const ti_c54x_arch_state_status status =
            sources == NULL || sources->arch_state == NULL
                ? TI_C54X_ARCH_STATE_INVALID_ARGUMENT
                : ti_c54x_arch_state_read_sp(sources->arch_state,
                                             &raw_value);

        if (status == TI_C54X_ARCH_STATE_OK) {
            staged.stack_pointer_raw16 = raw_value;
            staged.stack_pointer_known = true;
        } else if (status != TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
    }

    if ((resources & TI_C54X_INPUT_RESOURCES_RETURN_REGISTER) !=
        TI_C54X_INPUT_RESOURCES_NONE) {
        uint16_t raw_value = 0U;
        const ti_c54x_arch_state_status status =
            sources == NULL || sources->arch_state == NULL
                ? TI_C54X_ARCH_STATE_INVALID_ARGUMENT
                : ti_c54x_arch_state_read_rtn(sources->arch_state,
                                              &raw_value);

        if (status == TI_C54X_ARCH_STATE_OK) {
            staged.return_register_raw16 = raw_value;
            staged.return_register_known = true;
        } else if (status != TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_INPUT_COLLECT_INVALID_SOURCE;
        }
    }

    *output = staged;
    return TI_C54X_INPUT_COLLECT_OK;
}
