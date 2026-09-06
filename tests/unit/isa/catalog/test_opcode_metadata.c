/*
 * Module contract
 *
 * Owns: executable proof that catalog-owned layout and repeatability tokens
 * remain scalar, immutable metadata and are copied through decode without
 * pointer retention. It also covers the generic dispatch and NOP validation
 * seams that consume those values.
 * Does not own: catalog generation, operand extraction, repeat sequencing,
 * CPU/state integration, timing, buses, observers, diagnostics, or harness
 * policy.
 */

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/instruction_bundle.h"
#include "isa/decode/opcode_decode.h"
#include "isa/ops/opcode_dispatch.h"
#include "isa/ops/op_nop.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static const ti_c54x_op_input_snapshot zero_input = {0};

#define TI_C54X_NOP_CALL(instruction, effect) \
    ti_c54x_op_nop((instruction), &zero_input, (effect))

static ti_c54x_decoded_instruction
valid_nop(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    return (ti_c54x_decoded_instruction){
        .id = metadata->id,
        .operand_layout = metadata->operand_layout,
        .repeatability = metadata->repeatability,
        .bundle = {
            .program_words = {metadata->first_word_value, UINT16_C(0)},
            .program_word_count = metadata->program_word_count}};
}

static void
test_enum_ranges_and_metadata_values(void)
{
    const ti_c54x_opcode_metadata *unknown =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_UNKNOWN);
    const ti_c54x_opcode_metadata *nop =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    int id;

    TI_C54X_TEST_CHECK(TI_C54X_OPERAND_LAYOUT_UNKNOWN == 0);
    TI_C54X_TEST_CHECK(TI_C54X_OPERAND_LAYOUT_NONE >
                       TI_C54X_OPERAND_LAYOUT_UNKNOWN);
    TI_C54X_TEST_CHECK(TI_C54X_OPERAND_LAYOUT_COUNT >
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(TI_C54X_REPEATABILITY_UNRESOLVED == 0);
    TI_C54X_TEST_CHECK(TI_C54X_REPEATABILITY_ELIGIBLE >
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(TI_C54X_REPEATABILITY_INELIGIBLE >
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(TI_C54X_REPEATABILITY_COUNT >
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(unknown != NULL);
    TI_C54X_TEST_CHECK(unknown->id == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(unknown->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_UNKNOWN);
    TI_C54X_TEST_CHECK(unknown->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(nop != NULL);
    TI_C54X_TEST_CHECK(nop->operand_layout == TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(nop->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(nop->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           TI_C54X_OPCODE_ID_MAX_ACC)->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           TI_C54X_OPCODE_ID_CMPL_ACC)->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);

    for (id = (int)TI_C54X_OPCODE_ID_UNKNOWN;
         id < (int)TI_C54X_OPCODE_ID_COUNT;
         ++id) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id((ti_c54x_opcode_id)id);
        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == (ti_c54x_opcode_id)id);
        if (metadata->id != TI_C54X_OPCODE_ID_UNKNOWN) {
            TI_C54X_TEST_CHECK(metadata->operand_layout >
                               TI_C54X_OPERAND_LAYOUT_UNKNOWN);
            TI_C54X_TEST_CHECK(metadata->operand_layout <
                               TI_C54X_OPERAND_LAYOUT_COUNT);
            TI_C54X_TEST_CHECK(metadata->repeatability >=
                               TI_C54X_REPEATABILITY_UNRESOLVED);
            TI_C54X_TEST_CHECK(metadata->repeatability <
                               TI_C54X_REPEATABILITY_COUNT);
        }
    }
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           (ti_c54x_opcode_id)TI_C54X_OPCODE_ID_COUNT) == NULL);
}

static void
test_completion_copies_tokens_transactionally(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    ti_c54x_decode_result first =
        ti_c54x_decode_first_word(metadata->first_word_value);
    ti_c54x_instruction_word_bundle bundle = {
        .program_words = {metadata->first_word_value, UINT16_C(0)},
        .program_word_count = metadata->program_word_count};
    ti_c54x_decoded_instruction decoded = {0};

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &decoded) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(decoded.operand_layout == metadata->operand_layout);
    TI_C54X_TEST_CHECK(decoded.repeatability == metadata->repeatability);
    TI_C54X_TEST_CHECK(decoded.input_resources == metadata->input_resources);

    first.first_word = UINT16_C(0);
    bundle.program_words[0] = UINT16_C(0);
    TI_C54X_TEST_CHECK(decoded.operand_layout == TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(decoded.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
}

static void
test_forged_tokens_are_rejected_transactionally(void)
{
    ti_c54x_decoded_instruction instruction = valid_nop();
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    instruction.operand_layout = TI_C54X_OPERAND_LAYOUT_UNKNOWN;
    (void)memset(&effect, 0xa5, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction = valid_nop();
    instruction.operand_layout = (ti_c54x_operand_layout_id)-1;
    (void)memset(&effect, 0x96, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction = valid_nop();
    instruction.repeatability = TI_C54X_REPEATABILITY_COUNT;
    (void)memset(&effect, 0x5a, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(TI_C54X_NOP_CALL(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction = valid_nop();
    instruction.repeatability = (ti_c54x_repeatability_class)-1;
    (void)memset(&effect, 0x69, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(TI_C54X_NOP_CALL(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);

    instruction = valid_nop();
    instruction.input_resources = (ti_c54x_input_resource_mask)0x80U;
    (void)memset(&effect, 0x71, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
}

static void
test_null_completion_arguments_are_rejected(void)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0xF495));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0xF495), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    (void)memset(&output, 0xc3, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           NULL, &bundle, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, NULL, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, NULL) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_enum_ranges_and_metadata_values();
    test_completion_copies_tokens_transactionally();
    test_forged_tokens_are_rejected_transactionally();
    test_null_completion_arguments_are_rejected();
    return 0;
}
