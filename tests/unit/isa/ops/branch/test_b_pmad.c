/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for two-word B pmad effects, including
 * verbatim pmad carriage over the full 16-bit space, PC_WRITE kind
 * with no status/accumulator/T/memory effect, input independence
 * under NONE resources, checked/trusted/dispatch parity, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_b_pmad.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t B_PMAD_WORD0 = UINT16_C(0xF073);

static ti_c54x_decoded_instruction
decode(uint16_t pmad)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(B_PMAD_WORD0);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {B_PMAD_WORD0, pmad},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_B_PMAD);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_B_PMAD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t seed)
{
    /* NONE resources: the leaf must ignore every input field, so clean
     * and hostile seeds must produce identical effects. */
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
    input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
    input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_tc = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != 0U;
    input.status_ovm = (uint8_t)((seed >> 2U) & UINT8_C(1));
    input.status_ovm_known = (seed & UINT8_C(16)) != 0U;
    input.status_sxm = (uint8_t)((seed >> 3U) & UINT8_C(1));
    input.status_sxm_known = (seed & UINT8_C(32)) != 0U;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint16_t pmad)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = pmad;
    operands.values[0].width_bits = UINT8_C(16);
    return operands;
}

static void
assert_pc_write(const ti_c54x_op_effect *effect, uint16_t pmad)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_PC_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.pc_write.pmad == pmad);
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_B_PMAD);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_B_PMAD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == B_PMAD_WORD0);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_B_PMAD);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_b_pmad);

    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t pmad = (uint16_t)raw;
        ti_c54x_decoded_instruction instruction = decode(pmad);
        ti_c54x_trusted_execution_packet packet;
        const ti_c54x_op_input_snapshot clean = input_for(UINT8_C(0));
        const ti_c54x_op_input_snapshot hostile =
            input_for(UINT8_C(0x1D));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(pmad);
        ti_c54x_op_effect checked = {0};
        ti_c54x_op_effect checked_hostile = {0};
        ti_c54x_op_effect trusted = {0};
        ti_c54x_op_effect via_dispatch = {0};
        ti_c54x_op_effect via_trusted = {0};

        /* The extension word is a verbatim copy: varying it exercises
         * the leaf transform without re-decoding. */
        instruction.bundle.program_words[1] = pmad;
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &instruction.bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&instruction, &clean,
                                            &checked) ==
                           TI_C54X_OP_STATUS_OK);
        assert_pc_write(&checked, pmad);
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&instruction, &hostile,
                                            &checked_hostile) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                               &instruction, &operands, &clean,
                               &trusted) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                               &instruction, &clean,
                               &via_dispatch) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &clean,
                                                   &via_trusted) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                  sizeof(checked)) == 0);
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0x2000));
    const ti_c54x_op_input_snapshot valid = input_for(0U);
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT16_C(0x2000));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_input_and_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0x2000));
    const ti_c54x_op_input_snapshot valid = input_for(UINT8_C(0xA5));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    /* NONE resources: unknown and out-of-range SXM states are ignored. */
    {
        ti_c54x_op_input_snapshot unknown_sxm = valid;

        unknown_sxm.status_sxm = UINT8_C(0);
        unknown_sxm.status_sxm_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&instruction, &unknown_sxm,
                                            &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_pc_write(&effect, UINT16_C(0x2000));
        effect = before;
    }

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0001);
        TI_C54X_TEST_CHECK(ti_c54x_op_b_pmad(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0x2000));
    const ti_c54x_op_input_snapshot input = input_for(0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT16_C(0x2000));

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT16_C(0x2000));
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT16_C(0x2000));
    operands.values[0].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_b_pmad(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_argument_rejection();
    check_input_and_decode_rejection();
    check_trusted_operand_rejection();
    return 0;
}
