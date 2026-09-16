/* Module contract
 *
 * Owns: RED proofs for the MPY Xmem,Ymem dual leaves, including
 * nine-row MOD0/MOD1/MOD2 decode fan-out, checked/trusted
 * decode validation, dual-indirect read staging with read-phase
 * T load of the X-side word, pair completion through the
 * multiply core with the TI Example 2 STORE vector, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, T/accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/multiply/op_mpy_dual.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI SPRU172C MPY Example 2: MPY *AR2-, *AR4+0%, B with B before
 * FF FFFF FFE0, FRCT 0, mem[01FF]=0010, mem[0300]=0002 yields B
 * 00 0000 0020, AR2 01FE, AR4 0301. XMOD1/MOD2 prove the AR
 * updates; T takes the X-side word per the Execution section. */
#define TI_X_WORD UINT16_C(0x0010)
#define TI_Y_WORD UINT16_C(0x0002)
#define TI_B_AFTER_EXAMPLE2 UINT64_C(0x0020)

static const ti_c54x_opcode_id expected_ids[3][3] = {
    {TI_C54X_OPCODE_ID_MPY_DUAL_M0_M0, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M1,
     TI_C54X_OPCODE_ID_MPY_DUAL_M0_M2},
    {TI_C54X_OPCODE_ID_MPY_DUAL_M1_M0, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M1,
     TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2},
    {TI_C54X_OPCODE_ID_MPY_DUAL_M2_M0, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M1,
     TI_C54X_OPCODE_ID_MPY_DUAL_M2_M2}};

static const uint16_t row_values[3][3] = {
    {UINT16_C(0xA400), UINT16_C(0xA404), UINT16_C(0xA408)},
    {UINT16_C(0xA440), UINT16_C(0xA444), UINT16_C(0xA448)},
    {UINT16_C(0xA480), UINT16_C(0xA484), UINT16_C(0xA488)}};

static uint16_t
encode_word(uint8_t x_mode, uint8_t y_mode, uint8_t destination,
            uint8_t xar, uint8_t yar)
{
    return (uint16_t)(row_values[x_mode][y_mode] |
                      (uint16_t)((uint16_t)destination << 8U) |
                      (uint16_t)((uint16_t)xar << 4U) | (uint16_t)yar);
}

static ti_c54x_decoded_instruction
decode_expect(uint16_t word, ti_c54x_opcode_id expected_id)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == expected_id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_FRCT_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0xFFFFFFE0);
    input.accumulator_b_known = true;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_trusted_operand_values
trusted_for(uint8_t destination, uint8_t xar, uint8_t yar)
{
    ti_c54x_trusted_operand_values output = {0};

    output.values[0] =
        (ti_c54x_operand_field_value){(uint64_t)destination, UINT8_C(1)};
    output.values[1] =
        (ti_c54x_operand_field_value){(uint64_t)xar, UINT8_C(2)};
    output.values[2] =
        (ti_c54x_operand_field_value){(uint64_t)yar, UINT8_C(2)};
    output.count = UINT8_C(3);
    return output;
}

static void
test_decode_fanout(void)
{
    uint8_t x_mode;
    uint8_t y_mode;
    uint8_t destination;
    uint8_t xar;
    uint8_t yar;

    for (x_mode = UINT8_C(0); x_mode < UINT8_C(3); ++x_mode) {
        for (y_mode = UINT8_C(0); y_mode < UINT8_C(3); ++y_mode) {
            for (destination = UINT8_C(0); destination < UINT8_C(2);
                 ++destination) {
                for (xar = UINT8_C(0); xar < UINT8_C(4); ++xar) {
                    for (yar = UINT8_C(0); yar < UINT8_C(4); ++yar) {
                        const uint16_t word = encode_word(
                            x_mode, y_mode, destination, xar, yar);
                        const ti_c54x_decoded_instruction instruction =
                            decode_expect(word,
                                          expected_ids[x_mode][y_mode]);
                        (void)instruction;
                    }
                }
            }
        }
    }
}

static void
test_mod3_stays_unknown(void)
{
    /* YMOD=3 (bits 3:2 set) matches no row: MOD3 stays undecoded. */
    const uint16_t word =
        (uint16_t)(encode_word(UINT8_C(0), UINT8_C(0), UINT8_C(1),
                               UINT8_C(0), UINT8_C(2)) |
                   UINT16_C(0x000C));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_UNKNOWN_WORD ||
                       first.id == TI_C54X_OPCODE_ID_UNKNOWN);
}

static void
test_checked_stages_dual_t_load(void)
{
    /* 0xA54A: D=1 (dst B), XMOD1 XAR=0 (AR2), YMOD2 YAR=2 (AR4). */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA54A), TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m1_m2(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(effect.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete_dual != NULL);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&effect.payload.memory));
}

static void
check_completion(uint16_t word, ti_c54x_opcode_id expected_id,
                 uint64_t expected_value, uint8_t expected_destination)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(word, expected_id);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};
    ti_c54x_op_status (*leaf)(
        const ti_c54x_decoded_instruction *,
        const ti_c54x_op_input_snapshot *,
        ti_c54x_op_effect *) = NULL;

    if (expected_id == TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2) {
        leaf = ti_c54x_op_mpy_dual_m1_m2;
    } else {
        leaf = ti_c54x_op_mpy_dual_m0_m0;
    }
    TI_C54X_TEST_CHECK(leaf(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, TI_X_WORD, TI_Y_WORD,
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       expected_destination);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       expected_value);
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta.destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_example2(void)
{
    /* 0x0010 x 0x0002 = 0x20 stored into B, overwriting -32. */
    check_completion(UINT16_C(0xA54A),
                     TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2,
                     TI_B_AFTER_EXAMPLE2, UINT8_C(1));
}

static void
test_completion_dst_a(void)
{
    /* 0xA402: D=0 (dst A), XMOD0 XAR=0, YMOD0 YAR=2:
     * 0x0010 x 0x0002 = 0x20 stored into A. */
    check_completion(UINT16_C(0xA402),
                     TI_C54X_OPCODE_ID_MPY_DUAL_M0_M0,
                     TI_B_AFTER_EXAMPLE2, UINT8_C(0));
}

static void
test_completion_frct(void)
{
    /* FRCT=1 doubles the product independent of the destination:
     * 0x0010 x 0x0002 x 2 = 0x40 into A. */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA402), TI_C54X_OPCODE_ID_MPY_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.status_frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m0_m0(&instruction, &input,
                                                &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, TI_X_WORD, TI_Y_WORD,
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x40));
}

static void
test_trusted_equivalence(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA54A), TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(1), UINT8_C(0), UINT8_C(2));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted_effect = {0};
    ti_c54x_op_effect checked_next = {0};
    ti_c54x_op_effect trusted_next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m1_m2(&instruction, &input,
                                                &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_dual_m1_m2(
                           &instruction, &trusted, &input,
                           &trusted_effect) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked.kind == trusted_effect.kind);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.x_mode ==
                       trusted_effect.payload.memory.address_spec.dual
                           .x_mode);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.x_ar ==
                       trusted_effect.payload.memory.address_spec.dual
                           .x_ar);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.y_mode ==
                       trusted_effect.payload.memory.address_spec.dual
                           .y_mode);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.y_ar ==
                       trusted_effect.payload.memory.address_spec.dual
                           .y_ar);
    TI_C54X_TEST_CHECK(checked.payload.memory.load_t_with_read ==
                       trusted_effect.payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete_dual ==
                       trusted_effect.payload.memory.complete_dual);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete_dual(
                           &instruction, &input, TI_X_WORD, TI_Y_WORD,
                           &checked_next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted_effect.payload.memory.complete_dual(
                           &instruction, &input, TI_X_WORD, TI_Y_WORD,
                           &trusted_next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked_next.payload.accumulator_write.value_raw40 ==
                       trusted_next.payload.accumulator_write.value_raw40);
    TI_C54X_TEST_CHECK(checked_next.payload.accumulator_write.value_raw40 ==
                       TI_B_AFTER_EXAMPLE2);
}

static void
test_unknown_ar_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA54A), TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* Clear the AR2 (bit 2) known bit: X side unknown. */
    input.auxiliary_registers_known_mask = UINT8_C(0xFB);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m1_m2(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA54A), TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m1_m2(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m1_m2(&instruction, NULL,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_dual_m1_m2(&instruction, &input,
                                                NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_decode_fanout();
    test_mod3_stays_unknown();
    test_checked_stages_dual_t_load();
    test_completion_example2();
    test_completion_dst_a();
    test_completion_frct();
    test_trusted_equivalence();
    test_unknown_ar_rejects();
    test_null_rejects();
    return 0;
}
