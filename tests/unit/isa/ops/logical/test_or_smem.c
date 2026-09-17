/* Module contract
 * Owns: RED proofs for the OR Smem leaf (FORM-T2-08-01): direct and
 * standard-indirect MOD0--MOD3 memory-read staging, read-value
 * completion (zero-extended full-width OR into the destination
 * accumulator with C/TC preserved), exotic-mode deferral,
 * unknown-input stops, field/range rejection, effect-validity,
 * the trusted three-operand entry, and the plain decoded entry
 * (layout-gated extraction into the values core). Vectors are independently
 * recomputed from the TI execution formula (no TI worked example
 * exists for this row).
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch, memory addressing or buses, accumulator/status mutation,
 * CPU sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/logical/op_or_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/dispatch/trusted_handler.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0012345678);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0xFFFFFFFFFF);
    input.accumulator_b_known = true;
    return input;
}

static ti_c54x_decoded_instruction null_instruction(void)
{
    ti_c54x_decoded_instruction instruction;

    (void)memset(&instruction, 0, sizeof instruction);
    return instruction;
}

/* Direct staging: dst A, I=0, addr 0x34 reads DIRECT page-0. */
static void test_direct_stages_memory_read(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(staged.payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x34));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Completion: 0x0012345678 | 0x00FF = 0x00123456FF into A,
 * C/TC preserved. */
static void test_completion_ors_into_a(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x00FF),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(next.payload.accumulator_write.value_raw40,
                           UINT64_C(0x00123456FF));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Zero-extension preserves the upper accumulator: 0xFFFFFFFFFF-family
 * value OR 0x8000 keeps every upper bit set. */
static void test_completion_zero_extends_into_b(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(1), UINT8_C(0), UINT8_C(0x01), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8000),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK_U64(next.payload.accumulator_write.value_raw40,
                           UINT64_C(0xFFFFFFFFFF));
}

/* Upper accumulator bits are preserved while low bits set:
 * 0xFF12345678 | 0x00FF = 0xFF123456FF. */
static void test_completion_preserves_upper_bits(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFF12345678);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x00), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x00FF),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(next.payload.accumulator_write.value_raw40,
                           UINT64_C(0xFF123456FF));
}

/* Indirect MOD2: I=1, addr7 0x14 -> mode 2, AR4. */
static void test_indirect_mod2_stages_single_indirect(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(1), UINT8_C(1), UINT8_C(0x14), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Exotic mode bits (mode 4) defer without touching the effect. */
static void test_exotic_mode_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect before;

    (void)memset(&staged, 0xA5, sizeof staged);
    before = staged;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(1), UINT8_C(0x20), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof staged) == 0);
}

/* Unknown destination accumulator stops staging. */
static void test_unknown_destination_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

/* Unknown destination also stops the completion. */
static void test_completion_unknown_destination_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x00FF),
                           &next) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

/* Out-of-range fields reject: destination 2, indirect 2, addr 0x80. */
static void test_field_ranges_reject(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(2), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(2), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x80), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Out-of-range accumulator content rejects at stage and completion. */
static void test_wide_accumulator_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    input.accumulator_a_raw40 = UINT64_C(0x0012345678);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    input.accumulator_a_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x00FF),
                           &next) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* NULL borrowers reject on every entry. */
static void test_null_arguments_reject(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem_values(
                           UINT8_C(0), UINT8_C(0), UINT8_C(0x34), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0x00FF),
                           &next) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0x00FF),
                           &next) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x00FF),
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, &operands, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, &operands, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Trusted entry stages the same DIRECT read from (dst, I, addr7). */
static void test_trusted_stages_direct_read(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[2].width_bits = UINT8_C(7);
    operands.values[2].raw_value = UINT64_C(0x10);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x10));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0F0F),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    /* 0xFFFFFFFFFF | 0x0F0F = 0xFFFFFFFFFF. */
    TI_C54X_TEST_CHECK_U64(next.payload.accumulator_write.value_raw40,
                           UINT64_C(0xFFFFFFFFFF));
}

/* Trusted entry rejects bad arity and bad widths. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction = null_instruction();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};

    operands.count = UINT8_C(2);

    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    operands.values[2].width_bits = UINT8_C(8);
    operands.values[2].raw_value = UINT64_C(0x10);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[2].width_bits = UINT8_C(7);
    operands.values[2].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_or_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static ti_c54x_decoded_instruction make_plain_instruction(
    uint16_t first_word)
{
    ti_c54x_decoded_instruction instruction = null_instruction();

    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

/* Plain entry: 0x1A34 extracts S=0/I=0/addr 0x34, staging direct. */
static void test_plain_entry_stages_direct_read(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction =
        make_plain_instruction(UINT16_C(0x1A34));
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem(&instruction, &input,
                                         &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x34));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Plain entry: 0x1A95 extracts S=0/I=1/MOD2, staging indirect. */
static void test_plain_entry_stages_indirect_read(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction =
        make_plain_instruction(UINT16_C(0x1A95));
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem(&instruction, &input,
                                         &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Plain entry: wrong layout and nulls reject; effect untouched. */
static void test_plain_entry_rejects_bad_decode(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction instruction =
        make_plain_instruction(UINT16_C(0x1A34));
    ti_c54x_decoded_instruction wrong_layout = instruction;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect before = {0};

    wrong_layout.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem(&wrong_layout, &input,
                                         &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem(NULL, &input,
                                         &staged) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem(&instruction, NULL,
                                         &staged) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_or_smem(&instruction, &input,
                                         NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);
}

int main(void)
{
    test_direct_stages_memory_read();
    test_completion_ors_into_a();
    test_completion_zero_extends_into_b();
    test_completion_preserves_upper_bits();
    test_indirect_mod2_stages_single_indirect();
    test_exotic_mode_rejects();
    test_unknown_destination_rejects();
    test_completion_unknown_destination_rejects();
    test_field_ranges_reject();
    test_wide_accumulator_rejects();
    test_null_arguments_reject();
    test_trusted_stages_direct_read();
    test_trusted_shape_rejects();
    test_plain_entry_stages_direct_read();
    test_plain_entry_stages_indirect_read();
    test_plain_entry_rejects_bad_decode();
    return 0;
}
