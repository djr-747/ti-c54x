/* Module contract
 * Owns: proofs for the BIT Xmem leaf (TI SPRU172C Sec4 BIT,
 * p. 4-21: TC = Xmem(15 - BITC)): the TI worked example
 * BIT *AR5+,15-12, X-only dual-indirect read-leg staging with Y
 * self-alias, read-value completion into the TC STATUS_BIT_WRITE
 * over all sixteen BITC codes with edges 0 and 15 pinned, the
 * trusted three-operand entry, field/range rejection (AR outside
 * AR2..AR5, BITC above 15), forged completion rejection, and
 * checked-entry invalid-decode rejection (wrong layout, word count,
 * or first-word prefix). All four XMOD codes stage, including MOD3
 * (XMOD 3) indexed and circular commits owned by the shared X-only
 * dual resolver, as in the LD Xmem SHIFT sibling.
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch, memory addressing or buses, status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/compare/op_bit_xmem.h"
#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/dispatch/trusted_handler.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
bit_word(uint8_t xmod, uint8_t xar_raw, uint8_t bitc)
{
    return (uint16_t)(UINT16_C(0x9600) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_raw & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(bitc & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction make_instruction(uint16_t first_word)
{
    ti_c54x_decoded_instruction instruction;

    (void)memset(&instruction, 0, sizeof instruction);
    instruction.operand_layout = TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot blank_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    return input;
}

static void check_tc_effect(
    const ti_c54x_op_effect *effect, uint8_t expected)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_STATUS_BIT_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.target_register ==
                       TI_C54X_OP_STATUS_REGISTER_ST0);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.bit_index ==
                       UINT8_C(12));
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.raw_value ==
                       expected);
}

static void check_completion(
    uint8_t xmod, uint8_t xar_raw, uint8_t bitc, uint16_t read,
    uint8_t expected_tc)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(xmod, xar_raw, bitc));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           xmod, (uint8_t)(xar_raw + UINT8_C(2)), bitc,
                           &input, &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, read,
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, expected_tc);
}

/* TI worked example: BIT *AR5+,15-12 assembles to 0x967C and stages
 * an X-only dual-indirect READ selecting AR5; address post-increment
 * itself belongs to the resolver. Bit 3 of the read word drives TC:
 * 0x0008 stages TC 1, 0xFFF7 stages TC 0. */
static void test_ti_worked_example(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(UINT8_C(1), UINT8_C(3), UINT8_C(12)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0x967C));
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(5), UINT8_C(12), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(staged.payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, UINT8_C(1));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0xFFF7),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, UINT8_C(0));
}

/* BITC 0 addresses bit 15: 0x8000 stages TC 1, 0x7FFF stages TC 0. */
static void test_tc_top_bit(void)
{
    check_completion(
        UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT16_C(0x8000),
        UINT8_C(1));
    check_completion(
        UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT16_C(0x7FFF),
        UINT8_C(0));
}

/* BITC edges 0 and 15: bit 15 and bit 0 respectively. */
static void test_bitc_edges(void)
{
    check_completion(
        UINT8_C(2), UINT8_C(1), UINT8_C(0), UINT16_C(0x8000),
        UINT8_C(1));
    check_completion(
        UINT8_C(2), UINT8_C(1), UINT8_C(0), UINT16_C(0x7FFF),
        UINT8_C(0));
    check_completion(
        UINT8_C(2), UINT8_C(1), UINT8_C(15), UINT16_C(0x0001),
        UINT8_C(1));
    check_completion(
        UINT8_C(2), UINT8_C(1), UINT8_C(15), UINT16_C(0xFFFE),
        UINT8_C(0));
}

/* All sixteen BITC codes: a read with only the addressed bit set
 * stages TC 1, and the complement stages TC 0. */
static void test_all_bitc_codes(void)
{
    for (uint16_t code = UINT16_C(0); code < UINT16_C(16); ++code) {
        const uint16_t addressed =
            (uint16_t)(UINT16_C(1) << (15U - code));
        const uint8_t bitc = (uint8_t)code;

        check_completion(
            UINT8_C(1), UINT8_C(2), bitc, addressed, UINT8_C(1));
        check_completion(
            UINT8_C(1), UINT8_C(2), bitc,
            (uint16_t)(UINT16_C(0xFFFF) ^ addressed), UINT8_C(0));
    }
}

/* XMOD 3 (MOD3 indexed/circular) stages an X-only dual read like
 * every other XMOD code; the resolver owns the commit. AR outside
 * AR2..AR5 and BITC 16 still reject. */
static void test_field_ranges_stage_or_reject(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(3), UINT8_C(5), UINT8_C(12), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
    (void)memset(&staged, 0, sizeof staged);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(1), UINT8_C(12), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(6), UINT8_C(12), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(5), UINT8_C(16), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* NULL borrowers reject on every entry and on the completion. */
static void test_null_arguments_reject(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(UINT8_C(1), UINT8_C(3), UINT8_C(12)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(5), UINT8_C(12), NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(5), UINT8_C(12), &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(
                           NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(
                           &instruction, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(
                           &instruction, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(5), UINT8_C(12), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           NULL, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0008),
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Checked entry rejects a foreign layout, a two-word bundle, and
 * foreign first-word prefixes without touching the effect. The
 * XMOD-3 encoding stages normally. */
static void test_checked_entry_rejects_bad_decode(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(UINT8_C(1), UINT8_C(3), UINT8_C(12)));
    ti_c54x_decoded_instruction forged;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect before = {0};

    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x977C);
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_words[0] =
        bit_word(UINT8_C(3), UINT8_C(3), UINT8_C(12));
    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem(&forged, &input,
                                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
}

/* Forged completion instructions reject without touching the effect:
 * flipped prefix and foreign layout. The XMOD-3 encoding completes
 * normally: BITC still rides the low four bits. */
static void test_completion_forged_decode_rejects(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(UINT8_C(1), UINT8_C(3), UINT8_C(12)));
    ti_c54x_decoded_instruction forged;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next;
    ti_c54x_op_effect before;

    TI_C54X_TEST_CHECK(ti_c54x_op_bit_xmem_values(
                           UINT8_C(1), UINT8_C(5), UINT8_C(12), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);

    forged = instruction;
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    (void)memset(&next, 0xA5, sizeof next);
    before = next;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &forged, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof next) == 0);

    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    (void)memset(&next, 0xA5, sizeof next);
    before = next;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &forged, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof next) == 0);

    forged = instruction;
    forged.bundle.program_words[0] =
        bit_word(UINT8_C(3), UINT8_C(3), UINT8_C(12));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &forged, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, UINT8_C(1));
}

/* Trusted entry stages the same read from (XMOD, raw XAR, BITC) and
 * completes TC 1 from the 0x0008 read word. */
static void test_trusted_stages_read(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(UINT8_C(1), UINT8_C(3), UINT8_C(12)));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(3);
    operands.values[0].width_bits = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[1].raw_value = UINT64_C(3);
    operands.values[2].width_bits = UINT8_C(4);
    operands.values[2].raw_value = UINT64_C(12);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, UINT8_C(1));
}

/* Trusted entry rejects bad arity, bad widths, and out-of-range
 * values. XMOD 3 stages normally. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(bit_word(UINT8_C(1), UINT8_C(3), UINT8_C(12)));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};

    operands.count = UINT8_C(2);
    operands.values[0].width_bits = UINT8_C(2);
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);

    operands.count = UINT8_C(3);
    operands.values[2].width_bits = UINT8_C(5);
    operands.values[2].raw_value = UINT64_C(12);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);

    operands.values[2].width_bits = UINT8_C(4);
    operands.values[2].raw_value = UINT64_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);

    operands.values[0].raw_value = UINT64_C(3);
    operands.values[2].raw_value = UINT64_C(12);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bit_xmem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(3));
}

int main(void)
{
    test_ti_worked_example();
    test_tc_top_bit();
    test_bitc_edges();
    test_all_bitc_codes();
    test_field_ranges_stage_or_reject();
    test_null_arguments_reject();
    test_checked_entry_rejects_bad_decode();
    test_completion_forged_decode_rejects();
    test_trusted_stages_read();
    test_trusted_shape_rejects();
    return 0;
}
