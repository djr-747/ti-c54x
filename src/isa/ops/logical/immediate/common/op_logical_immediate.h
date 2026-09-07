/* Module contract
 * Owns: the shared pure semantics and metadata validation for the two-word
 * AND/OR/XOR immediate accumulator families. It selects one canonical source,
 * applies an unsigned zero-extended lk16 at the encoded or implicit shift,
 * and stages an accumulator-write effect.
 * Does not own: family wrappers, catalog definitions, generated lookup,
 * operand-layout descriptors, accumulator/status storage, fetch, sequencing,
 * timing, memory, buses, repeat/interrupt policy, observers, diagnostics,
 * environment, allocation, or public ABI.
 */

#ifndef TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_COMMON_H
#define TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_COMMON_H

#include "../../../op_effect.h"
#include "../../../op_input.h"
#include "../../../op_status.h"
#include "../../../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_logical_immediate_kind {
    TI_C54X_LOGICAL_IMMEDIATE_AND = 0,
    TI_C54X_LOGICAL_IMMEDIATE_OR = 1,
    TI_C54X_LOGICAL_IMMEDIATE_XOR = 2,
    TI_C54X_LOGICAL_IMMEDIATE_KIND_COUNT = 3
} ti_c54x_logical_immediate_kind;

/* Build one value-owned accumulator-write effect for a validated family row. */
ti_c54x_op_status ti_c54x_op_logical_immediate(
    ti_c54x_logical_immediate_kind kind,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

/* Apply one constructor-validated normalized operand set. */
ti_c54x_op_status ti_c54x_op_logical_immediate_values(
    ti_c54x_logical_immediate_kind kind,
    uint8_t source,
    uint8_t destination,
    uint8_t shift,
    uint16_t immediate,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_COMMON_H */
