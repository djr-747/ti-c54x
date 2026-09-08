/* Module contract
 * Owns: focused checked/trusted syntax-6 dual-memory ADD/SUB MOD0 semantic
 * proofs, including selector fan-out, pair completion, fixed-shift carry
 * behavior, SXM extension, modulo-40 arithmetic, OVM saturation, sticky OV,
 * and transactional malformed-input rejection.
 * Does not own: catalog generation, address resolution, bus execution, CPU
 * mutation, sequencing, timing, or repeat eligibility.
 */

#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_dual_arithmetic.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static ti_c54x_decoded_instruction decode(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, 0U}, .program_word_count = 1U};
    ti_c54x_decoded_instruction output;

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return output;
}

static ti_c54x_op_input_snapshot input_for(uint8_t sxm, uint8_t ovm)
{
    ti_c54x_op_input_snapshot input = {0};

    input.auxiliary_registers_known_mask = UINT8_C(0xff);
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_sxm = sxm;
    input.status_sxm_known = true;
    input.status_ovm = ovm;
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_trusted_operand_values operands_for(uint8_t destination,
                                                   uint8_t xar, uint8_t yar)
{
    ti_c54x_trusted_operand_values output = {0};

    output.values[0] = (ti_c54x_operand_field_value){destination, 1U};
    output.values[1] = (ti_c54x_operand_field_value){xar, 2U};
    output.values[2] = (ti_c54x_operand_field_value){yar, 2U};
    output.count = 3U;
    return output;
}

static ti_c54x_op_effect complete(const ti_c54x_op_effect *memory,
                                  const ti_c54x_decoded_instruction *instruction,
                                  const ti_c54x_op_input_snapshot *input,
                                  uint16_t x_word, uint16_t y_word)
{
    ti_c54x_op_effect output = {0};

    TI_C54X_TEST_CHECK(memory->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(memory->payload.memory.complete_dual != NULL);
    TI_C54X_TEST_CHECK(memory->payload.memory.complete_dual(
                           instruction, input, x_word, y_word, &output) ==
                       TI_C54X_OP_STATUS_OK);
    return output;
}

static void check_selector_fanout(void)
{
    uint8_t destination;
    uint8_t xar;
    uint8_t yar;
    const ti_c54x_op_input_snapshot input = input_for(0U, 0U);

    for (destination = 0U; destination < 2U; ++destination) {
        for (xar = 0U; xar < 4U; ++xar) {
            for (yar = 0U; yar < 4U; ++yar) {
                const uint16_t word = (uint16_t)(UINT16_C(0xa000) |
                                                  (uint16_t)(destination << 8U) |
                                                  (uint16_t)(xar << 4U) | yar);
                const ti_c54x_decoded_instruction instruction = decode(word);
                const ti_c54x_trusted_operand_values trusted =
                    operands_for(destination, xar, yar);
                ti_c54x_op_effect checked = {0};
                ti_c54x_op_effect trusted_effect = {0};

                TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(
                                       &instruction, &input, &checked) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_dual_mod0(
                                       &instruction, &trusted, &input,
                                       &trusted_effect) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(checked.kind == trusted_effect.kind);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.address_spec.kind ==
                    trusted_effect.payload.memory.address_spec.kind);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.address_spec.dual.x_mode ==
                    trusted_effect.payload.memory.address_spec.dual.x_mode);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.address_spec.dual.x_ar ==
                    trusted_effect.payload.memory.address_spec.dual.x_ar);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.address_spec.dual.y_mode ==
                    trusted_effect.payload.memory.address_spec.dual.y_mode);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.address_spec.dual.y_ar ==
                    trusted_effect.payload.memory.address_spec.dual.y_ar);
                TI_C54X_TEST_CHECK(checked.payload.memory.access ==
                                   trusted_effect.payload.memory.access);
                TI_C54X_TEST_CHECK(checked.payload.memory.value_word ==
                                   trusted_effect.payload.memory.value_word);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.rmw_operation ==
                    trusted_effect.payload.memory.rmw_operation);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.transfer_reserved ==
                    trusted_effect.payload.memory.transfer_reserved);
                TI_C54X_TEST_CHECK(
                    checked.payload.memory.complete_dual ==
                    trusted_effect.payload.memory.complete_dual);
                TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                                       &checked.payload.memory));
                TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.x_ar ==
                                   (uint8_t)(xar + 2U));
                TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.y_ar ==
                                   (uint8_t)(yar + 2U));
                TI_C54X_TEST_CHECK(complete(&checked, &instruction, &input,
                                            UINT16_C(1), UINT16_C(2))
                                       .payload.accumulator_write.destination ==
                                   destination);
            }
        }
    }
}

static void check_arithmetic_and_status(void)
{
    const ti_c54x_decoded_instruction add_instruction = decode(UINT16_C(0xa000));
    const ti_c54x_decoded_instruction sub_instruction = decode(UINT16_C(0xa200));
    ti_c54x_op_input_snapshot input = input_for(0U, 0U);
    ti_c54x_op_effect memory = {0};
    ti_c54x_op_effect result;

    TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(
                           &add_instruction, &input, &memory) ==
                       TI_C54X_OP_STATUS_OK);
    result = complete(&memory, &add_instruction, &input, UINT16_C(1), UINT16_C(2));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00030000));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);

    result = complete(&memory, &add_instruction, &input, UINT16_C(0xffff),
                      UINT16_C(1));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x100000000));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);

    input.status_sxm = UINT8_C(1);
    result = complete(&memory, &add_instruction, &input, UINT16_C(0xffff),
                      UINT16_C(1));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 == 0U);
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_1);

    input = input_for(0U, 0U);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_dual_mod0(
                           &sub_instruction, &input, &memory) ==
                       TI_C54X_OP_STATUS_OK);
    result = complete(&memory, &sub_instruction, &input, UINT16_C(1), UINT16_C(2));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xffffff0000));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_WRITE_0);
    result = complete(&memory, &sub_instruction, &input, UINT16_C(2), UINT16_C(1));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00010000));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);

    input = input_for(1U, 0U);
    result = complete(&memory, &sub_instruction, &input, UINT16_C(0x8000),
                      UINT16_C(1));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xff7fff0000));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta
                           .destination_overflow_action ==
                       TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);

    input = input_for(0U, 1U);
    TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(
                           &add_instruction, &input, &memory) ==
                       TI_C54X_OP_STATUS_OK);
    result = complete(&memory, &add_instruction, &input, UINT16_C(0x7fff),
                      UINT16_C(0x7fff));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007fffffff));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta
                           .destination_overflow_action ==
                       TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);

    input = input_for(1U, 1U);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_dual_mod0(
                           &sub_instruction, &input, &memory) ==
                       TI_C54X_OP_STATUS_OK);
    result = complete(&memory, &sub_instruction, &input, UINT16_C(0x8000),
                      UINT16_C(1));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xff80000000));
    TI_C54X_TEST_CHECK(result.payload.accumulator_write.status_delta
                           .destination_overflow_action ==
                       TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void check_transactional_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT16_C(0xa000));
    ti_c54x_op_input_snapshot input = input_for(0U, 0U);
    ti_c54x_op_effect effect;
    ti_c54x_op_effect before;
    ti_c54x_trusted_operand_values trusted = operands_for(0U, 0U, 0U);

    input.status_sxm_known = false;
    effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_NONE,
        .payload.repeat_start = {.operand = UINT16_C(0x5a5a)}};
    before = effect;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);

    input = input_for(0U, 0U);
    input.auxiliary_registers_known_mask = UINT8_C(0);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);

    trusted.values[1].width_bits = UINT8_C(3);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_dual_mod0(
                           &instruction, &trusted, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);

    trusted = operands_for(0U, 0U, 0U);
    trusted.count = UINT8_C(4);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_dual_mod0(
                           &instruction, &trusted, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);

    trusted = operands_for(0U, 0U, 0U);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_dual_mod0(
                           NULL, &trusted, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);

    input = input_for(0U, 0U);
    input.status_c_known = false;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);

    input = input_for(0U, 0U);
    input.status_ovm = UINT8_C(2);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_dual_mod0(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand ==
                       before.payload.repeat_start.operand);
}

int main(void)
{
    check_selector_fanout();
    check_arithmetic_and_status();
    check_transactional_rejection();
    return 0;
}
