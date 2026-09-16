/* Module contract
 * Owns: public step/run execution, per-call result construction, fetch/decode
 * orchestration, and the ordinary opcode execution loop. It resumes the
 * embedded memory root before fetching and accounts root deltas exactly once.
 * It also adopts deferred-branch reports into the CPU-owned two-word
 * countdown, retires delayed PC writes with sequential advance plus
 * countdown arming, retires auxiliary-register branches with a
 * CMPT=0 gate, flagged AR writeback, and taken/delay-selected PC
 * commit, and advances that countdown after retirements that arm
 * nothing new.
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
#include "sequencer/program_pc_target.h"
#include "repeat_activate.h"

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

static ti_c54x_stop_reason
target_status_to_stop(ti_c54x_program_pc_target_status status)
{
    switch (status) {
    case TI_C54X_PROGRAM_PC_TARGET_UNSUPPORTED_CARRIER:
        return TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS;
    case TI_C54X_PROGRAM_PC_TARGET_OK:
    case TI_C54X_PROGRAM_PC_TARGET_INVALID_ARGUMENT:
    case TI_C54X_PROGRAM_PC_TARGET_INVALID_EFFECT:
    case TI_C54X_PROGRAM_PC_TARGET_STALE_SOURCE:
    default:
        return TI_C54X_STOP_INVALID_STATE;
    }
}

/* Adopt an armed deferred-branch report from a retired MEMORY root,
 * replacing any pending countdown. The arming instruction consumes no
 * delay-slot words itself. Arming a live instance cannot fail. */
static void adopt_deferred_branch(
    ti_c54x_cpu *cpu, const ti_c54x_memory_instruction_result *result)
{
    if (result->deferred_branch_armed) {
        (void)ti_c54x_sequencer_delayed_branch_arm(
            &cpu->delayed_branch, result->deferred_branch_pc_word);
    }
}

/* An inner PC redirection cancels the pending countdown: a non-delayed
 * stack branch or PC_WRITE effect inside the delay slots wins under the
 * documented last-wins rule (TI forbids these outright). */
static bool memory_effect_redirects_pc(const ti_c54x_op_effect *effect)
{
    ti_c54x_op_memory_address_kind kind;

    if (effect == NULL || effect->kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    kind = effect->payload.memory.address_spec.kind;
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16;
}

/* Maintain the pending countdown after a retirement that armed nothing
 * new. Reports through took_effect whether the deferred target now
 * applies; the caller publishes it to the live PC. A zero word count
 * is rejected without mutation. */
static ti_c54x_sequencer_delayed_branch_status advance_delayed_branch(
    ti_c54x_cpu *cpu, uint8_t word_count, bool redirected_pc,
    bool *took_effect)
{
    if (!ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch)) {
        *took_effect = false;
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
    }
    if (redirected_pc) {
        (void)ti_c54x_sequencer_delayed_branch_disarm(&cpu->delayed_branch);
        *took_effect = false;
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
    }
    return ti_c54x_sequencer_delayed_branch_consume(
        &cpu->delayed_branch, word_count, took_effect);
}

/* Publish a taken deferred branch to the live PC. The countdown keeps
 * the 16-bit deferred target through the take-effect consume. */
static void apply_deferred_branch(ti_c54x_cpu *cpu)
{
    cpu->arch_state.pc_word =
        (ti_c54x_program_word_address)cpu->delayed_branch.deferred_pc_word;
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
            adopt_deferred_branch(cpu, &memory_result);
            if (!memory_result.deferred_branch_armed) {
                const uint8_t retired_words =
                    cpu->instruction_transaction.candidate.instruction.bundle
                        .program_word_count;
                const bool retired_redirects = memory_effect_redirects_pc(
                    &cpu->instruction_transaction.candidate.effect);
                bool took_effect = false;

                if (advance_delayed_branch(cpu, retired_words,
                                           retired_redirects,
                                           &took_effect) !=
                    TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                    return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                if (took_effect) {
                    apply_deferred_branch(cpu);
                }
            }
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

        /* Single-repeat predecode: only ELIGIBLE bodies execute
         * while ACTIVE (wired in repeat_activate.c). */
        if (ti_c54x_cpu_repeat_predecode_blocks(
                cpu, binding->metadata.repeatability)) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
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
                &(ti_c54x_input_sources){
                    .accumulators = &cpu->accumulators,
                    .status = &cpu->arch_state.status,
                    .arch_state = &cpu->arch_state,
                    .t_register = &cpu->t_register,
                },
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

        /* The sequential successor is prepared once per retirement for
         * every effect kind; the ordinary and delayed-PC routes commit
         * it while the MEMORY and immediate-PC routes ignore it. The
         * trusted hot path keeps a single preparation plan: failure is
         * reported only by the routes that advance sequentially, so
         * top-of-memory MEMORY and immediate branches keep their
         * existing behavior. Folding apply into the shared status is
         * exact: every apply failure maps through bridge_status_to_stop
         * to the same INVALID_STATE the split checks produced. */
        bridge_status = ti_c54x_program_pc_bridge_plan_trusted(
            cpu->arch_state.pc_word,
            packet.instruction.bundle.program_word_count,
            &pc_effect);
        candidate_pc = cpu->arch_state.pc_word;
        if (bridge_status == TI_C54X_PROGRAM_PC_BRIDGE_OK &&
            ti_c54x_program_pc_bridge_apply_trusted(
                &candidate_pc, &pc_effect) !=
                TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            bridge_status = TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE;
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
            adopt_deferred_branch(cpu, &memory_result);
            if (!memory_result.deferred_branch_armed) {
                const bool retired_redirects =
                    memory_effect_redirects_pc(&effect);
                bool took_effect = false;

                if (advance_delayed_branch(
                        cpu, packet.instruction.bundle.program_word_count,
                        retired_redirects, &took_effect) !=
                    TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                    return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                if (took_effect) {
                    apply_deferred_branch(cpu);
                }
            }
            if (!memory_status_ok(ti_c54x_cpu_memory_reset_transaction(
                    &cpu->instruction_transaction,
                    &cpu->bus.data_endpoint))) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        /* PC_WRITE effects carry no status, accumulator, or T-register
         * payload, so they bypass the effect transaction and the
         * sequential bridge plan. The target planner validates the
         * absolute successor and the apply step commits it only when
         * the live carrier still matches the frozen source, which is
         * the whole atomic commit for this effect kind. A PC_WRITE
         * inside delay slots is TI-forbidden; the inner redirect
         * deterministically cancels any pending countdown. */
        if (effect.kind == TI_C54X_OP_EFFECT_PC_WRITE) {
            ti_c54x_program_pc_target_effect target_effect;
            ti_c54x_program_pc_target_status target_status;
            bool took_effect = false;

            target_status = ti_c54x_program_pc_target_plan(
                cpu->arch_state.pc_word, effect.payload.pc_write.pmad,
                &target_effect);
            if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                return stop_result(cpu, target_status_to_stop(target_status),
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            candidate_pc = cpu->arch_state.pc_word;
            target_status = ti_c54x_program_pc_target_apply(
                &candidate_pc, &target_effect);
            if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->arch_state.pc_word = candidate_pc;
            cpu->instructions_retired++;
            instructions_retired++;
            if (advance_delayed_branch(
                    cpu, packet.instruction.bundle.program_word_count,
                    true, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        /* Delayed PC writes retire with sequential advance and arm the
         * CPU-owned countdown with the copied pmad, replacing any
         * pending countdown under the last-wins rule; the arming
         * instruction consumes no delay-slot words itself. The target
         * planner validates the absolute successor before any commit;
         * a full 16-bit pmad is always encodable, so planning fails
         * only on caller-argument defects. A delayed branch inside
         * delay slots is TI-forbidden; the inner arming wins
         * deterministically here. No MEMORY endpoint, no WAIT/FAULT,
         * and no status/accumulator/T effect is admitted. */
        if (effect.kind == TI_C54X_OP_EFFECT_DELAYED_PC_WRITE) {
            ti_c54x_program_pc_target_effect target_effect;
            ti_c54x_program_pc_target_status target_status;

            target_status = ti_c54x_program_pc_target_plan(
                cpu->arch_state.pc_word,
                effect.payload.delayed_pc_write.pmad, &target_effect);
            if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                return stop_result(cpu, target_status_to_stop(target_status),
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                return stop_result(cpu, bridge_status_to_stop(bridge_status),
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->arch_state.pc_word = candidate_pc;
            cpu->instructions_retired++;
            instructions_retired++;
            (void)ti_c54x_sequencer_delayed_branch_arm(
                &cpu->delayed_branch,
                effect.payload.delayed_pc_write.pmad);
            continue;
        }

        /* Auxiliary-register branches carry the selected AR with its
         * staged value-after, an update flag, the taken outcome, the
         * delayed discriminator, and the copied pmad. The route gates
         * CMPT=0 under the repo operating condition, plans the
         * absolute target for taken branches, and requires the
         * sequential bridge for not-taken and taken-delayed
         * retirements. The AR write commits when flagged; taken
         * non-delayed branches commit the planned target
         * immediately, taken-delayed branches advance sequentially
         * and arm the CPU-owned countdown, and not-taken branches
         * advance sequentially. Any taken branch inside delay slots
         * redirects deterministically under the last-wins rule. No
         * MEMORY endpoint, no WAIT/FAULT, and no status,
         * accumulator, or T effect is admitted. */
        if (effect.kind == TI_C54X_OP_EFFECT_BRANCH_AUXILIARY) {
            const ti_c54x_op_branch_auxiliary_effect *auxiliary =
                &effect.payload.branch_auxiliary;
            ti_c54x_program_pc_target_effect target_effect;
            ti_c54x_program_pc_target_status target_status;
            uint8_t cmpt = UINT8_C(0);
            bool took_effect = false;

            if (auxiliary->ar_index >= UINT8_C(8)) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (ti_c54x_arch_state_read_cmpt(&cpu->arch_state, &cmpt) !=
                    TI_C54X_ARCH_STATE_OK ||
                cmpt != UINT8_C(0)) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (auxiliary->taken) {
                target_status = ti_c54x_program_pc_target_plan(
                    cpu->arch_state.pc_word, auxiliary->pmad,
                    &target_effect);
                if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                    return stop_result(
                        cpu, target_status_to_stop(target_status),
                        TI_C54X_BUS_OK, instructions_attempted,
                        instructions_retired);
                }
            }
            if (!auxiliary->taken || auxiliary->delayed) {
                if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                    return stop_result(
                        cpu, bridge_status_to_stop(bridge_status),
                        TI_C54X_BUS_OK, instructions_attempted,
                        instructions_retired);
                }
            }
            if (auxiliary->update &&
                ti_c54x_arch_state_write_ar(
                    &cpu->arch_state, auxiliary->ar_index,
                    auxiliary->value_after) != TI_C54X_ARCH_STATE_OK) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (auxiliary->taken && !auxiliary->delayed) {
                candidate_pc = cpu->arch_state.pc_word;
                target_status = ti_c54x_program_pc_target_apply(
                    &candidate_pc, &target_effect);
                if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                    return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK,
                                       instructions_attempted,
                                       instructions_retired);
                }
                cpu->arch_state.pc_word = candidate_pc;
            } else {
                cpu->arch_state.pc_word = candidate_pc;
            }
            cpu->instructions_retired++;
            instructions_retired++;
            if (auxiliary->taken && auxiliary->delayed) {
                (void)ti_c54x_sequencer_delayed_branch_arm(
                    &cpu->delayed_branch, auxiliary->pmad);
                continue;
            }
            if (advance_delayed_branch(
                    cpu, packet.instruction.bundle.program_word_count,
                    auxiliary->taken, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (took_effect) {
                apply_deferred_branch(cpu);
            }
            continue;
        }

        /* RPT retires through the single-repeat wiring in
         * repeat_activate.c; the HOLD tails land with the first
         * ELIGIBLE body. */
        {
            const ti_c54x_cpu_repeat_route_status route_status =
                ti_c54x_cpu_repeat_route(cpu, &effect, bridge_status,
                                         candidate_pc);
            if (route_status != TI_C54X_CPU_REPEAT_ROUTE_NOT_REPEAT) {
                if (route_status != TI_C54X_CPU_REPEAT_ROUTE_RETIRED) {
                    return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                cpu->instructions_retired++;
                instructions_retired++;
                continue;
            }
        }

        if (ti_c54x_effect_transaction_stage(
                &effect, &cpu->arch_state.status, &cpu->accumulators,
                &cpu->t_register, &transaction) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
            return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return stop_result(cpu, bridge_status_to_stop(bridge_status),
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
        /* Ordinary effects always advance sequentially, so a retired
         * delay-slot instruction consumes its word count here. */
        {
            bool took_effect = false;

            if (advance_delayed_branch(
                    cpu, packet.instruction.bundle.program_word_count,
                    false, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (took_effect) {
                apply_deferred_branch(cpu);
            }
        }
    }

    cpu->execution_active = false;
    return result_from_cpu(cpu, TI_C54X_STOP_BUDGET_EXHAUSTED,
                           TI_C54X_BUS_OK, instructions_attempted,
                           instructions_retired);
}
