/* Module contract
 * Owns: pure PORTR I/O-pair memory-write effects; see op_portr.h.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_portr.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

/* Direct-DP pages are 9 bits; the resolver rejects wider pages. */
#define PORTR_DP_MAX UINT16_C(0x01FF)

/* Indirect Smem MOD allowlist mirrors the seam synthesizer: MOD0
 * through MOD8 plus MOD10; MOD9 is carrier-invalid. */
static bool
portr_indirect_mode_is_valid(uint8_t mode)
{
    return mode <= UINT8_C(8) || mode == UINT8_C(10);
}

static ti_c54x_op_status
portr_values(uint8_t indirect, uint8_t addr, uint16_t port_address,
             const ti_c54x_op_input_snapshot *input,
             ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (indirect == UINT8_C(0)) {
        if (addr > UINT8_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        if (!input->status_dp_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        if (input->status_dp > PORTR_DP_MAX) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged = (ti_c54x_op_effect){
            .kind = TI_C54X_OP_EFFECT_MEMORY,
            .payload.memory = {
                .address_spec = {
                    .kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
                    .port = {.indirect = UINT8_C(0),
                             .smem_dma7 = addr,
                             .port_address = port_address}},
                .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                .value_word = UINT16_C(0),
                .complete = NULL}};
        *effect = staged;
        return TI_C54X_OP_STATUS_OK;
    }
    if (indirect != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    {
        const uint8_t arf = (uint8_t)(addr & UINT8_C(0x07));
        const uint8_t mode = (uint8_t)((uint8_t)(addr >> 3U) & UINT8_C(0x0F));

        if (!portr_indirect_mode_is_valid(mode)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged = (ti_c54x_op_effect){
            .kind = TI_C54X_OP_EFFECT_MEMORY,
            .payload.memory = {
                .address_spec = {
                    .kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
                    .port = {.indirect = UINT8_C(1),
                             .smem_mode = mode,
                             .smem_arf = arf,
                             .port_address = port_address}},
                .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                .value_word = UINT16_C(0),
                .complete = NULL}};
        *effect = staged;
        return TI_C54X_OP_STATUS_OK;
    }
}

static ti_c54x_op_status
portr_checked(const ti_c54x_decoded_instruction *instruction,
              const ti_c54x_op_input_snapshot *input,
              ti_c54x_opcode_id expected_id,
              ti_c54x_input_resource_mask expected_resources,
              uint8_t expected_indirect, ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value addr_field = {0U, 0U};
    ti_c54x_operand_field_value pa_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_PA16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &addr_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &pa_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value != expected_indirect ||
            addr_field.width_bits != UINT8_C(7) ||
            addr_field.raw_value > UINT64_C(0x7F) ||
            pa_field.width_bits != UINT8_C(16)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    return portr_values((uint8_t)indirect_field.raw_value,
                        (uint8_t)addr_field.raw_value,
                        (uint16_t)pa_field.raw_value, input, effect);
}

ti_c54x_op_status
ti_c54x_op_portr_direct(const ti_c54x_decoded_instruction *instruction,
                        const ti_c54x_op_input_snapshot *input,
                        ti_c54x_op_effect *effect)
{
    return portr_checked(instruction, input, TI_C54X_OPCODE_ID_PORTR_DIRECT,
                         TI_C54X_INPUT_RESOURCES_STATUS_DP, UINT8_C(0),
                         effect);
}

ti_c54x_op_status
ti_c54x_op_portr_smem(const ti_c54x_decoded_instruction *instruction,
                       const ti_c54x_op_input_snapshot *input,
                       ti_c54x_op_effect *effect)
{
    return portr_checked(instruction, input, TI_C54X_OPCODE_ID_PORTR_SMEM,
                         TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS,
                         UINT8_C(1), effect);
}

static ti_c54x_op_status
portr_trusted_checked(const ti_c54x_trusted_operand_values *operands,
                      const ti_c54x_op_input_snapshot *input,
                      uint8_t expected_indirect, ti_c54x_op_effect *effect)
{
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value != expected_indirect ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F) ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[2].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return portr_values((uint8_t)operands->values[0].raw_value,
                        (uint8_t)operands->values[1].raw_value,
                        (uint16_t)operands->values[2].raw_value, input,
                        effect);
}

ti_c54x_op_status ti_c54x_trusted_op_portr_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return portr_trusted_checked(operands, input, UINT8_C(0), effect);
}

ti_c54x_op_status ti_c54x_trusted_op_portr_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return portr_trusted_checked(operands, input, UINT8_C(1), effect);
}

/* Absolute Smem entry (P3-S87): only the 0x78 absolute selector
 * composes. The leaf stages the PORT WORD16 WRITE arm with the
 * absolute marker (indirect=1, mode=15, arf=0, dma7=0) and the DATA
 * address in reserved_half; the sequencer synthesizes the absolute
 * DATA WRITE leg from the marker. */
#define PORTR_ABSOLUTE_SELECTOR UINT8_C(0x78)
#define PORTR_ABSOLUTE_MODE UINT8_C(15)

static ti_c54x_op_status
portr_absolute_values(uint16_t absolute, uint16_t port_address,
                      ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16;
    staged.payload.memory.address_spec.port.indirect = UINT8_C(1);
    staged.payload.memory.address_spec.port.smem_mode =
        PORTR_ABSOLUTE_MODE;
    staged.payload.memory.address_spec.port.smem_arf = UINT8_C(0);
    staged.payload.memory.address_spec.port.smem_dma7 = UINT8_C(0);
    staged.payload.memory.address_spec.port.port_address = port_address;
    staged.payload.memory.address_spec.port.reserved_half = absolute;
    staged.payload.memory.address_spec.port.reserved_word0 = UINT32_C(0);
    staged.payload.memory.address_spec.port.reserved_word1 = UINT32_C(0);
    staged.payload.memory.address_spec.port.reserved_word2 = UINT32_C(0);
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = NULL;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
portr_absolute_extract(const ti_c54x_decoded_instruction *instruction,
                       uint16_t *absolute, uint16_t *port_address)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};
    ti_c54x_operand_field_value pa_field = {0U, 0U};

    if (instruction == NULL || absolute == NULL || port_address == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_PORTR_SMEM_ABSOLUTE ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_ABSOLUTE16_PA16 ||
        instruction->bundle.program_word_count != UINT8_C(3) ||
        instruction->bundle.program_words[0] != UINT16_C(0x74F8)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_ABSOLUTE16_PA16,
            UINT8_C(3), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value != UINT64_C(1) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &selector_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            selector_field.width_bits != UINT8_C(7) ||
            selector_field.raw_value != PORTR_ABSOLUTE_SELECTOR ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &absolute_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            absolute_field.width_bits != UINT8_C(16) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(3), &pa_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pa_field.width_bits != UINT8_C(16)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *absolute = (uint16_t)absolute_field.raw_value;
    *port_address = (uint16_t)pa_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_portr_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint16_t absolute;
    uint16_t port_address;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)input;
    status = portr_absolute_extract(instruction, &absolute, &port_address);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return portr_absolute_values(absolute, port_address, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_portr_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    (void)input;

    if (operands == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value != UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value != UINT64_C(0x78) ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[3].width_bits != UINT8_C(16)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return portr_absolute_values((uint16_t)operands->values[2].raw_value,
                                 (uint16_t)operands->values[3].raw_value,
                                 effect);
}
