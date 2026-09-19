/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the two-word absolute-DATA
 * MMR load effect, including MMR/address extraction,
 * checked/trusted/dispatch parity, ignored input state,
 * transactional rejection, and the trusted registry binding.
 * Does not own: catalog rows, operand-layout descriptors,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, or the
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
#include "../../../../../src/isa/ops/store/op_mvmd.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t
mvmd_word0(uint8_t mmr)
{
    return (uint16_t)(UINT16_C(0x7300) | (mmr & UINT8_C(0x1F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t mmr, uint16_t address)
{
    const uint16_t first_word = mvmd_word0(mmr);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, address},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MVMD);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_MVMD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_MMR5_ABSOLUTE16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == address);
    return instruction;
}

static ti_c54x_op_input_snapshot
ignored_input(void)
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

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t mmr, uint16_t address)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = (uint64_t)(mmr & UINT8_C(0x1F));
    operands.values[0].width_bits = UINT8_C(5);
    operands.values[1].raw_value = address;
    operands.values[1].width_bits = UINT8_C(16);
    return operands;
}

static void
assert_absolute_load(const ti_c54x_op_effect *effect, uint8_t mmr,
                     uint16_t address)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.absolute_data.address ==
        address);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.absolute_data
            .mmr_sidecar_kind ==
        (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.absolute_data
            .mmr_sidecar_number == (mmr & UINT8_C(0x1F)));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    /* The resume path snapshots the MMR: the leaf stages zero so
     * forged nonzero values cannot bypass the snapshot. */
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_absolute_form(uint8_t mmr, uint16_t address)
{
    const ti_c54x_decoded_instruction instruction =
        decode(mmr, address);
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(mmr, address);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_MVMD);

    TI_C54X_TEST_CHECK(ti_c54x_op_mvmd(&instruction, &input,
                                       &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_absolute_load(&checked, mmr, address);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmd(&instruction, &operands,
                                               &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_mvmd);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_opcode_binding_is_canonical(binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) ==
                       0);
}

static void
test_absolute_forms(void)
{
    /* MMR edges with low and high addresses. */
    check_absolute_form(UINT8_C(0x00), UINT16_C(0x0000));
    check_absolute_form(UINT8_C(0x05), UINT16_C(0x0020));
    check_absolute_form(UINT8_C(0x14), UINT16_C(0x0000));
    check_absolute_form(UINT8_C(0x14), UINT16_C(0xABCD));
    check_absolute_form(UINT8_C(0x1F), UINT16_C(0xFFFF));
}

static void
test_short_bundle_rejects(void)
{
    const ti_c54x_decoded_instruction full =
        decode(UINT8_C(0x14), UINT16_C(0x0020));
    ti_c54x_decoded_instruction truncated = full;
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    truncated.bundle.program_word_count = UINT8_C(1);
    (void)memset(&effect, 0xA5, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_op_mvmd(&truncated, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
}

static void
test_trusted_operand_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x14), UINT16_C(0x0020));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x14), UINT16_C(0x0020));
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmd(&instruction, &operands,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands = trusted_operands(UINT8_C(0x14), UINT16_C(0x0020));
    operands.values[0].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmd(&instruction, &operands,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void
test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x14), UINT16_C(0x0020));
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x14), UINT16_C(0x0020));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mvmd(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvmd(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvmd(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmd(&instruction, NULL,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmd(&instruction, &operands,
                                               &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_absolute_forms();
    test_short_bundle_rejects();
    test_trusted_operand_rejects();
    test_null_rejects();
    return 0;
}
