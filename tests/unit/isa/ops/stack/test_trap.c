/*
 * Module contract
 *
 * Owns: RED proofs for the TRAP K leaf (TI SPRU172C section
 * TRAP, p. 4-195, 1 word): decode identity, the TI worked
 * example word, trap-push staging with the K operand, the
 * trusted single-operand entry, unknown-SP rejection, and
 * checked-entry invalid-decode rejection.
 * Does not own: catalog rows, operand-layout descriptors,
 * decode, dispatch, memory addressing or buses, status
 * mutation, CPU sequencing, timing, observers, diagnostics,
 * or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/stack/op_trap.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
trap_word(uint8_t k)
{
    return (uint16_t)(UINT16_C(0xF4C0) | (k & UINT8_C(0x1F)));
}

static ti_c54x_decoded_instruction
make_instruction(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_TRAP_K);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_TRAP_IMMEDIATE_K);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    return instruction;
}

static ti_c54x_op_input_snapshot
known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.stack_pointer_known = true;
    input.stack_pointer_raw16 = UINT16_C(0x03FF);
    return input;
}

static void
check_trap_staging(const ti_c54x_op_effect *effect)
{
    const ti_c54x_op_memory_effect *memory;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    memory = &effect->payload.memory;
    TI_C54X_TEST_CHECK(memory->address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16);
    TI_C54X_TEST_CHECK(memory->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(memory->value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(memory->complete == NULL);
    TI_C54X_TEST_CHECK(memory->complete_dual == NULL);
}

static void
test_ti_example_word(void)
{
    /* TI p. 4-195: TRAP 10h pushes PC+1 and vectors to FFC0h. */
    const ti_c54x_decoded_instruction instruction =
        make_instruction(trap_word(UINT8_C(0x10)));
    ti_c54x_op_input_snapshot input = known_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect checked = before;
    ti_c54x_op_effect trusted = before;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(trap_word(UINT8_C(0x10)) == UINT16_C(0xF4D0));
    TI_C54X_TEST_CHECK(ti_c54x_op_trap(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    check_trap_staging(&checked);
    operands.count = UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(5);
    operands.values[0].raw_value = UINT64_C(0x10);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_trap(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    check_trap_staging(&trusted);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);
}

static void
test_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(trap_word(UINT8_C(0)));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_input_snapshot unknown = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_trap(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_trap(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_trap(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_trap(&instruction, &unknown, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_trap(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(2);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_trap(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_ti_example_word();
    test_rejection_is_transactional();
    return 0;
}
