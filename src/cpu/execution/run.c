/* Module contract
 * Owns: public step/run execution, per-call result construction, fetch/decode
 * orchestration, and the ordinary opcode execution loop. It resumes the
 * embedded memory root before fetching and accounts root deltas exactly once.
 * Does not own: CPU lifetime/storage, bus descriptor validation, lifecycle
 * cancellation, trace assembly, or opcode semantics.
 */

#include "../cpu_internal.h"

#include <stdint.h>

#include "../instruction_fetch.h"
#include "../input/collect.h"
#include "../memory/pending_work.h"
#include "../trace/emit.h"
#include "isa/decode/trusted_execution_packet.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/opcode_dispatch.h"
#include "isa/ops/dispatch/trusted_opcode_binding.h"
#include "sequencer/effect_transaction.h"
#include "sequencer/program_pc_bridge.h"

static ti_c54x_run_result result_from_cpu(const ti_c54x_cpu *cpu,
                                          ti_c54x_stop_reason reason,
                                          ti_c54x_bus_status bus_status,
                                          uint64_t instructions_attempted,
                                          uint64_t instructions_retired)
{
    ti_c54x_run_result result;

    result.reason = reason;
    result.bus_status = bus_status;
    result.pc_word = cpu->arch_state.pc_word;
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

    TI_C54X_CPU_EMIT_TRACE(cpu, TI_C54X_TRACE_STOP);
    result = result_from_cpu(cpu, reason, bus_status,
                             instructions_attempted, instructions_retired);
    cpu->execution_active = false;
    return result;
}

static ti_c54x_run_result
wait_result(ti_c54x_cpu *cpu,
            uint64_t instructions_attempted,
            uint64_t instructions_retired)
{
    ti_c54x_run_result result = result_from_cpu(
        cpu, TI_C54X_STOP_BUS_WAIT, TI_C54X_BUS_OK,
        instructions_attempted, instructions_retired);

    /* BUS_WAIT is a resumable work-unit suspension, not a terminal stop. */
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

static bool memory_status_ok(ti_c54x_memory_instruction_status status)
{
    return status == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
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
        const ti_c54x_trusted_opcode_binding *binding;
        ti_c54x_instruction_extension_fetch_result fetched;
        ti_c54x_trusted_execution_packet packet;
        ti_c54x_op_effect effect = {0};
        ti_c54x_effect_transaction transaction;
        ti_c54x_program_pc_bridge_effect pc_effect;
        ti_c54x_program_pc_bridge_status bridge_status;
        ti_c54x_op_status op_status;
        ti_c54x_program_word_address candidate_pc;
        ti_c54x_op_input_snapshot op_input = {0};
        ti_c54x_memory_instruction_result memory_result = {0};
        ti_c54x_memory_instruction_status memory_status;
        ti_c54x_cpu_memory_resume_result memory_resume;

        /* A prepared or pending root transaction owns the current work unit.
         * Resume it before fetching any new program word. */
        if (ti_c54x_cpu_memory_is_pending(
                &cpu->instruction_transaction)) {
            const ti_c54x_cpu_memory_resume_result resume =
                ti_c54x_cpu_memory_resume(
                    &cpu->instruction_transaction, &cpu->bus.data_endpoint,
                    &cpu->arch_state, &cpu->accumulators, &cpu->t_register);
            memory_status = resume.transaction_status;
            memory_result = resume.result;
            if (!memory_status_ok(memory_status)) {
                if (memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
                    memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
                    return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.instructions_attempted >
                    UINT64_MAX - cpu->instructions_attempted ||
                memory_result.instructions_attempted >
                    UINT64_MAX - instructions_attempted) {
                return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->instructions_attempted += memory_result.instructions_attempted;
            instructions_attempted += memory_result.instructions_attempted;
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT) {
                return wait_result(cpu, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT) {
                return stop_result(cpu, TI_C54X_STOP_BUS_FAULT,
                                   memory_result.bus_status,
                                   instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status !=
                TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.instructions_retired >
                    UINT64_MAX - cpu->instructions_retired ||
                memory_result.instructions_retired >
                    UINT64_MAX - instructions_retired) {
                return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->instructions_retired += memory_result.instructions_retired;
            instructions_retired += memory_result.instructions_retired;
            if (!memory_status_ok(ti_c54x_cpu_memory_reset_transaction(
                    &cpu->instruction_transaction,
                    &cpu->bus.data_endpoint))) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        /* A terminal root can only be left by a caller-visible reset. */
        if (cpu->instruction_transaction.phase ==
            TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        if (cpu->instruction_transaction.phase ==
                TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE &&
            !memory_status_ok(ti_c54x_cpu_memory_reset_transaction(
                &cpu->instruction_transaction, &cpu->bus.data_endpoint))) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (cpu->instructions_attempted == UINT64_MAX ||
            cpu->instructions_retired == UINT64_MAX) {
            return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* Account an attempt immediately before the first bus read. */
        cpu->instructions_attempted++;
        instructions_attempted++;
        if (cpu->bus.program_read_word == NULL) {
            bus_status = TI_C54X_BUS_NO_CALLBACK;
        } else {
            bus_status = cpu->bus.program_read_word(cpu->bus.context,
                                                    cpu->arch_state.pc_word,
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
        TI_C54X_CPU_EMIT_TRACE(cpu, TI_C54X_TRACE_FETCH);
        binding = ti_c54x_trusted_opcode_binding_for_first_word(opcode_word);
        if (binding == NULL ||
            binding->metadata.id == TI_C54X_OPCODE_ID_UNKNOWN) {
            return stop_result(cpu, TI_C54X_STOP_UNIMPLEMENTED_OPCODE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (binding->metadata.program_word_count == UINT8_C(1)) {
            fetched = (ti_c54x_instruction_extension_fetch_result){
                .status = TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK,
                .bus_status = TI_C54X_BUS_OK,
                .bundle = {{opcode_word, UINT16_C(0)}, UINT8_C(1)}};
        } else {
            fetched = ti_c54x_instruction_extension_fetch(
                &cpu->bus, cpu->arch_state.pc_word, opcode_word,
                binding->metadata.program_word_count);
        }
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

        if (ti_c54x_trusted_execution_packet_prepare_fast(
                binding, &fetched.bundle, &packet) !=
            TI_C54X_TRUSTED_EXECUTION_PACKET_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (packet.instruction.input_resources !=
                TI_C54X_INPUT_RESOURCES_NONE &&
            ti_c54x_input_collect(
                packet.instruction.input_resources,
                &(ti_c54x_input_sources){&cpu->accumulators,
                                         &cpu->arch_state.status},
                &op_input) != TI_C54X_INPUT_COLLECT_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        op_status = ti_c54x_dispatch_trusted(&packet, &op_input, &effect);
        if (op_status != TI_C54X_OP_STATUS_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* MEMORY effects own the persistent root transaction. The fetch-side
         * attempt was accounted immediately before the program read, so the
         * root receives the pre-attempt totals and publishes only its READY
         * retirement delta. Routing is by effect kind; opcode identity stays
         * inside the trusted semantic handler and completion callback. */
        if (effect.kind == TI_C54X_OP_EFFECT_MEMORY) {
            if (cpu->instructions_attempted == UINT64_C(0)) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            memory_status = ti_c54x_memory_instruction_transaction_prepare(
                &cpu->instruction_transaction, &packet.instruction, &op_input,
                &effect, &cpu->arch_state,
                cpu->instructions_attempted - UINT64_C(1),
                cpu->instructions_retired);
            if (!memory_status_ok(memory_status)) {
                if (memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
                    memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
                    return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }

            memory_resume = ti_c54x_cpu_memory_resume(
                &cpu->instruction_transaction, &cpu->bus.data_endpoint,
                &cpu->arch_state, &cpu->accumulators, &cpu->t_register);
            if (!memory_status_ok(memory_resume.transaction_status)) {
                if (memory_resume.transaction_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
                    memory_resume.transaction_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
                    return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            memory_result = memory_resume.result;
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT) {
                return wait_result(cpu, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT) {
                return stop_result(cpu, TI_C54X_STOP_BUS_FAULT,
                                   memory_result.bus_status,
                                   instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status !=
                TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.instructions_retired != UINT64_C(1) ||
                cpu->instructions_retired == UINT64_MAX) {
                return stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->instructions_retired++;
            instructions_retired++;
            if (!memory_status_ok(ti_c54x_cpu_memory_reset_transaction(
                    &cpu->instruction_transaction,
                    &cpu->bus.data_endpoint))) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        if (ti_c54x_effect_transaction_stage(
                &effect, &cpu->arch_state.status, &cpu->accumulators,
                &cpu->t_register, &transaction) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        candidate_pc = cpu->arch_state.pc_word;

        bridge_status = ti_c54x_program_pc_bridge_plan_trusted(
            cpu->arch_state.pc_word,
            packet.instruction.bundle.program_word_count,
            &pc_effect);
        if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return stop_result(cpu, bridge_status_to_stop(bridge_status),
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        bridge_status = ti_c54x_program_pc_bridge_apply_trusted(
            &candidate_pc, &pc_effect);
        if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (ti_c54x_effect_transaction_commit(
                &transaction, &cpu->arch_state.status, &cpu->accumulators,
                &cpu->t_register) != TI_C54X_EFFECT_TRANSACTION_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        cpu->arch_state.pc_word = candidate_pc;
        cpu->instructions_retired++;
        instructions_retired++;
    }

    cpu->execution_active = false;
    return result_from_cpu(cpu, TI_C54X_STOP_BUDGET_EXHAUSTED,
                           TI_C54X_BUS_OK, instructions_attempted,
                           instructions_retired);
}
