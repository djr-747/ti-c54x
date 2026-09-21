/* Module contract
 * Owns: RED unit proofs for the ST parallel-MAC/MAS/MPY opcode
 * leaves (TI SPRU172C Table 2-25): row fan-out over R/S/D and the
 * FIRS-split X/Y modes, the TI §ST||MAC/§ST||MAS/§ST||MPY worked
 * vectors with driven OV and preserved C, S/D routing, R rounding,
 * OVM saturation, trusted parity, and transactional rejection.
 * Does not own: opcode semantics, sequencing, T/FRCT/OVM policy,
 * or the store-compute child root (owned by the S115 seam).
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_st_parallel_mac.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"

#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI §ST||MAC Example 1: ST A,*AR4- || MAC *AR5,B with A
 * 00 0011 1111, B 00 0000 1111, T 0x0400, X 0x4321, ASM 5,
 * FRCT 0 accumulates B to 00 010C 9511 and stores 0x0222. */
#define TI_MAC_WORD UINT16_C(0xD136)
#define TI_MACR_WORD UINT16_C(0xD536)
#define TI_A_MAC UINT64_C(0x0000111111)
#define TI_B_BEFORE UINT64_C(0x1111)
#define TI_B_AFTER_MAC UINT64_C(0x00010C9511)
#define TI_B_AFTER_MACR UINT64_C(0x00010D0000)
#define TI_T_MAC UINT16_C(0x0400)
#define TI_X_MAC UINT16_C(0x4321)
#define TI_ST_MAC UINT16_C(0x0222)
/* TI §ST||MAS Example 1: ST A,*AR4+ || MAS *AR5,B pins B at
 * FF FEF3 8D11 with the same ST triple; Example 2 (MASR
 * *AR5+) pins B at FF FEF4 0000 with ASM 1 staging 0x0022. */
#define TI_MAS_WORD UINT16_C(0xD93A)
#define TI_MASR_WORD UINT16_C(0xDDBA)
#define TI_B_AFTER_MAS UINT64_C(0xFFFEF38D11)
#define TI_B_AFTER_MASR UINT64_C(0xFFFEF40000)
#define TI_ST_MASR UINT16_C(0x0022)
/* TI §ST||MPY example: ST A,*AR3+ || MPY *AR5+,B with A
 * FF 8421 1234, T/X 0x4000, FRCT 1, ASM 0 pins B at
 * 00 2000 0000 and stores 0x8421. */
#define TI_MPY_WORD UINT16_C(0xCDB9)
#define TI_A_MPY UINT64_C(0xFF84211234)
#define TI_B_AFTER_MPY UINT64_C(0x0020000000)
#define TI_T_MPY UINT16_C(0x4000)
#define TI_X_MPY UINT16_C(0x4000)
#define TI_ST_MPY UINT16_C(0x8421)

static ti_c54x_op_input_snapshot
known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_known = true;
    input.accumulator_a_raw40 = TI_A_MAC;
    input.accumulator_b_known = true;
    input.accumulator_b_raw40 = TI_B_BEFORE;
    input.status_asm_known = true;
    input.status_asm = UINT8_C(5);
    input.status_ovm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_frct_known = true;
    input.status_frct = UINT8_C(0);
    input.t_register_known = true;
    input.t_register_raw16 = TI_T_MAC;
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_decoded_instruction
decode_expect(uint16_t word, ti_c54x_opcode_id id,
              ti_c54x_operand_layout_id layout,
              ti_c54x_input_resource_mask resources)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout == layout);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources == resources);
    return instruction;
}

static void
check_single(uint16_t word, ti_c54x_opcode_id id,
             ti_c54x_op_input_snapshot input, uint16_t x_word,
             uint16_t expected_st, uint64_t expected_value,
             ti_c54x_op_destination_overflow_action ov_action)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        word, id,
        id == TI_C54X_OPCODE_ID_ST_PARALLEL_MPY
            ? TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY
            : TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    ti_c54x_op_effect next = {0};
    ti_c54x_op_effect *effect = NULL;
    ti_c54x_op_status status;

    switch (id) {
    case TI_C54X_OPCODE_ID_ST_PARALLEL_MAC:
        status = ti_c54x_op_st_parallel_mac(&instruction, &input, &next);
        break;
    case TI_C54X_OPCODE_ID_ST_PARALLEL_MAS:
        status = ti_c54x_op_st_parallel_mas(&instruction, &input, &next);
        break;
    default:
        status = ti_c54x_op_st_parallel_mpy(&instruction, &input, &next);
        break;
    }
    TI_C54X_TEST_CHECK(status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(next.payload.memory.value_word == expected_st);
    TI_C54X_TEST_CHECK(next.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(next.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(next.payload.memory.complete != NULL);
    effect = &next;
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16);
    {
        ti_c54x_op_effect completion = {0};

        TI_C54X_TEST_CHECK(effect->payload.memory.complete(
                               &instruction, &input, x_word,
                               &completion) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(completion.kind ==
                           TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
        TI_C54X_TEST_CHECK(completion.payload.accumulator_write.value_raw40 ==
                           expected_value);
        /* The multiply core reports no carry: C preserves on all
         * three compute halves (standalone MAC/MPY precedent). */
        TI_C54X_TEST_CHECK(completion.payload.accumulator_write.status_delta
                               .c_action ==
                           TI_C54X_OP_STATUS_BIT_PRESERVE);
        TI_C54X_TEST_CHECK(completion.payload.accumulator_write.status_delta
                               .destination_overflow_action == ov_action);
    }
}

static void
test_decode_fanout(void)
{
    static const uint16_t bases[3] = {UINT16_C(0xD000), UINT16_C(0xD800),
                                      UINT16_C(0xCC00)};
    static const ti_c54x_opcode_id ids[3] = {
        TI_C54X_OPCODE_ID_ST_PARALLEL_MAC, TI_C54X_OPCODE_ID_ST_PARALLEL_MAS,
        TI_C54X_OPCODE_ID_ST_PARALLEL_MPY};
    size_t row;

    for (row = 0U; row < 3U; ++row) {
        uint32_t round;
        uint32_t src;
        uint32_t dst;

        for (round = 0U; round < 2U; ++round) {
            for (src = 0U; src < 2U; ++src) {
                for (dst = 0U; dst < 2U; ++dst) {
                    uint32_t x_mode;
                    uint32_t x_ar;

                    if (row == 2U && round != 0U) {
                        continue;
                    }
                    for (x_mode = 0U; x_mode < 4U; ++x_mode) {
                        for (x_ar = 0U; x_ar < 4U; ++x_ar) {
                            uint32_t y_mode;
                            uint32_t y_ar;

                            for (y_mode = 0U; y_mode < 4U; ++y_mode) {
                                for (y_ar = 0U; y_ar < 4U; ++y_ar) {
                                    uint16_t dualxy = (uint16_t)(
                                        (x_mode << 6) | (x_ar << 4) |
                                        (y_mode << 2) | y_ar);
                                    uint16_t word = (uint16_t)(
                                        bases[row] | (uint16_t)(round << 10) |
                                        (uint16_t)(src << 9) |
                                        (uint16_t)(dst << 8) | dualxy);
                                    ti_c54x_decode_result result =
                                        ti_c54x_decode_first_word(word);

                                    TI_C54X_TEST_CHECK(
                                        result.status ==
                                        TI_C54X_DECODE_STATUS_OK);
                                    TI_C54X_TEST_CHECK(result.id ==
                                                       ids[row]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
test_completion_mac_example(void)
{
    /* TI §ST||MAC Example 1: 0xD136 routes the MAC half onto B
     * with dst_ B; 0x1111 + 0x0400*0x4321 pins B at 00 010C 9511
     * with no 32-bit overflow, so OV preserves (C preserves: the
     * multiply core reports no carry). */
    check_single(TI_MAC_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAC, known_input(),
                 TI_X_MAC, TI_ST_MAC, TI_B_AFTER_MAC,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_macr_rounding(void)
{
    /* R variant on the Example 1 addressing: rounding pins B at
     * 00 010D 0000 (S114 rounding precedent), OV still preserves. */
    check_single(TI_MACR_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAC, known_input(),
                 TI_X_MAC, TI_ST_MAC, TI_B_AFTER_MACR,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_mas_example(void)
{
    /* TI §ST||MAS Example 1: 0xD93A routes the MAS half onto B;
     * 0x1111 - 0x0400*0x4321 pins B at FF FEF3 8D11, OV preserves. */
    check_single(TI_MAS_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAS, known_input(),
                 TI_X_MAC, TI_ST_MAC, TI_B_AFTER_MAS,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_masr_example(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* TI §ST||MAS Example 2: 0xDDBA with ASM 1 stages the same
     * STH shape to 0x0022 and rounds B to FF FEF4 0000. */
    input.status_asm = UINT8_C(1);
    check_single(TI_MASR_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAS, input, TI_X_MAC,
                 TI_ST_MASR, TI_B_AFTER_MASR,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_completion_mpy_example(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* TI §ST||MPY example: 0xCDB9 routes the MPY half onto B;
     * 0x4000*0x4000 with FRCT pins B at 00 2000 0000, the STH
     * shape stages 0x8421 from A with ASM 0, OV preserves. */
    input.accumulator_a_raw40 = TI_A_MPY;
    input.status_asm = UINT8_C(0);
    input.status_frct = UINT8_C(1);
    input.t_register_raw16 = TI_T_MPY;
    check_single(TI_MPY_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MPY, input, TI_X_MPY,
                 TI_ST_MPY, TI_B_AFTER_MPY,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_ovm_saturation(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* dst_ 0x007FFFFFFF plus a positive product overflows the
     * 32-bit range with OVM set: the MAC half saturates B at
     * 0x007FFFFFFF and writes OV. */
    input.status_ovm = UINT8_C(1);
    input.accumulator_b_raw40 = UINT64_C(0x007FFFFFFF);
    input.t_register_raw16 = UINT16_C(0x7FFF);
    check_single(TI_MAC_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAC, input,
                 UINT16_C(0x7FFF), TI_ST_MAC, UINT64_C(0x007FFFFFFF),
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void
test_destination_mirror(void)
{
    /* S=0, D=0 lands the MAC half on A with dst_ A: the 0xD036
     * word accumulates A-old plus T*X into A while the store
     * still uses src A. */
    check_single(UINT16_C(0xD036),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_MAC, known_input(), TI_X_MAC,
                 TI_ST_MAC, UINT64_C(0x0011D9511),
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
test_src_equals_dst(void)
{
    ti_c54x_op_input_snapshot input = known_input();

    /* S=1, D=1: the store uses B-before while the MAS half
     * overwrites B from dst_ B. 0xDB3A keeps the Example 1
     * addressing with S set: 0x111111 - 0x0400*0x4321 pins B at
     * FF FF04 8D11. */
    input.accumulator_b_raw40 = TI_A_MAC;
    check_single(UINT16_C(0xDB3A),
                 TI_C54X_OPCODE_ID_ST_PARALLEL_MAS, input, TI_X_MAC, TI_ST_MAC,
                 UINT64_C(0xFFFF048D11),
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static ti_c54x_trusted_operand_values
trusted_for(uint8_t round, uint8_t src, uint8_t dst, uint16_t dualxy)
{
    ti_c54x_trusted_operand_values trusted = {0};

    trusted.count = UINT8_C(4);
    trusted.values[0].raw_value = round;
    trusted.values[0].width_bits = UINT8_C(1);
    trusted.values[1].raw_value = src;
    trusted.values[1].width_bits = UINT8_C(1);
    trusted.values[2].raw_value = dst;
    trusted.values[2].width_bits = UINT8_C(1);
    trusted.values[3].raw_value = dualxy;
    trusted.values[3].width_bits = UINT8_C(8);
    return trusted;
}

static void
test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        TI_MAC_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAC,
        TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT16_C(0x36));
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect completion = {0};

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_parallel_mac(
                           &instruction, &trusted, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.value_word == TI_ST_MAC);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete(
                           &instruction, &input, TI_X_MAC,
                           &completion) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(completion.payload.accumulator_write.value_raw40 ==
                       TI_B_AFTER_MAC);
}

static void
test_unknown_inputs_reject(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        TI_MAC_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAC,
        TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_input_snapshot input = known_input();

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mac(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mac(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_b_known = false;
    {
        ti_c54x_decoded_instruction mas_instruction = decode_expect(
            TI_MAS_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MAS,
            TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY,
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
        TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mas(&mas_instruction, &input,
                                                      &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    }
    input = known_input();
    input.status_asm_known = false;
    {
        ti_c54x_decoded_instruction mpy_instruction = decode_expect(
            TI_MPY_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MPY,
            TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY,
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
        TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mpy(&mpy_instruction, &input,
                                                      &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    }
}

static void
test_null_rejection(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        TI_MPY_WORD, TI_C54X_OPCODE_ID_ST_PARALLEL_MPY,
        TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mac(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mac(&instruction, NULL,
                                                   &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_parallel_mac(&instruction, &input,
                                                   NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_parallel_mac(
                           NULL, NULL, NULL, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_decode_fanout();
    test_completion_mac_example();
    test_completion_macr_rounding();
    test_completion_mas_example();
    test_completion_masr_example();
    test_completion_mpy_example();
    test_ovm_saturation();
    test_destination_mirror();
    test_src_equals_dst();
    test_trusted_parity();
    test_unknown_inputs_reject();
    test_null_rejection();
    puts("st_parallel_mac: ok");
    return 0;
    return 0;
}
