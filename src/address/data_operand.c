/*
 * Module contract
 *
 * Owns: canonical value construction and validation for 16-bit and 32-bit
 * data operand address descriptors.
 * Does not own: auxiliary-register state, address-mode arithmetic, memory,
 * buses, timing, opcode decoding, or diagnostics.
 */

#include "data_operand.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_data_operand
ti_c54x_data_operand_word16(ti_c54x_data_word_address first_word)
{
    const ti_c54x_data_operand operand = {
        .width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
        .first_word = first_word,
        .second_word = UINT16_C(0)
    };
    return operand;
}

ti_c54x_data_operand
ti_c54x_data_operand_long32(ti_c54x_data_word_address first_word)
{
    ti_c54x_data_operand operand = {
        .width = TI_C54X_DATA_OPERAND_WIDTH_LONG32,
        .first_word = first_word,
        .second_word = UINT16_C(0)
    };

    if ((first_word & UINT16_C(1)) == UINT16_C(0)) {
        operand.second_word = (ti_c54x_data_word_address)(first_word +
                                                           UINT16_C(1));
    } else {
        operand.second_word = (ti_c54x_data_word_address)(first_word -
                                                           UINT16_C(1));
    }
    return operand;
}

bool
ti_c54x_data_operand_is_valid(const ti_c54x_data_operand *operand)
{
    if (operand == NULL) {
        return false;
    }
    if (operand->width == TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
        return operand->second_word == UINT16_C(0);
    }
    if (operand->width == TI_C54X_DATA_OPERAND_WIDTH_LONG32) {
        const ti_c54x_data_operand canonical =
            ti_c54x_data_operand_long32(operand->first_word);
        return operand->second_word == canonical.second_word;
    }
    return false;
}
