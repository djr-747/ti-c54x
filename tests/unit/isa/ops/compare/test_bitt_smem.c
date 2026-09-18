/* Module contract
 * Owns: row-bound proofs for the BITT Smem leaf (TI SPRU172C Sec4 BITT,
 * p. 4-23: TC = Smem(15 - T(3-0))): direct and standard-indirect
 * MOD0--MOD3 memory-read staging gated on a known T register with no
 * T load, read-value completion into the TC STATUS_BIT_WRITE over
 * all sixteen T bit codes, unknown-T stops at stage and completion,
 * exotic-mode deferral, field/range rejection, effect-validity, the
 * trusted two-operand entry, and the plain checked entry (UNKNOWN
 * id, local first-word mask, layout-gated extraction into the
 * values core).
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch, memory addressing or buses, status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/compare/op_bitt_smem.h"
#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/dispatch/trusted_handler.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_op_input_snapshot known_input(uint16_t t)
{
    ti_c54x_op_input_snapshot input = {0};

    input.t_register_raw16 = t;
    input.t_register_known = true;
    return input;
}

static ti_c54x_decoded_instruction null_instruction(void)
{
    ti_c54x_decoded_instruction instruction;

    (void)memset(&instruction, 0, sizeof instruction);
    return instruction;
}

static ti_c54x_decoded_instruction make_instruction(
    uint8_t indirect, uint8_t addr7)
{
    ti_c54x_decoded_instruction instruction = null_instruction();

    instruction.id = TI_C54X_OPCODE_ID_BITT_SMEM;

    instruction.operand_layout = TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7;
    instruction.bundle.program_words[0] =
        (uint16_t)(UINT16_C(0x3400) |
                   ((uint16_t)(indirect & UINT8_C(1)) << 7U) |
                   (uint16_t)(addr7 & UINT8_C(0x7F)));
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
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

static void check_completion(uint16_t t, uint16_t read, uint8_t expected_tc)
{
    ti_c54x_op_input_snapshot input = known_input(t);
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x05));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, read,
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, expected_tc);
}

/* Direct staging: I=0, addr 0x05 reads DIRECT page-0 with no T load. */
static void test_direct_stages_memory_read(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(staged.payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x05));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Indirect MOD0--MOD3 stage single-indirect reads (arf 3 each). */
static void test_indirect_modes_stage_single_indirect(void)
{
    static const uint8_t addr7_for_mode[4] = {
        UINT8_C(0x03), UINT8_C(0x0B), UINT8_C(0x13), UINT8_C(0x1B)};
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));

    for (uint8_t mode = UINT8_C(0); mode < UINT8_C(4); ++mode) {
        ti_c54x_op_effect staged = {0};

        TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                               UINT8_C(1), addr7_for_mode[mode], &input,
                               &staged) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                           TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
        TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                           (ti_c54x_indirect_single_mode)mode);
        TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf ==
                           UINT8_C(3));
        TI_C54X_TEST_CHECK(staged.payload.memory.load_t_with_read == false);
        /* MOD3 reads stay carrier-invalid by the single-leg write-only
         * rule; MOD0--MOD2 carriers validate. */
        if (mode != UINT8_C(3)) {
            TI_C54X_TEST_CHECK(
                ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
        }
    }
}

/* All sixteen T bit codes: a read with only the addressed bit set
 * stages TC 1, and the complement stages TC 0. */
static void test_all_bit_codes(void)
{
    for (uint16_t code = UINT16_C(0); code < UINT16_C(16); ++code) {
        const uint16_t addressed =
            (uint16_t)(UINT16_C(1) << (15U - code));

        check_completion(code, addressed, UINT8_C(1));
        check_completion(
            code, (uint16_t)(UINT16_C(0xFFFF) ^ addressed), UINT8_C(0));
    }
}

/* Only the low nibble of T selects the bit: T 0x000C addresses bit 3,
 * so the TI-style Smem 0x0008 (bit 3 set) stages TC 1 while T 0xFF0C
 * stages the same TC. */
static void test_t_nibble_selects_bit(void)
{
    check_completion(UINT16_C(0x000C), UINT16_C(0x0008), UINT8_C(1));
    check_completion(UINT16_C(0xFF0C), UINT16_C(0x0008), UINT8_C(1));
    check_completion(UINT16_C(0x000C), UINT16_C(0xFFF7), UINT8_C(0));
    check_completion(UINT16_C(0x0000), UINT16_C(0x7FFF), UINT8_C(0));
    check_completion(UINT16_C(0x000F), UINT16_C(0xFFFE), UINT8_C(0));
}

/* Unknown T stops staging. */
static void test_unknown_t_rejects_at_stage(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));
    ti_c54x_op_effect staged = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
                           &staged) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

/* Unknown T also stops the completion. */
static void test_unknown_t_rejects_at_completion(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x000C));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x05));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    input.t_register_known = false;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

/* Exotic mode bits (mode 4 and mode 15) defer without touching the effect. */
static void test_exotic_modes_reject(void)
{
    static const uint8_t exotic[2] = {UINT8_C(0x20), UINT8_C(0x7F)};
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));

    for (size_t i = 0U; i < sizeof exotic / sizeof exotic[0]; ++i) {
        ti_c54x_op_effect staged;
        ti_c54x_op_effect before;

        (void)memset(&staged, 0xA5, sizeof staged);
        before = staged;
        TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                               UINT8_C(1), exotic[i], &input,
                               &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&staged, &before, sizeof staged) == 0);
    }
}

/* Out-of-range fields reject: indirect 2, addr 0x80. */
static void test_field_ranges_reject(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(2), UINT8_C(0x05), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x80), &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* NULL borrowers reject on every entry and on the completion. */
static void test_null_arguments_reject(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x000C));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x05));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(
                           NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(
                           &instruction, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(
                           &instruction, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
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
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, &operands, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, &operands, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Forged completion instruction rejects without touching the effect. */
static void test_completion_forged_decode_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x000C));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x05));
    ti_c54x_decoded_instruction forged = instruction;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next;
    ti_c54x_op_effect before;

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem_values(
                           UINT8_C(0), UINT8_C(0x05), &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    (void)memset(&next, 0xA5, sizeof next);
    before = next;
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &forged, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof next) == 0);
}

/* Trusted entry stages the same DIRECT read from (I, addr7). */
static void test_trusted_stages_direct_read(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x000C));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x10));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[1].raw_value = UINT64_C(0x10);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x10));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_OK);
    /* T nibble 0xC addresses bit 3; 0x0008 has bit 3 set -> TC 1. */
    check_tc_effect(&next, UINT8_C(1));
}

/* Trusted entry rejects bad arity, bad widths, and out-of-range values. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));
    ti_c54x_decoded_instruction instruction = null_instruction();

    instruction.id = TI_C54X_OPCODE_ID_BITT_SMEM;
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};

    operands.count = UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(2);
    operands.values[1].width_bits = UINT8_C(8);
    operands.values[1].raw_value = UINT64_C(0x10);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[1].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_bitt_smem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Checked entry: 0x3405 extracts I=0/addr 0x05, staging direct. */
static void test_checked_entry_stages_direct_read(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x000C));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x05));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(&instruction, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x05));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0008),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_tc_effect(&next, UINT8_C(1));
}

/* Checked entry: 0x3495 extracts I=1/MOD2/AR5, staging indirect. */
static void test_checked_entry_stages_indirect_read(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(1), UINT8_C(0x15));
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0x3495));
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(&instruction, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Checked entry rejects foreign ids, foreign masks, wrong layout or
 * word count; the effect stays untouched. */
static void test_checked_entry_rejects_bad_decode(void)
{
    ti_c54x_op_input_snapshot input = known_input(UINT16_C(0x0000));
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x05));
    ti_c54x_decoded_instruction forged;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect before = {0};

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_CMPR_AR;
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x3505);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_bitt_smem(&forged, &input,
                                           &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);
}

int main(void)
{
    test_direct_stages_memory_read();
    test_indirect_modes_stage_single_indirect();
    test_all_bit_codes();
    test_t_nibble_selects_bit();
    test_unknown_t_rejects_at_stage();
    test_unknown_t_rejects_at_completion();
    test_exotic_modes_reject();
    test_field_ranges_reject();
    test_null_arguments_reject();
    test_completion_forged_decode_rejects();
    test_trusted_stages_direct_read();
    test_trusted_shape_rejects();
    test_checked_entry_stages_direct_read();
    test_checked_entry_stages_indirect_read();
    test_checked_entry_rejects_bad_decode();
    return 0;
}
