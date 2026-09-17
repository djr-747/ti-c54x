/* Module contract
 *
 * Owns: RED proofs for the MAC Xmem,Ymem dual leaves, including
 * nine-row MOD0/MOD1/MOD2 decode fan-out, checked/trusted
 * decode validation, dual-indirect read staging with read-phase
 * T load of the X-side word, pair completion through the
 * multiply core with the TI Example 4/6-derived vectors,
 * and transactional rejection.
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
#include "../../../../../src/isa/ops/multiply/op_mac_dual.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI SPRU172C MAC Example 4 derived vector: the dual seam addresses
 * AR2..AR5, so X rides *AR5+ (was *AR5+) and Y rides *AR4+ (TI uses
 * *AR6+, which the frozen P2 dual root does not cover yet). Values,
 * FRCT, T load, and post-increments follow TI exactly. */
#define TI_X_WORD UINT16_C(0x5678)
#define TI_Y_WORD UINT16_C(0x1234)
#define TI_SRC_A UINT64_C(0x1000)
#define TI_ACC_B_BEFORE UINT64_C(0x0004)
#define TI_T_BEFORE UINT16_C(0x0008)
#define TI_B_AFTER_EXAMPLE4 UINT64_C(0x0C4C10C0)
#define TI_B_AFTER_EXAMPLE6 UINT64_C(0x0C4C0000)

static const ti_c54x_opcode_id expected_ids[3][3] = {
    {TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0, TI_C54X_OPCODE_ID_MAC_DUAL_M0_M1,
     TI_C54X_OPCODE_ID_MAC_DUAL_M0_M2},
    {TI_C54X_OPCODE_ID_MAC_DUAL_M1_M0, TI_C54X_OPCODE_ID_MAC_DUAL_M1_M1,
     TI_C54X_OPCODE_ID_MAC_DUAL_M1_M2},
    {TI_C54X_OPCODE_ID_MAC_DUAL_M2_M0, TI_C54X_OPCODE_ID_MAC_DUAL_M2_M1,
     TI_C54X_OPCODE_ID_MAC_DUAL_M2_M2}};

static const uint16_t row_values[3][3] = {
    {UINT16_C(0xB000), UINT16_C(0xB004), UINT16_C(0xB008)},
    {UINT16_C(0xB040), UINT16_C(0xB044), UINT16_C(0xB048)},
    {UINT16_C(0xB080), UINT16_C(0xB084), UINT16_C(0xB088)}};

static uint16_t
encode_word(uint8_t x_mode, uint8_t y_mode, uint8_t round, uint8_t source,
            uint8_t destination, uint8_t xar, uint8_t yar)
{
    return (uint16_t)(row_values[x_mode][y_mode] |
                      (uint16_t)((uint16_t)round << 10U) |
                      (uint16_t)((uint16_t)source << 9U) |
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
    TI_C54X_TEST_CHECK(
        instruction.operand_layout ==
        TI_C54X_OPERAND_LAYOUT_DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = TI_SRC_A;
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = TI_ACC_B_BEFORE;
    input.accumulator_b_known = true;
    input.t_register_raw16 = TI_T_BEFORE;
    input.t_register_known = true;
    input.status_frct = UINT8_C(1);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_trusted_operand_values
trusted_for(uint8_t round, uint8_t source, uint8_t destination, uint8_t xar,
            uint8_t yar)
{
    ti_c54x_trusted_operand_values output = {0};
    const uint64_t rsd =
        (uint64_t)((uint64_t)((uint64_t)round << 2U) |
                   (uint64_t)((uint64_t)source << 1U) |
                   (uint64_t)destination);

    output.values[0] = (ti_c54x_operand_field_value){rsd, UINT8_C(3)};
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
    uint8_t round;
    uint8_t source;
    uint8_t destination;
    uint8_t xar;
    uint8_t yar;

    for (x_mode = UINT8_C(0); x_mode < UINT8_C(3); ++x_mode) {
        for (y_mode = UINT8_C(0); y_mode < UINT8_C(3); ++y_mode) {
            for (round = UINT8_C(0); round < UINT8_C(2); ++round) {
                for (source = UINT8_C(0); source < UINT8_C(2); ++source) {
                    for (destination = UINT8_C(0); destination < UINT8_C(2);
                         ++destination) {
                        for (xar = UINT8_C(0); xar < UINT8_C(4); ++xar) {
                            for (yar = UINT8_C(0); yar < UINT8_C(4); ++yar) {
                                const uint16_t word = encode_word(
                                    x_mode, y_mode, round, source,
                                    destination, xar, yar);
                                const ti_c54x_decoded_instruction
                                    instruction = decode_expect(
                                        word,
                                        expected_ids[x_mode][y_mode]);
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
test_mod3_stays_unknown(void)
{
    /* XMOD=3 (bits 7:6 set) matches no row: MOD3 stays undecoded. */
    const uint16_t word =
        (uint16_t)(encode_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                               UINT8_C(0), UINT8_C(1), UINT8_C(3),
                               UINT8_C(2)) |
                   UINT16_C(0x00C0));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_UNKNOWN_WORD ||
                       first.id == TI_C54X_OPCODE_ID_UNKNOWN);
}

static void
test_checked_stages_dual_t_load(void)
{
    /* 0xB132: R=0, S=0 (src A), D=1 (dst B), XMOD0 XAR=3 (AR5),
     * YMOD0 YAR=2 (AR4). */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xB132), TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(0));
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

    if (expected_id == TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0) {
        leaf = ti_c54x_op_mac_dual_m0_m0;
    } else {
        leaf = ti_c54x_op_mac_dual_m1_m2;
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
test_completion_example4_derived(void)
{
    /* (0x5678<<1) x 0x1234 + 0x1000 = 0x0C4C10C0 into B. */
    check_completion(UINT16_C(0xB132),
                     TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0,
                     TI_B_AFTER_EXAMPLE4, UINT8_C(1));
}

static void
test_completion_macr_example6_derived(void)
{
    /* 0xB532 sets R=1: rounded 0x0C4C0000 into B. */
    check_completion(UINT16_C(0xB532),
                     TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0,
                     TI_B_AFTER_EXAMPLE6, UINT8_C(1));
}

static void
test_completion_src_b_dst_a(void)
{
    /* 0xB232: S=1 (src B=4), D=0 (dst A):
     * (0x5678<<1) x 0x1234 + 4 = 0x0C4C00C4 into A. */
    check_completion(UINT16_C(0xB232),
                     TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0,
                     UINT64_C(0x0C4C00C4), UINT8_C(0));
}

static void
test_completion_m1_m2_row(void)
{
    /* Row identity proves through the same completion: word 0xB17A
     * (XMOD1 XAR=3, YMOD2 YAR=2, src A, dst B). */
    check_completion(UINT16_C(0xB17A),
                     TI_C54X_OPCODE_ID_MAC_DUAL_M1_M2,
                     TI_B_AFTER_EXAMPLE4, UINT8_C(1));
}

static void
test_trusted_equivalence(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xB132), TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT8_C(3),
                    UINT8_C(2));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted_effect = {0};
    ti_c54x_op_effect checked_next = {0};
    ti_c54x_op_effect trusted_next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, &input,
                                                &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mac_dual_m0_m0(
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
                       TI_B_AFTER_EXAMPLE4);
}

static void
test_trusted_macr_rounding_equivalence(void)
{
    /* 0xB532: M0_M0, R=1, src A, dst B, XMOD0 XAR=3 (AR5),
     * YMOD0 YAR=2 (AR4). The trusted path must stage the rounding
     * select and complete the rounded MACR value. */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xB532), TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(1), UINT8_C(0), UINT8_C(1), UINT8_C(3),
                    UINT8_C(2));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted_effect = {0};
    ti_c54x_op_effect checked_next = {0};
    ti_c54x_op_effect trusted_next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, &input,
                                                &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mac_dual_m0_m0(
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
                       TI_B_AFTER_EXAMPLE6);
}

static void
test_unknown_src_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xB132), TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_unknown_ar_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xB132), TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* Clear the AR5 (bit 5) known bit: X side unknown. */
    input.auxiliary_registers_known_mask = UINT8_C(0xDF);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xB132), TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, NULL,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_dual_m0_m0(&instruction, &input,
                                                NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_decode_fanout();
    test_mod3_stays_unknown();
    test_checked_stages_dual_t_load();
    test_completion_example4_derived();
    test_completion_macr_example6_derived();
    test_completion_src_b_dst_a();
    test_completion_m1_m2_row();
    test_trusted_equivalence();
    test_trusted_macr_rounding_equivalence();
    test_unknown_src_rejects();
    test_unknown_ar_rejects();
    test_null_rejects();
    return 0;
}
