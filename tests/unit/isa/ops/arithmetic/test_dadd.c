/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure DADD Lmem effects, including
 * S/D selection, direct page-0 and indirect MOD0-MOD3 legs, the
 * C16-gated completion (40-bit OVM/C/OV add versus dual
 * independent 16-bit adds with sign-extended guard), SXM
 * sign-extension of the Lmem addend, trusted equivalence, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_dadd.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
dadd_word(uint8_t source, uint8_t destination, uint8_t indirect,
          uint8_t cookie)
{
    return (uint16_t)(UINT16_C(0x5000) |
                      ((uint16_t)(source & UINT8_C(1)) << 9U) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      ((uint16_t)(indirect & UINT8_C(1)) << 7U) |
                      (uint16_t)(cookie & UINT8_C(0x7F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t source, uint8_t destination, uint8_t indirect, uint8_t cookie)
{
    const uint16_t word = dadd_word(source, destination, indirect, cookie);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_DADD);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_C16);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known, uint8_t sxm,
          uint8_t ovm, uint8_t c16, bool status_known)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_sxm = sxm,
        .status_sxm_known = status_known,
        .status_ovm = ovm,
        .status_ovm_known = status_known,
        .status_c16 = c16,
        .status_c16_known = status_known};
    return input;
}

static void
assert_read(const ti_c54x_op_effect *effect, uint8_t indirect, uint8_t dma7,
            uint8_t mode, uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_single
                           .indirect == indirect);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_single
                           .dma7 == dma7);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_single
                           .mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_single
                           .arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_single
                           .dp_page == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_single
                           .reserved_word0 == UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete_long32 != NULL);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t source, uint8_t destination, uint8_t indirect,
                 uint8_t cookie)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = UINT8_C(4);
    operands.values[0].raw_value = source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = destination;
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = indirect;
    operands.values[2].width_bits = UINT8_C(1);
    operands.values[3].raw_value = cookie;
    operands.values[3].width_bits = UINT8_C(7);
    return operands;
}

typedef struct complete_vector {
    uint64_t src;
    uint16_t msw;
    uint16_t lsw;
    uint8_t sxm;
    uint8_t ovm;
    uint8_t c16;
    uint64_t expected;
    ti_c54x_op_status_bit_action c_action;
    ti_c54x_op_destination_overflow_action ov_action;
} complete_vector;

/* Hand-computed against TI SPRU172C DADD examples plus OVM,
 * sign-extension, and dual-mode corners. */
static const complete_vector VECTORS[] = {
    /* Ex1 C16=0: 0056788933 + 15343456 = 006BACBD89, no carry. */
    {UINT64_C(0x0056788933), UINT16_C(0x1534), UINT16_C(0x3456),
     UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT64_C(0x006BACBD89),
     TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* Ex3 C16=0: 0056783933 + 34561534 = 008ACE4E67, no carry,
     * positive overflow (result bit 31 set). */
    {UINT64_C(0x0056783933), UINT16_C(0x3456), UINT16_C(0x1534),
     UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT64_C(0x008ACE4E67),
     TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* OVM saturation: 0070000000 + 0010000000 overflows positive. */
    {UINT64_C(0x0070000000), UINT16_C(0x1000), UINT16_C(0x0000),
     UINT8_C(0), UINT8_C(1), UINT8_C(0), UINT64_C(0x007fffffff),
     TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* Same sum without OVM wraps. */
    {UINT64_C(0x0070000000), UINT16_C(0x1000), UINT16_C(0x0000),
     UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT64_C(0x0080000000),
     TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* Negative overflow with carry: FF80000000 + FF80000000. */
    {UINT64_C(0xFF80000000), UINT16_C(0x8000), UINT16_C(0x0000),
     UINT8_C(1), UINT8_C(1), UINT8_C(0), UINT64_C(0xFF80000000),
     TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* SXM sign-extends a negative Lmem addend. */
    {UINT64_C(0), UINT16_C(0x8000), UINT16_C(0x0001), UINT8_C(1),
     UINT8_C(0), UINT8_C(0), UINT64_C(0xFF80000001),
     TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* SXM clear zero-extends the same addend: the 40-bit sum
     * +2147483649 exceeds int32, so positive overflow raises OV
     * while the unsaturated value stands. */
    {UINT64_C(0), UINT16_C(0x8000), UINT16_C(0x0001), UINT8_C(0),
     UINT8_C(0), UINT8_C(0), UINT64_C(0x0080000001),
     TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* Ex2 C16=1: dual 5678+1534=6BAC, 3933+3456=6D89. */
    {UINT64_C(0x0056783933), UINT16_C(0x1534), UINT16_C(0x3456),
     UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT64_C(0x006BAC6D89),
     TI_C54X_OP_STATUS_BIT_PRESERVE,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* C16=1 negative high sum sign-fills the guard. */
    {UINT64_C(0), UINT16_C(0x8000), UINT16_C(0x8000), UINT8_C(0),
     UINT8_C(0), UINT8_C(1), UINT64_C(0xFF80008000),
     TI_C54X_OP_STATUS_BIT_PRESERVE,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* C16=1 ignores OVM on an overflowing 40-bit sum; the dual
     * high sum 0xFFFE has bit 15 set, so the guard sign-fills. */
    {UINT64_C(0x007FFF1234), UINT16_C(0x7FFF), UINT16_C(0x4321),
     UINT8_C(1), UINT8_C(1), UINT8_C(1), UINT64_C(0xFFFFFE5555),
     TI_C54X_OP_STATUS_BIT_PRESERVE,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
};

static void
check_vector(const ti_c54x_decoded_instruction *instruction,
             const ti_c54x_trusted_operand_values *operands,
             uint8_t source, uint8_t destination, size_t index)
{
    const complete_vector *vector = &VECTORS[index];
    const ti_c54x_op_input_snapshot input = input_for(
        source == UINT8_C(0) ? vector->src : UINT64_C(0x0AAAAAAAAA),
        source == UINT8_C(1) ? vector->src : UINT64_C(0x0555555555), true,
        true, vector->sxm, vector->ovm, vector->c16, true);
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect completed = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_dadd(instruction, &input, &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dadd(instruction, operands, &input,
                                              &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(direct.payload.memory.complete_long32(
                           instruction, &input, vector->msw, vector->lsw,
                           &completed) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(completed.kind ==
                       TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                       vector->expected);
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.status_delta
                           .c_action == vector->c_action);
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.status_delta
                           .tc_action == TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.status_delta
                           .destination_overflow_action ==
                       vector->ov_action);
}

static void
check_direct_legs(void)
{
    static const uint8_t DMAS[] = {0x00, 0x0A, 0x7F};

    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t destination = 0U; destination <= UINT8_C(1);
             ++destination) {
            for (size_t d = 0U; d < sizeof(DMAS); ++d) {
                const ti_c54x_decoded_instruction instruction =
                    decode(source, destination, UINT8_C(0), DMAS[d]);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(source, destination, UINT8_C(0),
                                     DMAS[d]);
                const ti_c54x_op_input_snapshot input = input_for(
                    UINT64_C(0x0056788933), UINT64_C(0xFF87654321), true,
                    true, UINT8_C(0), UINT8_C(0), UINT8_C(0), true);
                ti_c54x_op_effect effect = {0};

                TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&instruction, &input,
                                                  &effect) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_read(&effect, UINT8_C(0), DMAS[d], UINT8_C(0),
                            UINT8_C(0));
                for (size_t index = 0U;
                     index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
                    check_vector(&instruction, &operands, source,
                                 destination, index);
                }
            }
        }
    }
}

static void
check_indirect_legs(void)
{
    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t mode4 = 0U; mode4 <= UINT8_C(3); ++mode4) {
            for (uint8_t arf = 0U; arf <= UINT8_C(7); ++arf) {
                const uint8_t cookie =
                    (uint8_t)((mode4 << 3U) | arf);
                const ti_c54x_decoded_instruction instruction =
                    decode(source, (uint8_t)(source ^ UINT8_C(1)),
                           UINT8_C(1), cookie);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(source, (uint8_t)(source ^ UINT8_C(1)),
                                     UINT8_C(1), cookie);
                const ti_c54x_op_input_snapshot input = input_for(
                    UINT64_C(0x0056788933), UINT64_C(0xFF87654321), true,
                    true, UINT8_C(0), UINT8_C(0), UINT8_C(0), true);
                ti_c54x_op_effect effect = {0};

                TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&instruction, &input,
                                                  &effect) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_read(&effect, UINT8_C(1), UINT8_C(0), mode4, arf);
                check_vector(&instruction, &operands, source,
                             (uint8_t)(source ^ UINT8_C(1)), 0U);
                check_vector(&instruction, &operands, source,
                             (uint8_t)(source ^ UINT8_C(1)), 7U);
            }
        }
    }
}

static void
check_rejects(void)
{
    const ti_c54x_decoded_instruction valid =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(0), UINT8_C(0x0A));
    const ti_c54x_op_input_snapshot ready =
        input_for(UINT64_C(0x0056788933), UINT64_C(0xFF87654321), true,
                  true, UINT8_C(0), UINT8_C(0), UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = before;

    /* I-set cookie with mode4 > 3. */
    {
        const ti_c54x_decoded_instruction bad_cookie =
            decode(UINT8_C(0), UINT8_C(1), UINT8_C(1), UINT8_C(0x20));

        TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&bad_cookie, &ready, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Unknown C16 / SXM / accumulator. */
    {
        const ti_c54x_op_input_snapshot unknown_c16 =
            input_for(UINT64_C(0x0056788933), UINT64_C(0xFF87654321),
                      true, true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                      false);

        TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&valid, &unknown_c16, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot unknown_src =
            input_for(UINT64_C(0x0056788933), UINT64_C(0xFF87654321),
                      false, true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                      true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&valid, &unknown_src, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Malformed accumulator and status. */
    {
        const ti_c54x_op_input_snapshot malformed =
            input_for(TI_C54X_U40_MASK + UINT64_C(1),
                      UINT64_C(0xFF87654321), true, true, UINT8_C(0),
                      UINT8_C(0), UINT8_C(0), true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&valid, &malformed, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot bad_c16 =
            input_for(UINT64_C(0x0056788933), UINT64_C(0xFF87654321),
                      true, true, UINT8_C(0), UINT8_C(0), UINT8_C(2),
                      true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&valid, &bad_c16, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Forged word and null arguments. */
    forged.bundle.program_words[0] ^= UINT16_C(0x0400);
    TI_C54X_TEST_CHECK(ti_c54x_op_dadd(&forged, &ready, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_dadd(NULL, &ready, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_direct_legs();
    check_indirect_legs();
    check_rejects();
    return 0;
}
