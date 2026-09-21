/* Module contract
 *
 * Owns: RED proofs for the LD parallel-MAC/MAS dual leaves,
 * including two-row 0xA800/0xAC00 decode fan-out, checked/trusted
 * validation, dual-indirect read staging with no T load, pair
 * completion through the multiply core with the TI §LD||MAC[R]
 * Example 1/2 and §LD||MAS[R] Example 1/2 vectors, D-mirrored
 * accumulator routing, SXM-gated LD-half widening, OVM
 * saturation, and transactional rejection.
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
#include "../../../../../src/isa/ops/multiply/op_ld_parallel.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI SPRU172C §LD||MAC[R] Example 1 (p. 4-74): LD *AR4+,A with
 * Xmem 0x1234, MAC *AR5+,B with Ymem 0x4321, A before
 * 00 0000 1000, B before 00 0000 1111, T 0x0400, FRCT 0,
 * SXM 0, OVM 0. A pins the full-40 LD-half zeroing load;
 * B pins (0x4321 * 0x0400) + 0x1111 = 0x010C9511. Example 2
 * (MACR) rounds B to 0x010D0000. §LD||MAS[R] Examples 1/2
 * (p. 4-76) pin B to 0xFFFEF38D11 and 0xFFFEF40000. */
#define TI_X_WORD UINT16_C(0x1234)
#define TI_Y_WORD UINT16_C(0x4321)
#define TI_A_BEFORE UINT64_C(0x1000)
#define TI_B_BEFORE UINT64_C(0x1111)
#define TI_T_BEFORE UINT16_C(0x0400)
#define TI_A_AFTER_LD UINT64_C(0x0012340000)
#define TI_B_AFTER_MAC UINT64_C(0x00010C9511)
#define TI_B_AFTER_MACR UINT64_C(0x00010D0000)
#define TI_B_AFTER_MAS UINT64_C(0xFFFEF38D11)
#define TI_B_AFTER_MASR UINT64_C(0xFFFEF40000)

static uint16_t
encode_word(uint16_t base, uint8_t round, uint8_t destination,
            uint8_t x_mode, uint8_t x_ar, uint8_t y_mode, uint8_t y_ar)
{
    return (uint16_t)(base | (uint16_t)((uint16_t)round << 9U) |
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
                       TI_C54X_OPERAND_LAYOUT_ROUND_DESTINATION_DUALXY);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER_STATUS_SXM_FRCT_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = TI_A_BEFORE;
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = TI_B_BEFORE;
    input.accumulator_b_known = true;
    input.t_register_raw16 = TI_T_BEFORE;
    input.t_register_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sxm_known = true;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_trusted_operand_values
trusted_for(uint8_t round, uint8_t destination, uint8_t dualxy)
{
    ti_c54x_trusted_operand_values output = {0};

    output.values[0] =
        (ti_c54x_operand_field_value){(uint64_t)round, UINT8_C(1)};
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
    static const uint16_t bases[2] = {UINT16_C(0xA800),
                                      UINT16_C(0xAC00)};
    static const ti_c54x_opcode_id ids[2] = {
        TI_C54X_OPCODE_ID_LD_PARALLEL_MAC,
        TI_C54X_OPCODE_ID_LD_PARALLEL_MAS};
    unsigned row;
    uint8_t round;
    uint8_t destination;
    uint8_t x_mode;
    uint8_t y_mode;
    uint8_t xar;
    uint8_t yar;

    for (row = 0U; row < 2U; ++row) {
        for (round = UINT8_C(0); round < UINT8_C(2); ++round) {
            for (destination = UINT8_C(0); destination < UINT8_C(2);
                 ++destination) {
                for (x_mode = UINT8_C(0); x_mode < UINT8_C(4); ++x_mode) {
                    for (y_mode = UINT8_C(0); y_mode < UINT8_C(4);
                         ++y_mode) {
                        for (xar = UINT8_C(0); xar < UINT8_C(4); ++xar) {
                            for (yar = UINT8_C(0); yar < UINT8_C(4);
                                 ++yar) {
                                const uint16_t word = encode_word(
                                    bases[row], round, destination,
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
test_checked_stages_dual_without_t_load(void)
{
    /* 0xA8AB: R=0, D=0, XMOD2 XAR=2 (AR4), YMOD2 YAR=3 (AR5). */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA8AB), TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    /* T is a live input here, never a read-phase load. */
    TI_C54X_TEST_CHECK(!effect.payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete_dual != NULL);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&effect.payload.memory));
}

static void
check_pair(ti_c54x_op_status (*leaf)(
               const ti_c54x_decoded_instruction *,
               const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *),
           uint16_t word, ti_c54x_opcode_id expected_id,
           ti_c54x_op_input_snapshot input, uint16_t x_word,
           uint16_t y_word, uint8_t first_destination,
           uint64_t first_value, uint8_t second_destination,
           uint64_t second_value,
           ti_c54x_op_destination_overflow_action second_overflow)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(word, expected_id);
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(leaf(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, x_word, y_word,
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind ==
                       TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first
                           .destination == first_destination);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first
                           .value_raw40 == first_value);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first
                           .status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first
                           .status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.first
                           .status_delta.destination_overflow_action ==
                       TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second
                           .destination == second_destination);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second
                           .value_raw40 == second_value);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second
                           .status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second
                           .status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write_pair.second
                           .status_delta.destination_overflow_action ==
                       second_overflow);
}

static void
test_completion_mac_example(void)
{
    /* §LD||MAC[R] Example 1: D=0 routes the LD half to A and
     * the MAC half to B. */
    check_pair(ti_c54x_op_ld_parallel_mac, UINT16_C(0xA8AB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAC, known_input(),
               TI_X_WORD, TI_Y_WORD, UINT8_C(0),
               TI_A_AFTER_LD, UINT8_C(1), TI_B_AFTER_MAC,
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_macr_example(void)
{
    /* §LD||MAC[R] Example 2: 0xAAAB sets R=1 (bit 9) and
     * rounds B to 0x010D0000. */
    check_pair(ti_c54x_op_ld_parallel_mac, UINT16_C(0xAAAB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAC, known_input(),
               TI_X_WORD, TI_Y_WORD, UINT8_C(0),
               TI_A_AFTER_LD, UINT8_C(1), TI_B_AFTER_MACR,
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_mas_examples(void)
{
    /* §LD||MAS[R] Examples 1/2: the subtract half pins
     * 0xFFFEF38D11 and the rounded 0xFFFEF40000. */
    check_pair(ti_c54x_op_ld_parallel_mas, UINT16_C(0xACAB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAS, known_input(),
               TI_X_WORD, TI_Y_WORD, UINT8_C(0),
               TI_A_AFTER_LD, UINT8_C(1), TI_B_AFTER_MAS,
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    check_pair(ti_c54x_op_ld_parallel_mas, UINT16_C(0xAEAB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAS, known_input(),
               TI_X_WORD, TI_Y_WORD, UINT8_C(0),
               TI_A_AFTER_LD, UINT8_C(1), TI_B_AFTER_MASR,
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_destination_mirror(void)
{
    /* 0xA9AB is R=0,D=1 (bit 8 set, bit 9 clear): D=1
     * routes the LD half to B and the MAC half to A. The MAC
     * half accumulates into A-before 0x1000, so A pins
     * 0x10C8400 + 0x1000 = 0x00010C9400. */
    check_pair(ti_c54x_op_ld_parallel_mac, UINT16_C(0xA9AB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAC, known_input(),
               TI_X_WORD, TI_Y_WORD, UINT8_C(1),
               TI_A_AFTER_LD, UINT8_C(0), UINT64_C(0x00010C9400),
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_sxm_negative_load(void)
{
    /* SXM=1 widens a negative X word into the guard bits
     * (shift16-load precedent): 0x8000 -> FF 8000 0000. */
    ti_c54x_op_input_snapshot input = known_input();

    input.status_sxm = UINT8_C(1);
    check_pair(ti_c54x_op_ld_parallel_mac, UINT16_C(0xA8AB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAC, input,
               UINT16_C(0x8000), TI_Y_WORD, UINT8_C(0),
               UINT64_C(0xFF80000000), UINT8_C(1),
               UINT64_C(0x00010C9511),
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_ovm_saturation(void)
{
    /* FRCT=1 doubles 0x7FFF x 0x7FFF to 0x7FFE0002; adding
     * B=0x007FFFFFFF overflows 32-bit signed range, so OVM=1
     * saturates the dst_ channel and writes OV. */
    ti_c54x_op_input_snapshot input = known_input();

    input.status_frct = UINT8_C(1);
    input.status_ovm = UINT8_C(1);
    input.t_register_raw16 = UINT16_C(0x7FFF);
    input.accumulator_b_raw40 = UINT64_C(0x007FFFFFFF);
    check_pair(ti_c54x_op_ld_parallel_mac, UINT16_C(0xA8AB),
               TI_C54X_OPCODE_ID_LD_PARALLEL_MAC, input, TI_X_WORD,
               UINT16_C(0x7FFF), UINT8_C(0), TI_A_AFTER_LD,
               UINT8_C(1), UINT64_C(0x007FFFFFFF),
               TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void
test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(UINT16_C(0xA8AB),
                      TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(0), UINT8_C(0), UINT8_C(0xAB));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_parallel_mac(
                           &instruction, &trusted, &input,
                           &effect) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(5));
}

static void
test_unknown_inputs_reject(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(UINT16_C(0xA8AB),
                      TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect before = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);

    input = known_input();
    input.auxiliary_registers_known_mask = UINT8_C(0xEF);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);

    input = known_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == before.kind);
}

static void
test_null_rejection(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_expect(UINT16_C(0xA8AB),
                      TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(&instruction, NULL,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_parallel_mac(&instruction, &input,
                                                 NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_decode_fanout();
    test_checked_stages_dual_without_t_load();
    test_completion_mac_example();
    test_completion_macr_example();
    test_completion_mas_examples();
    test_destination_mirror();
    test_sxm_negative_load();
    test_ovm_saturation();
    test_trusted_parity();
    test_unknown_inputs_reject();
    test_null_rejection();
    puts("ld_parallel: ok");
    return 0;
}
