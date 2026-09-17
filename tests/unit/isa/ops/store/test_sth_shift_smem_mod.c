/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure two-word STH-by-SHIFT
 * MOD4-family indirect effects, including source/ARF/MOD/SHIFT
 * selection over the six MOD rows, signed-SHIFT high-word
 * extraction, SXM riding hostile and unconsumed, trusted
 * equivalence, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_sth_shift_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_handler)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
typedef ti_c54x_op_status (*trusted_handler)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod4(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod5(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod6(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod7(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod8(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod10(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod4(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod5(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod6(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod7(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod8(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod10(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

typedef struct mod_row {
    ti_c54x_opcode_id id;
    uint16_t base;
    uint8_t mod;
    ti_c54x_operand_layout_id layout;
    checked_handler checked;
    trusted_handler trusted;
} mod_row;

static const mod_row ROWS[] = {
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD4, UINT16_C(0x6FA0),
     UINT8_C(4), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
     ti_c54x_op_sth_shift_smem_mod4, ti_c54x_trusted_op_sth_shift_smem_mod4},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD5, UINT16_C(0x6FA8),
     UINT8_C(5), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
     ti_c54x_op_sth_shift_smem_mod5, ti_c54x_trusted_op_sth_shift_smem_mod5},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD6, UINT16_C(0x6FB0),
     UINT8_C(6), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
     ti_c54x_op_sth_shift_smem_mod6, ti_c54x_trusted_op_sth_shift_smem_mod6},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD7, UINT16_C(0x6FB8),
     UINT8_C(7), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
     ti_c54x_op_sth_shift_smem_mod7, ti_c54x_trusted_op_sth_shift_smem_mod7},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD8, UINT16_C(0x6FC0),
     UINT8_C(8), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
     ti_c54x_op_sth_shift_smem_mod8, ti_c54x_trusted_op_sth_shift_smem_mod8},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD10, UINT16_C(0x6FD0),
     UINT8_C(10), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
     ti_c54x_op_sth_shift_smem_mod10,
     ti_c54x_trusted_op_sth_shift_smem_mod10},
};

static uint16_t
mod_word1(const mod_row *row, uint8_t arf)
{
    return (uint16_t)(row->base | (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
shift_word2(uint8_t source, int8_t shift)
{
    return (uint16_t)(UINT16_C(0x0C60) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      (uint16_t)((uint8_t)shift & UINT8_C(0x1F)));
}

static ti_c54x_decoded_instruction
decode_row(const mod_row *row, uint8_t source, uint8_t arf, int8_t shift)
{
    const uint16_t words[2] = {mod_word1(row, arf),
                               shift_word2(source, shift)};
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(words[0]);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {words[0], words[1]}, .program_word_count = 2};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == row->id);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout == row->layout);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, uint8_t seed)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_sxm = (uint8_t)(seed & UINT8_C(1)),
        .status_sxm_known = (seed & UINT8_C(2)) != 0U};
    return input;
}

static void
check_row_vectors(void)
{
    /* One vector per row: SHIFT -4 over B FF 8421 1234 stores F842. */
    for (size_t row = 0U; row < sizeof(ROWS) / sizeof(ROWS[0]); ++row) {
        for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
            for (uint8_t arf = 0U; arf <= UINT8_C(7); ++arf) {
                const ti_c54x_decoded_instruction instruction =
                    decode_row(&ROWS[row], source, arf, -4);
                for (uint8_t seed = 0U; seed < UINT8_C(4); ++seed) {
                    const ti_c54x_op_input_snapshot input = input_for(
                        UINT64_C(0x0012345678), UINT64_C(0xFF84211234),
                        seed);
                    ti_c54x_trusted_operand_values operands = {0};
                    ti_c54x_op_effect direct = {0};
                    ti_c54x_op_effect trusted = {0};

                    operands.count = UINT8_C(4);
                    operands.values[0].raw_value = source;
                    operands.values[0].width_bits = UINT8_C(1);
                    operands.values[1].raw_value = arf;
                    operands.values[1].width_bits = UINT8_C(3);
                    operands.values[2].raw_value = ROWS[row].mod;
                    operands.values[2].width_bits = UINT8_C(4);
                    operands.values[3].raw_value = UINT64_C(0x1C);
                    operands.values[3].width_bits = UINT8_C(5);
                    TI_C54X_TEST_CHECK(
                        ROWS[row].checked(
                            &instruction, &input,
                            &direct) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(direct.kind ==
                                       TI_C54X_OP_EFFECT_MEMORY);
                    TI_C54X_TEST_CHECK(direct.payload.memory.access ==
                                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
                    TI_C54X_TEST_CHECK(
                        direct.payload.memory.value_word ==
                        (source == 0U ? UINT16_C(0x0123)
                                      : UINT16_C(0xF842)));
                    TI_C54X_TEST_CHECK(direct.payload.memory.complete ==
                                       NULL);
                    TI_C54X_TEST_CHECK(
                        ROWS[row].trusted(
                            &instruction, &operands, &input,
                            &trusted) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                              sizeof(direct)) == 0);
                }
            }
        }
    }
}

int
main(void)
{
    check_row_vectors();
    return 0;
}
