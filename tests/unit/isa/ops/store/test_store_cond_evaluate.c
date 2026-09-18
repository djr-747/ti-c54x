/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the store-owned 4-bit
 * accumulator-condition evaluator: TI SPRU172C cond codes
 * (AGEQ=0010, ALT=0011, ANEQ=0100, AEQ=0101, AGT=0110,
 * ALEQ=0111 over accumulator A; BGEQ=1010, BLT=1011,
 * BNEQ=1100, BEQ=1101, BGT=1110, BLEQ=1111 over accumulator
 * B) as full-40-bit two's-complement signed compares against
 * zero with guard bits included, consumed-input-only
 * knownness, and transactional rejection of the four
 * unassigned nibbles (0000, 0001, 1000, 1001).
 * Does not own: the evaluator, catalog rows, operand
 * extraction, effect staging, or status register storage.
 */

#include "../../../../../src/isa/ops/store/op_store_cond_common.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define COND_AGEQ UINT8_C(0x2)
#define COND_ALT UINT8_C(0x3)
#define COND_ANEQ UINT8_C(0x4)
#define COND_AEQ UINT8_C(0x5)
#define COND_AGT UINT8_C(0x6)
#define COND_ALEQ UINT8_C(0x7)
#define COND_BGEQ UINT8_C(0xA)
#define COND_BLT UINT8_C(0xB)
#define COND_BNEQ UINT8_C(0xC)
#define COND_BEQ UINT8_C(0xD)
#define COND_BGT UINT8_C(0xE)
#define COND_BLEQ UINT8_C(0xF)

typedef struct cond_vector {
    uint8_t cond4;
    uint64_t acc_raw40;
    bool expected_taken;
} cond_vector;

static ti_c54x_op_input_snapshot
make_input(uint64_t acc_raw40, bool acc_known, bool use_b)
{
    ti_c54x_op_input_snapshot input = {0};

    if (use_b) {
        input.accumulator_b_raw40 = acc_raw40;
        input.accumulator_b_known = acc_known;
        input.accumulator_a_raw40 = UINT64_C(0);
        input.accumulator_a_known = true;
    } else {
        input.accumulator_a_raw40 = acc_raw40;
        input.accumulator_a_known = acc_known;
        input.accumulator_b_raw40 = UINT64_C(0);
        input.accumulator_b_known = true;
    }
    return input;
}

/* Zero accumulator: EQ/GEQ/LEQ take, NEQ/GT/LT do not. */
static const cond_vector zero_a_vectors[] = {
    {COND_AEQ, UINT64_C(0x0000000000), true},
    {COND_ANEQ, UINT64_C(0x0000000000), false},
    {COND_AGT, UINT64_C(0x0000000000), false},
    {COND_AGEQ, UINT64_C(0x0000000000), true},
    {COND_ALT, UINT64_C(0x0000000000), false},
    {COND_ALEQ, UINT64_C(0x0000000000), true},
};

/* TI SRCCD worked-example shape: A = 00_70FF_FFFF is positive
 * nonzero, so AGT takes. */
static const cond_vector positive_a_vectors[] = {
    {COND_AEQ, UINT64_C(0x0070FFFFFF), false},
    {COND_ANEQ, UINT64_C(0x0070FFFFFF), true},
    {COND_AGT, UINT64_C(0x0070FFFFFF), true},
    {COND_AGEQ, UINT64_C(0x0070FFFFFF), true},
    {COND_ALT, UINT64_C(0x0070FFFFFF), false},
    {COND_ALEQ, UINT64_C(0x0070FFFFFF), false},
};

/* Bit 39 set is negative: LT/LEQ/NEQ take, EQ/GT/GEQ do not. */
static const cond_vector negative_a_vectors[] = {
    {COND_AEQ, UINT64_C(0xFF80000000), false},
    {COND_ANEQ, UINT64_C(0xFF80000000), true},
    {COND_AGT, UINT64_C(0xFF80000000), false},
    {COND_AGEQ, UINT64_C(0xFF80000000), false},
    {COND_ALT, UINT64_C(0xFF80000000), true},
    {COND_ALEQ, UINT64_C(0xFF80000000), true},
};

/* Guard bits participate: a lone guard bit is nonzero positive. */
static const cond_vector guard_a_vectors[] = {
    {COND_AEQ, UINT64_C(0x0100000000), false},
    {COND_ANEQ, UINT64_C(0x0100000000), true},
    {COND_AGT, UINT64_C(0x0100000000), true},
    {COND_ALT, UINT64_C(0x0100000000), false},
};

/* B codes read accumulator B, not A. */
static const cond_vector b_vectors[] = {
    {COND_BEQ, UINT64_C(0x0000000000), true},
    {COND_BNEQ, UINT64_C(0x0000000000), false},
    {COND_BGT, UINT64_C(0x0070FFFFFF), true},
    {COND_BGEQ, UINT64_C(0xFF80000000), false},
    {COND_BLT, UINT64_C(0xFF80000000), true},
    {COND_BLEQ, UINT64_C(0x0000000000), true},
};

static void
check_vectors(const cond_vector *vectors, size_t count, bool use_b)
{
    size_t index;

    for (index = 0U; index < count; ++index) {
        const ti_c54x_op_input_snapshot input = make_input(
            vectors[index].acc_raw40, true, use_b);
        bool taken = !vectors[index].expected_taken;

        TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(
                               vectors[index].cond4, &input, &taken) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(taken == vectors[index].expected_taken);
    }
}

static void
check_unknown_accumulator_rejects(void)
{
    const ti_c54x_op_input_snapshot unknown_a =
        make_input(UINT64_C(0x0070FFFFFF), false, false);
    const ti_c54x_op_input_snapshot unknown_b =
        make_input(UINT64_C(0x0070FFFFFF), false, true);
    bool taken = false;

    TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(COND_AGT, &unknown_a,
                                                  &taken) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(COND_BEQ, &unknown_b,
                                                  &taken) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
check_unconsumed_unknown_accumulator_ignored(void)
{
    /* Consumed-input-only knownness: an A code ignores unknown B
     * and vice versa. */
    ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0000000000), true, false);
    bool taken = false;

    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(COND_AEQ, &input,
                                                  &taken) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(taken);
}

static void
check_unassigned_nibbles_reject(void)
{
    static const uint8_t unassigned[] = {UINT8_C(0x0), UINT8_C(0x1),
                                         UINT8_C(0x8), UINT8_C(0x9)};
    size_t index;

    for (index = 0U;
         index < sizeof(unassigned) / sizeof(unassigned[0]); ++index) {
        const ti_c54x_op_input_snapshot input =
            make_input(UINT64_C(0x0000000000), true, false);
        bool taken = false;

        TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(
                               unassigned[index], &input, &taken) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
}

static void
check_malformed_arguments_reject(void)
{
    const ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0000000000), true, false);
    ti_c54x_op_input_snapshot wide_input = input;
    bool taken = false;

    TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(COND_AEQ, NULL,
                                                  &taken) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(COND_AEQ, &input,
                                                  NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    wide_input.accumulator_a_raw40 = UINT64_C(0x10000000000);
    TI_C54X_TEST_CHECK(ti_c54x_store_cond_evaluate(COND_AEQ, &wide_input,
                                                  &taken) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

int
main(void)
{
    check_vectors(zero_a_vectors,
                  sizeof(zero_a_vectors) / sizeof(zero_a_vectors[0]),
                  false);
    check_vectors(positive_a_vectors,
                  sizeof(positive_a_vectors) /
                      sizeof(positive_a_vectors[0]),
                  false);
    check_vectors(negative_a_vectors,
                  sizeof(negative_a_vectors) /
                      sizeof(negative_a_vectors[0]),
                  false);
    check_vectors(guard_a_vectors,
                  sizeof(guard_a_vectors) / sizeof(guard_a_vectors[0]),
                  false);
    check_vectors(b_vectors,
                  sizeof(b_vectors) / sizeof(b_vectors[0]), true);
    check_unknown_accumulator_rejects();
    check_unconsumed_unknown_accumulator_ignored();
    check_malformed_arguments_reject();
    check_unassigned_nibbles_reject();
    return 0;
}
