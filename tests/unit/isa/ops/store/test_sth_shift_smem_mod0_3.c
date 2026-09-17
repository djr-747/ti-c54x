/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure two-word STH-by-SHIFT MOD0-MOD3
 * standard-indirect effects: first-word lookup stays LD_SMEM_SHIFT while
 * completion re-resolves the STH row, source/ARF/MOD/SHIFT selection over
 * the four rows, signed-SHIFT high-word extraction, SXM riding hostile
 * and unconsumed, trusted equivalence through the re-resolved binding,
 * and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing, observers,
 * or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
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

extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod0(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod1(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod2(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod3(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod0(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod1(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod2(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod3(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

typedef struct mod_row {
    ti_c54x_opcode_id id;
    uint16_t base;
    uint8_t mod;
    checked_handler checked;
    trusted_handler trusted;
} mod_row;

static const mod_row ROWS[] = {
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD0, UINT16_C(0x6F80),
     UINT8_C(0), ti_c54x_op_sth_shift_smem_mod0,
     ti_c54x_trusted_op_sth_shift_smem_mod0},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD1, UINT16_C(0x6F88),
     UINT8_C(1), ti_c54x_op_sth_shift_smem_mod1,
     ti_c54x_trusted_op_sth_shift_smem_mod1},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD2, UINT16_C(0x6F90),
     UINT8_C(2), ti_c54x_op_sth_shift_smem_mod2,
     ti_c54x_trusted_op_sth_shift_smem_mod2},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD3, UINT16_C(0x6F98),
     UINT8_C(3), ti_c54x_op_sth_shift_smem_mod3,
     ti_c54x_trusted_op_sth_shift_smem_mod3},
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
    /* First-word lookup stays LD; completion re-resolves the STH row. */
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == row->id);
    TI_C54X_TEST_CHECK(
        instruction.operand_layout ==
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5);
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
check_effect(const ti_c54x_op_effect *effect, uint8_t mode, uint8_t arf,
             uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
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
                    const ti_c54x_trusted_opcode_binding *binding =
                        ti_c54x_trusted_opcode_binding_for_id(ROWS[row].id);
                    ti_c54x_trusted_execution_packet packet;
                    const ti_c54x_instruction_word_bundle bundle = {
                        .program_words =
                            {instruction.bundle.program_words[0],
                             instruction.bundle.program_words[1]},
                        .program_word_count = UINT8_C(2)};
                    ti_c54x_trusted_operand_values operands = {0};
                    ti_c54x_op_effect direct = {0};
                    ti_c54x_op_effect trusted = {0};
                    ti_c54x_op_effect packet_effect = {0};
                    ti_c54x_op_effect dispatched = {0};
                    const uint16_t expected =
                        (source == 0U ? UINT16_C(0x0123)
                                      : UINT16_C(0xF842));

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
                        ROWS[row].checked(&instruction, &input, &direct) ==
                        TI_C54X_OP_STATUS_OK);
                    check_effect(&direct, ROWS[row].mod, arf, expected);
                    TI_C54X_TEST_CHECK(
                        ROWS[row].trusted(&instruction, &operands, &input,
                                          &trusted) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                              sizeof(direct)) == 0);
                    /* The trusted packet path re-resolves the binding
                     * from the catalog instead of the first word. */
                    TI_C54X_TEST_CHECK(binding != NULL);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_trusted_execution_packet_prepare(
                            binding, &bundle, &packet) ==
                        TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
                    TI_C54X_TEST_CHECK(packet.handler ==
                                       ROWS[row].trusted);
                    TI_C54X_TEST_CHECK(
                        packet.handler(&packet.instruction,
                                       &packet.operands, &input,
                                       &packet_effect) ==
                        TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &packet_effect,
                                              sizeof(direct)) == 0);
                    /* Checked dispatch selects the same leaf. */
                    TI_C54X_TEST_CHECK(
                        ti_c54x_dispatch_decoded_with_input_snapshot(
                            &instruction, &input,
                            &dispatched) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched,
                                              sizeof(direct)) == 0);
                }
            }
        }
    }
}

static void
check_transactional_rejection(void)
{
    const ti_c54x_decoded_instruction sth =
        decode_row(&ROWS[1], UINT8_C(0), UINT8_C(0), -4);
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0x0012345678), UINT64_C(0xFF84211234),
                  UINT8_C(0));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect before;
    ti_c54x_op_effect effect;

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(3);
    operands.values[2].raw_value = UINT64_C(1);
    operands.values[2].width_bits = UINT8_C(4);
    operands.values[3].raw_value = UINT64_C(0x1C);
    operands.values[3].width_bits = UINT8_C(5);

    /* An LD bundle completed to LD must not execute as STH MOD1. */
    {
        const uint16_t words[2] = {mod_word1(&ROWS[1], UINT8_C(0)),
                                   UINT16_C(0x0C40)};
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(words[0]);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {words[0], words[1]},
            .program_word_count = UINT8_C(2)};
        ti_c54x_decoded_instruction ld;

        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &ld) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(ld.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
        (void)memset(&effect, 0xA5, sizeof(effect));
        (void)memcpy(&before, &effect, sizeof(before));
        TI_C54X_TEST_CHECK(ROWS[1].checked(&ld, &input, &effect) !=
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&before, &effect, sizeof(before)) == 0);
    }

    /* Trusted operands naming another mode must not execute. */
    (void)memset(&effect, 0xA5, sizeof(effect));
    (void)memcpy(&before, &effect, sizeof(before));
    operands.values[2].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ROWS[1].trusted(&sth, &operands, &input, &effect) !=
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&before, &effect, sizeof(before)) == 0);
}

int
main(void)
{
    check_row_vectors();
    check_transactional_rejection();
    return 0;
}
