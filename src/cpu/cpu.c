/* Module contract
 * Owns: lifecycle bookkeeping and orchestration of one caller-owned CPU
 * instance, including the copied bus descriptor, reset state, and lifecycle
 * event values.
 * Does not own: caller storage, bus memory, callback contexts, observer sink
 * copying/gating/emission, or opcode semantics. Those belong to their owning
 * modules.
 */

#include "cpu_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "instruction_fetch.h"
#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/opcode_dispatch.h"
#include "sequencer/program_pc_bridge.h"

#if TI_C54X_ENABLE_OBSERVERS
static void emit_trace(ti_c54x_cpu *cpu, ti_c54x_trace_kind kind)
{
    const uint64_t event_mask = ti_c54x_trace_mask_for_kind(kind);
    ti_c54x_trace_event event;

    if (!TI_C54X_OBSERVE_ENABLED(&cpu->observer, event_mask)) {
        return;
    }

    event.kind = kind;
    event.pc_word = cpu->pc_word;
    event.opcode_word = cpu->last_fetched_word;
    event.opcode_valid = cpu->last_fetch_valid;
    event.instructions_attempted_total = cpu->instructions_attempted;
    event.instructions_retired_total = cpu->instructions_retired;
    ti_c54x_observer_emit(&cpu->observer, &event);
}
#else
#define emit_trace(cpu, kind)                                                     \
    do {                                                                          \
        (void)(cpu);                                                              \
        (void)(kind);                                                             \
    } while (0)
#endif

static ti_c54x_run_result result_from_cpu(const ti_c54x_cpu *cpu,
                                          ti_c54x_stop_reason reason,
                                          ti_c54x_bus_status bus_status,
                                          uint64_t instructions_attempted,
                                          uint64_t instructions_retired)
{
    ti_c54x_run_result result;

    result.reason = reason;
    result.bus_status = bus_status;
    result.pc_word = cpu->pc_word;
    result.opcode_word = cpu->last_fetched_word;
    result.opcode_valid = cpu->last_fetch_valid;
    result.instructions_attempted = instructions_attempted;
    result.instructions_retired = instructions_retired;
    return result;
}

static ti_c54x_run_result invalid_result(void)
{
    ti_c54x_run_result result;

    result.reason = TI_C54X_STOP_INVALID_STATE;
    result.bus_status = TI_C54X_BUS_NO_CALLBACK;
    result.pc_word = 0U;
    result.opcode_word = 0U;
    result.opcode_valid = false;
    result.instructions_attempted = 0U;
    result.instructions_retired = 0U;
    return result;
}

static ti_c54x_run_result
stop_result(ti_c54x_cpu *cpu,
            ti_c54x_stop_reason reason,
            ti_c54x_bus_status bus_status,
            uint64_t instructions_attempted,
            uint64_t instructions_retired)
{
    ti_c54x_run_result result;

    emit_trace(cpu, TI_C54X_TRACE_STOP);
    result = result_from_cpu(cpu, reason, bus_status,
                             instructions_attempted, instructions_retired);
    cpu->execution_active = false;
    return result;
}

static ti_c54x_stop_reason
bridge_status_to_stop(ti_c54x_program_pc_bridge_status status)
{
    switch (status) {
    case TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER:
        return TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS;
    case TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY:
        return TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY;
    case TI_C54X_PROGRAM_PC_BRIDGE_OK:
    case TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT:
    case TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT:
    case TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT:
    case TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE:
    default:
        return TI_C54X_STOP_INVALID_STATE;
    }
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

    if (out_cpu == NULL) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    *out_cpu = NULL;
    if (storage == NULL || bus == NULL) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }
    if (storage_size < sizeof(ti_c54x_cpu)) {
        return TI_C54X_ERROR_STORAGE_TOO_SMALL;
    }
    if ((uintptr_t)storage % _Alignof(ti_c54x_cpu) != 0U) {
        return TI_C54X_ERROR_STORAGE_MISALIGNED;
    }

    cpu = (ti_c54x_cpu *)storage;
    /* Initialization is deliberately a deterministic overwrite.  The caller
     * may provide fresh, indeterminate storage, so no field is inspected
     * before this memset.  Reinitializing an existing instance is permitted
     * and releases no resources because the shell owns none. */
    memset(cpu, 0, sizeof(*cpu));
    cpu->reset_pc_word = config != NULL ? config->reset_pc_word : 0U;
    cpu->bus = *bus;
    ti_c54x_observer_init(&cpu->observer,
                          config != NULL ? &config->trace : NULL);
    cpu->pc_word = cpu->reset_pc_word;
    *out_cpu = cpu;
    cpu->execution_active = true;
    emit_trace(cpu, TI_C54X_TRACE_RESET);
    cpu->execution_active = false;
    return TI_C54X_ERROR_OK;
}

void ti_c54x_cpu_deinit(ti_c54x_cpu *cpu)
{
    if (cpu == NULL || cpu->execution_active) {
        return;
    }
    memset(cpu, 0, sizeof(*cpu));
}

ti_c54x_error ti_c54x_cpu_reset(ti_c54x_cpu *cpu)
{
    if (cpu == NULL || cpu->execution_active) {
        return TI_C54X_ERROR_INVALID_ARGUMENT;
    }

    cpu->pc_word = cpu->reset_pc_word;
    cpu->last_fetched_word = 0U;
    cpu->last_fetch_valid = false;
    cpu->instructions_attempted = 0U;
    cpu->instructions_retired = 0U;
    cpu->execution_active = true;
    emit_trace(cpu, TI_C54X_TRACE_RESET);
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

    state->pc_word = cpu->pc_word;
    state->last_fetched_word = cpu->last_fetched_word;
    state->last_fetch_valid = cpu->last_fetch_valid;
    state->instructions_attempted_total = cpu->instructions_attempted;
    state->instructions_retired_total = cpu->instructions_retired;
    return TI_C54X_ERROR_OK;
}

ti_c54x_run_result ti_c54x_cpu_step(ti_c54x_cpu *cpu)
{
    return ti_c54x_cpu_run(cpu, 1U);
}

ti_c54x_run_result ti_c54x_cpu_run(ti_c54x_cpu *cpu,
                                   uint64_t instruction_budget)
{
    uint64_t instructions_attempted = 0U;
    uint64_t instructions_retired = 0U;

    if (cpu == NULL || cpu->execution_active) {
        return invalid_result();
    }

    if (instruction_budget == 0U) {
        return result_from_cpu(cpu, TI_C54X_STOP_BUDGET_EXHAUSTED,
                               TI_C54X_BUS_OK, 0U, 0U);
    }

    cpu->execution_active = true;
    while (instructions_retired < instruction_budget) {
        uint16_t opcode_word = 0U;
        ti_c54x_bus_status bus_status;
        ti_c54x_decode_result first_word_result;
        ti_c54x_instruction_extension_fetch_result fetched;
        ti_c54x_decoded_instruction instruction;
        ti_c54x_instruction_completion_status completion_status;
        ti_c54x_op_effect effect = {0};
        ti_c54x_program_pc_bridge_effect pc_effect;
        ti_c54x_program_pc_bridge_status bridge_status;
        ti_c54x_op_status op_status;

        /* Per-call deltas start at zero and never exceed their cumulative
         * counterparts, so this preflight also proves the local increments
         * below are representable. */
        if (cpu->instructions_attempted == UINT64_MAX ||
            cpu->instructions_retired == UINT64_MAX) {
            return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* The attempt is accounted for immediately before the first bus read.
         * This keeps failed reads observable as attempts without ever making
         * them retirements. */
        cpu->instructions_attempted++;
        instructions_attempted++;
        if (cpu->bus.program_read_word == NULL) {
            bus_status = TI_C54X_BUS_NO_CALLBACK;
        } else {
            bus_status = cpu->bus.program_read_word(cpu->bus.context,
                                                    cpu->pc_word,
                                                    &opcode_word);
        }
        if (bus_status != TI_C54X_BUS_OK) {
            cpu->last_fetched_word = 0U;
            cpu->last_fetch_valid = false;
            return stop_result(cpu, TI_C54X_STOP_BUS_FAULT, bus_status,
                               instructions_attempted,
                               instructions_retired);
        }

        cpu->last_fetched_word = opcode_word;
        cpu->last_fetch_valid = true;
        emit_trace(cpu, TI_C54X_TRACE_FETCH);
        first_word_result = ti_c54x_decode_first_word(opcode_word);
        if (first_word_result.status != TI_C54X_DECODE_STATUS_OK) {
            return stop_result(cpu, TI_C54X_STOP_UNIMPLEMENTED_OPCODE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        fetched = ti_c54x_instruction_extension_fetch(
            &cpu->bus, cpu->pc_word, opcode_word,
            first_word_result.program_word_count);
        if (fetched.status ==
            TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT) {
            return stop_result(cpu, TI_C54X_STOP_BUS_FAULT,
                               fetched.bus_status, instructions_attempted,
                               instructions_retired);
        }
        if (fetched.status != TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        completion_status = ti_c54x_decode_complete_instruction(
            &first_word_result, &fetched.bundle, &instruction);
        if (completion_status !=
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        op_status = ti_c54x_dispatch_decoded(&instruction, &effect);
        if (op_status != TI_C54X_OP_STATUS_OK ||
            effect.kind != TI_C54X_OP_EFFECT_NONE) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        bridge_status = ti_c54x_program_pc_bridge_plan(
            cpu->pc_word, instruction.bundle.program_word_count, &pc_effect);
        if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return stop_result(cpu, bridge_status_to_stop(bridge_status),
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        bridge_status = ti_c54x_program_pc_bridge_apply(&cpu->pc_word,
                                                        &pc_effect);
        if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        cpu->instructions_retired++;
        instructions_retired++;
    }

    cpu->execution_active = false;
    return result_from_cpu(cpu, TI_C54X_STOP_BUDGET_EXHAUSTED,
                           TI_C54X_BUS_OK, instructions_attempted,
                           instructions_retired);
}
