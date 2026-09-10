/*
 * Module contract
 *
 * Owns: executable proof for all eight indexed standard-indirect WORD16 load
 * families. Each MOD5/MOD6 checked and trusted leaf must stage an
 * INDEXED_AR0_WORD16 read and complete to the family-specific architectural
 * effect. Does not own: catalog generation, decode/layout implementation,
 * address planning, buses, CPU state, sequencing, timing, observers, or
 * diagnostics.
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

/* Keep the wrapper declarations local so this aggregate exercises the exact
 * indexed family surface without coupling to unrelated family headers. */
extern ti_c54x_op_status ti_c54x_op_ld_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_smem_ts_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_smem_ts_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ldu_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ldu_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_t_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_t_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_dp_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_dp_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_asm_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_asm_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_smem_shift16_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ld_smem_shift16_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ldr_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_ldr_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

typedef enum load_transform {
    LOAD_SIGNED = 0,
    LOAD_SIGNED_TS = 1,
    LOAD_UNSIGNED = 2,
    LOAD_T = 3,
    LOAD_DP = 4,
    LOAD_ASM = 5,
    LOAD_SHIFT16 = 6,
    LOAD_ROUNDED = 7
} load_transform;

typedef struct load_case {
    ti_c54x_opcode_id id5;
    ti_c54x_opcode_id id6;
    uint16_t base5;
    uint16_t base6;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    bool has_destination;
    load_transform transform;
    checked_handler checked5;
    checked_handler checked6;
    ti_c54x_trusted_op_handler trusted5;
    ti_c54x_trusted_op_handler trusted6;
} load_case;

static const load_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD5, TI_C54X_OPCODE_ID_LD_SMEM_MOD6,
     UINT16_C(0x10A8), UINT16_C(0x10B0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, true, LOAD_SIGNED,
     ti_c54x_op_ld_smem_mod5, ti_c54x_op_ld_smem_mod6,
     ti_c54x_trusted_op_ld_smem_mod5, ti_c54x_trusted_op_ld_smem_mod6},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD5, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD6,
     UINT16_C(0x14A8), UINT16_C(0x14B0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, true, LOAD_SIGNED_TS,
     ti_c54x_op_ld_smem_ts_mod5, ti_c54x_op_ld_smem_ts_mod6,
     ti_c54x_trusted_op_ld_smem_ts_mod5,
     ti_c54x_trusted_op_ld_smem_ts_mod6},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD5, TI_C54X_OPCODE_ID_LDU_SMEM_MOD6,
     UINT16_C(0x12A8), UINT16_C(0x12B0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, true, LOAD_UNSIGNED,
     ti_c54x_op_ldu_smem_mod5, ti_c54x_op_ldu_smem_mod6,
     ti_c54x_trusted_op_ldu_smem_mod5, ti_c54x_trusted_op_ldu_smem_mod6},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD5, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD6,
     UINT16_C(0x30A8), UINT16_C(0x30B0), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, false, LOAD_T, ti_c54x_op_ld_t_smem_mod5,
     ti_c54x_op_ld_t_smem_mod6, ti_c54x_trusted_op_ld_t_smem_mod5,
     ti_c54x_trusted_op_ld_t_smem_mod6},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD5,
     TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD6, UINT16_C(0x46A8), UINT16_C(0x46B0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, false,
     LOAD_DP, ti_c54x_op_ld_dp_smem_mod5, ti_c54x_op_ld_dp_smem_mod6,
     ti_c54x_trusted_op_ld_dp_smem_mod5,
     ti_c54x_trusted_op_ld_dp_smem_mod6},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD5,
     TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD6, UINT16_C(0x32A8), UINT16_C(0x32B0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, false,
     LOAD_ASM, ti_c54x_op_ld_asm_smem_mod5, ti_c54x_op_ld_asm_smem_mod6,
     ti_c54x_trusted_op_ld_asm_smem_mod5,
     ti_c54x_trusted_op_ld_asm_smem_mod6},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD5,
     TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD6, UINT16_C(0x44A8),
     UINT16_C(0x44B0), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, true, LOAD_SHIFT16,
     ti_c54x_op_ld_smem_shift16_mod5, ti_c54x_op_ld_smem_shift16_mod6,
     ti_c54x_trusted_op_ld_smem_shift16_mod5,
     ti_c54x_trusted_op_ld_smem_shift16_mod6},
    {TI_C54X_OPCODE_ID_LDR_MOD5, TI_C54X_OPCODE_ID_LDR_MOD6,
     UINT16_C(0x16A8), UINT16_C(0x16B0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, true, LOAD_ROUNDED,
     ti_c54x_op_ldr_mod5, ti_c54x_op_ldr_mod6,
     ti_c54x_trusted_op_ldr_mod5, ti_c54x_trusted_op_ldr_mod6},
};

static ti_c54x_decoded_instruction
decode_case(const load_case *test_case, uint8_t mode)
{
    const uint8_t destination = UINT8_C(1);
    const uint8_t arf = UINT8_C(5);
    const uint16_t base = mode == UINT8_C(5) ? test_case->base5
                                             : test_case->base6;
    const uint16_t word = (uint16_t)(base | arf |
                                     (test_case->has_destination
                                          ? (uint16_t)(destination << 8U)
                                          : UINT16_C(0)));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0xCAFE)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id ==
                       (mode == UINT8_C(5) ? test_case->id5 : test_case->id6));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == first.id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(instruction.input_resources == test_case->resources);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_operands(const ti_c54x_decoded_instruction *instruction,
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
    TI_C54X_TEST_CHECK(operands.count <= TI_C54X_TRUSTED_OPERAND_CAPACITY);
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return operands;
}

static ti_c54x_op_input_snapshot
valid_input(const load_case *test_case)
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

static void
assert_indexed_read(const ti_c54x_op_effect *effect, uint8_t mode)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == 0);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.indexed_mode ==
                       (ti_c54x_indirect_indexed_mode)mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == UINT8_C(5));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static uint64_t
sign_extend_word(uint16_t value)
{
    return (value & UINT16_C(0x8000)) != 0U
               ? (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | value
               : value;
}

static void
assert_completion(const load_case *test_case,
                  const ti_c54x_decoded_instruction *instruction,
                  const ti_c54x_op_input_snapshot *input,
                  const ti_c54x_op_effect *read_effect,
                  uint16_t read_value)
{
    ti_c54x_op_effect completed = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    const uint64_t signed_value = sign_extend_word(read_value);

    TI_C54X_TEST_CHECK(read_effect->payload.memory.complete(
                           instruction, input, read_value, &completed) ==
                       TI_C54X_OP_STATUS_OK);
    switch (test_case->transform) {
    case LOAD_SIGNED:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination == 1U);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           signed_value);
        break;
    case LOAD_SIGNED_TS:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination == 1U);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           ((signed_value << 1U) & TI_C54X_U40_MASK));
        break;
    case LOAD_UNSIGNED:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination == 1U);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           read_value);
        break;
    case LOAD_T:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_T_REGISTER_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.t_register_write.raw_value ==
                           read_value);
        break;
    case LOAD_DP:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.target ==
                           TI_C54X_OP_STATUS_FIELD_TARGET_DP);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.raw_value ==
                           (read_value & UINT16_C(0x01FF)));
        break;
    case LOAD_ASM:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.target ==
                           TI_C54X_OP_STATUS_FIELD_TARGET_ASM);
        TI_C54X_TEST_CHECK(completed.payload.status_field_write.raw_value ==
                           (read_value & UINT16_C(0x001F)));
        break;
    case LOAD_SHIFT16:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination == 1U);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           ((signed_value << 16U) & TI_C54X_U40_MASK));
        break;
    case LOAD_ROUNDED:
        TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination == 1U);
        TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                           (((signed_value << 16U) + UINT64_C(0x8000)) &
                            TI_C54X_U40_MASK));
        TI_C54X_TEST_CHECK(
            completed.payload.accumulator_write.status_delta.c_action ==
            TI_C54X_OP_STATUS_BIT_PRESERVE);
        TI_C54X_TEST_CHECK(
            completed.payload.accumulator_write.status_delta.tc_action ==
            TI_C54X_OP_STATUS_BIT_PRESERVE);
        TI_C54X_TEST_CHECK(
            completed.payload.accumulator_write.status_delta
                .destination_overflow_action ==
            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
        break;
    }
}

static void
check_case(const load_case *test_case, uint8_t mode)
{
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, mode);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction, test_case->layout);
    const ti_c54x_op_input_snapshot input = valid_input(test_case);
    checked_handler checked = mode == UINT8_C(5) ? test_case->checked5
                                                 : test_case->checked6;
    ti_c54x_trusted_op_handler trusted = mode == UINT8_C(5)
                                             ? test_case->trusted5
                                             : test_case->trusted6;
    ti_c54x_op_effect checked_effect = {0};
    ti_c54x_op_effect trusted_effect = {0};

    TI_C54X_TEST_CHECK(checked(&instruction, &input, &checked_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted(&instruction, &operands, &input, &trusted_effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_indexed_read(&checked_effect, mode);
    assert_indexed_read(&trusted_effect, mode);
    TI_C54X_TEST_CHECK(checked_effect.payload.memory.address_spec.arf ==
                       trusted_effect.payload.memory.address_spec.arf);
    assert_completion(test_case, &instruction, &input, &checked_effect,
                      UINT16_C(0x8123));
    assert_completion(test_case, &instruction, &input, &trusted_effect,
                      UINT16_C(0x8123));
}

static void
check_ts_input_rejection(void)
{
    const load_case *test_case = &cases[1];
    const ti_c54x_decoded_instruction instruction =
        decode_case(test_case, UINT8_C(5));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction, test_case->layout);
    ti_c54x_op_input_snapshot input = valid_input(test_case);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect effect = sentinel;

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(test_case->checked5(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->trusted5(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    input.t_register_known = true;
    input.t_register_raw16 = UINT16_C(0x20);
    TI_C54X_TEST_CHECK(test_case->checked5(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->trusted5(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

int
main(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_case(&cases[index], UINT8_C(5));
        check_case(&cases[index], UINT8_C(6));
    }
    check_ts_input_rejection();
    return 0;
}
