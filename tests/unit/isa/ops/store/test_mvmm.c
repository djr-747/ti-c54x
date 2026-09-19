/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the one-word MMR-to-MMR move
 * effect, including nibble extraction, checked/trusted/dispatch
 * parity, ignored input state, transactional rejection of the
 * seven illegal nibble codes per side, and the trusted registry
 * binding.
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
#include "../../../../../src/isa/ops/store/op_mvmm.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* TI SPRU172C p. 4-116: MMRX rides bits 7:4, MMRY rides bits
 * 3:0; only codes 0-8 (AR0-AR7, SP) are legal. */
static uint16_t
mvmm_word0(uint8_t mmrx, uint8_t mmry)
{
    return (uint16_t)(UINT16_C(0xE700) | ((uint16_t)(mmrx & UINT8_C(0x0F))
                                          << 4U) |
                      (mmry & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t mmrx, uint8_t mmry)
{
    const uint16_t first_word = mvmm_word0(mmrx, mmry);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MVMM);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_MVMM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_MMR4_MMR4);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       UINT8_C(1));
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
trusted_operands(uint8_t mmrx, uint8_t mmry)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = (uint64_t)(mmrx & UINT8_C(0x0F));
    operands.values[0].width_bits = UINT8_C(4);
    operands.values[1].raw_value = (uint64_t)(mmry & UINT8_C(0x0F));
    operands.values[1].width_bits = UINT8_C(4);
    return operands;
}

static void
assert_mmr_move(const ti_c54x_op_effect *effect, uint8_t mmrx,
                uint8_t mmry)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16);
    /* The MMRx cell is read through its DATA-mapped address. */
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.absolute_data.address ==
        (uint16_t)(UINT16_C(0x0010) + (mmrx & UINT8_C(0x0F))));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.absolute_data
            .mmr_sidecar_kind ==
        (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.absolute_data
            .mmr_sidecar_number ==
        (uint8_t)(UINT16_C(0x0010) + (mmry & UINT8_C(0x0F))));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_move_form(uint8_t mmrx, uint8_t mmry)
{
    const ti_c54x_decoded_instruction instruction =
        decode(mmrx, mmry);
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(mmrx, mmry);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_MVMM);

    TI_C54X_TEST_CHECK(ti_c54x_op_mvmm(&instruction, &input,
                                       &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_mmr_move(&checked, mmrx, mmry);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, &operands,
                                               &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_mvmm);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_opcode_binding_is_canonical(binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) ==
                       0);
}

static void
test_move_forms(void)
{
    /* Nibble edges: AR0/AR7/SP corners and the TI example. */
    check_move_form(UINT8_C(0x0), UINT8_C(0x0));
    check_move_form(UINT8_C(0x8), UINT8_C(0x1));
    check_move_form(UINT8_C(0x1), UINT8_C(0x8));
    check_move_form(UINT8_C(0x7), UINT8_C(0x7));
    check_move_form(UINT8_C(0x8), UINT8_C(0x8));
    check_move_form(UINT8_C(0x0), UINT8_C(0x8));
}

static void
test_illegal_nibble_rejects(void)
{
    /* Codes 9-15 address routed BK/BRC/RSA/REA/PMST cells the
     * hardware form never touches: the leaf must reject them
     * even though the first-word row still classifies. */
    static const uint8_t illegal[] = {9, 10, 11, 12, 13, 14, 15};
    const ti_c54x_op_input_snapshot input = ignored_input();
    size_t index;

    for (index = 0U; index < sizeof(illegal) / sizeof(illegal[0]);
         ++index) {
        const ti_c54x_decoded_instruction bad_source =
            decode(illegal[index], UINT8_C(0x1));
        const ti_c54x_decoded_instruction bad_destination =
            decode(UINT8_C(0x1), illegal[index]);
        ti_c54x_op_effect effect;

        (void)memset(&effect, 0xA5, sizeof(effect));
        TI_C54X_TEST_CHECK(ti_c54x_op_mvmm(&bad_source, &input,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        (void)memset(&effect, 0xA5, sizeof(effect));
        TI_C54X_TEST_CHECK(ti_c54x_op_mvmm(&bad_destination, &input,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
}

static void
test_trusted_operand_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x8), UINT8_C(0x1));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x8), UINT8_C(0x1));
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, &operands,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands = trusted_operands(UINT8_C(0x8), UINT8_C(0x1));
    operands.values[0].width_bits = UINT8_C(5);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, &operands,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands = trusted_operands(UINT8_C(0x9), UINT8_C(0x1));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, &operands,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands = trusted_operands(UINT8_C(0x1), UINT8_C(0xF));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, &operands,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void
test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x8), UINT8_C(0x1));
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x8), UINT8_C(0x1));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mvmm(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvmm(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvmm(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, NULL,
                                               &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvmm(&instruction, &operands,
                                               &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_move_forms();
    test_illegal_nibble_rejects();
    test_trusted_operand_rejects();
    test_null_rejects();
    return 0;
}
