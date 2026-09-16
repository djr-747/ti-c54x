/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the two-word page-0 direct
 * WORD16 immediate store effect, including flag/address/immediate
 * extraction, checked/trusted/dispatch parity, ignored input
 * state, deferred indirect-form rejection, and transactional
 * rejection. Does not own: catalog rows, operand-layout
 * descriptors, address resolution, buses, CPU or architectural
 * state, sequencing, timing, observers, diagnostics, or the
 * production semantic leaf.
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
#include "../../../../../src/isa/ops/store/op_stm.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t stm_word(uint8_t suffix)
{
    return (uint16_t)(UINT16_C(0x7700) | suffix);
}

static ti_c54x_decoded_instruction decode(uint8_t suffix, uint16_t immediate)
{
    const uint16_t first_word = stm_word(suffix);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, immediate},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == immediate);
    return instruction;
}

static ti_c54x_op_input_snapshot ignored_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_a_known = true;
    input.accumulator_b_known = false;
    input.auxiliary_registers_raw16[3] = UINT16_C(0xAAAA);
    input.auxiliary_registers_known_mask = UINT8_C(0);
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t suffix,
                                                        uint16_t immediate)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = (uint64_t)(suffix >> 7);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = (uint64_t)(suffix & 0x7FU);
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[2].raw_value = immediate;
    operands.values[2].width_bits = UINT8_C(16);
    return operands;
}

static void assert_direct_write(const ti_c54x_op_effect *effect,
                                uint8_t dma, uint16_t immediate)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.direct_dma7 ==
                       dma);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == immediate);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_direct_form(uint8_t dma, uint16_t immediate)
{
    const uint8_t suffix = (uint8_t)(dma & UINT8_C(0x7F));
    const ti_c54x_decoded_instruction instruction =
        decode(suffix, immediate);
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(suffix, immediate);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_STM);

    TI_C54X_TEST_CHECK(ti_c54x_op_stm(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_direct_write(&checked, dma, immediate);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stm(&instruction, &operands,
                                              &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_stm);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_is_canonical(
                           binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) == 0);
}

static void test_direct_forms(void)
{
    /* MMR file, DP0 RAM, and both range edges. */
    check_direct_form(UINT8_C(0x00), UINT16_C(0x0000));
    check_direct_form(UINT8_C(0x06), UINT16_C(0x1234));
    check_direct_form(UINT8_C(0x0E), UINT16_C(0x2468));
    check_direct_form(UINT8_C(0x1F), UINT16_C(0xFFFF));
    check_direct_form(UINT8_C(0x20), UINT16_C(0xABCD));
    check_direct_form(UINT8_C(0x7F), UINT16_C(0x0001));
}

static void test_indirect_form_defers(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x80), UINT16_C(0x1234));
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x80), UINT16_C(0x1234));
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    (void)memset(&effect, 0xA5, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_op_stm(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stm(&instruction, &operands,
                                              &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
}

static void test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x06), UINT16_C(0x1234));
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x06), UINT16_C(0x1234));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_stm(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_stm(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_stm(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stm(&instruction, NULL, &input,
                                              &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stm(&instruction, &operands,
                                              &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_direct_forms();
    test_indirect_form_defers();
    test_null_rejects();
    return 0;
}
