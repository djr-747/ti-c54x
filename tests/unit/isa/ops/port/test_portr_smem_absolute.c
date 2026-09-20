/*
 * Module contract
 *
 * Owns: RED proof of the pure PORTR absolute-Smem I/O-pair memory
 * effect (P3-S87, TI SPRU172C §4 p. 4-129, 3 words):
 * decode identity, absolute DATA-leg address plus absolute port
 * address, DATA-WRITE direction, zero staged value, trusted
 * equivalence, non-cookie deferral, input-independence, and
 * transactional rejection.
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
decode_absolute(uint16_t absolute, uint16_t pa)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0x74F8));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0x74F8), absolute, pa},
        .program_word_count = UINT8_C(3)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_PORTR_SMEM_ABSOLUTE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_ABSOLUTE16_PA16);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_absolute_operands(uint16_t absolute, uint16_t pa)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT8_C(0x78);
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[2].raw_value = absolute;
    operands.values[2].width_bits = UINT8_C(16);
    operands.values[3].raw_value = pa;
    operands.values[3].width_bits = UINT8_C(16);
    return operands;
}

static void
assert_absolute_effect(const ti_c54x_op_effect *effect, uint16_t absolute,
                       uint16_t pa)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.indirect ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.smem_mode ==
                       UINT8_C(15));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.smem_arf ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port.smem_dma7 ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.port.port_address == pa);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.port.reserved_half ==
        absolute);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port
                           .reserved_word0 == UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port
                           .reserved_word1 == UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.port
                           .reserved_word2 == UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
test_absolute_effect_and_parity(void)
{
    /* Evidenced boot words plus boundary addresses and ports. */
    static const uint16_t addresses[] = {UINT16_C(0x0000),
                                         UINT16_C(0x0008),
                                         UINT16_C(0x87F0),
                                         UINT16_C(0xFFFF)};
    static const uint16_t ports[] = {UINT16_C(0x0000),
                                     UINT16_C(0x0008),
                                     UINT16_C(0xFFFF)};

    for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]);
         ++a) {
        for (size_t p = 0U; p < sizeof(ports) / sizeof(ports[0]); ++p) {
            const ti_c54x_decoded_instruction instruction =
                decode_absolute(addresses[a], ports[p]);
            const ti_c54x_trusted_operand_values operands =
                trusted_absolute_operands(addresses[a], ports[p]);
            const ti_c54x_op_input_snapshot input = {0};
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted = before;

            TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem_absolute(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_portr_smem_absolute(
                                   &instruction, &operands, &input,
                                   &trusted) == TI_C54X_OP_STATUS_OK);
            assert_absolute_effect(&checked, addresses[a], ports[p]);
            assert_absolute_effect(&trusted, addresses[a], ports[p]);
            TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                      sizeof(checked)) == 0);
        }
    }
}

static void
test_cookie_deferral(void)
{
    /* Only the 0x78 absolute selector composes here: I=0
     * direct words and other I=1 cookies reject through
     * both paths with effects untouched. */
    static const uint16_t first_words[] = {UINT16_C(0x7400),
                                           UINT16_C(0x7480),
                                           UINT16_C(0x74F7),
                                           UINT16_C(0x74F9),
                                           UINT16_C(0x74FF)};

    for (size_t w = 0U;
         w < sizeof(first_words) / sizeof(first_words[0]); ++w) {
        ti_c54x_decoded_instruction instruction =
            decode_absolute(UINT16_C(0x0008), UINT16_C(0x0000));
        ti_c54x_trusted_operand_values operands =
            trusted_absolute_operands(UINT16_C(0x0008),
                                      UINT16_C(0x0000));
        const ti_c54x_op_input_snapshot input = {0};
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        instruction.bundle.program_words[0] = first_words[w];
        instruction.id = TI_C54X_OPCODE_ID_PORTR_SMEM_ABSOLUTE;
        operands.values[1].raw_value =
            (uint64_t)(first_words[w] & UINT16_C(0x7F));
        TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem_absolute(
                               &instruction, &input, &checked) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_portr_smem_absolute(
                               &instruction, &operands, &input,
                               &trusted) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &before,
                                  sizeof(trusted)) == 0);
    }
}

static void
test_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_absolute(UINT16_C(0x0008), UINT16_C(0x0000));
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                        \
    do {                                                                       \
        effect = before;                                                       \
        TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem_absolute(                     \
                               &(candidate), &input, &effect) ==               \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                 \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);     \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem_absolute(
                           NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem_absolute(
                           &instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_portr_smem_absolute(
                           &instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(2);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x7480);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

int
main(void)
{
    test_absolute_effect_and_parity();
    test_cookie_deferral();
    test_rejection_is_transactional();
    return 0;
}
