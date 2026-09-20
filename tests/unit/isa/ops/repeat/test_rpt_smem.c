/*
 * Module contract
 *
 * Owns: RED proofs for the one-word `RPT Smem` repeat-start
 * effect, including direct/indirect cookie extraction, the
 * staged DATA read address, read-value RC completion,
 * exotic-mode deferral, checked/trusted/dispatch parity, and
 * transactional rejection. Does not own: catalog rows,
 * operand-layout descriptors, RC state, buses, CPU or
 * architectural state, sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
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
#include "../../../../../src/isa/ops/repeat/op_rpt_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RPT_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RPT_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    return instruction;
}

static ti_c54x_op_input_snapshot ignored_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_a_known = true;
    input.accumulator_b_known = false;
    return input;
}

static void check_direct_case(uint8_t dma7, uint16_t read_value)
{
    /* TI SPRU172C section RPT syntax 1: 0100 0111 I AAAAAAA
     * with I=0 selects direct page-0 word dma7. */
    const uint16_t first_word =
        (uint16_t)(UINT16_C(0x4700) | dma7);
    const ti_c54x_decoded_instruction instruction = decode(first_word);
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    ti_c54x_op_effect next = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_RPT_SMEM);

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = dma7;
    operands.values[1].width_bits = UINT8_C(7);

    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_smem(&instruction, &input,
                                          &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(checked.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.direct_dma7 ==
                       dma7);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete(
                           &instruction, &input, read_value, &next) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(next.payload.repeat_start.operand == read_value);

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rpt_smem(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_rpt_smem);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_is_canonical(
                           binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) == 0);
}

static void check_indirect_case(uint8_t arf, uint16_t read_value)
{
    /* I=1 with MOD0 picks AR(arf) with no displacement:
     * cookie I AAAAAAA over mode 0. */
    const uint16_t first_word =
        (uint16_t)(UINT16_C(0x4780) | arf);
    const ti_c54x_decoded_instruction instruction = decode(first_word);
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_smem(&instruction, &input,
                                          &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete(
                           &instruction, &input, read_value, &next) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(next.payload.repeat_start.operand == read_value);
}

static void test_cases(void)
{
    /* Direct corners plus the worked DAT127 shape, and two
     * indirect registers with distinct counts. */
    check_direct_case(UINT8_C(0x00), UINT16_C(0x0000));
    check_direct_case(UINT8_C(0x7F), UINT16_C(0x000C));
    check_direct_case(UINT8_C(0x2A), UINT16_C(0xFFFF));
    check_indirect_case(UINT8_C(1), UINT16_C(0x0002));
    check_indirect_case(UINT8_C(5), UINT16_C(0x1111));
}

static void test_exotic_mode_defers(void)
{
    /* mode4 above MOD3 (addr7 0x20) defers as INVALID_OPERAND. */
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0x47A0));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_smem(&instruction, &input,
                                          &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0x4700));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rpt_smem(&instruction, NULL,
                                                  &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rpt_smem(&instruction, &operands,
                                                  &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_cases();
    test_exotic_mode_defers();
    test_null_rejects();
    return 0;
}
