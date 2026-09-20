/* Module contract
 * Owns: lifecycle bookkeeping and orchestration of one caller-owned CPU
 * instance, including the copied bus descriptor, configured reset carrier,
 * common architectural-state reset sequencing, and lifecycle event values.
 * All live PC and status access is routed through the embedded arch_state.
 * Does not own: caller storage, bus memory, callback contexts, observer sink
 * copying/gating/emission, or opcode semantics. Those belong to their owning
 * modules.
 */

#include "cpu_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "memory/pending_work.h"
#include "trace/emit.h"

#include "../state/interrupt_flag_register.h"

static ti_c54x_error
memory_status_to_error(ti_c54x_memory_instruction_status status)
{
    if (status == TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED) {
        return TI_C54X_ERROR_BUS_CANCEL_FAILED;
    }
    return status == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK ||
                   status == TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED
               ? TI_C54X_ERROR_OK
               : TI_C54X_ERROR_INVALID_ARGUMENT;
}

size_t ti_c54x_cpu_size(void)
{
    return sizeof(ti_c54x_cpu);
}

size_t ti_c54x_cpu_alignment(void)
{
    return _Alignof(ti_c54x_cpu);
}

ti_c54x_error ti_c54x_cpu_init(void *storage,
                                size_t storage_size,
                                const ti_c54x_config *config,
                                const ti_c54x_bus *bus,
                                ti_c54x_cpu **out_cpu)
{
    ti_c54x_cpu *cpu;
    ti_c54x_bus_word16_adapter_status endpoint_status;

    if (out_cpu == NULL) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    *out_cpu = NULL;
    if (storage == NULL || bus == NULL) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    endpoint_status = ti_c54x_cpu_memory_validate_endpoint(
        &bus->data_endpoint);
    if (endpoint_status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT ||
        endpoint_status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    if (storage_size < sizeof(ti_c54x_cpu)) {
        return TI_C54X_ERROR_STORAGE_TOO_SMALL;
    }
    if ((uintptr_t)storage % _Alignof(ti_c54x_cpu) != 0U) {
        return TI_C54X_ERROR_STORAGE_MISALIGNED;
    }

    cpu = (ti_c54x_cpu *)storage;
    /* The caller owns the fresh/deinitialized-storage precondition.  The
     * object is therefore safe to initialize without probing indeterminate
     * bytes or trying to discover a live instance. */
    memset(cpu, 0, sizeof(*cpu));
    if (ti_c54x_arch_state_reset_common(&cpu->arch_state) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    if (ti_c54x_accumulator_reset(&cpu->accumulators) !=
        TI_C54X_ACCUMULATOR_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    if (ti_c54x_t_register_reset(&cpu->t_register) !=
        TI_C54X_T_REGISTER_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    cpu->reset_pc_word = config != NULL ? config->reset_pc_word : 0U;
    cpu->bus = *bus;
    ti_c54x_observer_init(&cpu->observer,
                          config != NULL ? &config->trace : NULL);
    cpu->arch_state.pc_word = cpu->reset_pc_word;
    *out_cpu = cpu;
    cpu->execution_active = true;
    TI_C54X_CPU_EMIT_TRACE(cpu, TI_C54X_TRACE_RESET);
    cpu->execution_active = false;
    return TI_C54X_ERROR_OK;
}

ti_c54x_error ti_c54x_cpu_deinit(ti_c54x_cpu *cpu)
{
    ti_c54x_error status;

    if (cpu == NULL) {
        return TI_C54X_ERROR_OK;
    }
    if (cpu->execution_active) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    cpu->execution_active = true;
    status = memory_status_to_error(
        ti_c54x_cpu_memory_reset_transaction(
            &cpu->instruction_transaction, &cpu->bus.data_endpoint));
    cpu->execution_active = false;
    if (status != TI_C54X_ERROR_OK) {
        return status;
    }
    memset(cpu, 0, sizeof(*cpu));
    return TI_C54X_ERROR_OK;
}

ti_c54x_error ti_c54x_cpu_reset(ti_c54x_cpu *cpu)
{
    ti_c54x_error root_status;

    if (cpu == NULL || cpu->execution_active) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }

    cpu->execution_active = true;
    root_status = memory_status_to_error(
        ti_c54x_cpu_memory_reset_transaction(
            &cpu->instruction_transaction, &cpu->bus.data_endpoint));
    cpu->execution_active = false;
    if (root_status != TI_C54X_ERROR_OK) {
        return root_status;
    }

    if (ti_c54x_arch_state_reset_common(&cpu->arch_state) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    cpu->arch_state.pc_word = cpu->reset_pc_word;
    cpu->last_fetched_word = 0U;
    cpu->last_fetch_valid = false;
    cpu->instructions_attempted = 0U;
    cpu->instructions_retired = 0U;
    if (ti_c54x_sequencer_delayed_branch_init(&cpu->delayed_branch) !=
        TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    cpu->repeat_progress = (ti_c54x_repeat_progress){0};
    cpu->repeat_pending = (ti_c54x_repeat_pending_request){0};
    cpu->repeat_body_pc_word = UINT32_C(0);
    cpu->repeat_block_progress = (ti_c54x_repeat_block_progress){0};
    cpu->last_fetch_pc_word = UINT32_C(0);
    cpu->last_fetch_word_count = UINT8_C(0);
    if (ti_c54x_accumulator_reset(&cpu->accumulators) !=
        TI_C54X_ACCUMULATOR_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    if (ti_c54x_t_register_reset(&cpu->t_register) !=
        TI_C54X_T_REGISTER_OK) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    cpu->execution_active = true;
    TI_C54X_CPU_EMIT_TRACE(cpu, TI_C54X_TRACE_RESET);
    cpu->execution_active = false;
    return TI_C54X_ERROR_OK;
}

ti_c54x_error ti_c54x_cpu_inspect(const ti_c54x_cpu *cpu,
                                  ti_c54x_state *state)
{
    if (state == NULL) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    if (cpu == NULL || cpu->execution_active) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }

    state->pc_word = cpu->arch_state.pc_word;
    state->last_fetched_word = cpu->last_fetched_word;
    state->last_fetch_valid = cpu->last_fetch_valid;
    state->instructions_attempted_total = cpu->instructions_attempted;
    state->instructions_retired_total = cpu->instructions_retired;
    {
        const ti_c54x_status_register_field_result dp =
            ti_c54x_status_register_read_dp(&cpu->arch_state.status);
        const ti_c54x_status_register_field_result asm_field =
            ti_c54x_status_register_read_asm(&cpu->arch_state.status);
        const ti_c54x_status_register_field_result arp =
            ti_c54x_status_register_read_arp(&cpu->arch_state.status);
        const ti_c54x_status_register_field_result c_status =
            ti_c54x_status_register_read_c(&cpu->arch_state.status);
        const ti_c54x_status_register_field_result intm =
            ti_c54x_status_register_read_intm(&cpu->arch_state.status);
        uint16_t ifr_value = UINT16_C(0);
        const ti_c54x_arch_state_status ifr_status =
            ti_c54x_arch_state_read_ifr(&cpu->arch_state, &ifr_value);
        ti_c54x_accumulator_snapshot accumulators;
        const ti_c54x_accumulator_status accumulator_status =
            ti_c54x_accumulator_read_snapshot(&cpu->accumulators,
                                               &accumulators);
        if (accumulator_status != TI_C54X_ACCUMULATOR_OK &&
            accumulator_status != TI_C54X_ACCUMULATOR_UNKNOWN) {
            return TI_C54X_ERROR_INVALID_ARGUMENT;
        }
        state->dp = dp.raw_value;
        state->dp_known = dp.status == TI_C54X_STATUS_REGISTER_OK;
        state->asm_raw = asm_field.raw_value;
        state->asm_known = asm_field.status == TI_C54X_STATUS_REGISTER_OK;
        state->arp = (uint8_t)arp.raw_value;
        state->arp_known = arp.status == TI_C54X_STATUS_REGISTER_OK;
        state->c = (uint8_t)c_status.raw_value;
        state->c_known = c_status.status == TI_C54X_STATUS_REGISTER_OK;
        state->ifr_value = ifr_value;
        state->ifr_known = ifr_status == TI_C54X_ARCH_STATE_OK;
        state->intm_value = (uint8_t)intm.raw_value;
        state->intm_known = intm.status == TI_C54X_STATUS_REGISTER_OK;
        state->accumulator_a_raw40 = accumulators.a_raw40;
        state->accumulator_b_raw40 = accumulators.b_raw40;
        state->accumulator_a_known = accumulators.a_known;
        state->accumulator_b_known = accumulators.b_known;
    }
    return TI_C54X_ERROR_OK;
}
