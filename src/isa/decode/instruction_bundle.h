#ifndef TI_C54X_ISA_INSTRUCTION_BUNDLE_H
#define TI_C54X_ISA_INSTRUCTION_BUNDLE_H

/*
 * Module contract
 *
 * Owns: the decode-owned, value-only carrier for one copied complete
 * instruction. The two uint16_t program words and count are copied values;
 * this header retains no caller pointer and performs no fetch or validation.
 * Does not own: bus or memory I/O, CPU or architectural state, program-counter
 * sequencing, catalog lookup, operand interpretation, opcode effects, timing,
 * observers, diagnostics, or allocation.
 *
 * TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY is the current helper capacity,
 * not an ISA maximum. A bundle's program_words array always has exactly this
 * many copied slots, while program_word_count identifies the valid prefix.
 * Three slots carry the longest current instructions (first word plus two
 * extension words, e.g. absolute immediate stores).
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY = 3
};

typedef struct ti_c54x_instruction_word_bundle {
    uint16_t program_words[TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY];
    uint8_t program_word_count;
} ti_c54x_instruction_word_bundle;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_INSTRUCTION_BUNDLE_H */
