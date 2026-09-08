/*
 * Module contract
 *
 * Owns: executable proof for the three circular standard-indirect WORD16
 * store families. Every MOD8/MOD10 checked and trusted leaf must stage a
 * CIRCULAR_WORD16 write with the exact source payload, AR0--AR7 selection,
 * and no completion callback. Does not own: catalog generation, decode/layout
 * implementation, address planning, buses, CPU state, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

extern ti_c54x_op_status ti_c54x_op_stl_smem_mod8(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_stl_smem_mod10(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_immediate_smem_mod8(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_immediate_smem_mod10(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_t_smem_mod8(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_t_smem_mod10(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

extern ti_c54x_op_status ti_c54x_trusted_op_stl_smem_mod8(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_stl_smem_mod10(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_st_immediate_smem_mod8(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_st_immediate_smem_mod10(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_mod8(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_mod10(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

typedef enum store_payload {
    STORE_LOW_ACCUMULATOR = 0,
    STORE_IMMEDIATE = 1,
    STORE_T_REGISTER = 2
} store_payload;

typedef struct circular_store_case {
    ti_c54x_opcode_id id8;
    ti_c54x_opcode_id id10;
    uint16_t base8;
    uint16_t base10;
    uint8_t words;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    store_payload payload;
    checked_handler checked8;
    checked_handler checked10;
    ti_c54x_trusted_op_handler trusted8;
    ti_c54x_trusted_op_handler trusted10;
} circular_store_case;

static const circular_store_case cases[] = {
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD8,
     TI_C54X_OPCODE_ID_STL_SMEM_MOD10,
     UINT16_C(0x80C0), UINT16_C(0x80D0), UINT8_C(1),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, STORE_LOW_ACCUMULATOR,
     ti_c54x_op_stl_smem_mod8, ti_c54x_op_stl_smem_mod10,
     ti_c54x_trusted_op_stl_smem_mod8, ti_c54x_trusted_op_stl_smem_mod10},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD8,
     TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD10,
     UINT16_C(0x76C0), UINT16_C(0x76D0), UINT8_C(2),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, STORE_IMMEDIATE,
     ti_c54x_op_st_immediate_smem_mod8, ti_c54x_op_st_immediate_smem_mod10,
     ti_c54x_trusted_op_st_immediate_smem_mod8,
     ti_c54x_trusted_op_st_immediate_smem_mod10},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD8,
     TI_C54X_OPCODE_ID_ST_T_SMEM_MOD10,
     UINT16_C(0x8CC0), UINT16_C(0x8CD0), UINT8_C(1),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_T_REGISTER, STORE_T_REGISTER,
     ti_c54x_op_st_t_smem_mod8, ti_c54x_op_st_t_smem_mod10,
     ti_c54x_trusted_op_st_t_smem_mod8,
     ti_c54x_trusted_op_st_t_smem_mod10},
};

static ti_c54x_decoded_instruction
decode_case(const circular_store_case *test_case, uint8_t mode, uint8_t arf)
{
    const uint16_t base = mode == UINT8_C(8) ? test_case->base8
                                             : test_case->base10;
    const uint16_t word = (uint16_t)(base | (uint16_t)(arf & UINT8_C(7)) |
                                     (test_case->payload == STORE_LOW_ACCUMULATOR
                                          ? UINT16_C(0x0100)
                                          : UINT16_C(0)));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0xC53A)},
        .program_word_count = test_case->words};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id ==
                       (mode == UINT8_C(8) ? test_case->id8
                                           : test_case->id10));
    TI_C54X_TEST_CHECK(first.program_word_count == test_case->words);
    {
        const ti_c54x_instruction_completion_status completion =
            ti_c54x_decode_complete_instruction(&first, &bundle, &instruction);
        TI_C54X_TEST_CHECK(completion ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    }
    TI_C54X_TEST_CHECK(instruction.id == first.id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(instruction.input_resources == test_case->resources);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_operands(const circular_store_case *test_case,
                 const ti_c54x_decoded_instruction *instruction)
{
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words, instruction->bundle.program_word_count};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           test_case->layout, test_case->words, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    operands.count = view.descriptor->operand_count;
    TI_C54X_TEST_CHECK(operands.count <= TI_C54X_TRUSTED_OPERAND_CAPACITY);
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return operands;
}

static ti_c54x_op_input_snapshot valid_input(void)
{
    ti_c54x_op_input_snapshot input = {0};
    input.accumulator_a_raw40 = UINT64_C(0x0000001234);
    input.accumulator_b_raw40 = UINT64_C(0x000000BEEF);
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0xCAFE);
    input.t_register_known = true;
    return input;
}

static void assert_circular_write(const ti_c54x_op_effect *effect, uint8_t mode,
                                  uint8_t arf, uint16_t expected_value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == 0);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.indexed_mode == 0);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.circular_mode ==
                       (ti_c54x_indirect_circular_mode)mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word ==
                       expected_value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_case(const circular_store_case *test_case, uint8_t mode,
                       uint8_t arf)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(test_case, mode, arf);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(test_case, &instruction);
    const ti_c54x_op_input_snapshot input = valid_input();
    checked_handler checked = mode == UINT8_C(8) ? test_case->checked8
                                                 : test_case->checked10;
    ti_c54x_trusted_op_handler trusted = mode == UINT8_C(8)
                                             ? test_case->trusted8
                                             : test_case->trusted10;
    const uint16_t expected =
        test_case->payload == STORE_LOW_ACCUMULATOR ? UINT16_C(0xBEEF)
        : test_case->payload == STORE_IMMEDIATE ? UINT16_C(0xC53A)
                                                : UINT16_C(0xCAFE);
    ti_c54x_op_effect checked_effect = {0};
    ti_c54x_op_effect trusted_effect = {0};

    TI_C54X_TEST_CHECK(checked(&instruction, &input, &checked_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted(&instruction, &operands, &input,
                               &trusted_effect) == TI_C54X_OP_STATUS_OK);
    assert_circular_write(&checked_effect, mode, arf, expected);
    assert_circular_write(&trusted_effect, mode, arf, expected);
    TI_C54X_TEST_CHECK(memcmp(&checked_effect, &trusted_effect,
                              sizeof(checked_effect)) == 0);
}

static void test_all_families_modes_and_arfs(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        for (uint8_t mode = UINT8_C(8); mode <= UINT8_C(10); mode += 2U) {
            check_case(&cases[index], mode, UINT8_C(0));
            check_case(&cases[index], mode, UINT8_C(7));
        }
    }
}

static void test_malformed_checked_and_trusted_inputs_are_transactional(void)
{
    const circular_store_case *test_case = &cases[2];
    const ti_c54x_decoded_instruction valid =
        decode_case(test_case, UINT8_C(8), UINT8_C(0));
    const ti_c54x_trusted_operand_values trusted =
        trusted_operands(test_case, &valid);
    const ti_c54x_op_input_snapshot input = valid_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
        .payload.t_register_write = {.raw_value = UINT16_C(0x55AA)}};

    {
        ti_c54x_decoded_instruction forged = valid;
        ti_c54x_op_effect effect = before;
        forged.id = test_case->id10;
        TI_C54X_TEST_CHECK(test_case->checked8(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_trusted_operand_values malformed = trusted;
        ti_c54x_op_effect effect = before;
        malformed.values[1].raw_value = UINT64_C(16);
        TI_C54X_TEST_CHECK(test_case->trusted8(
                               &valid, &malformed, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_trusted_operand_values malformed = trusted;
        ti_c54x_op_effect effect = before;
        malformed.values[1].width_bits = UINT8_C(2);
        TI_C54X_TEST_CHECK(test_case->trusted8(
                               &valid, &malformed, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void test_unknown_payload_inputs_are_transactional(void)
{
    const ti_c54x_op_input_snapshot input = valid_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
        .payload.t_register_write = {.raw_value = UINT16_C(0x55AA)}};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const circular_store_case *test_case = &cases[index];
        ti_c54x_decoded_instruction instruction =
            decode_case(test_case, UINT8_C(8), UINT8_C(7));
        ti_c54x_op_input_snapshot unknown = input;
        ti_c54x_op_effect effect = before;

        if (test_case->payload == STORE_LOW_ACCUMULATOR) {
            unknown.accumulator_b_known = false;
            TI_C54X_TEST_CHECK(test_case->checked8(
                                   &instruction, &unknown, &effect) ==
                               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        } else if (test_case->payload == STORE_T_REGISTER) {
            unknown.t_register_known = false;
            TI_C54X_TEST_CHECK(test_case->checked8(
                                   &instruction, &unknown, &effect) ==
                               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        } else {
            continue;
        }
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    test_all_families_modes_and_arfs();
    test_malformed_checked_and_trusted_inputs_are_transactional();
    test_unknown_payload_inputs_are_transactional();
    return 0;
}
