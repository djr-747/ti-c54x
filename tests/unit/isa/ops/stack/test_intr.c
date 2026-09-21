/*
 * Module contract
 *
 * Owns: RED proofs for the INTR K leaf (TI SPRU172C section
 * INTR, 1 word): decode identity, the TI worked example word,
 * intr-push staging with the K operand and the INTM-setting
 * completion, the trusted single-operand entry, unknown-SP
 * rejection, and checked-entry invalid-decode rejection.
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
#include "../../../../../src/isa/ops/stack/op_intr.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../../src/sequencer/interrupt/take_complete.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
intr_word(uint8_t k)
{
    return (uint16_t)(UINT16_C(0xF7C0) | (k & UINT8_C(0x1F)));
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
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_INTR_K);
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
check_intr_staging(const ti_c54x_op_effect *effect)
{
    const ti_c54x_op_memory_effect *memory;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    memory = &effect->payload.memory;
    TI_C54X_TEST_CHECK(memory->address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16);
    TI_C54X_TEST_CHECK(memory->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(memory->value_word == UINT16_C(0));
    /* The completion union holds the INTM-setting take
     * completion; no dual or triple arm exists on this kind. */
    TI_C54X_TEST_CHECK(memory->complete == ti_c54x_take_complete_intm);
}

static void
test_ti_example_word(void)
{
    /* SPRU172C section INTR: INTR 3 pushes PC+1, vectors to the K
     * vector, sets INTM, and clears the IFR bit. */
    const ti_c54x_decoded_instruction instruction =
        make_instruction(intr_word(UINT8_C(0x03)));
    ti_c54x_op_input_snapshot input = known_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect checked = before;
    ti_c54x_op_effect trusted = before;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(intr_word(UINT8_C(0x03)) == UINT16_C(0xF7C3));
    TI_C54X_TEST_CHECK(ti_c54x_op_intr(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    check_intr_staging(&checked);
    operands.count = UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(5);
    operands.values[0].raw_value = UINT64_C(0x03);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_intr(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    check_intr_staging(&trusted);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);
}

static void
test_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(intr_word(UINT8_C(0)));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_input_snapshot unknown = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_intr(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_intr(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_intr(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_intr(&instruction, &unknown, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_intr(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(2);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_intr(
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
