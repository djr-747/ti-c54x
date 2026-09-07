#ifndef TI_C54X_ISA_DECODE_TRUSTED_EXECUTION_PACKET_H
#define TI_C54X_ISA_DECODE_TRUSTED_EXECUTION_PACKET_H

/*
 * Module contract
 *
 * Owns: the private value-owned trusted execution packet, its checked
 * constructor from a canonical immutable binding, and its fast constructor
 * from the already-selected binding used by the CPU loop. The packet copies
 * decoded facts, extracted operand values, and one process-lifetime code
 * pointer; it retains no data or caller-owned pointer.
 * Does not own: fetch, architectural inputs or effects, CPU state, sequencing,
 * buses, timing, diagnostics, allocation, or public ABI. Constructor inputs
 * are borrowed for one call and failed construction leaves output unchanged.
 */

#include "isa/ops/dispatch/trusted_opcode_binding.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_trusted_execution_packet_status {
    TI_C54X_TRUSTED_EXECUTION_PACKET_OK = 0,
    TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_ARGUMENT = 1,
    TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_BINDING = 2,
    TI_C54X_TRUSTED_EXECUTION_PACKET_UNKNOWN_OPCODE = 3,
    TI_C54X_TRUSTED_EXECUTION_PACKET_BUNDLE_COUNT_MISMATCH = 4,
    TI_C54X_TRUSTED_EXECUTION_PACKET_FIRST_WORD_MISMATCH = 5,
    TI_C54X_TRUSTED_EXECUTION_PACKET_OPERAND_PLAN_UNAVAILABLE = 6
} ti_c54x_trusted_execution_packet_status;

typedef struct ti_c54x_trusted_execution_packet {
    ti_c54x_decoded_instruction instruction;
    ti_c54x_trusted_operand_values operands;
    ti_c54x_trusted_op_handler handler;
} ti_c54x_trusted_execution_packet;

ti_c54x_trusted_execution_packet_status
ti_c54x_trusted_execution_packet_prepare(
    const ti_c54x_trusted_opcode_binding *binding,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_trusted_execution_packet *output);

/* Prepare from an immutable binding already selected by the CPU.  The
 * caller's binding and bundle are trusted constructor inputs: this seam does
 * not reselect the binding, perform canonical-identity checks, or resolve a
 * layout by ID. */
ti_c54x_trusted_execution_packet_status
ti_c54x_trusted_execution_packet_prepare_fast(
    const ti_c54x_trusted_opcode_binding *binding,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_trusted_execution_packet *output);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_DECODE_TRUSTED_EXECUTION_PACKET_H */
