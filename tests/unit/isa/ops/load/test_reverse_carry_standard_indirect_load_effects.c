/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the seven load-family
 * reverse-carry MOD4/MOD7 WORD16 effects, checked/trusted parity, and ARF
 * rejection. Does not own: catalog generation, decode/layout, address
 * planning, buses, CPU state, sequencing, timing, or diagnostics.
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

#include "../../../../../src/foundation/arithmetic40.h"
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
typedef ti_c54x_op_status (*trusted_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

typedef struct reverse_load_case {
    ti_c54x_opcode_id mod4_id;
    ti_c54x_opcode_id mod7_id;
    uint16_t mod4_base;
    uint16_t mod7_base;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    uint8_t has_destination;
    checked_handler checked4;
    checked_handler checked7;
    trusted_handler trusted4;
    trusted_handler trusted7;
} reverse_load_case;

static const reverse_load_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD4, TI_C54X_OPCODE_ID_LD_SMEM_MOD7,
     UINT16_C(0x10A0), UINT16_C(0x10B8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U,
     ti_c54x_op_ld_smem_mod4, ti_c54x_op_ld_smem_mod7,
     ti_c54x_trusted_op_ld_smem_mod4, ti_c54x_trusted_op_ld_smem_mod7},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD4, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD7,
     UINT16_C(0x14A0), UINT16_C(0x14B8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, 1U,
     ti_c54x_op_ld_smem_ts_mod4, ti_c54x_op_ld_smem_ts_mod7,
     ti_c54x_trusted_op_ld_smem_ts_mod4, ti_c54x_trusted_op_ld_smem_ts_mod7},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD4, TI_C54X_OPCODE_ID_LDU_SMEM_MOD7,
     UINT16_C(0x12A0), UINT16_C(0x12B8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 1U,
     ti_c54x_op_ldu_smem_mod4, ti_c54x_op_ldu_smem_mod7,
     ti_c54x_trusted_op_ldu_smem_mod4, ti_c54x_trusted_op_ldu_smem_mod7},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD4, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD7,
     UINT16_C(0x30A0), UINT16_C(0x30B8), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 0U,
     ti_c54x_op_ld_t_smem_mod4, ti_c54x_op_ld_t_smem_mod7,
     ti_c54x_trusted_op_ld_t_smem_mod4, ti_c54x_trusted_op_ld_t_smem_mod7},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD4, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD7,
     UINT16_C(0x46A0), UINT16_C(0x46B8), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 0U,
     ti_c54x_op_ld_dp_smem_mod4, ti_c54x_op_ld_dp_smem_mod7,
     ti_c54x_trusted_op_ld_dp_smem_mod4, ti_c54x_trusted_op_ld_dp_smem_mod7},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD4, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD7,
     UINT16_C(0x32A0), UINT16_C(0x32B8), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 0U,
     ti_c54x_op_ld_asm_smem_mod4, ti_c54x_op_ld_asm_smem_mod7,
     ti_c54x_trusted_op_ld_asm_smem_mod4, ti_c54x_trusted_op_ld_asm_smem_mod7},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD4,
     TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD7, UINT16_C(0x44A0),
     UINT16_C(0x44B8), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U,
     ti_c54x_op_ld_smem_shift16_mod4, ti_c54x_op_ld_smem_shift16_mod7,
     ti_c54x_trusted_op_ld_smem_shift16_mod4,
     ti_c54x_trusted_op_ld_smem_shift16_mod7},
};

static ti_c54x_decoded_instruction decode_case(const reverse_load_case *test_case,
                                               uint8_t mode, uint8_t arf)
{
    const uint16_t base = mode == UINT8_C(4) ? test_case->mod4_base
                                             : test_case->mod7_base;
    const uint16_t word = (uint16_t)(base | arf |
                                     (test_case->has_destination ? UINT16_C(0x0100)
                                                                  : UINT16_C(0)));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0xCAFE)}, .program_word_count = 1U};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == (mode == UINT8_C(4) ? test_case->mod4_id
                                                        : test_case->mod7_id));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(instruction.input_resources == test_case->resources);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_values(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_operand_layout_id layout)
{
    ti_c54x_trusted_operand_values values = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words, instruction->bundle.program_word_count};
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           layout, instruction->bundle.program_word_count,
                           &view) == TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    values.count = view.descriptor->operand_count;
    for (uint8_t index = 0U; index < values.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &values.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return values;
}

static ti_c54x_op_input_snapshot valid_input(
    const reverse_load_case *test_case)
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

static void check_effect(const ti_c54x_op_effect *effect, uint8_t mode,
                         uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.reverse_carry_mode ==
                       (ti_c54x_indirect_reverse_carry_mode)mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void check_family(const reverse_load_case *test_case, uint8_t mode,
                         uint8_t arf)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(test_case, mode, arf);
    const ti_c54x_trusted_operand_values values =
        trusted_values(&instruction, test_case->layout);
    const ti_c54x_op_input_snapshot input = valid_input(test_case);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    checked_handler checked_fn = mode == UINT8_C(4) ? test_case->checked4
                                                   : test_case->checked7;
    trusted_handler trusted_fn = mode == UINT8_C(4) ? test_case->trusted4
                                                   : test_case->trusted7;

    TI_C54X_TEST_CHECK(checked_fn(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted_fn(&instruction, &values, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    check_effect(&checked, mode, arf);
    check_effect(&trusted, mode, arf);
}

static void check_ar0_rejected(void)
{
    const reverse_load_case *test_case = &cases[0];
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, 4U, 0U);
    const ti_c54x_trusted_operand_values values = trusted_values(
        &instruction, test_case->layout);
    const ti_c54x_op_input_snapshot input = valid_input(test_case);
    ti_c54x_op_effect checked = {.kind = TI_C54X_OP_EFFECT_REPEAT_START};
    ti_c54x_op_effect trusted = {.kind = TI_C54X_OP_EFFECT_REPEAT_START};
    TI_C54X_TEST_CHECK(test_case->checked4(&instruction, &input, &checked) !=
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(test_case->trusted4(&instruction, &values, &input,
                                           &trusted) != TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(trusted.kind == TI_C54X_OP_EFFECT_REPEAT_START);
}

static void check_null_contracts(void)
{
    const size_t indices[] = {2U, 3U, 4U, 5U};
    for (size_t index = 0U; index < sizeof(indices) / sizeof(indices[0]); ++index) {
        const reverse_load_case *test_case = &cases[indices[index]];
        const ti_c54x_decoded_instruction instruction =
            decode_case(test_case, 4U, UINT8_C(1));
        const ti_c54x_trusted_operand_values values =
            trusted_values(&instruction, test_case->layout);
        const ti_c54x_op_input_snapshot input = valid_input(test_case);
        const ti_c54x_op_effect sentinel = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
        ti_c54x_op_effect effect;

        effect = sentinel;
        TI_C54X_TEST_CHECK(test_case->checked4(&instruction, NULL, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
        effect = sentinel;
        TI_C54X_TEST_CHECK(test_case->checked4(&instruction, &input, NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(test_case->trusted4(&instruction, &values, NULL,
                                               &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
        effect = sentinel;
        TI_C54X_TEST_CHECK(test_case->trusted4(&instruction, &values, &input,
                                               NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    }
}

static void check_completion_null_contracts(void)
{
    static const size_t indices[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U};

    for (size_t index = 0U; index < sizeof(indices) / sizeof(indices[0]);
         ++index) {
        const reverse_load_case *test_case = &cases[indices[index]];
        const ti_c54x_decoded_instruction instruction =
            decode_case(test_case, 4U, UINT8_C(1));
        const ti_c54x_op_input_snapshot input = valid_input(test_case);
        ti_c54x_op_effect read_effect = {0};
        const ti_c54x_op_effect sentinel = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
        ti_c54x_op_effect effect;

        TI_C54X_TEST_CHECK(test_case->checked4(&instruction, &input,
                                               &read_effect) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(read_effect.kind == TI_C54X_OP_EFFECT_MEMORY);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete != NULL);

        effect = sentinel;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               NULL, &input, UINT16_C(0x8123), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

        effect = sentinel;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &instruction, NULL, UINT16_C(0x8123), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

        effect = sentinel;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &instruction, &input, UINT16_C(0x8123), NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    }
}

static void check_transform_outputs(void)
{
    const uint16_t read_value = UINT16_C(0x8123);

    for (uint8_t mode = UINT8_C(4); mode <= UINT8_C(7); mode += UINT8_C(3)) {
        for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
            const reverse_load_case *test_case = &cases[index];
            const ti_c54x_decoded_instruction instruction =
                decode_case(test_case, mode, UINT8_C(1));
            const ti_c54x_op_input_snapshot input = valid_input(test_case);
            ti_c54x_op_effect memory = {0};
            ti_c54x_op_effect completed = {0};
            checked_handler checked = mode == UINT8_C(4)
                                          ? test_case->checked4
                                          : test_case->checked7;

            TI_C54X_TEST_CHECK(checked(&instruction, &input, &memory) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memory.payload.memory.complete(
                                   &instruction, &input, read_value,
                                   &completed) == TI_C54X_OP_STATUS_OK);
            switch (index) {
            case 0U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                                   ((TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                                    read_value));
                break;
            case 1U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                                   ((((TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                                      read_value)
                                     << 1U) &
                                    TI_C54X_U40_MASK));
                break;
            case 2U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                                   read_value);
                break;
            case 3U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_T_REGISTER_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.t_register_write.raw_value ==
                                   read_value);
                break;
            case 4U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.status_field_write.target ==
                                   TI_C54X_OP_STATUS_FIELD_TARGET_DP);
                TI_C54X_TEST_CHECK(completed.payload.status_field_write.raw_value ==
                                   (read_value & UINT16_C(0x01FF)));
                break;
            case 5U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.status_field_write.target ==
                                   TI_C54X_OP_STATUS_FIELD_TARGET_ASM);
                TI_C54X_TEST_CHECK(completed.payload.status_field_write.raw_value ==
                                   (read_value & UINT16_C(0x001F)));
                break;
            case 6U:
                TI_C54X_TEST_CHECK(completed.kind ==
                                   TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
                TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                                   ((((TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                                      read_value)
                                     << 16U) &
                                    TI_C54X_U40_MASK));
                break;
            default:
                TI_C54X_TEST_CHECK(false);
                break;
            }
        }
    }
}

static void check_trusted_operands_drive_effects(void)
{
    const reverse_load_case *acc_case = &cases[0];
    const ti_c54x_decoded_instruction acc_instruction =
        decode_case(acc_case, 4U, UINT8_C(1));
    const ti_c54x_op_input_snapshot acc_input = valid_input(acc_case);
    ti_c54x_trusted_operand_values acc_values =
        trusted_values(&acc_instruction, acc_case->layout);
    ti_c54x_op_effect acc_effect = {0};
    ti_c54x_op_effect acc_completed = {0};

    acc_values.values[0].raw_value = UINT64_C(1);
    acc_values.values[1].raw_value = UINT64_C(7);
    TI_C54X_TEST_CHECK(acc_case->trusted4(
                           &acc_instruction, &acc_values, &acc_input,
                           &acc_effect) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(acc_effect.payload.memory.address_spec.arf ==
                       UINT8_C(7));
    TI_C54X_TEST_CHECK(acc_effect.payload.memory.complete(
                           &acc_instruction, &acc_input, UINT16_C(0x8000),
                           &acc_completed) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(acc_completed.kind ==
                       TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(acc_completed.payload.accumulator_write.destination ==
                       UINT8_C(1));

    {
        const reverse_load_case *arf_case = &cases[3];
        const ti_c54x_decoded_instruction arf_instruction =
            decode_case(arf_case, 4U, UINT8_C(1));
        const ti_c54x_op_input_snapshot arf_input = valid_input(arf_case);
        ti_c54x_trusted_operand_values arf_values =
            trusted_values(&arf_instruction, arf_case->layout);
        ti_c54x_op_effect arf_effect = {0};

        arf_values.values[0].raw_value = UINT64_C(7);
        TI_C54X_TEST_CHECK(arf_case->trusted4(
                               &arf_instruction, &arf_values, &arf_input,
                               &arf_effect) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(arf_effect.payload.memory.address_spec.arf ==
                           UINT8_C(7));
    }
}

int main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_family(&cases[i], 4U, UINT8_C(1));
        check_family(&cases[i], 4U, UINT8_C(7));
        check_family(&cases[i], 7U, UINT8_C(1));
        check_family(&cases[i], 7U, UINT8_C(7));
    }
    check_ar0_rejected();
    check_null_contracts();
    check_completion_null_contracts();
    check_trusted_operands_drive_effects();
    check_transform_outputs();
    return 0;
}
