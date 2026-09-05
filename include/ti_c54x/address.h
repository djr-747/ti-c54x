#ifndef TI_C54X_ADDRESS_H
#define TI_C54X_ADDRESS_H

/*
 * Module contract
 *
 * Owns: scalar word-address units and their carrier widths for the three
 * architectural address spaces.
 * Does not own: address mapping, memory, XPC or page semantics, timing, or
 * CPU state.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Program addresses retain a wide carrier while extended-page semantics are
 * deliberately outside this common foundation.  Data and I/O addresses are
 * 16-bit word-address carriers. */
typedef uint32_t ti_c54x_program_word_address;
typedef uint16_t ti_c54x_data_word_address;
typedef uint16_t ti_c54x_io_word_address;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ADDRESS_H */
