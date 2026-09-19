/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the direct-only `PSHD Smem`,
 * `POPD Smem`, `PSHM MMR`, and `POPM MMR` pair effects, including
 * cookie extraction, Smem versus MMR cookie ranges (MMR forms
 * reject at and above 0x20), I-set rejection, checked/trusted/
 * dispatch parity, unknown-SP transactional rejection, and
 * hostile-input independence.
 * Does not own: catalog rows, operand-layout descriptors, SP
 * state, buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, or the production semantic leaves.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/stack/op_pshd_direct.h"
#include "../../../../../src/isa/ops/stack/op_popd_direct.h"
#include "../../../../../src/isa/ops/stack/op_pshm_direct.h"
#include "../../../../../src/isa/ops/stack/op_popm_direct.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_fn)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);
typedef ti_c54x_op_status (*trusted_fn)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

typedef struct pair_leaf_case {
    ti_c54x_opcode_id id;
    uint16_t base_word;
    ti_c54x_op_memory_address_kind arm_kind;
    ti_c54x_bus_word16_request_access access;
    checked_fn checked;
    trusted_fn trusted;
} pair_leaf_case;

static const pair_leaf_case LEAVES[] = {
    {TI_C54X_OPCODE_ID_PSHD_DIRECT, UINT16_C(0x4B00),
     TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
     TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, ti_c54x_op_pshd_direct,
     ti_c54x_trusted_op_pshd_direct},
    {TI_C54X_OPCODE_ID_POPD_DIRECT, UINT16_C(0x8B00),
     TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16,
     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, ti_c54x_op_popd_direct,
     ti_c54x_trusted_op_popd_direct},
    {TI_C54X_OPCODE_ID_PSHM_DIRECT, UINT16_C(0x4A00),
     TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
     TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, ti_c54x_op_pshm_direct,
     ti_c54x_trusted_op_pshm_direct},
    {TI_C54X_OPCODE_ID_POPM_DIRECT, UINT16_C(0x8A00),
     TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16,
     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, ti_c54x_op_popm_direct,
     ti_c54x_trusted_op_popm_direct},
};

static ti_c54x_decoded_instruction
decode_for(const pair_leaf_case *leaf, uint8_t suffix)
{
    const uint16_t first_word =
        (uint16_t)(leaf->base_word | suffix);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == leaf->id);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == leaf->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint16_t sp, bool sp_known, uint8_t seed)
{
    /* Unrelated accumulator, AR, and status state rides hostile
     * seeds; only SP is consumed. */
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    input.auxiliary_registers_raw16[3] = UINT16_C(0xAAAA);
    input.auxiliary_registers_known_mask = seed;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_sxm = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_sxm_known = (seed & UINT8_C(8)) != 0U;
    input.stack_pointer_raw16 = sp;
    input.stack_pointer_known = sp_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t indirect, uint8_t addr)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = indirect;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = addr;
    operands.values[1].width_bits = UINT8_C(7);
    return operands;
}

static void
assert_pair_effect(const pair_leaf_case *leaf, uint8_t cookie,
                   const ti_c54x_op_effect *effect)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       leaf->arm_kind);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.stack_pair
                           .indirect == UINT8_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.stack_pair
                           .smem_dma7 == cookie);
    TI_C54X_TEST_CHECK(effect->payload.memory.access == leaf->access);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation ==
                       (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_case(const pair_leaf_case *leaf, uint8_t cookie)
{
    const ti_c54x_decoded_instruction instruction =
        decode_for(leaf, cookie);
    const ti_c54x_op_input_snapshot clean =
        input_for(UINT16_C(0x1111), true, UINT8_C(0));
    const ti_c54x_op_input_snapshot hostile =
        input_for(UINT16_C(0x1111), true, UINT8_C(0x0F));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), cookie);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(leaf->id);
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect checked_hostile = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect via_dispatch = {0};
    ti_c54x_op_effect via_trusted = {0};

    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle,
                           &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(leaf->checked(&instruction, &clean,
                                     &checked) == TI_C54X_OP_STATUS_OK);
    assert_pair_effect(leaf, cookie, &checked);
    TI_C54X_TEST_CHECK(leaf->checked(&instruction, &hostile,
                                     &checked_hostile) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                              sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(leaf->trusted(&instruction, &operands, &clean,
                                     &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                              sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == leaf->trusted);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &clean,
                           &via_dispatch) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                              sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &clean,
                                                &via_trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                              sizeof(checked)) == 0);
}

static void
test_cases(void)
{
    /* Smem forms admit the full page-0 cookie range. */
    check_case(&LEAVES[0], UINT8_C(0x00));
    check_case(&LEAVES[0], UINT8_C(0x60));
    check_case(&LEAVES[0], UINT8_C(0x7F));
    check_case(&LEAVES[1], UINT8_C(0x00));
    check_case(&LEAVES[1], UINT8_C(0x60));
    check_case(&LEAVES[1], UINT8_C(0x7F));
    /* MMR forms admit register cookies only: BRC/AR5 edges plus
     * the file ends. */
    check_case(&LEAVES[2], UINT8_C(0x00));
    check_case(&LEAVES[2], UINT8_C(0x15));
    check_case(&LEAVES[2], UINT8_C(0x1A));
    check_case(&LEAVES[2], UINT8_C(0x1F));
    check_case(&LEAVES[3], UINT8_C(0x00));
    check_case(&LEAVES[3], UINT8_C(0x15));
    check_case(&LEAVES[3], UINT8_C(0x1A));
    check_case(&LEAVES[3], UINT8_C(0x1F));
}

static void
test_rejection(void)
{
    for (size_t li = 0U;
         li < sizeof(LEAVES) / sizeof(LEAVES[0]); ++li) {
        const pair_leaf_case *leaf = &LEAVES[li];
        const ti_c54x_decoded_instruction instruction =
            decode_for(leaf, UINT8_C(0x10));
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT16_C(0x1111), true, UINT8_C(0xA5));
        const ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(0), UINT8_C(0x10));
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(leaf->checked(NULL, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(leaf->checked(&instruction, NULL,
                                         &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(leaf->checked(&instruction, &valid,
                                         NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(leaf->trusted(&instruction, NULL, &valid,
                                         &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(leaf->trusted(&instruction, &operands, NULL,
                                         &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(leaf->trusted(&instruction, &operands, &valid,
                                         NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);

        /* Unknown SP rejects without an effect (CALA precedent). */
        {
            ti_c54x_op_input_snapshot unknown_sp = valid;

            unknown_sp.stack_pointer_known = false;
            TI_C54X_TEST_CHECK(leaf->checked(&instruction,
                                             &unknown_sp,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
            effect = before;
        }

        /* Forged identity rejects without an effect. */
        {
            ti_c54x_decoded_instruction forged = instruction;

            forged.id = TI_C54X_OPCODE_ID_NOP;
            TI_C54X_TEST_CHECK(leaf->checked(&forged, &valid,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
            effect = before;
        }

        /* The I-set half rejects in the leaf (indirect defers for
         * Smem forms; no indirect MMR form exists). */
        {
            const ti_c54x_decoded_instruction indirect =
                decode_for(leaf, UINT8_C(0x90));
            const ti_c54x_trusted_operand_values indirect_operands =
                trusted_operands(UINT8_C(1), UINT8_C(0x10));

            TI_C54X_TEST_CHECK(leaf->checked(&indirect, &valid,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
            effect = before;
            TI_C54X_TEST_CHECK(leaf->trusted(&instruction,
                                             &indirect_operands, &valid,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
            effect = before;
        }

        /* Malformed trusted operands reject. */
        {
            ti_c54x_trusted_operand_values empty = {0};
            ti_c54x_trusted_operand_values narrow = operands;

            narrow.values[1].width_bits = UINT8_C(6);
            TI_C54X_TEST_CHECK(leaf->trusted(&instruction, &empty,
                                             &valid,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(leaf->trusted(&instruction, &narrow,
                                             &valid,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
        }
    }

    /* MMR forms reject endpoint-RAM cookies on both paths. */
    for (size_t li = 2U;
         li < sizeof(LEAVES) / sizeof(LEAVES[0]); ++li) {
        const pair_leaf_case *leaf = &LEAVES[li];
        const ti_c54x_decoded_instruction instruction =
            decode_for(leaf, UINT8_C(0x20));
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT16_C(0x1111), true, UINT8_C(0xA5));
        const ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(0), UINT8_C(0x20));
        ti_c54x_op_effect effect = {0};

        TI_C54X_TEST_CHECK(leaf->checked(&instruction, &valid,
                                         &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(leaf->trusted(&instruction, &operands,
                                         &valid,
                                         &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
}

int
main(void)
{
    test_cases();
    test_rejection();
    return 0;
}
