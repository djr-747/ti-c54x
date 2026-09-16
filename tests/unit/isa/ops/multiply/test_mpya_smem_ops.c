/*
 * Module contract
 *
 * Owns: RED proofs for the MPYA Smem leaves, including checked/trusted
 * decode validation, direct versus standard-indirect memory-read
 * staging with the read-phase T load, exotic-mode deferral, the
 * read-value STORE completion of Smem times A-high into B through
 * the multiply core, T-input independence, and transactional
 * rejection.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/T/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/multiply/op_mpya_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode_word(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MPYA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    /* TI MPYA Example 1 pre-state; T is written, never read. */
    input.accumulator_a_raw40 = UINT64_C(0xFF87651111);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x320);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x1234);
    input.t_register_known = false;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static void test_checked_direct_stages_read_with_t_load(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3134));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* 0x3134: I=0, addr=0x34: direct page-0 read with T load. */
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_checked_indirect_mod0_stages_read(void)
{
    /* addr7 0x02: mode4=0 (MOD0, no update), arf=2 (*AR2). */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3182));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

static void test_checked_exotic_mode_defers(void)
{
    /* addr7 0x2D: mode4=5 (AR0-indexed), deferred like MAC indirect. */
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x31AD));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3134));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3134));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0x34);
    operands.values[1].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpya_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
}

static void test_completion_ti_example1(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3182));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    /* TI MPYA Example 1: Smem=0x5678 times A-high=0x8765 into B. */
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x5678),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFD7436558));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void test_completion_frct_shifts(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3134));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x00010000);
    input.status_frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    /* A-high=1 times Smem=2, FRCT-shifted, into B. */
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(2),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(4));
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(UINT16_C(0x3134));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpya_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_checked_direct_stages_read_with_t_load();
    test_checked_indirect_mod0_stages_read();
    test_checked_exotic_mode_defers();
    test_checked_unknown_inputs_stop();
    test_trusted_parity();
    test_completion_ti_example1();
    test_completion_frct_shifts();
    test_null_rejects();
    return 0;
}
