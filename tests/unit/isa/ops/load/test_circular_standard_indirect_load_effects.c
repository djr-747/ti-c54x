/*
 * Module contract
 *
 * Owns: executable proof for the seven load families' circular MOD8/MOD10
 * WORD16 effects. Every checked and trusted wrapper must stage the exact typed
 * circular address spec for AR0 and AR7 and complete to its family effect;
 * malformed metadata and unknown inputs must leave a sentinel unchanged.
 * Does not own: catalog generation, decode/layout, address planning, buses,
 * CPU state, sequencing, timing, observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/load/op_ld_smem.h"
#include "../../../../../src/isa/ops/load/op_ld_status_smem.h"
#include "../../../../../src/isa/ops/load/op_ld_t_smem.h"
#include "../../../../../src/isa/ops/load/op_ldu_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

typedef enum circular_transform {
    CIRCULAR_SIGNED = 0,
    CIRCULAR_TS,
    CIRCULAR_UNSIGNED,
    CIRCULAR_T,
    CIRCULAR_DP,
    CIRCULAR_ASM,
    CIRCULAR_SHIFT16
} circular_transform;

typedef struct load_case {
    ti_c54x_opcode_id mod8_id;
    ti_c54x_opcode_id mod10_id;
    uint16_t mod8_base;
    uint16_t mod10_base;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    bool has_destination;
    circular_transform transform;
    checked_handler checked8;
    checked_handler checked10;
    ti_c54x_trusted_op_handler trusted8;
    ti_c54x_trusted_op_handler trusted10;
} load_case;

static const load_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD8, TI_C54X_OPCODE_ID_LD_SMEM_MOD10,
     UINT16_C(0x10C0), UINT16_C(0x10D0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, true, CIRCULAR_SIGNED,
     ti_c54x_op_ld_smem_mod8, ti_c54x_op_ld_smem_mod10,
     ti_c54x_trusted_op_ld_smem_mod8, ti_c54x_trusted_op_ld_smem_mod10},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD8, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD10,
     UINT16_C(0x14C0), UINT16_C(0x14D0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, true, CIRCULAR_TS,
     ti_c54x_op_ld_smem_ts_mod8, ti_c54x_op_ld_smem_ts_mod10,
     ti_c54x_trusted_op_ld_smem_ts_mod8, ti_c54x_trusted_op_ld_smem_ts_mod10},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD8, TI_C54X_OPCODE_ID_LDU_SMEM_MOD10,
     UINT16_C(0x12C0), UINT16_C(0x12D0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE,
     true, CIRCULAR_UNSIGNED, ti_c54x_op_ldu_smem_mod8,
     ti_c54x_op_ldu_smem_mod10, ti_c54x_trusted_op_ldu_smem_mod8,
     ti_c54x_trusted_op_ldu_smem_mod10},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD8, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD10,
     UINT16_C(0x30C0), UINT16_C(0x30D0), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, false, CIRCULAR_T, ti_c54x_op_ld_t_smem_mod8,
     ti_c54x_op_ld_t_smem_mod10, ti_c54x_trusted_op_ld_t_smem_mod8,
     ti_c54x_trusted_op_ld_t_smem_mod10},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD8, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD10,
     UINT16_C(0x46C0), UINT16_C(0x46D0), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, false, CIRCULAR_DP, ti_c54x_op_ld_dp_smem_mod8,
     ti_c54x_op_ld_dp_smem_mod10, ti_c54x_trusted_op_ld_dp_smem_mod8,
     ti_c54x_trusted_op_ld_dp_smem_mod10},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD8, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD10,
     UINT16_C(0x32C0), UINT16_C(0x32D0), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, false, CIRCULAR_ASM,
     ti_c54x_op_ld_asm_smem_mod8, ti_c54x_op_ld_asm_smem_mod10,
     ti_c54x_trusted_op_ld_asm_smem_mod8, ti_c54x_trusted_op_ld_asm_smem_mod10},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD8,
     TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD10, UINT16_C(0x44C0),
     UINT16_C(0x44D0), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, true, CIRCULAR_SHIFT16,
     ti_c54x_op_ld_smem_shift16_mod8, ti_c54x_op_ld_smem_shift16_mod10,
     ti_c54x_trusted_op_ld_smem_shift16_mod8,
     ti_c54x_trusted_op_ld_smem_shift16_mod10},
};

static ti_c54x_decoded_instruction decode_case(const load_case *test_case,
                                               uint8_t mode, uint8_t arf)
{
    const uint16_t base = mode == UINT8_C(8) ? test_case->mod8_base
                                             : test_case->mod10_base;
    const uint16_t word = (uint16_t)(base | arf |
                                     (test_case->has_destination
                                          ? UINT16_C(0x0100)
                                          : UINT16_C(0)));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0xCAFE)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == (mode == UINT8_C(8) ? test_case->mod8_id
                                                        : test_case->mod10_id));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(instruction.input_resources == test_case->resources);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_operand_layout_id layout)
{
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words, instruction->bundle.program_word_count};
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           layout, instruction->bundle.program_word_count,
                           &view) == TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    operands.count = view.descriptor->operand_count;
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return operands;
}

static ti_c54x_op_input_snapshot valid_input(const load_case *test_case)
{
    ti_c54x_op_input_snapshot input = {0};
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known =
        (test_case->resources & TI_C54X_INPUT_RESOURCES_STATUS_SXM) != 0U;
    input.t_register_raw16 = UINT16_C(1);
    input.t_register_known =
        (test_case->resources & TI_C54X_INPUT_RESOURCES_T_REGISTER) != 0U;
    return input;
}

static void assert_read(const ti_c54x_op_effect *effect, uint8_t mode,
                        uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.circular_mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word == 0U);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void assert_completion(const load_case *test_case,
                              const ti_c54x_decoded_instruction *instruction,
                              const ti_c54x_op_input_snapshot *input,
                              const ti_c54x_op_effect *memory)
{
    ti_c54x_op_effect completed = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    const uint16_t read_value = UINT16_C(0x8123);
    TI_C54X_TEST_CHECK(memory->payload.memory.complete(
                           instruction, input, read_value, &completed) ==
                       TI_C54X_OP_STATUS_OK);
    switch (test_case->transform) {
    case CIRCULAR_SIGNED:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           ((TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | read_value));
        break;
    case CIRCULAR_TS:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           ((((TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | read_value)
                             << 1U) & TI_C54X_U40_MASK));
        break;
    case CIRCULAR_UNSIGNED:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           read_value);
        break;
    case CIRCULAR_SHIFT16:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           ((((TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | read_value)
                             << 16U) & TI_C54X_U40_MASK));
        break;
    case CIRCULAR_T:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_T_REGISTER_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.t_register_write.raw_value == read_value);
        break;
    case CIRCULAR_DP:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.target ==
                           TI_C54X_OP_STATUS_FIELD_TARGET_DP);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.raw_value ==
                           (read_value & UINT16_C(0x01FF)));
        break;
    case CIRCULAR_ASM:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.target ==
                           TI_C54X_OP_STATUS_FIELD_TARGET_ASM);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.raw_value ==
                           (read_value & UINT16_C(0x001F)));
        break;
    }
}

static void check_case(const load_case *test_case, uint8_t mode, uint8_t arf)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(test_case, mode, arf);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction, test_case->layout);
    const ti_c54x_op_input_snapshot input = valid_input(test_case);
    checked_handler checked = mode == UINT8_C(8) ? test_case->checked8
                                                : test_case->checked10;
    ti_c54x_trusted_op_handler trusted = mode == UINT8_C(8)
                                             ? test_case->trusted8
                                             : test_case->trusted10;
    ti_c54x_op_effect checked_effect = {0};
    ti_c54x_op_effect trusted_effect = {0};
    TI_C54X_TEST_CHECK(checked(&instruction, &input, &checked_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted(&instruction, &operands, &input, &trusted_effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_read(&checked_effect, mode, arf);
    assert_read(&trusted_effect, mode, arf);
    assert_completion(test_case, &instruction, &input, &checked_effect);
    assert_completion(test_case, &instruction, &input, &trusted_effect);
}

static void check_transactionality(void)
{
    const load_case *signed_case = &cases[0];
    const load_case *ts_case = &cases[1];
    const ti_c54x_decoded_instruction instruction =
        decode_case(signed_case, UINT8_C(8), UINT8_C(0));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction, signed_case->layout);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect effect = sentinel;
    ti_c54x_op_input_snapshot input = valid_input(signed_case);

    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(signed_case->checked8(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(signed_case->trusted8(&instruction, &operands, &input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    input = valid_input(ts_case);
    const ti_c54x_decoded_instruction ts_instruction =
        decode_case(ts_case, UINT8_C(10), UINT8_C(7));
    const ti_c54x_trusted_operand_values ts_operands =
        trusted_operands(&ts_instruction, ts_case->layout);
    input.t_register_known = false;
    effect = sentinel;
    TI_C54X_TEST_CHECK(ts_case->checked10(&ts_instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ts_case->trusted10(&ts_instruction, &ts_operands, &input,
                                          &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    input = valid_input(signed_case);
    ti_c54x_decoded_instruction malformed = instruction;
    malformed.operand_layout = TI_C54X_OPERAND_LAYOUT_UNKNOWN;
    effect = sentinel;
    TI_C54X_TEST_CHECK(signed_case->checked8(&malformed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

int main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_case(&cases[i], UINT8_C(8), UINT8_C(0));
        check_case(&cases[i], UINT8_C(8), UINT8_C(7));
        check_case(&cases[i], UINT8_C(10), UINT8_C(0));
        check_case(&cases[i], UINT8_C(10), UINT8_C(7));
    }
    check_transactionality();
    return 0;
}
