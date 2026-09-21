/* Module contract
 *
 * Owns: RED proofs for the ST parallel-ADD/SUB dual leaves,
 * including two-row 0xC000/0xC400 decode fan-out, checked/trusted
 * validation, dual write-plus-read staging with the frozen STH
 * value and no T involvement, single-read completion through the
 * mirrored dual-arithmetic core with the TI §ST||ADD and
 * §ST||SUB example vectors, S/D routing, carry/borrow-driven C,
 * OVM saturation, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, memory
 * addressing or buses, accumulator/status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/arithmetic/op_st_parallel.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI SPRU172C §ST||ADD Example (p. 4-177): ST A,*AR3 with A
 * 00 0000 1111-style value FF 8421 1000 and ASM 1 stores
 * 0x0842, while ADD *AR5+0%,B with Xmem 0x8001 accumulates
 * SXM-widened X into dst_ A to pin B at FF 0422 1000 (SXM 1,
 * OVM 0). §ST||SUB Example (p. 4-185): ST A,*AR3- with A
 * FF 8421 0000 stores 0x0842, while SUB *AR5+0%,B pins B at
 * FF FBE0 0000. A stays untouched in the SUB example (and in
 * every ST||MAC/MAS/MPY example); the ADD example's changed
 * A is recorded as a one-nibble TI typo with src preserved. */
#define TI_X_WORD UINT16_C(0x8001)
#define TI_A_BEFORE_ADD UINT64_C(0xFF84211000)
#define TI_A_BEFORE_SUB UINT64_C(0xFF84210000)
#define TI_B_BEFORE UINT64_C(0x1111)
#define TI_B_AFTER_ADD UINT64_C(0xFF04221000)
#define TI_B_AFTER_SUB UINT64_C(0xFFFBE00000)
#define TI_ST_VALUE UINT16_C(0x0842)

static uint16_t
encode_word(uint16_t base, uint8_t src, uint8_t destination,
            uint8_t x_mode, uint8_t x_ar, uint8_t y_mode, uint8_t y_ar)
{
    return (uint16_t)(base | (uint16_t)((uint16_t)src << 9U) |
                      (uint16_t)((uint16_t)destination << 8U) |
                      (uint16_t)((uint16_t)x_mode << 6U) |
                      (uint16_t)((uint16_t)x_ar << 4U) |
                      (uint16_t)((uint16_t)y_mode << 2U) | (uint16_t)y_ar);
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
                       TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_STATUS_SXM_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = TI_A_BEFORE_ADD;
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = TI_B_BEFORE;
    input.accumulator_b_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    input.status_asm = UINT8_C(1);
    input.status_asm_known = true;
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_trusted_operand_values
trusted_for(uint8_t src, uint8_t destination, uint8_t dualxy)
{
    ti_c54x_trusted_operand_values output = {0};

    output.values[0] =
        (ti_c54x_operand_field_value){(uint64_t)src, UINT8_C(1)};
    output.values[1] =
        (ti_c54x_operand_field_value){(uint64_t)destination, UINT8_C(1)};
    output.values[2] =
        (ti_c54x_operand_field_value){(uint64_t)dualxy, UINT8_C(8)};
    output.count = UINT8_C(3);
    return output;
}

static void
test_decode_fanout(void)
{
    static const uint16_t bases[2] = {UINT16_C(0xC000),
                                      UINT16_C(0xC400)};
    static const ti_c54x_opcode_id ids[2] = {
        TI_C54X_OPCODE_ID_ST_PARALLEL_ADD,
        TI_C54X_OPCODE_ID_ST_PARALLEL_SUB};
    unsigned row;
    uint8_t src;
    uint8_t destination;
    uint8_t x_mode;
    uint8_t y_mode;
    uint8_t xar;
    uint8_t yar;

    for (row = 0U; row < 2U; ++row) {
        for (src = UINT8_C(0); src < UINT8_C(2); ++src) {
            for (destination = UINT8_C(0); destination < UINT8_C(2);
                 ++destination) {
                for (x_mode = UINT8_C(0); x_mode < UINT8_C(4); ++x_mode) {
                    for (y_mode = UINT8_C(0); y_mode < UINT8_C(4);
                         ++y_mode) {
                        for (xar = UINT8_C(0); xar < UINT8_C(4); ++xar) {
                            for (yar = UINT8_C(0); yar < UINT8_C(4);
                                 ++yar) {
                                const uint16_t word = encode_word(
                                    bases[row], src, destination,
                                    x_mode, xar, y_mode, yar);
                                const ti_c54x_decoded_instruction
                                    instruction =
                                        decode_expect(word, ids[row]);
                                (void)instruction;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
test_checked_stages_write_read(void)
{
    /* TI §ST||ADD example word 0xC1F1: S=0 (src A), D=1
     * (dst B), XMOD3 XAR=3 (*AR5+0%), YMOD0 YAR=1 (*AR3). */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xC1F1), TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(effect.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    /* The ST value freezes at stage from the input snapshot:
     * STH-by-ASM of A FF 8421 1000 with ASM 1 is 0x0842. */
    TI_C54X_TEST_CHECK(effect.payload.memory.value_word == TI_ST_VALUE);
    TI_C54X_TEST_CHECK(!effect.payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&effect.payload.memory));
}

static void
check_single(ti_c54x_op_status (*leaf)(
                 const ti_c54x_decoded_instruction *,
                 const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *),
             uint16_t word, ti_c54x_opcode_id expected_id,
             ti_c54x_op_input_snapshot input, uint16_t x_word,
             uint8_t destination, uint64_t value,
             ti_c54x_op_status_bit_action c_action,
             ti_c54x_op_destination_overflow_action overflow_action)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(word, expected_id);
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(leaf(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, x_word,
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       value);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta
                           .c_action == c_action);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta
                           .tc_action == TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta
                           .destination_overflow_action == overflow_action);
}

static void
test_completion_add_example(void)
{
    /* §ST||ADD example: S=0, D=1 routes the ADD half onto B
     * with dst_ A; B pins FF 0422 1000. TI states the row
     * "Affects C and OVdst": 0x84211000 + 0x80010000 carries
     * out of bit 31 (C writes 1) and the signed-40 sum
     * -4223328256 is below INT32_MIN (OV writes 1). */
    check_single(ti_c54x_op_st_parallel_add, UINT16_C(0xC1F1),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_ADD, known_input(),
                 TI_X_WORD, UINT8_C(1), TI_B_AFTER_ADD,
                 TI_C54X_OP_STATUS_BIT_WRITE_1,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void
test_completion_sub_example(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* §ST||SUB example: same routing; B pins FF FBE0 0000.
     * TI states the row "Affects C and OVdst": 0x80010000 <
     * 0x84210000 borrows (C writes 0) while the signed-40
     * difference -71507968 fits INT32 (OV preserves). The TI
     * B-before word 00 1000 0001 is overwritten either way. */
    input.accumulator_a_raw40 = TI_A_BEFORE_SUB;
    input.accumulator_b_raw40 = UINT64_C(0x00100001);
    check_single(ti_c54x_op_st_parallel_sub, UINT16_C(0xC5F5),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_SUB, input, TI_X_WORD,
                 UINT8_C(1), TI_B_AFTER_SUB,
                 TI_C54X_OP_STATUS_BIT_WRITE_0,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_destination_mirror(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* 0xC0B1 is S=0,D=0: the ADD half lands on A with dst_ B,
     * pinning A to B-old + SXM(X)<<16 = 0xFF80011111. */
    input.accumulator_b_raw40 = TI_B_BEFORE;
    check_single(ti_c54x_op_st_parallel_add, UINT16_C(0xC0B1),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_ADD, input, TI_X_WORD,
                 UINT8_C(0), UINT64_C(0xFF80011111),
                 TI_C54X_OP_STATUS_BIT_PRESERVE,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_carry_and_borrow(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* dst_ 0x00FFFFFF00 plus X 0x0001<<16 carries out of bit
     * 31 and the signed-40 sum 4295032576 exceeds INT32_MAX:
     * C writes 1, OV writes 1. */
    input.status_sxm = UINT8_C(0);
    input.accumulator_a_raw40 = UINT64_C(0x00FFFFFF00);
    check_single(ti_c54x_op_st_parallel_add, UINT16_C(0xC1B1),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_ADD, input,
                 UINT16_C(0x0001), UINT8_C(1), UINT64_C(0x010000FF00),
                 TI_C54X_OP_STATUS_BIT_WRITE_1,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    /* X 0x0000<<16 minus dst_ 1 borrows: C writes 0. */
    input.accumulator_a_raw40 = UINT64_C(0x00000001);
    check_single(ti_c54x_op_st_parallel_sub, UINT16_C(0xC5B1),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_SUB, input,
                 UINT16_C(0x0000), UINT8_C(1), UINT64_C(0xFFFFFFFFFF),
                 TI_C54X_OP_STATUS_BIT_WRITE_0,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_ovm_saturation(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* dst_ 0x007FFFFFFF plus 0x7FFF0000 overflows positive:
     * OVM saturates the dst channel and writes OV. */
    input.status_sxm = UINT8_C(0);
    input.status_ovm = UINT8_C(1);
    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    check_single(ti_c54x_op_st_parallel_add, UINT16_C(0xC1B1),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_ADD, input,
                 UINT16_C(0x7FFF), UINT8_C(1), UINT64_C(0x007FFFFFFF),
                 TI_C54X_OP_STATUS_BIT_PRESERVE,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void
test_src_equals_dst(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* S=1,D=1 (0xC3B1): the store uses B-before while the ADD
     * half overwrites B from the same pre-state. */
    input.accumulator_b_raw40 = TI_A_BEFORE_ADD;
    {
        ti_c54x_decoded_instruction staged_instruction =
            decode_expect(UINT16_C(0xC3B1),
                          TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
        ti_c54x_op_effect staged_effect = {0};

        TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(
                               &staged_instruction, &input,
                               &staged_effect) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(staged_effect.payload.memory.value_word ==
                           TI_ST_VALUE);
    }
    /* Same ADD arithmetic as the TI example, so the same
     * carry and signed-40 overflow drive C and OV. */
    check_single(ti_c54x_op_st_parallel_add, UINT16_C(0xC3B1),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_ADD, input, TI_X_WORD,
                 UINT8_C(1), TI_B_AFTER_ADD,
                 TI_C54X_OP_STATUS_BIT_WRITE_1,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void
test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(UINT16_C(0xC1B1),
                      TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(0), UINT8_C(1), UINT8_C(0xB1));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_parallel_add(
                           &instruction, &trusted, &input,
                           &effect) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.value_word == TI_ST_VALUE);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(3));
}

static void
test_unknown_inputs_reject(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(UINT16_C(0xC1B1),
                      TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect before = {0};

    input.status_asm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);

    input = known_input();
    input.auxiliary_registers_known_mask = UINT8_C(0xDF);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);

    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
}

static void
test_null_rejection(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(UINT16_C(0xC1B1),
                      TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(&instruction, NULL,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_add(&instruction, &input,
                                                 NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_decode_fanout();
    test_checked_stages_write_read();
    test_completion_add_example();
    test_completion_sub_example();
    test_destination_mirror();
    test_carry_and_borrow();
    test_ovm_saturation();
    test_src_equals_dst();
    test_trusted_parity();
    test_unknown_inputs_reject();
    test_null_rejection();
    puts("st_parallel: ok");
    return 0;
}
