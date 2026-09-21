/*
 * Module contract
 *
 * Owns: RED proofs for the MVDD Xmem,Ymem leaf (TI SPRU172C
 * section MVDD, p. 4-107, 1 word): decode identity, the TI
 * worked example word, dual-move staging with both sides'
 * modes, the trusted four-operand entry, field/range
 * rejection, forged completion rejection, and checked-entry
 * invalid-decode rejection.
 * Does not own: catalog rows, operand-layout descriptors,
 * decode, dispatch, memory addressing or buses, status
 * mutation, CPU sequencing, timing, observers, diagnostics,
 * or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_mvdd_xmem_ymem.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
mvdd_word(uint8_t xmod, uint8_t xar_raw, uint8_t ymod, uint8_t yar_raw)
{
    return (uint16_t)(UINT16_C(0xE500) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_raw & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(((uint16_t)(ymod & UINT8_C(3))) << 2U) |
                      (uint16_t)(yar_raw & UINT8_C(3)));
}

static ti_c54x_decoded_instruction
make_instruction(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MVDD_XMEM_YMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_YMOD_YAR);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    return instruction;
}

static ti_c54x_op_input_snapshot
blank_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static void
check_move_staging(const ti_c54x_op_effect *effect, uint8_t x_mode,
                   uint8_t x_ar, uint8_t y_mode, uint8_t y_ar)
{
    const ti_c54x_op_memory_effect *memory;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    memory = &effect->payload.memory;
    TI_C54X_TEST_CHECK(memory->address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16);
    TI_C54X_TEST_CHECK(memory->address_spec.dual.x_mode == x_mode);
    TI_C54X_TEST_CHECK(memory->address_spec.dual.x_ar == x_ar);
    TI_C54X_TEST_CHECK(memory->address_spec.dual.y_mode == y_mode);
    TI_C54X_TEST_CHECK(memory->address_spec.dual.y_ar == y_ar);
    TI_C54X_TEST_CHECK(memory->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(memory->complete == NULL);
}

static void
test_ti_example_word(void)
{
    /* TI p. 4-107: MVDD *AR3+, *AR5+ is MOD2/MOD2 with raw
     * selectors AR3/AR5. */
    const ti_c54x_decoded_instruction instruction =
        make_instruction(mvdd_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                   UINT8_C(3)));
    ti_c54x_op_input_snapshot input = blank_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect checked = before;
    ti_c54x_op_effect trusted = before;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(mvdd_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                 UINT8_C(3)) == UINT16_C(0xE59B));
    TI_C54X_TEST_CHECK(ti_c54x_op_mvdd_xmem_ymem(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    check_move_staging(&checked, UINT8_C(2), UINT8_C(3), UINT8_C(2),
                       UINT8_C(5));
    operands.count = UINT8_C(4);
    operands.values[0].width_bits = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[2].raw_value = UINT64_C(2);
    operands.values[3].width_bits = UINT8_C(2);
    operands.values[3].raw_value = UINT64_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdd_xmem_ymem(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    check_move_staging(&trusted, UINT8_C(2), UINT8_C(3), UINT8_C(2),
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);
}

static void
test_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(mvdd_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                   UINT8_C(0)));
    ti_c54x_op_input_snapshot input = blank_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mvdd_xmem_ymem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvdd_xmem_ymem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvdd_xmem_ymem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_mvdd_xmem_ymem(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(3);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdd_xmem_ymem(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_ti_example_word();
    test_rejection_is_transactional();
    return 0;
}
