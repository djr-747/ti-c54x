/*
 * Module contract
 *
 * Owns: the pure Table 3-3 MMR request router against borrowed
 * state. Does not own: register storage or knownness policy, bus
 * transfers, endpoint callbacks, arbitration, opcode semantics,
 * CPU lifecycle, timing, or diagnostics.
 */

#include "mmr_route.h"

#include <stddef.h>
#include <stdint.h>

#include "../../state/arch_state.h"
#include "../../state/block_size_register.h"
#include "../../state/interrupt_flag_register.h"
#include "../../state/mmr_file.h"
#include "../../state/status/registers.h"

static ti_c54x_bus_mmr_route_result mmr_result(
    ti_c54x_bus_mmr_status status, uint16_t read_value_word)
{
    ti_c54x_bus_mmr_route_result result = {status, read_value_word};

    return result;
}

static ti_c54x_bus_mmr_route_result declined(void)
{
    return mmr_result(TI_C54X_BUS_MMR_STATUS_DECLINED, UINT16_C(0));
}

static ti_c54x_bus_mmr_route_result unknown(void)
{
    return mmr_result(TI_C54X_BUS_MMR_STATUS_UNKNOWN, UINT16_C(0));
}

static ti_c54x_bus_mmr_route_result invalid(void)
{
    return mmr_result(TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT,
                      UINT16_C(0));
}

static ti_c54x_bus_mmr_route_result serviced(uint16_t read_value_word)
{
    return mmr_result(TI_C54X_BUS_MMR_STATUS_SERVICED, read_value_word);
}

static ti_c54x_bus_mmr_route_result route_arch(uint16_t value,
                                               ti_c54x_arch_state_status op)
{
    if (op == TI_C54X_ARCH_STATE_OK) {
        return serviced(value);
    }
    if (op == TI_C54X_ARCH_STATE_UNKNOWN) {
        return unknown();
    }
    return invalid();
}

static ti_c54x_bus_mmr_route_result route_file(
    uint16_t value, ti_c54x_mmr_file_status op)
{
    if (op == TI_C54X_MMR_FILE_OK) {
        return serviced(value);
    }
    if (op == TI_C54X_MMR_FILE_UNKNOWN) {
        return unknown();
    }
    return invalid();
}

static ti_c54x_bus_mmr_route_result route_swwsr(
    ti_c54x_arch_state *state, bool is_write,
    uint16_t write_value_word)
{
    uint16_t value = UINT16_C(0);
    ti_c54x_swwsr_status op;

    if (is_write) {
        op = ti_c54x_swwsr_write(&state->swwsr, write_value_word);
    } else {
        op = ti_c54x_swwsr_read(&state->swwsr, &value);
    }
    if (op == TI_C54X_SWWSR_OK) {
        return serviced(value);
    }
    if (op == TI_C54X_SWWSR_UNKNOWN) {
        return unknown();
    }
    return invalid();
}

static ti_c54x_bus_mmr_route_result route_ar(
    ti_c54x_arch_state *state, uint16_t address, bool is_write,
    uint16_t write_value_word)
{
    uint8_t index = (uint8_t)(address - TI_C54X_BUS_MMR_ADDRESS_AR0);
    uint16_t value = UINT16_C(0);

    if (is_write) {
        return route_arch(0U, ti_c54x_arch_state_write_ar(state, index,
                                                          write_value_word));
    }
    return route_arch(value, ti_c54x_arch_state_read_ar(state, index,
                                                       &value));
}

static ti_c54x_accumulator_word_id accumulator_word_for(uint16_t address)
{
    switch (address) {
    case TI_C54X_BUS_MMR_ADDRESS_AL:
        return TI_C54X_ACCUMULATOR_WORD_AL;
    case TI_C54X_BUS_MMR_ADDRESS_AH:
        return TI_C54X_ACCUMULATOR_WORD_AH;
    case TI_C54X_BUS_MMR_ADDRESS_AG:
        return TI_C54X_ACCUMULATOR_WORD_AG;
    case TI_C54X_BUS_MMR_ADDRESS_BL:
        return TI_C54X_ACCUMULATOR_WORD_BL;
    case TI_C54X_BUS_MMR_ADDRESS_BH:
        return TI_C54X_ACCUMULATOR_WORD_BH;
    case TI_C54X_BUS_MMR_ADDRESS_BG:
    default:
        return TI_C54X_ACCUMULATOR_WORD_BG;
    }
}

static ti_c54x_bus_mmr_route_result route_accumulator(
    ti_c54x_accumulator_state *accumulators, uint16_t address,
    bool is_write, uint16_t write_value_word)
{
    ti_c54x_accumulator_word_id word = accumulator_word_for(address);
    uint16_t value = UINT16_C(0);
    ti_c54x_accumulator_status op;

    if (is_write) {
        op = ti_c54x_accumulator_write_word(accumulators, word,
                                            write_value_word);
    } else {
        op = ti_c54x_accumulator_read_word(accumulators, word, &value);
    }
    if (op == TI_C54X_ACCUMULATOR_OK) {
        return serviced(value);
    }
    if (op == TI_C54X_ACCUMULATOR_UNKNOWN) {
        return unknown();
    }
    return invalid();
}

static ti_c54x_bus_mmr_route_result route_status(
    ti_c54x_arch_state *state, uint16_t address, bool is_write,
    uint16_t write_value_word)
{
    ti_c54x_status_register_field_result read_back;

    if (address == TI_C54X_BUS_MMR_ADDRESS_ST0) {
        if (is_write) {
            return route_arch(
                0U, (ti_c54x_arch_state_status)
                        ti_c54x_status_register_write_st0(&state->status,
                                                          write_value_word));
        }
        read_back = ti_c54x_status_register_read_st0(&state->status);
    } else if (address == TI_C54X_BUS_MMR_ADDRESS_ST1) {
        if (is_write) {
            return route_arch(
                0U, (ti_c54x_arch_state_status)
                        ti_c54x_status_register_write_st1(&state->status,
                                                          write_value_word));
        }
        read_back = ti_c54x_status_register_read_st1(&state->status);
    } else {
        if (is_write) {
            return route_arch(
                0U, (ti_c54x_arch_state_status)
                        ti_c54x_status_register_write_pmst(&state->status,
                                                           write_value_word));
        }
        read_back = ti_c54x_status_register_read_pmst(&state->status);
    }
    if (read_back.status == TI_C54X_STATUS_REGISTER_OK) {
        return serviced(read_back.raw_value);
    }
    if (read_back.status == TI_C54X_STATUS_REGISTER_UNKNOWN) {
        return unknown();
    }
    return invalid();
}

static ti_c54x_bus_mmr_route_result route_file_cell(
    ti_c54x_arch_state *state, uint16_t address, bool is_write,
    uint16_t write_value_word)
{
    ti_c54x_mmr_file_state *file = &state->mmr_file;
    uint16_t value = UINT16_C(0);

    switch (address) {
    case TI_C54X_BUS_MMR_ADDRESS_IMR:
        if (is_write) {
            return route_file(
                0U, ti_c54x_mmr_file_write_imr(file, write_value_word));
        }
        return route_file(value,
                          ti_c54x_mmr_file_read_imr(file, &value));
    case TI_C54X_BUS_MMR_ADDRESS_TRN:
        if (is_write) {
            return route_file(
                0U, ti_c54x_mmr_file_write_trn(file, write_value_word));
        }
        return route_file(value,
                          ti_c54x_mmr_file_read_trn(file, &value));
    case TI_C54X_BUS_MMR_ADDRESS_BRC:
        if (is_write) {
            return route_file(
                0U, ti_c54x_mmr_file_write_brc(file, write_value_word));
        }
        return route_file(value,
                          ti_c54x_mmr_file_read_brc(file, &value));
    case TI_C54X_BUS_MMR_ADDRESS_RSA:
        if (is_write) {
            return route_file(
                0U, ti_c54x_mmr_file_write_rsa(file, write_value_word));
        }
        return route_file(value,
                          ti_c54x_mmr_file_read_rsa(file, &value));
    case TI_C54X_BUS_MMR_ADDRESS_REA:
    default:
        if (is_write) {
            return route_file(
                0U, ti_c54x_mmr_file_write_rea(file, write_value_word));
        }
        return route_file(value,
                          ti_c54x_mmr_file_read_rea(file, &value));
    }
}

bool ti_c54x_bus_mmr_is_routed_address(
    ti_c54x_bus_word16_request_space space, uint32_t address_word)
{
    uint16_t address;

    if (space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA) {
        return false;
    }
    /* The SWWSR peripheral cell routes explicitly; the CPU
     * window itself stays capped at MAX. */
    if (address_word == TI_C54X_BUS_MMR_ADDRESS_SWWSR) {
        return true;
    }
    if (address_word > TI_C54X_BUS_MMR_ADDRESS_MAX) {
        return false;
    }
    address = (uint16_t)address_word;
    switch (address) {
    case UINT16_C(0x0002):
    case UINT16_C(0x0003):
    case UINT16_C(0x0004):
    case UINT16_C(0x0005):
    case UINT16_C(0x001E):
    case TI_C54X_BUS_MMR_ADDRESS_MAX:
        return false;
    default:
        return true;
    }
}

ti_c54x_bus_mmr_route_result ti_c54x_bus_mmr_route(
    const ti_c54x_bus_word16_request *request,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register)
{
    bool is_write;
    uint16_t address;
    uint16_t value = UINT16_C(0);

    if (request == NULL || state == NULL || accumulators == NULL ||
        t_register == NULL) {
        return invalid();
    }
    if ((request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
         request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) ||
        (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM &&
         request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
         request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_IO)) {
        return invalid();
    }
    if (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        (request->address_word > TI_C54X_BUS_MMR_ADDRESS_MAX &&
         request->address_word != TI_C54X_BUS_MMR_ADDRESS_SWWSR)) {
        return declined();
    }
    is_write = request->access ==
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    address = (uint16_t)request->address_word;
    switch (address) {
    case TI_C54X_BUS_MMR_ADDRESS_IMR:
    case TI_C54X_BUS_MMR_ADDRESS_TRN:
    case TI_C54X_BUS_MMR_ADDRESS_BRC:
    case TI_C54X_BUS_MMR_ADDRESS_RSA:
    case TI_C54X_BUS_MMR_ADDRESS_REA:
        return route_file_cell(state, address, is_write,
                               request->write_value_word);
    case TI_C54X_BUS_MMR_ADDRESS_IFR:
        if (is_write) {
            return route_arch(0U, ti_c54x_arch_state_write_ifr(
                                      state, request->write_value_word));
        }
        return route_arch(value, ti_c54x_arch_state_read_ifr(state,
                                                             &value));
    case TI_C54X_BUS_MMR_ADDRESS_ST0:
    case TI_C54X_BUS_MMR_ADDRESS_ST1:
    case TI_C54X_BUS_MMR_ADDRESS_PMST:
        return route_status(state, address, is_write,
                            request->write_value_word);
    case TI_C54X_BUS_MMR_ADDRESS_AL:
    case TI_C54X_BUS_MMR_ADDRESS_AH:
    case TI_C54X_BUS_MMR_ADDRESS_AG:
    case TI_C54X_BUS_MMR_ADDRESS_BL:
    case TI_C54X_BUS_MMR_ADDRESS_BH:
    case TI_C54X_BUS_MMR_ADDRESS_BG:
        return route_accumulator(accumulators, address, is_write,
                                 request->write_value_word);
    case TI_C54X_BUS_MMR_ADDRESS_T:
        if (is_write) {
            return ti_c54x_t_register_write(t_register,
                                            request->write_value_word) ==
                           TI_C54X_T_REGISTER_OK
                       ? serviced(UINT16_C(0))
                       : invalid();
        }
        return ti_c54x_t_register_read(t_register, &value) ==
                       TI_C54X_T_REGISTER_OK
                   ? serviced(value)
                   : unknown();
    case TI_C54X_BUS_MMR_ADDRESS_AR0:
    case TI_C54X_BUS_MMR_ADDRESS_AR0 + 1U:
    case TI_C54X_BUS_MMR_ADDRESS_AR0 + 2U:
    case TI_C54X_BUS_MMR_ADDRESS_AR0 + 3U:
    case TI_C54X_BUS_MMR_ADDRESS_AR0 + 4U:
    case TI_C54X_BUS_MMR_ADDRESS_AR0 + 5U:
    case TI_C54X_BUS_MMR_ADDRESS_AR0 + 6U:
    case TI_C54X_BUS_MMR_ADDRESS_AR7:
        return route_ar(state, address, is_write,
                        request->write_value_word);
    case TI_C54X_BUS_MMR_ADDRESS_SP:
        if (is_write) {
            return route_arch(0U, ti_c54x_arch_state_write_sp(
                                      state, request->write_value_word));
        }
        return route_arch(value, ti_c54x_arch_state_read_sp(state,
                                                            &value));
    case TI_C54X_BUS_MMR_ADDRESS_BK:
        if (is_write) {
            return route_arch(0U, ti_c54x_arch_state_write_bk(
                                      state, request->write_value_word));
        }
        return route_arch(value, ti_c54x_arch_state_read_bk(state,
                                                            &value));
    case TI_C54X_BUS_MMR_ADDRESS_SWWSR:
        return route_swwsr(state, is_write,
                           request->write_value_word);
    default:
        return declined();
    }
}
