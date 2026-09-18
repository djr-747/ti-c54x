/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the OVERWRITE and WRITEBACK RMW
 * transforms: OVERWRITE writes the staged value word and ignores the
 * read word; WRITEBACK rewrites the read word and ignores the staged
 * value. Rejects the NONE carrier and NULL arguments.
 * Does not own: the carrier, flag staging, the sequencer commit that
 * consumes the write word, or opcode identity.
 */

#include "../../../../src/sequencer/instruction/memory_instruction_rmw.h"
#include "../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

static ti_c54x_op_memory_effect
make_effect(ti_c54x_op_memory_rmw_operation operation,
            uint16_t staged_word)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.address_spec.arf = UINT8_C(2);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.value_word = staged_word;
    effect.rmw_operation = (uint8_t)operation;
    effect.complete = NULL;
    return effect;
}

static void
check_overwrite_ignores_read_word(void)
{
    static const uint16_t read_words[] = {
        UINT16_C(0x0000), UINT16_C(0xFFFF), UINT16_C(0x8007)};
    size_t index;

    for (index = 0U;
         index < sizeof(read_words) / sizeof(read_words[0]); ++index) {
        const ti_c54x_op_memory_effect effect =
            make_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
                        UINT16_C(0x123B));
        uint16_t write_word = UINT16_C(0xBEEF);

        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_rmw_transform(
            &effect, read_words[index], &write_word));
        TI_C54X_TEST_CHECK(write_word == UINT16_C(0x123B));
    }
}

static void
check_writeback_passes_read_word(void)
{
    static const uint16_t read_words[] = {
        UINT16_C(0x0000), UINT16_C(0xFFFF), UINT16_C(0x8007)};
    size_t index;

    for (index = 0U;
         index < sizeof(read_words) / sizeof(read_words[0]); ++index) {
        const ti_c54x_op_memory_effect effect =
            make_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK,
                        UINT16_C(0x123B));
        uint16_t write_word = UINT16_C(0xBEEF);

        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_rmw_transform(
            &effect, read_words[index], &write_word));
        TI_C54X_TEST_CHECK(write_word == read_words[index]);
    }
}

static void
check_rejects_none_carrier(void)
{
    const ti_c54x_op_memory_rmw_operation operations[] = {
        TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
        TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK};
    size_t index;
    uint16_t write_word;

    for (index = 0U;
         index < sizeof(operations) / sizeof(operations[0]); ++index) {
        ti_c54x_op_memory_effect none_effect =
            make_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
                        UINT16_C(0x123B));

        (void)operations[index];
        write_word = UINT16_C(0xBEEF);
        TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform(
            &none_effect, UINT16_C(0x0002), &write_word));
        TI_C54X_TEST_CHECK(write_word == UINT16_C(0xBEEF));
    }
}

static void
check_rejects_null_arguments(void)
{
    const ti_c54x_op_memory_effect effect =
        make_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
                    UINT16_C(0x0001));
    uint16_t write_word;

    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform(
        NULL, UINT16_C(0x0002), &write_word));
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform(
        &effect, UINT16_C(0x0002), NULL));
}

int
main(void)
{
    check_overwrite_ignores_read_word();
    check_writeback_passes_read_word();
    check_rejects_none_carrier();
    check_rejects_null_arguments();
    return 0;
}
