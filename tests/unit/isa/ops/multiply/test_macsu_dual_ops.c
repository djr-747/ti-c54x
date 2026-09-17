/* Module contract
 *
 * Owns: RED proofs for the MACSU Xmem,Ymem dual leaves, including
 * nine-row MOD0/MOD1/MOD2 decode fan-out, checked/trusted
 * decode validation, dual-indirect read staging with read-phase
 * T load of the X-side word, pair completion through the
 * multiply core with unsigned-X/signed-Y vectors (oracle, FRCT,
 * and mixed-sign lanes), and transactional rejection.
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
#include "../../../../../src/isa/ops/multiply/op_macsu_dual.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Independently recomputed oracle: uns(0x4000) * 0x4000 + 0. */
#define ORACLE_X_WORD UINT16_C(0x4000)
#define ORACLE_Y_WORD UINT16_C(0x4000)
#define ORACLE_AFTER UINT64_C(0x10000000)
#define ORACLE_FRCT_AFTER UINT64_C(0x20000000)
/* Mixed-sign lane: uns(0x8765) * -16384. */
#define MIXED_X_WORD UINT16_C(0x8765)
#define MIXED_Y_WORD UINT16_C(0xC000)
#define MIXED_AFTER UINT64_C(0xFFDE26C000)

static const ti_c54x_opcode_id expected_ids[3][3] = {
    {TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0, TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M1,
     TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M2},
    {TI_C54X_OPCODE_ID_MACSU_DUAL_M1_M0, TI_C54X_OPCODE_ID_MACSU_DUAL_M1_M1,
     TI_C54X_OPCODE_ID_MACSU_DUAL_M1_M2},
    {TI_C54X_OPCODE_ID_MACSU_DUAL_M2_M0, TI_C54X_OPCODE_ID_MACSU_DUAL_M2_M1,
     TI_C54X_OPCODE_ID_MACSU_DUAL_M2_M2}};

static const uint16_t row_values[3][3] = {
    {UINT16_C(0xA800), UINT16_C(0xA804), UINT16_C(0xA808)},
    {UINT16_C(0xA840), UINT16_C(0xA844), UINT16_C(0xA848)},
    {UINT16_C(0xA880), UINT16_C(0xA884), UINT16_C(0xA888)}};

static uint16_t
encode_word(uint8_t x_mode, uint8_t y_mode, uint8_t select, uint8_t xar,
            uint8_t yar)
{
    return (uint16_t)(row_values[x_mode][y_mode] |
                      (uint16_t)((uint16_t)select << 8U) |
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
        TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR);
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

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0000);
    input.t_register_known = true;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static ti_c54x_trusted_operand_values
trusted_for(uint8_t select, uint8_t xar, uint8_t yar)
{
    ti_c54x_trusted_operand_values output = {0};

    output.values[0] =
        (ti_c54x_operand_field_value){(uint64_t)select, UINT8_C(1)};
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
    uint8_t select;
    uint8_t xar;
    uint8_t yar;

    /* All 288 S/Xar/Yar encodings land on their mode row. */
    for (x_mode = UINT8_C(0); x_mode < UINT8_C(3); ++x_mode) {
        for (y_mode = UINT8_C(0); y_mode < UINT8_C(3); ++y_mode) {
            for (select = UINT8_C(0); select < UINT8_C(2); ++select) {
                for (xar = UINT8_C(0); xar < UINT8_C(4); ++xar) {
                    for (yar = UINT8_C(0); yar < UINT8_C(4); ++yar) {
                        const uint16_t word = encode_word(
                            x_mode, y_mode, select, xar, yar);
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
    /* XMOD=3 (bits 7:6 set) matches no row: MOD3 stays undecoded. */
    const uint16_t word =
        (uint16_t)(encode_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                               UINT8_C(0), UINT8_C(0)) |
                   UINT16_C(0x00C0));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_UNKNOWN_WORD ||
                       first.id == TI_C54X_OPCODE_ID_UNKNOWN);
}

static void
test_checked_stages_dual_t_load(void)
{
    /* 0xA832: S=0 (src/dst A), XMOD0 XAR=3 (AR5), YMOD0 YAR=2 (AR4). */
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA832), TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, &input,
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
                 uint16_t x_word, uint16_t y_word, bool frct,
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

    if (expected_id == TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0) {
        leaf = ti_c54x_op_macsu_dual_m0_m0;
    } else {
        leaf = ti_c54x_op_macsu_dual_m1_m2;
    }
    input.status_frct = frct ? UINT8_C(1) : UINT8_C(0);
    TI_C54X_TEST_CHECK(leaf(&instruction, &input, &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, x_word, y_word,
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
test_completion_oracle(void)
{
    /* uns(0x4000) x 0x4000 + 0 = 0x10000000 into A. */
    check_completion(UINT16_C(0xA832),
                     TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0, ORACLE_X_WORD,
                     ORACLE_Y_WORD, false, ORACLE_AFTER, UINT8_C(0));
}

static void
test_completion_frct_doubles(void)
{
    /* FRCT=1 doubles the mixed product: 0x20000000 into A. */
    check_completion(UINT16_C(0xA832),
                     TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0, ORACLE_X_WORD,
                     ORACLE_Y_WORD, true, ORACLE_FRCT_AFTER, UINT8_C(0));
}

static void
test_completion_mixed_sign(void)
{
    /* uns(0x8765) x -16384 lands the mixed product into A; either
     * uniform-signedness core reading gives a different value. */
    check_completion(UINT16_C(0xA832),
                     TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0, MIXED_X_WORD,
                     MIXED_Y_WORD, false, MIXED_AFTER, UINT8_C(0));
}

static void
test_completion_selects_b(void)
{
    /* 0xA932: S=1 selects B as the shared src/dst. */
    check_completion(UINT16_C(0xA932),
                     TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0, ORACLE_X_WORD,
                     ORACLE_Y_WORD, false, ORACLE_AFTER, UINT8_C(1));
}

static void
test_completion_m1_m2_row(void)
{
    /* Row identity proves through the same completion: word 0xA87A
     * (XMOD1 XAR=3, YMOD2 YAR=2, S=0). */
    check_completion(UINT16_C(0xA87A),
                     TI_C54X_OPCODE_ID_MACSU_DUAL_M1_M2, ORACLE_X_WORD,
                     ORACLE_Y_WORD, false, ORACLE_AFTER, UINT8_C(0));
}

static void
test_trusted_equivalence(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA832), TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_trusted_operand_values trusted =
        trusted_for(UINT8_C(0), UINT8_C(3), UINT8_C(2));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted_effect = {0};
    ti_c54x_op_effect checked_next = {0};
    ti_c54x_op_effect trusted_next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, &input,
                                                  &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_macsu_dual_m0_m0(
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
                           &instruction, &input, ORACLE_X_WORD,
                           ORACLE_Y_WORD,
                           &checked_next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted_effect.payload.memory.complete_dual(
                           &instruction, &input, ORACLE_X_WORD,
                           ORACLE_Y_WORD,
                           &trusted_next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked_next.payload.accumulator_write.value_raw40 ==
                       trusted_next.payload.accumulator_write.value_raw40);
    TI_C54X_TEST_CHECK(checked_next.payload.accumulator_write.value_raw40 ==
                       ORACLE_AFTER);
}

static void
test_crossed_row_rejects(void)
{
    /* An MPY dual word through the MACSU leaf is a decode mismatch. */
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0xA400));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0xA400), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction mpy;
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MPY_DUAL_M0_M0);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &mpy) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&mpy, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void
test_unknown_src_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA832), TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* S=0 selects A: unknown A stops. */
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    /* S=1 word selects B: unknown B stops, unknown A is tolerated. */
    instruction = decode_expect(UINT16_C(0xA932),
                                TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0);
    input.accumulator_b_known = false;
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_unknown_ar_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA832), TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* Clear the AR5 (bit 5) known bit: X side unknown. */
    input.auxiliary_registers_known_mask = UINT8_C(0xDF);
    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction = decode_expect(
        UINT16_C(0xA832), TI_C54X_OPCODE_ID_MACSU_DUAL_M0_M0);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, NULL,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_macsu_dual_m0_m0(&instruction, &input,
                                                  NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_decode_fanout();
    test_mod3_stays_unknown();
    test_checked_stages_dual_t_load();
    test_completion_oracle();
    test_completion_frct_doubles();
    test_completion_mixed_sign();
    test_completion_selects_b();
    test_completion_m1_m2_row();
    test_trusted_equivalence();
    test_crossed_row_rejects();
    test_unknown_src_rejects();
    test_unknown_ar_rejects();
    test_null_rejects();
    return 0;
}
