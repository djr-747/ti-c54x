/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for one-word LD Xmem,SHFT,dst
 * effects, including D/Xmod/Xar/SHFT selection over all 512
 * encodings, exhaustive read coverage, SXM-controlled widening
 * with OVM saturation and sticky destination overflow, C/TC
 * preservation with no OVM input, unknown-SXM/OVM rejection,
 * checked/trusted/dispatch parity, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ld_xmem_shift.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
ld_word(uint8_t destination, uint8_t xmod, uint8_t xar_field,
        uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x9400) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(shift & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t destination, uint8_t xmod, uint8_t xar_field,
       uint8_t shift)
{
    const uint16_t word = ld_word(destination, xmod, xar_field, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t sxm, bool sxm_known, uint8_t ovm, bool ovm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    input.status_ovm = ovm;
    input.status_ovm_known = ovm_known;
    return input;
}

/* Independent oracle: SXM-widen, shift left, guard overflow, saturate. */
static uint64_t
expected_shifted(uint16_t read_value, uint8_t sxm, uint8_t shift,
                 uint8_t ovm, bool *overflow)
{
    uint64_t widened = (uint64_t)read_value;
    uint64_t value;
    uint64_t guard;
    uint8_t bit31;
    bool negative;

    if (sxm != UINT8_C(0) &&
        (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= UINT64_C(0xFFFFFFFF0000);
    }
    negative = (widened & UINT64_C(0x8000000000)) != UINT64_C(0);
    value = (widened << shift) & TI_C54X_U40_MASK;
    guard = (value >> 32U) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value >> 31U) & UINT64_C(1));
    *overflow =
        guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF) : UINT64_C(0));
    if (*overflow && ovm != UINT8_C(0)) {
        value = negative ? UINT64_C(0xFF80000000)
                         : UINT64_C(0x007FFFFFFF);
    }
    return value;
}

static void
assert_memory_read(const ti_c54x_op_effect *effect, uint8_t xmod,
                   uint8_t xar)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.x_mode ==
                       xmod);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.x_ar ==
                       xar);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.y_mode ==
                       xmod);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.y_ar ==
                       xar);
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word0 == UINT32_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word1 == UINT32_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word2 == UINT32_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word3 == UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void
assert_accumulator_write(const ti_c54x_op_effect *effect,
                         uint8_t destination, uint64_t value, bool overflow)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect->payload.accumulator_write.status_delta
            .destination_overflow_action ==
        (overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                  : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t destination, uint8_t xmod, uint8_t xar_field,
                 uint8_t shift)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = xmod;
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].raw_value = xar_field;
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[3].raw_value = shift;
    operands.values[3].width_bits = UINT8_C(4);
    return operands;
}

static void
check_valid_forms_and_completion(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x0001),
                                     UINT16_C(0x7FFF), UINT16_C(0x8000),
                                     UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t xmod = 0U; xmod <= UINT8_C(3); ++xmod) {
            for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3);
                 ++xar_field) {
                const uint8_t xar = (uint8_t)(xar_field + UINT8_C(2));
                for (uint16_t shift_raw = 0U; shift_raw <= UINT16_C(15);
                     ++shift_raw) {
                    const uint8_t shift = (uint8_t)shift_raw;
                    const ti_c54x_decoded_instruction instruction = decode(
                        destination, xmod, xar_field, shift);
                    const ti_c54x_trusted_operand_values operands =
                        trusted_operands(destination, xmod, xar_field,
                                         shift);

                    for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                        for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                            const ti_c54x_op_input_snapshot input =
                                input_for(sxm, true, ovm, true);
                            ti_c54x_op_effect direct = {0};
                            ti_c54x_op_effect trusted = {0};

                            TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(
                                                   &instruction, &input,
                                                   &direct) ==
                                               TI_C54X_OP_STATUS_OK);
                            assert_memory_read(&direct, xmod, xar);
                            TI_C54X_TEST_CHECK(
                                ti_c54x_trusted_op_ld_xmem_shift(
                                    &instruction, &operands, &input,
                                    &trusted) == TI_C54X_OP_STATUS_OK);
                            TI_C54X_TEST_CHECK(
                                memcmp(&direct, &trusted,
                                       sizeof(direct)) == 0);

                            for (size_t read_index = 0U;
                                 read_index <
                                 sizeof(reads) / sizeof(reads[0]);
                                 ++read_index) {
                                ti_c54x_op_effect completed = {
                                    .kind =
                                        TI_C54X_OP_EFFECT_REPEAT_START,
                                    .payload.repeat_start = {
                                        .operand = UINT16_C(0xA55A)}};
                                bool overflow = false;
                                const uint64_t expected = expected_shifted(
                                    reads[read_index], sxm, shift, ovm,
                                    &overflow);

                                TI_C54X_TEST_CHECK(
                                    direct.payload.memory.complete(
                                        &instruction, &input,
                                        reads[read_index],
                                        &completed) ==
                                    TI_C54X_OP_STATUS_OK);
                                assert_accumulator_write(
                                    &completed, destination, expected,
                                    overflow);
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0));
    const ti_c54x_op_input_snapshot unknown_sxm =
        input_for(UINT8_C(0), false, UINT8_C(0), true);
    const ti_c54x_op_input_snapshot unknown_ovm =
        input_for(UINT8_C(0), true, UINT8_C(0), false);
    const ti_c54x_op_input_snapshot malformed =
        input_for(UINT8_C(2), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;
    unsigned char before_bytes[sizeof(effect)];
    (void)memcpy(before_bytes, &before, sizeof(before_bytes));

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(NULL, &unknown_sxm,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&instruction, NULL,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&instruction, &unknown_sxm,
                                               NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, before_bytes, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&instruction, &unknown_sxm,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&instruction, &unknown_ovm,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&instruction, &malformed,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT8_C(0), true, UINT8_C(0), true);

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&forged, &valid,
                                                   &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&forged, &valid,
                                                   &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&forged, &valid,
                                                   &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        /* Bit 9 is fixed: flipping it leaves the row. Bit 6 would stay
         * inside the row (Xmod is variable). */
        forged.bundle.program_words[0] ^= UINT16_C(0x0200);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&forged, &valid,
                                                   &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
test_dispatch_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(2), UINT8_C(3), UINT8_C(15));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT8_C(1), true, UINT8_C(1), true);
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect via_dispatch = {0};
    ti_c54x_op_effect via_trusted = {0};

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_ld_xmem_shift);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_xmem_shift(&instruction, &input,
                                               &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &via_dispatch) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &via_dispatch, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input,
                                               &via_trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &via_trusted, sizeof(direct)) == 0);
}

int
main(void)
{
    check_valid_forms_and_completion();
    check_unknown_and_malformed_input_is_transactional();
    test_dispatch_parity();
    return 0;
}
