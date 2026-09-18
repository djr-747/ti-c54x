/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the one-word `FRAME K`
 * stack-pointer-offset effect, including K extraction and sign
 * extension, SP+K mod-65536 carriage (including wrap both ways),
 * checked/trusted/dispatch parity, unknown-SP transactional
 * rejection, and hostile-input independence.
 * Does not own: catalog rows, operand-layout descriptors, SP
 * state, buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/stack/op_frame_k.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
frame_k_word(uint8_t suffix)
{
    return (uint16_t)(UINT16_C(0xEE00) | suffix);
}

static ti_c54x_decoded_instruction
decode(uint8_t suffix)
{
    const uint16_t first_word = frame_k_word(suffix);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_FRAME);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_FRAME);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_FRAME_K8);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       first_word);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint16_t sp, bool sp_known, uint8_t seed)
{
    /* Unrelated accumulator, AR, and status state rides hostile
     * seeds; only SP is consumed. */
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    input.auxiliary_registers_raw16[3] = UINT16_C(0xAAAA);
    input.auxiliary_registers_known_mask = seed;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_sxm = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_sxm_known = (seed & UINT8_C(8)) != 0U;
    input.stack_pointer_raw16 = sp;
    input.stack_pointer_known = sp_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t suffix)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = suffix;
    operands.values[0].width_bits = UINT8_C(8);
    return operands;
}

static void
assert_sp_write(const ti_c54x_op_effect *effect, uint16_t value_after)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_SP_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.sp_write.value_after ==
                       value_after);
}

static void
check_case(uint16_t sp, uint8_t suffix, uint16_t expected_after)
{
    const ti_c54x_decoded_instruction instruction = decode(suffix);
    const ti_c54x_op_input_snapshot clean =
        input_for(sp, true, UINT8_C(0));
    const ti_c54x_op_input_snapshot hostile =
        input_for(sp, true, UINT8_C(0x0F));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(suffix);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_FRAME);
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect checked_hostile = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect via_dispatch = {0};
    ti_c54x_op_effect via_trusted = {0};

    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle,
                           &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(&instruction, &clean,
                                          &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_sp_write(&checked, expected_after);
    TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(&instruction, &hostile,
                                          &checked_hostile) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                              sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_frame_k(
                           &instruction, &operands, &clean,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                              sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_frame_k);
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

static void
test_cases(void)
{
    /* TI FRAME 10h example: SP 0x1000 -> 0x1010. */
    check_case(UINT16_C(0x1000), UINT8_C(0x10), UINT16_C(0x1010));
    check_case(UINT16_C(0x1000), UINT8_C(0x00), UINT16_C(0x1000));
    check_case(UINT16_C(0x1000), UINT8_C(0x7F), UINT16_C(0x107F));
    check_case(UINT16_C(0x1000), UINT8_C(0x80), UINT16_C(0x0F80));
    check_case(UINT16_C(0x1000), UINT8_C(0xFF), UINT16_C(0x0FFF));
    /* Wrap both ways across the 16-bit boundary. */
    check_case(UINT16_C(0x0000), UINT8_C(0xFF), UINT16_C(0xFFFF));
    check_case(UINT16_C(0xFFFF), UINT8_C(0x01), UINT16_C(0x0000));
    check_case(UINT16_C(0x8000), UINT8_C(0x80), UINT16_C(0x7F80));
}

static void
test_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x10));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT16_C(0x1000), true, UINT8_C(0xA5));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x10));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(&instruction, NULL,
                                          &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(&instruction, &valid,
                                          NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_frame_k(&instruction, NULL,
                                                 &valid,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_frame_k(&instruction,
                                                 &operands, NULL,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_frame_k(&instruction,
                                                 &operands, &valid,
                                                 NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    /* Unknown SP rejects without an effect (CALA precedent). */
    {
        ti_c54x_op_input_snapshot unknown_sp = valid;

        unknown_sp.stack_pointer_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(&instruction,
                                             &unknown_sp,
                                             &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                  sizeof(effect)) == 0);
        effect = before;
    }

    /* Forged identity rejects without an effect. */
    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_frame_k(&forged, &valid,
                                             &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                  sizeof(effect)) == 0);
        effect = before;
    }

    /* Malformed trusted operands reject. */
    {
        ti_c54x_trusted_operand_values empty = {0};
        ti_c54x_trusted_operand_values narrow = operands;

        narrow.values[0].width_bits = UINT8_C(7);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_frame_k(
                               &instruction, &empty, &valid,
                               &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_frame_k(
                               &instruction, &narrow, &valid,
                               &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
}

int
main(void)
{
    test_cases();
    test_rejection();
    return 0;
}
