/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure DSADT Lmem effects, including
 * D selection, direct page-0 and indirect MOD0-MOD3 legs, the
 * C16-gated completion (40-bit Lmem minus T:T composite with
 * SXM/OVM/C/OVdst versus dual Lmem-T high subtract with Lmem+T
 * low add, sign-filled guard, low-half C, and no saturation),
 * SXM widening of both 32-bit lanes, trusted equivalence, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_dsadt.h"
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
dsadt_word(uint8_t destination, uint8_t indirect, uint8_t cookie)
{
    return (uint16_t)(UINT16_C(0x5E00) | UINT16_C(0x0200) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      ((uint16_t)(indirect & UINT8_C(1)) << 7U) |
                      (uint16_t)(cookie & UINT8_C(0x7F)));
}

static ti_c54x_decoded_instruction
make_instruction(uint8_t destination, uint8_t indirect, uint8_t cookie)
{
    const uint16_t word = dsadt_word(destination, indirect, cookie);
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK((uint16_t)(word & UINT16_C(0xFE00)) ==
                       UINT16_C(0x5E00));
    memset(&instruction, 0, sizeof(instruction));
    instruction.id = TI_C54X_OPCODE_ID_UNKNOWN;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7;
    instruction.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
    instruction.input_resources =
        (ti_c54x_input_resource_mask)(
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_C16 |
            TI_C54X_INPUT_RESOURCES_T_REGISTER);
    instruction.bundle.program_words[0] = word;
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known, uint8_t sxm,
          uint8_t ovm, uint8_t c16, uint16_t t, bool status_known,
          bool t_known)
{
    ti_c54x_op_input_snapshot input;

    memset(&input, 0, sizeof(input));
    input.accumulator_a_raw40 = a;
    input.accumulator_b_raw40 = b;
    input.accumulator_a_known = a_known;
    input.accumulator_b_known = b_known;
    input.status_sxm = sxm;
    input.status_sxm_known = status_known;
    input.status_ovm = ovm;
    input.status_ovm_known = status_known;
    input.status_c16 = c16;
    input.status_c16_known = status_known;
    input.t_register_raw16 = t;
    input.t_register_known = t_known;
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
trusted_operands(uint8_t destination, uint8_t indirect, uint8_t cookie)
{
    ti_c54x_trusted_operand_values operands;

    memset(&operands, 0, sizeof(operands));
    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(1);
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
    uint64_t dst_init;
    uint16_t msw;
    uint16_t lsw;
    uint8_t sxm;
    uint8_t ovm;
    uint8_t c16;
    uint16_t t;
    uint64_t expected;
    ti_c54x_op_status_bit_action c_action;
    ti_c54x_op_destination_overflow_action ov_action;
} complete_vector;

/* Hand-computed against TI SPRU172C DSADT (p.4-45) worked examples
 * plus OVM, SXM, and dual-mode corners. */
static const complete_vector VECTORS[] = {
    /* TI Ex1 C16=0: 15343456-23452345=F1EF1111, borrow clears C. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x1534), UINT16_C(0x3456),
     UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT16_C(0x2345),
     UINT64_C(0xFFF1EF1111), TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* TI Ex2 C16=1: hi 1534-2345=F1EF, lo 3456+2345=579B. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x1534), UINT16_C(0x3456),
     UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT16_C(0x2345),
     UINT64_C(0xFFF1EF579B), TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* OVM positive saturation: FFFFFFFF-00000000 exceeds int32. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0xFFFF), UINT16_C(0xFFFF),
     UINT8_C(0), UINT8_C(1), UINT8_C(0), UINT16_C(0x0000),
     UINT64_C(0x007fffffff), TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* OVM negative saturation: 80000000-00000001 wraps positive. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x8000), UINT16_C(0x0000),
     UINT8_C(1), UINT8_C(1), UINT8_C(0), UINT16_C(0x0001),
     UINT64_C(0xFF80000000), TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1},
    /* C16=0 clean subtract, no borrow. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x5678), UINT16_C(0x0000),
     UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT16_C(0x0001),
     UINT64_C(0x005676FFFF), TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* C16=1 low add carry clears C: hi 0000-0001, lo FFFF+0001. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x0000), UINT16_C(0xFFFF),
     UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT16_C(0x0001),
     UINT64_C(0xFFFFFF0000), TI_C54X_OP_STATUS_BIT_WRITE_0,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* C16=1 clean low add sets C: hi 0000-FFFF=0001, lo 0000+FFFF. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x0000), UINT16_C(0x0000),
     UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT16_C(0xFFFF),
     UINT64_C(0x000001FFFF), TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
    /* SXM sign-extends a negative Lmem minuend. */
    {UINT64_C(0x0AAAAAAAAA), UINT16_C(0x8000), UINT16_C(0x0001),
     UINT8_C(1), UINT8_C(0), UINT8_C(0), UINT16_C(0x0000),
     UINT64_C(0xFF80000001), TI_C54X_OP_STATUS_BIT_WRITE_1,
     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE},
};

static void
check_vector(const ti_c54x_decoded_instruction *instruction,
             const ti_c54x_trusted_operand_values *operands,
             uint8_t destination, size_t index)
{
    const complete_vector *vector = &VECTORS[index];
    ti_c54x_op_input_snapshot input;
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect completed = {0};

    if (destination == UINT8_C(0)) {
        input = input_for(vector->dst_init, UINT64_C(0), true, false,
                          vector->sxm, vector->ovm, vector->c16,
                          vector->t, true, true);
    } else {
        input = input_for(UINT64_C(0), vector->dst_init, false, true,
                          vector->sxm, vector->ovm, vector->c16,
                          vector->t, true, true);
    }
    TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(instruction, &input, &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dsadt(instruction, operands,
                                               &input, &trusted) ==
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

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (size_t d = 0U; d < sizeof(DMAS); ++d) {
            const ti_c54x_decoded_instruction instruction =
                make_instruction(destination, UINT8_C(0), DMAS[d]);
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(destination, UINT8_C(0), DMAS[d]);
            const ti_c54x_op_input_snapshot input = input_for(
                UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555), true,
                true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                UINT16_C(0x2345), true, true);
            ti_c54x_op_effect effect = {0};

            TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&instruction, &input,
                                               &effect) ==
                               TI_C54X_OP_STATUS_OK);
            assert_read(&effect, UINT8_C(0), DMAS[d], UINT8_C(0),
                        UINT8_C(0));
            for (size_t index = 0U;
                 index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
                check_vector(&instruction, &operands, destination,
                             index);
            }
        }
    }
}

static void
check_indirect_legs(void)
{
    for (uint8_t mode4 = 0U; mode4 <= UINT8_C(3); ++mode4) {
        for (uint8_t arf = 0U; arf <= UINT8_C(7); ++arf) {
            const uint8_t cookie = (uint8_t)((mode4 << 3U) | arf);
            const ti_c54x_decoded_instruction instruction =
                make_instruction(UINT8_C(0), UINT8_C(1), cookie);
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(UINT8_C(0), UINT8_C(1), cookie);
            const ti_c54x_op_input_snapshot input = input_for(
                UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555), true,
                true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                UINT16_C(0x2345), true, true);
            ti_c54x_op_effect effect = {0};

            TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&instruction, &input,
                                               &effect) ==
                               TI_C54X_OP_STATUS_OK);
            assert_read(&effect, UINT8_C(1), UINT8_C(0), mode4, arf);
            check_vector(&instruction, &operands, UINT8_C(0), 0U);
            check_vector(&instruction, &operands, UINT8_C(0), 1U);
        }
    }
}

static void
check_rejects(void)
{
    const ti_c54x_decoded_instruction valid =
        make_instruction(UINT8_C(1), UINT8_C(0), UINT8_C(0x0A));
    const ti_c54x_trusted_operand_values valid_operands =
        trusted_operands(UINT8_C(1), UINT8_C(0), UINT8_C(0x0A));
    const ti_c54x_op_input_snapshot ready =
        input_for(UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555), true,
                  true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                  UINT16_C(0x2345), true, true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = before;

    /* I-set cookie with mode4 > 3. */
    {
        const ti_c54x_decoded_instruction bad_cookie =
            make_instruction(UINT8_C(1), UINT8_C(1), UINT8_C(0x38));

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&bad_cookie, &ready,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Unknown status / accumulator / T. */
    {
        const ti_c54x_op_input_snapshot unknown_status =
            input_for(UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555),
                      true, true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                      UINT16_C(0x2345), false, true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &unknown_status,
                                           &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot unknown_dst =
            input_for(UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555),
                      true, false, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                      UINT16_C(0x2345), true, true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &unknown_dst,
                                           &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot unknown_t =
            input_for(UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555),
                      true, true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                      UINT16_C(0x2345), true, false);

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &unknown_t,
                                           &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Malformed accumulator and status. */
    {
        const ti_c54x_op_input_snapshot malformed =
            input_for(UINT64_C(0x0AAAAAAAAA), TI_C54X_U40_MASK + UINT64_C(1),
                      true, true, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                      UINT16_C(0x2345), true, true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &malformed,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot bad_ovm =
            input_for(UINT64_C(0x0AAAAAAAAA), UINT64_C(0x0555555555),
                      true, true, UINT8_C(0), UINT8_C(3), UINT8_C(0),
                      UINT16_C(0x2345), true, true);

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &bad_ovm, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Forged words: flipped fixed bit, flipped base bit, and every
     * neighbor family first word. */
    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&forged, &ready, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = valid;
    forged.bundle.program_words[0] ^= UINT16_C(0x0800);
    TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&forged, &ready, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    {
        static const uint16_t NEIGHBORS[] = {UINT16_C(0x5000),
                                             UINT16_C(0x5400),
                                             UINT16_C(0x5800),
                                             UINT16_C(0x5A00),
                                             UINT16_C(0x5C00)};
        size_t n;

        for (n = 0U; n < sizeof(NEIGHBORS) / sizeof(NEIGHBORS[0]);
             ++n) {
            ti_c54x_decoded_instruction neighbor = valid;

            neighbor.bundle.program_words[0] =
                (uint16_t)(NEIGHBORS[n] | UINT16_C(0x010A));
            TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&neighbor, &ready,
                                               &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
        }
    }
    /* Wrong layout, resources, repeatability, and word count. */
    {
        ti_c54x_decoded_instruction wrong = valid;

        wrong.operand_layout =
            TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7;
        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&wrong, &ready, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction wrong = valid;

        wrong.input_resources =
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_C16;
        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&wrong, &ready, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction wrong = valid;

        wrong.repeatability = TI_C54X_REPEATABILITY_ELIGIBLE;
        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&wrong, &ready, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction wrong = valid;

        wrong.bundle.program_word_count = UINT8_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&wrong, &ready, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    /* Completion revalidates decode and inputs transactionally. */
    {
        ti_c54x_op_effect staged = {0};
        ti_c54x_op_effect next = before;
        ti_c54x_decoded_instruction bad = valid;

        TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &ready, &staged) ==
                           TI_C54X_OP_STATUS_OK);
        bad.bundle.program_words[0] ^= UINT16_C(0x0200);
        TI_C54X_TEST_CHECK(staged.payload.memory.complete_long32(
                               &bad, &ready, UINT16_C(0x1534),
                               UINT16_C(0x3456),
                               &next) == TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof(next)) == 0);
        {
            const ti_c54x_op_input_snapshot unknown_dst =
                input_for(UINT64_C(0x0AAAAAAAAA),
                          UINT64_C(0x0555555555), true, false,
                          UINT8_C(0), UINT8_C(0), UINT8_C(0),
                          UINT16_C(0x2345), true, true);

            TI_C54X_TEST_CHECK(staged.payload.memory.complete_long32(
                                   &valid, &unknown_dst, UINT16_C(0x1534),
                                   UINT16_C(0x3456),
                                   &next) ==
                               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
            TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof(next)) ==
                               0);
        }
        TI_C54X_TEST_CHECK(staged.payload.memory.complete_long32(
                               &valid, &ready, UINT16_C(0x1534),
                               UINT16_C(0x3456),
                               NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof(next)) == 0);
    }
    /* Null arguments. */
    TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(NULL, &ready, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_dsadt(&valid, &ready, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    /* Trusted rejects. */
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dsadt(&valid, &valid_operands,
                                               NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    {
        ti_c54x_trusted_operand_values bad_count = valid_operands;

        bad_count.count = UINT8_C(5);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dsadt(
                               &valid, &bad_count, &ready,
                               &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_trusted_operand_values bad_fixed = valid_operands;

        bad_fixed.values[0].raw_value = UINT64_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dsadt(
                               &valid, &bad_fixed, &ready,
                               &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_trusted_operand_values bad_width = valid_operands;

        bad_width.values[1].width_bits = UINT8_C(7);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dsadt(
                               &valid, &bad_width, &ready,
                               &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int
main(void)
{
    check_direct_legs();
    check_indirect_legs();
    check_rejects();
    return 0;
}
