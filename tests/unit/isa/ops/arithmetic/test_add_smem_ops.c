/*
 * Module contract
 *
 * Owns: RED proofs for the ADD Smem leaves (syntaxes 1-3),
 * including checked/trusted decode validation, direct versus
 * standard-indirect memory-read staging, exotic-mode deferral,
 * SXM extension, T-register and fixed-16 shifts, per-syntax C
 * rules, OVM saturation with sticky OVdst, unknown-input stops,
 * and transactional rejection. Vectors are derived from the TI
 * execution formulas (no TI worked example exists for syntaxes
 * 1-3); each expectation was independently recomputed.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/T/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaves.
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
#include "../../../../../src/isa/ops/arithmetic/op_add_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode_id(uint16_t first_word,
                                             ti_c54x_opcode_id id)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0012340000);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x00020000);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0400);
    input.t_register_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

/* Syntax-1 direct: 0x0034 (S=0, I=0, addr 0x34):
 * 0x0012340000 + 0x5678 = 0x12345678, C=0. */
static void test_syntax1_direct_adds(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x5678),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x12345678));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Syntax-1 S=1 selects B: 0x0134: 0x00020000 + 0x5678 = 0x00025678. */
static void test_syntax1_selects_b(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0134), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x5678),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00025678));
}

/* Syntax-1 SXM=1 sign-extends a negative Smem: 0 + 0x8000 -> 0xFFFFFF8000. */
static void test_syntax1_sxm_sign_extends(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8000),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFF8000));
}

/* Syntax-1 SXM=0 zero-extends: 0 + 0x8000 -> 0x008000. */
static void test_syntax1_sxm_zero_extends(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8000),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x008000));
}

/* Syntax-1 carry: 0x00FFFFFFFF + 1 -> 0x0100000000 with C=1. */
static void test_syntax1_carry_sets_c(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x00FFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0001),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0100000000));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* Syntax-1 OVM saturation: 0x007FFFFFFF + 1 -> 0x007FFFFFFF, OVdst set. */
static void test_syntax1_ovm_saturates(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.status_ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0001),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Syntax-1 indirect MOD2: 0x0095 (mode4=2, arf=5) stages the read. */
static void test_syntax1_indirect_mod2_stages_read(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0095), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
}

/* Syntax-1 exotic mode defers: 0x00AD (mode4=5) is not standard. */
static void test_syntax1_exotic_mode_defers(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x00AD), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Syntax-2 TS shift: 0x0434, T=0x0004: 0x100 + (0x3 << 4) = 0x130. */
static void test_syntax2_ts_shifts(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0434), TI_C54X_OPCODE_ID_ADD_SMEM_TS);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x100);
    input.t_register_raw16 = UINT16_C(0x0004);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_ts(&instruction, &input,
                                              &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0003),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x130));
}

/* Syntax-2 unknown T stops; out-of-band TS rejects. */
static void test_syntax2_t_gates(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0434), TI_C54X_OPCODE_ID_ADD_SMEM_TS);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_ts(&instruction, &input,
                                              &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.t_register_raw16 = UINT16_C(0x0020);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_ts(&instruction, &input,
                                              &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Syntax-3: 0x3C34 (S=0, D=0): 0x100 + (0x2 << 16) = 0x20100, C kept. */
static void test_syntax3_basic_preserves_c(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x3C34), TI_C54X_OPCODE_ID_ADD_SMEM_SH16);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x100);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_sh16(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0002),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x20100));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

/* Syntax-3 carry: 0x00FFFFFFFF + (1 << 16) -> 0x10000FFFF with C=1. */
static void test_syntax3_carry_sets_c(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x3C34), TI_C54X_OPCODE_ID_ADD_SMEM_SH16);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x00FFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_sh16(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0001),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x10000FFFF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);
}

/* Syntax-3 S=1/D=1: 0x3F34: B = 0x20000 + (0x1234 << 16) = 0x12360000. */
static void test_syntax3_cross_accumulators(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x3F34), TI_C54X_OPCODE_ID_ADD_SMEM_SH16);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_sh16(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x1234),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x12360000));
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_trusted_syntax1_completion(void)
{
    /* Trusted operands [S=0, I=0, addr 0x34] complete the basic add. */
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x34);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_smem(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x5678),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x12345678));
}

static void test_trusted_syntax2_completion(void)
{
    /* Trusted operands [S=0, I=0, addr 0x34] with T=0x0004 complete
     * 0x100 + (0x3 << 4) = 0x130. */
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0434), TI_C54X_OPCODE_ID_ADD_SMEM_TS);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x100);
    input.t_register_raw16 = UINT16_C(0x0004);
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x34);
    operands.values[2].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_smem_ts(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0003),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x130));
}

static void test_trusted_syntax3_completion(void)
{
    /* Trusted operands [S=1, D=1, I=0, addr 0x34] complete
     * B = 0x20000 + (0x1234 << 16) = 0x12360000. */
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x3F34), TI_C54X_OPCODE_ID_ADD_SMEM_SH16);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(1);
    operands.values[3].raw_value = UINT64_C(0x34);
    operands.values[3].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_smem_sh16(
                           &instruction, &operands, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x1234),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x12360000));
}

static void test_crossed_rows_reject(void)
{
    /* A TS word through the unshifted leaf, and an SH16 word through
     * the TS leaf, are decode mismatches. */
    ti_c54x_decoded_instruction ts =
        decode_id(UINT16_C(0x0434), TI_C54X_OPCODE_ID_ADD_SMEM_TS);
    ti_c54x_decoded_instruction sh16 =
        decode_id(UINT16_C(0x3C34), TI_C54X_OPCODE_ID_ADD_SMEM_SH16);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&ts, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem_ts(&sh16, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_id(UINT16_C(0x0034), TI_C54X_OPCODE_ID_ADD_SMEM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_syntax1_direct_adds();
    test_syntax1_selects_b();
    test_syntax1_sxm_sign_extends();
    test_syntax1_sxm_zero_extends();
    test_syntax1_carry_sets_c();
    test_syntax1_ovm_saturates();
    test_syntax1_indirect_mod2_stages_read();
    test_syntax1_exotic_mode_defers();
    test_syntax2_ts_shifts();
    test_syntax2_t_gates();
    test_syntax3_basic_preserves_c();
    test_syntax3_carry_sets_c();
    test_syntax3_cross_accumulators();
    test_checked_unknown_inputs_stop();
    test_trusted_syntax1_completion();
    test_trusted_syntax2_completion();
    test_trusted_syntax3_completion();
    test_crossed_rows_reject();
    test_null_rejects();
    return 0;
}
