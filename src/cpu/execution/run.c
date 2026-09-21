/* Module contract
 * Owns: public step/run execution, fetch/decode orchestration, and
 * the ordinary opcode execution loop. It resumes the embedded
 * memory root before fetching and accounts root deltas exactly
 * once. Result/stop construction and deferred-branch plumbing live
 * in run_support; repeat wiring lives in repeat_activate.
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
#include "sequencer/stack/sp_write_apply.h"
#include "sequencer/auxiliary/ar_write_apply.h"
#include "sequencer/program_pc_bridge.h"
#include "sequencer/program_pc_target.h"
#include "sequencer/reset/reset_apply.h"
#include "repeat_activate.h"
#include "run_support.h"
#include "take_issue.h"

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
        return ti_c54x_run_invalid_result();
    }

    if (instruction_budget == 0U) {
        return ti_c54x_run_result_from_cpu(cpu, TI_C54X_STOP_BUDGET_EXHAUSTED,
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
        uint8_t bridge_words;
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
            if (!ti_c54x_run_memory_status_ok(memory_status)) {
                if (memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
                    memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.instructions_attempted >
                    UINT64_MAX - cpu->instructions_attempted ||
                memory_result.instructions_attempted >
                    UINT64_MAX - instructions_attempted) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->instructions_attempted += memory_result.instructions_attempted;
            instructions_attempted += memory_result.instructions_attempted;
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT) {
                return ti_c54x_run_wait_result(cpu, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_BUS_FAULT,
                                   memory_result.bus_status,
                                   instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status !=
                TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.instructions_retired >
                    UINT64_MAX - cpu->instructions_retired ||
                memory_result.instructions_retired >
                    UINT64_MAX - instructions_retired) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->instructions_retired += memory_result.instructions_retired;
            instructions_retired += memory_result.instructions_retired;
            if (!ti_c54x_cpu_take_note_resumed(cpu)) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (!ti_c54x_run_memory_retirement_tail(
                    cpu, &memory_result,
                    cpu->instruction_transaction.candidate.instruction.bundle
                        .program_word_count,
                    ti_c54x_run_memory_effect_redirects_pc(
                        &cpu->instruction_transaction.candidate.effect))) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (!ti_c54x_run_route_pending_repeat_start(cpu, &memory_result)) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        /* A terminal root can only be left by a caller-visible reset. */
        if (cpu->instruction_transaction.phase ==
            TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        if (cpu->instruction_transaction.phase ==
                TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE &&
            !ti_c54x_run_memory_status_ok(ti_c54x_cpu_memory_reset_transaction(
                &cpu->instruction_transaction, &cpu->bus.data_endpoint))) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (cpu->instructions_attempted == UINT64_MAX ||
            cpu->instructions_retired == UINT64_MAX) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* NMI takes preempt fetch at the instruction
         * boundary; deferred edges fall through to the fetch
         * below. See take_issue for the arbitration and the
         * root producer. */
        {
            ti_c54x_cpu_take_issue_result take_issue =
                ti_c54x_cpu_take_issue(cpu, &instructions_attempted,
                                       &instructions_retired);
            if (take_issue.outcome == TI_C54X_CPU_TAKE_ISSUE_WAIT) {
                return ti_c54x_run_wait_result(cpu, instructions_attempted,
                                              instructions_retired);
            }
            if (take_issue.outcome == TI_C54X_CPU_TAKE_ISSUE_STOP) {
                return ti_c54x_run_stop_result(cpu, take_issue.stop_reason,
                                              take_issue.bus_status,
                                              instructions_attempted,
                                              instructions_retired);
            }
            if (take_issue.outcome == TI_C54X_CPU_TAKE_ISSUE_CONTINUE) {
                continue;
            }
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
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_BUS_FAULT, bus_status,
                               instructions_attempted,
                               instructions_retired);
        }

        cpu->last_fetched_word = opcode_word;
        cpu->last_fetch_valid = true;
        cpu->last_fetch_pc_word = cpu->arch_state.pc_word;
        TI_C54X_CPU_EMIT_TRACE(cpu, TI_C54X_TRACE_FETCH);
        binding = ti_c54x_trusted_opcode_binding_for_first_word(opcode_word);
        if (binding == NULL ||
            binding->metadata.id == TI_C54X_OPCODE_ID_UNKNOWN) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_UNIMPLEMENTED_OPCODE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* Single-repeat predecode: only ELIGIBLE bodies execute
         * while ACTIVE (wired in repeat_activate.c). */
        if (ti_c54x_cpu_repeat_predecode_blocks(
                cpu, binding->metadata.repeatability)) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (binding->metadata.program_word_count == UINT8_C(1)) {
            fetched = (ti_c54x_instruction_extension_fetch_result){
                .status = TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK,
                .bus_status = TI_C54X_BUS_OK,
                .bundle = {{opcode_word, UINT16_C(0), UINT16_C(0)},
                             UINT8_C(1)}};
        } else {
            fetched = ti_c54x_instruction_extension_fetch(
                &cpu->bus, cpu->arch_state.pc_word, opcode_word,
                binding->metadata.program_word_count);
        }
        if (fetched.status ==
            TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_BUS_FAULT) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_BUS_FAULT,
                               fetched.bus_status, instructions_attempted,
                               instructions_retired);
        }
        if (fetched.status != TI_C54X_INSTRUCTION_EXTENSION_FETCH_STATUS_OK) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (ti_c54x_trusted_execution_packet_prepare_fast(
                binding, &fetched.bundle, &packet) !=
            TI_C54X_TRUSTED_EXECUTION_PACKET_OK) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        cpu->last_fetch_word_count =
            packet.instruction.bundle.program_word_count;

        if (packet.instruction.input_resources !=
                TI_C54X_INPUT_RESOURCES_NONE &&
            ti_c54x_input_collect(
                packet.instruction.input_resources,
                &(ti_c54x_input_sources){
                    .accumulators = &cpu->accumulators,
                    .status = &cpu->arch_state.status,
                    .arch_state = &cpu->arch_state,
                    .t_register = &cpu->t_register,
                    .mmr_file = &cpu->arch_state.mmr_file,
                },
                &op_input) != TI_C54X_INPUT_COLLECT_OK) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        op_status = ti_c54x_dispatch_trusted(&packet, &op_input, &effect);
        if (op_status != TI_C54X_OP_STATUS_OK) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* IDLE effects wait dormant until interrupt. The library
         * owns no interrupt source, so retiring through the wait
         * would invent control flow the hardware never takes:
         * the wait surfaces as the reserved IDLE stop with the
         * attempt counted, nothing retired, and the pc held. */
        if (effect.kind == TI_C54X_OP_EFFECT_IDLE) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_IDLE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        /* RESET effects retire through the reset tail: unknown
         * IPTR stops with nothing retired, otherwise one
         * instruction retires. */
        if (effect.kind == TI_C54X_OP_EFFECT_RESET) {
            if (!ti_c54x_run_retire_reset(
                    cpu,
                    packet.instruction.bundle.program_word_count)) {
                return ti_c54x_run_stop_result(
                    cpu, TI_C54X_STOP_INVALID_STATE, TI_C54X_BUS_OK,
                    instructions_attempted, instructions_retired);
            }
            cpu->instructions_retired++;
            instructions_retired++;
            continue;
        }

        /* The sequential successor is prepared once per retirement for
         * every effect kind; the ordinary and delayed-PC routes commit
         * it while the MEMORY and immediate-PC routes ignore it. The
         * trusted hot path keeps a single preparation plan: failure is
         * reported only by the routes that advance sequentially, so
         * top-of-memory MEMORY and immediate branches keep their
         * existing behavior. Not-taken XC widens this same plan past
         * the skipped words (1 + n instead of the bare instruction
         * width), so no second plan site is admitted. Folding apply
         * into the shared status is exact: every apply failure maps
         * through ti_c54x_run_bridge_status_to_stop
         * to the same INVALID_STATE the split checks produced. */
        bridge_words = packet.instruction.bundle.program_word_count;
        if (effect.kind == TI_C54X_OP_EFFECT_CONDITIONAL_SKIP &&
            effect.payload.conditional_skip.skip_words >= UINT8_C(1) &&
            effect.payload.conditional_skip.skip_words <= UINT8_C(2)) {
            bridge_words =
                (uint8_t)(bridge_words +
                          effect.payload.conditional_skip.skip_words);
        }
        bridge_status = ti_c54x_program_pc_bridge_plan_trusted(
            cpu->arch_state.pc_word, bridge_words, &pc_effect);
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
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            memory_status = ti_c54x_memory_instruction_transaction_prepare(
                &cpu->instruction_transaction, &packet.instruction, &op_input,
                &effect, &cpu->arch_state,
                cpu->instructions_attempted - UINT64_C(1),
                cpu->instructions_retired);
            if (!ti_c54x_run_memory_status_ok(memory_status)) {
                if (memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
                    memory_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }

            memory_resume = ti_c54x_cpu_memory_resume(
                &cpu->instruction_transaction, &cpu->bus.data_endpoint,
                &cpu->arch_state, &cpu->accumulators, &cpu->t_register);
            if (!ti_c54x_run_memory_status_ok(memory_resume.transaction_status)) {
                if (memory_resume.transaction_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
                    memory_resume.transaction_status ==
                        TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            memory_result = memory_resume.result;
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT) {
                return ti_c54x_run_wait_result(cpu, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status ==
                TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_BUS_FAULT,
                                   memory_result.bus_status,
                                   instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.status !=
                TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (memory_result.instructions_retired != UINT64_C(1) ||
                cpu->instructions_retired == UINT64_MAX) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_COUNTER_OVERFLOW,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->instructions_retired++;
            instructions_retired++;
            if (!ti_c54x_run_memory_retirement_tail(
                    cpu, &memory_result,
                    packet.instruction.bundle.program_word_count,
                    ti_c54x_run_memory_effect_redirects_pc(&effect))) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (!ti_c54x_run_route_pending_repeat_start(cpu, &memory_result)) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
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
                return ti_c54x_run_stop_result(cpu, ti_c54x_run_target_status_to_stop(target_status),
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            candidate_pc = cpu->arch_state.pc_word;
            target_status = ti_c54x_program_pc_target_apply(
                &candidate_pc, &target_effect);
            if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->arch_state.pc_word = candidate_pc;
            cpu->instructions_retired++;
            instructions_retired++;
            if (ti_c54x_run_advance_delayed_branch(
                    cpu, packet.instruction.bundle.program_word_count,
                    true, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
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
                return ti_c54x_run_stop_result(cpu, ti_c54x_run_target_status_to_stop(target_status),
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                return ti_c54x_run_stop_result(cpu, ti_c54x_run_bridge_status_to_stop(bridge_status),
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
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (ti_c54x_arch_state_read_cmpt(&cpu->arch_state, &cmpt) !=
                    TI_C54X_ARCH_STATE_OK ||
                cmpt != UINT8_C(0)) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (auxiliary->taken) {
                target_status = ti_c54x_program_pc_target_plan(
                    cpu->arch_state.pc_word, auxiliary->pmad,
                    &target_effect);
                if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                    return ti_c54x_run_stop_result(
                        cpu, ti_c54x_run_target_status_to_stop(target_status),
                        TI_C54X_BUS_OK, instructions_attempted,
                        instructions_retired);
                }
            }
            if (!auxiliary->taken || auxiliary->delayed) {
                if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                    return ti_c54x_run_stop_result(
                        cpu, ti_c54x_run_bridge_status_to_stop(bridge_status),
                        TI_C54X_BUS_OK, instructions_attempted,
                        instructions_retired);
                }
            }
            if (auxiliary->update &&
                ti_c54x_arch_state_write_ar(
                    &cpu->arch_state, auxiliary->ar_index,
                    auxiliary->value_after) != TI_C54X_ARCH_STATE_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (auxiliary->taken && !auxiliary->delayed) {
                candidate_pc = cpu->arch_state.pc_word;
                target_status = ti_c54x_program_pc_target_apply(
                    &candidate_pc, &target_effect);
                if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
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
            if (ti_c54x_run_advance_delayed_branch(
                    cpu, packet.instruction.bundle.program_word_count,
                    auxiliary->taken, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (took_effect) {
                ti_c54x_run_apply_deferred_branch(cpu);
            }
            continue;
        }

        /* Not-taken XC retires n NOP words: the program counter
         * advances past the skipped words with no fetch, no effect,
         * and no bus traffic. Taken XC emits NONE and retires through
         * the ordinary route below. The skip commits the widened plan
         * prepared once at the shared site above, so top-of-memory
         * behavior matches sequential advance. Skipped words consume
         * delay-slot countdown as NOP occupants; taken followers
         * retire on their own. No MEMORY endpoint, no WAIT/FAULT,
         * and no status, accumulator, or T effect is admitted. */
        if (effect.kind == TI_C54X_OP_EFFECT_CONDITIONAL_SKIP) {
            uint8_t skip_advance;
            bool took_effect = false;

            if (effect.payload.conditional_skip.skip_words < UINT8_C(1) ||
                effect.payload.conditional_skip.skip_words > UINT8_C(2)) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            skip_advance =
                (uint8_t)(UINT8_C(1) +
                          effect.payload.conditional_skip.skip_words);
            if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                return ti_c54x_run_stop_result(
                    cpu, ti_c54x_run_bridge_status_to_stop(bridge_status),
                    TI_C54X_BUS_OK, instructions_attempted,
                    instructions_retired);
            }
            cpu->arch_state.pc_word = candidate_pc;
            cpu->instructions_retired++;
            instructions_retired++;
            if (ti_c54x_run_advance_delayed_branch(
                    cpu, skip_advance, false, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (took_effect) {
                ti_c54x_run_apply_deferred_branch(cpu);
            }
            /* Retirement-tail HOLD re-drive for repeated bodies. */
            if (ti_c54x_cpu_repeat_body_retired(cpu) ==
                TI_C54X_CPU_REPEAT_RETIRE_INVALID) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        /* RPT retires through the single-repeat wiring in
         * repeat_activate.c, which also owns the retirement-tail
         * HOLD re-drive. */
        {
            const ti_c54x_cpu_repeat_route_status route_status =
                ti_c54x_cpu_repeat_route(cpu, &effect, bridge_status,
                                         candidate_pc);
            if (route_status != TI_C54X_CPU_REPEAT_ROUTE_NOT_REPEAT) {
                if (route_status != TI_C54X_CPU_REPEAT_ROUTE_RETIRED) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                cpu->instructions_retired++;
                instructions_retired++;
                continue;
            }
        }

        /* FRAME and MAR retire through pointer-write leaves: the
         * bridge resolves first (no pointer mutation on bridge
         * failure), then the staged value-after commits through
         * the checked architectural state writer for the SP or
         * the addressed AR (marking it known) ahead of the
         * effect-transaction stage, which admits neither target.
         * Rejected calls publish no state mutation. */
        if (effect.kind == TI_C54X_OP_EFFECT_SP_WRITE ||
            effect.kind == TI_C54X_OP_EFFECT_AR_WRITE) {
            if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
                return ti_c54x_run_stop_result(cpu, ti_c54x_run_bridge_status_to_stop(bridge_status),
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (effect.kind == TI_C54X_OP_EFFECT_SP_WRITE) {
                if (ti_c54x_sp_write_effect_apply(&effect,
                                                  &cpu->arch_state) !=
                    TI_C54X_SP_WRITE_EFFECT_OK) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
            } else if (ti_c54x_ar_write_effect_apply(&effect,
                                                     &cpu->arch_state) !=
                       TI_C54X_AR_WRITE_EFFECT_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            cpu->arch_state.pc_word = candidate_pc;
            cpu->instructions_retired++;
            instructions_retired++;
            {
                bool took_effect = false;

                if (ti_c54x_run_advance_delayed_branch(
                        cpu, packet.instruction.bundle.program_word_count,
                        false, &took_effect) !=
                    TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                    return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                       TI_C54X_BUS_OK, instructions_attempted,
                                       instructions_retired);
                }
                if (took_effect) {
                    ti_c54x_run_apply_deferred_branch(cpu);
                }
            }
            if (ti_c54x_cpu_repeat_body_retired(cpu) ==
                TI_C54X_CPU_REPEAT_RETIRE_INVALID) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            continue;
        }

        if (ti_c54x_effect_transaction_stage(
                &effect, &cpu->arch_state.status, &cpu->accumulators,
                &cpu->t_register, &cpu->arch_state.mmr_file,
                &transaction) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
        if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
            return ti_c54x_run_stop_result(cpu, ti_c54x_run_bridge_status_to_stop(bridge_status),
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }

        if (ti_c54x_effect_transaction_commit(
                &transaction, &cpu->arch_state.status, &cpu->accumulators,
                &cpu->t_register, &cpu->arch_state.mmr_file) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
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

            if (ti_c54x_run_advance_delayed_branch(
                    cpu, packet.instruction.bundle.program_word_count,
                    false, &took_effect) !=
                TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
                return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                   TI_C54X_BUS_OK, instructions_attempted,
                                   instructions_retired);
            }
            if (took_effect) {
                ti_c54x_run_apply_deferred_branch(cpu);
            }
        }
        /* Retirement-tail HOLD re-drive for repeated bodies. */
        if (ti_c54x_cpu_repeat_body_retired(cpu) ==
            TI_C54X_CPU_REPEAT_RETIRE_INVALID) {
            return ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                               TI_C54X_BUS_OK, instructions_attempted,
                               instructions_retired);
        }
    }

    cpu->execution_active = false;
    return ti_c54x_run_result_from_cpu(cpu, TI_C54X_STOP_BUDGET_EXHAUSTED,
                           TI_C54X_BUS_OK, instructions_attempted,
                           instructions_retired);
}
