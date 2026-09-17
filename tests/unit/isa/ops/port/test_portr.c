/*
 * Module contract
 *
 * Owns: RED proof of pure PORTR I/O-pair memory effects over direct
 * (I=0, 7-bit dma, DP-relative) and indirect (I=1, ARF plus 4-bit
 * MOD) Smem cookies with a 16-bit absolute port address: decode
 * identity, DATA-WRITE direction, zero staged value, trusted
 * equivalence, DP-unknownness gating, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/port/op_portr.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction
decode_words(uint16_t word0, uint16_t word1,
             ti_c54x_opcode_id expected_id,
             ti_c54x_input_resource_mask expected_resources)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word0);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, word1}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == expected_id);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_PA16);
    TI_C54X_TEST_CHECK(instruction.input_resources == expected_resources);
    return instruction;
}

static ti_c54x_op_input_snapshot
known_dp_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_dp = UINT8_C(0);
    input.status_dp_known = true;
    return input;
}

static void
assert_portr_effect(const ti_c54x_op_effect *effect, uint8_t indirect,
                    uint8_t smem_mode, uint8_t smem_arf, uint8_t smem_dma7,
                    uint16_t port_address)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.indirect ==
                       indirect);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.smem_mode ==
                       smem_mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.smem_arf ==
                       smem_arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.smem_dma7 ==
                       smem_dma7);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.port.port_address ==
        port_address);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t indirect, uint8_t addr, uint16_t pa)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = indirect;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = addr;
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[2].raw_value = pa;
    operands.values[2].width_bits = UINT8_C(16);
    return operands;
}

static void
test_portr_direct_retires_effect(void)
{
    /* PORTR 60h,5h: I=0 direct dma with an absolute port address. */
    const uint16_t word0 = (uint16_t)(UINT16_C(0x7400) | UINT16_C(0x60));
    const uint16_t word1 = UINT16_C(0x0005);
    const ti_c54x_decoded_instruction instruction = decode_words(
        word0, word1, TI_C54X_OPCODE_ID_PORTR_DIRECT,
        TI_C54X_INPUT_RESOURCES_STATUS_DP);
    const ti_c54x_op_input_snapshot input = known_dp_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0x60), word1);
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_direct(&instruction, &input,
                                               &direct) ==
                       TI_C54X_OP_STATUS_OK);
    assert_portr_effect(&direct, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                        UINT8_C(0x60), word1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_portr_direct(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
}

static void
test_portr_direct_dp_unknown_rejects(void)
{
    const uint16_t word0 = (uint16_t)(UINT16_C(0x7400) | UINT16_C(0x60));
    const uint16_t word1 = UINT16_C(0x0005);
    const ti_c54x_decoded_instruction instruction = decode_words(
        word0, word1, TI_C54X_OPCODE_ID_PORTR_DIRECT,
        TI_C54X_INPUT_RESOURCES_STATUS_DP);
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_direct(&instruction, &input,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_portr_indirect_retires_effect(void)
{
    /* PORTR 5h,*AR2+: I=1, MOD2 in bits 6:3 with ARF=2 in bits 2:0. */
    const uint16_t cookie =
        (uint16_t)(((uint16_t)UINT8_C(2) << 3U) | UINT16_C(2));
    const uint16_t word0 =
        (uint16_t)(UINT16_C(0x7480) | cookie);
    const uint16_t word1 = UINT16_C(0x0005);
    const ti_c54x_decoded_instruction instruction = decode_words(
        word0, word1, TI_C54X_OPCODE_ID_PORTR_SMEM,
        TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), (uint8_t)cookie, word1);
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem(&instruction, &input,
                                             &direct) ==
                       TI_C54X_OP_STATUS_OK);
    assert_portr_effect(&direct, UINT8_C(1), UINT8_C(2), UINT8_C(2),
                        UINT8_C(0), word1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_portr_smem(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
}

static void
test_portr_indirect_bad_mode_rejects(void)
{
    /* MOD9 is carrier-invalid for the PORT Smem cookie. */
    const uint16_t cookie =
        (uint16_t)(((uint16_t)UINT8_C(9) << 3U) | UINT16_C(2));
    const uint16_t word0 =
        (uint16_t)(UINT16_C(0x7480) | cookie);
    const uint16_t word1 = UINT16_C(0x0005);
    const ti_c54x_decoded_instruction instruction = decode_words(
        word0, word1, TI_C54X_OPCODE_ID_PORTR_SMEM,
        TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem(&instruction, &input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void
test_portr_crossed_row_rejects(void)
{
    /* An I=1 word never decodes through the DIRECT row entry point. */
    const uint16_t word0 = UINT16_C(0x7492);
    const uint16_t word1 = UINT16_C(0x0005);
    const ti_c54x_decoded_instruction instruction = decode_words(
        word0, word1, TI_C54X_OPCODE_ID_PORTR_SMEM,
        TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    const ti_c54x_op_input_snapshot input = known_dp_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_direct(&instruction, &input,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void
test_portr_null_arguments_reject(void)
{
    const uint16_t word0 = (uint16_t)(UINT16_C(0x7400) | UINT16_C(0x60));
    const uint16_t word1 = UINT16_C(0x0005);
    const ti_c54x_decoded_instruction instruction = decode_words(
        word0, word1, TI_C54X_OPCODE_ID_PORTR_DIRECT,
        TI_C54X_INPUT_RESOURCES_STATUS_DP);
    const ti_c54x_op_input_snapshot input = known_dp_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_direct(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_portr_direct(&instruction, NULL,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_portr_direct(&instruction, &input,
                                               NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_portr_direct_retires_effect();
    test_portr_direct_dp_unknown_rejects();
    test_portr_indirect_retires_effect();
    test_portr_indirect_bad_mode_rejects();
    test_portr_crossed_row_rejects();
    test_portr_null_arguments_reject();
    return 0;
}
