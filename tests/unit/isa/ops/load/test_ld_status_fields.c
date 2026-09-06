/*
 * Module contract
 *
 * Owns: exhaustive proof that immediate DP/ASM/ARP handlers extract raw
 * fields into typed value-owned effects and reject forged descriptors
 * transactionally. Does not own: CPU/state commit, timing, memory, buses,
 * repeat policy, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/load/op_ld_status_fields.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef ti_c54x_op_status (*ld_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

typedef struct ld_case {
    ti_c54x_opcode_id id;
    ti_c54x_operand_layout_id layout;
    uint16_t value;
    uint16_t mask;
    uint16_t limit;
    uint8_t width;
    ti_c54x_op_status_field_target target;
    ld_handler handler;
} ld_case;

static const ti_c54x_op_input_snapshot zero_input = {0};

static const ld_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP,
     TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP, UINT16_C(0xEA00),
     UINT16_C(0xFE00), UINT16_C(0x0200), UINT8_C(9),
     TI_C54X_OP_STATUS_FIELD_TARGET_DP, ti_c54x_op_ld_immediate_dp},
    {TI_C54X_OPCODE_ID_LD_IMMEDIATE_ASM,
     TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM, UINT16_C(0xED00),
     UINT16_C(0xFFE0), UINT16_C(0x0020), UINT8_C(5),
     TI_C54X_OP_STATUS_FIELD_TARGET_ASM, ti_c54x_op_ld_immediate_asm},
    {TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP,
     TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP, UINT16_C(0xF4A0),
     UINT16_C(0xFFF8), UINT16_C(0x0008), UINT8_C(3),
     TI_C54X_OP_STATUS_FIELD_TARGET_ARP, ti_c54x_op_ld_immediate_arp},
};

static ti_c54x_decoded_instruction
decode_word(const ld_case *test_case, uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static void
test_success_and_dispatch(const ld_case *test_case)
{
    uint32_t raw;
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(test_case->id);

    for (raw = 0U; raw < (uint32_t)test_case->limit; ++raw) {
        const ti_c54x_decoded_instruction instruction =
            decode_word(test_case, (uint16_t)(test_case->value | raw));
        ti_c54x_op_effect effect;
        ti_c54x_op_effect dispatched;

        memset(&effect, 0xA5, sizeof(effect));
        TI_C54X_TEST_CHECK(test_case->handler(&instruction, &zero_input, &effect) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(effect.kind ==
                           TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(effect.payload.status_field_write.target ==
                           test_case->target);
        TI_C54X_TEST_CHECK(effect.payload.status_field_write.raw_value ==
                           (uint16_t)raw);

        memset(&dispatched, 0, sizeof(dispatched));
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded(&instruction, &dispatched) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(dispatched.kind ==
                           TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(dispatched.payload.status_field_write.target ==
                           test_case->target);
        TI_C54X_TEST_CHECK(dispatched.payload.status_field_write.raw_value ==
                           (uint16_t)raw);
        TI_C54X_TEST_CHECK(metadata->operand_layout == instruction.operand_layout);
        TI_C54X_TEST_CHECK(metadata->repeatability == instruction.repeatability);
    }
}

static void
test_forged_and_failed_inputs_are_transactional(const ld_case *test_case)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(test_case, test_case->value);
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    memset(&effect, 0x31, sizeof(effect));
    memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(test_case->handler(NULL, &zero_input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->handler(&instruction, &zero_input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(test_case->handler(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction.id = TI_C54X_OPCODE_ID_NOP;
    memset(&effect, 0x42, sizeof(effect));
    memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(test_case->handler(&instruction, &zero_input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction = decode_word(test_case, test_case->value);
    instruction.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    memset(&effect, 0x53, sizeof(effect));
    memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(test_case->handler(&instruction, &zero_input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction = decode_word(test_case, test_case->value);
    instruction.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS;
    memset(&effect, 0x54, sizeof(effect));
    memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(test_case->handler(&instruction, &zero_input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
}

int
main(void)
{
    size_t index;
    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        test_success_and_dispatch(&cases[index]);
        test_forged_and_failed_inputs_are_transactional(&cases[index]);
    }
    puts("ld_status_fields_ops: ok");
    return 0;
}
