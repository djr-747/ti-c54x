#ifndef TI_C54X_DATA_OPERAND_H
#define TI_C54X_DATA_OPERAND_H

/*
 * Module contract
 *
 * Owns: value-owned descriptors for the data-word addresses consumed by one
 * 16-bit or 32-bit data operand, including the architectural parity pairing
 * of a 32-bit operand.
 * Does not own: auxiliary-register state, address-mode arithmetic, memory
 * access, bus order, timing, opcode decoding, or diagnostics. Descriptors
 * are returned by value; callers own them and no descriptor borrows storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/address.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Zero is reserved so a zero-initialized failed effect cannot be valid. */
typedef enum ti_c54x_data_operand_width {
    TI_C54X_DATA_OPERAND_WIDTH_INVALID = 0,
    TI_C54X_DATA_OPERAND_WIDTH_WORD16 = 1,
    TI_C54X_DATA_OPERAND_WIDTH_LONG32 = 2
} ti_c54x_data_operand_width;

typedef struct ti_c54x_data_operand {
    ti_c54x_data_operand_width width;
    ti_c54x_data_word_address first_word;
    ti_c54x_data_word_address second_word;
} ti_c54x_data_operand;

/* Construct the canonical one-word shape. second_word is always zero. */
ti_c54x_data_operand ti_c54x_data_operand_word16(
    ti_c54x_data_word_address first_word);

/*
 * Construct the canonical two-word shape. An even first word pairs with the
 * next higher data word; an odd first word pairs with the previous lower word.
 */
ti_c54x_data_operand ti_c54x_data_operand_long32(
    ti_c54x_data_word_address first_word);

/*
 * Return true only for one of the two canonical, nonzero-width shapes. The
 * descriptor pointer is borrowed read-only until return; no pointer is kept.
 */
bool ti_c54x_data_operand_is_valid(const ti_c54x_data_operand *operand);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_DATA_OPERAND_H */
