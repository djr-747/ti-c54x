/*
 * Module contract
 *
 * Owns: a hosted DSP-image smoke runner. It loads one raw program-word
 * image file plus an optional data image file, serves them to the
 * standalone core through caller-owned PROGRAM/DATA buses, runs an
 * instruction budget from an explicit entry PC, and prints a
 * machine-readable stop report (stop reason, stop PC/opcode, retired
 * and attempted counts, program-word coverage, bus fault counters).
 * All configuration arrives on the command line; no environment,
 * registry, or repository state is read. The image bytes are never
 * copied into the repository and never become test vectors.
 * Does not own: core implementation, ISA semantics, device models,
 * timing, or any profile default. Unmapped program/data words stop
 * the run with an explicit bus stop, never with invented content.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ti_c54x/bus.h>
#include <ti_c54x/cpu.h>
#include <ti_c54x/state.h>

#include "mcu_upload_peer.h"
#include "port_peer.h"

/* Journal capacity (address-plus-value entries). A pump poll fills
 * it instantly; overflow still counts and flags. */
enum { RUN_IMAGE_WATCH_WORDS = 32 };

typedef struct run_image_watch_entry {
    uint32_t address_word;
    uint16_t value_word;
    bool is_write;
} run_image_watch_entry;

typedef struct run_image_bus {
    const uint16_t *program_words;
    uint32_t program_base;
    uint32_t program_count;
    uint16_t *data_words;
    uint32_t data_base;
    uint32_t data_count;
    ti_c54x_mcu_upload_peer_state mcu_peer;
    bool have_mcu_peer;
    ti_c54x_port_peer_state port_peer;
    bool have_port_peer;
    /* Writable program overlay: MVDP/WRITA-style PROGRAM writes
     * land here and are visible to the fetch callback, so the
     * boot can copy code into program space. Reads fall back
     * to the loaded image; never-written, never-loaded words
     * still fault explicitly. */
    uint16_t *program_overlay;
    unsigned char *program_overlay_written;
    unsigned program_writes;
    uint32_t last_program_write_address;
    uint16_t last_program_write_value;
    unsigned char *visited;
    uint64_t program_reads;
    uint64_t program_unmapped;
    uint64_t data_transactions;
    uint64_t data_faults;
    uint32_t first_data_fault;
    bool have_data_fault;
    uint64_t io_calls;
    uint32_t first_io_address;
    bool have_io;
    /* Windowed DATA watch (P3-S91): passive tap on served DATA
     * accesses inside [watch_base, watch_base + watch_count).
     * Reads journal the served value, writes the written value.
     * Gated by want_watch; the journal resets per phase. */
    bool want_watch;
    uint32_t watch_base;
    uint32_t watch_count;
    run_image_watch_entry watch_journal[RUN_IMAGE_WATCH_WORDS];
    uint32_t watch_total;
    uint32_t watch_journaled;
    bool watch_overflow;
} run_image_bus;

static ti_c54x_bus_status
run_image_program_read(void *context,
                       ti_c54x_program_word_address address_word,
                       uint16_t *value_word)
{
    run_image_bus *bus = (run_image_bus *)context;
    uint32_t index;

    if (bus == NULL || value_word == NULL) {
        return TI_C54X_BUS_FAULT;
    }
    bus->program_reads++;
    /* A PROGRAM write through the endpoint shadows the loaded
     * image for later fetches (boot code overlays). */
    if (bus->program_overlay != NULL &&
        bus->program_overlay_written != NULL &&
        bus->program_overlay_written[address_word & UINT32_C(0xFFFF)] !=
            (unsigned char)0) {
        *value_word =
            bus->program_overlay[address_word & UINT32_C(0xFFFF)];
        return TI_C54X_BUS_OK;
    }
    if (address_word < bus->program_base) {
        bus->program_unmapped++;
        return TI_C54X_BUS_UNMAPPED;
    }
    index = (uint32_t)(address_word - bus->program_base);
    if (index >= bus->program_count) {
        bus->program_unmapped++;
        return TI_C54X_BUS_UNMAPPED;
    }
    if (bus->visited != NULL) {
        bus->visited[index] = 1U;
    }
    *value_word = bus->program_words[index];
    return TI_C54X_BUS_OK;
}

static void
run_image_advance(void *context,
                  ti_c54x_bus_word16_advance_phase phase,
                  uint64_t transaction_id,
                  const ti_c54x_bus_word16_request *request,
                  ti_c54x_bus_word16_observation *observation)
{
    run_image_bus *bus = (run_image_bus *)context;

    (void)phase;
    if (bus == NULL || request == NULL || observation == NULL) {
        return;
    }
    bus->data_transactions++;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_FAULT,
        .read_value_word = UINT16_C(0),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT};
    if (request->space ==
        TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM) {
        if (request->address_word > UINT32_C(0xFFFF) ||
            bus->program_overlay == NULL ||
            bus->program_overlay_written == NULL) {
            bus->data_faults++;
            if (!bus->have_data_fault) {
                bus->have_data_fault = true;
                bus->first_data_fault = request->address_word;
            }
            return;
        }
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            bus->program_overlay[request->address_word] =
                request->write_value_word;
            bus->program_overlay_written[request->address_word] =
                (unsigned char)1;
            bus->program_writes++;
            bus->last_program_write_address = request->address_word;
            bus->last_program_write_value = request->write_value_word;
            observation->bus_status = TI_C54X_BUS_OK;
            observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
            return;
        }
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
            bus->program_overlay_written[request->address_word] !=
                (unsigned char)0) {
            observation->read_value_word =
                bus->program_overlay[request->address_word];
            observation->bus_status = TI_C54X_BUS_OK;
            observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
            return;
        }
        bus->data_faults++;
        if (!bus->have_data_fault) {
            bus->have_data_fault = true;
            bus->first_data_fault = request->address_word;
        }
        return;
    }
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_IO) {
        /* Hardware-port leg: only the attached port peer serves
         * it. Detached runs keep the historical explicit fault
         * with untouched IO counters. */
        if (!bus->have_port_peer) {
            bus->data_faults++;
            if (!bus->have_data_fault) {
                bus->have_data_fault = true;
                bus->first_data_fault = request->address_word;
            }
            return;
        }
        bus->io_calls++;
        if (!bus->have_io) {
            bus->have_io = true;
            bus->first_io_address = request->address_word;
        }
        if (request->access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            if (ti_c54x_port_peer_notify_write(
                    &bus->port_peer,
                    (uint16_t)(request->address_word & UINT32_C(0xFFFF)),
                    request->write_value_word) !=
                TI_C54X_PORT_PEER_OK) {
                bus->data_faults++;
                return;
            }
        } else if (request->access ==
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            uint16_t read_value = UINT16_C(0);

            if (ti_c54x_port_peer_notify_read(
                    &bus->port_peer,
                    (uint16_t)(request->address_word & UINT32_C(0xFFFF)),
                    &read_value) != TI_C54X_PORT_PEER_OK) {
                bus->data_faults++;
                return;
            }
            observation->read_value_word = read_value;
        } else {
            bus->data_faults++;
            return;
        }
        observation->bus_status = TI_C54X_BUS_OK;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    if (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        bus->data_words == NULL) {
        bus->data_faults++;
        if (!bus->have_data_fault) {
            bus->have_data_fault = true;
            bus->first_data_fault = request->address_word;
        }
        return;
    }
    if (request->address_word < bus->data_base ||
        (uint32_t)(request->address_word - bus->data_base) >=
            bus->data_count) {
        bus->data_faults++;
        if (!bus->have_data_fault) {
            bus->have_data_fault = true;
            bus->first_data_fault = request->address_word;
        }
        return;
    }
    {
        uint32_t index =
            (uint32_t)(request->address_word - bus->data_base);

        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            observation->read_value_word = bus->data_words[index];
        } else if (request->access ==
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            bus->data_words[index] = request->write_value_word;
        } else {
            bus->data_faults++;
            return;
        }
        observation->bus_status = TI_C54X_BUS_OK;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        if (bus->want_watch &&
            request->address_word >= bus->watch_base &&
            request->address_word - bus->watch_base < bus->watch_count) {
            uint16_t tap_value =
                (request->access ==
                 TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE)
                    ? request->write_value_word
                    : observation->read_value_word;

            bus->watch_total++;
            if (bus->watch_journaled <
                (uint32_t)RUN_IMAGE_WATCH_WORDS) {
                bus->watch_journal[bus->watch_journaled].address_word =
                    request->address_word;
                bus->watch_journal[bus->watch_journaled].value_word =
                    tap_value;
                bus->watch_journal[bus->watch_journaled].is_write =
                    (request->access ==
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
                bus->watch_journaled++;
            } else {
                bus->watch_overflow = true;
            }
        }
        if (bus->have_mcu_peer &&
            request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            ti_c54x_mcu_upload_peer_notify_data_write(
                &bus->mcu_peer, request->address_word,
                request->write_value_word);
        }
    }
}

static ti_c54x_bus_word16_cancel_result
run_image_cancel(void *context, uint64_t transaction_id,
                 const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_status
run_image_io_read(void *context, ti_c54x_io_word_address address_word,
                  uint16_t *value_word)
{
    run_image_bus *bus = (run_image_bus *)context;

    if (bus != NULL) {
        bus->io_calls++;
        if (!bus->have_io) {
            bus->have_io = true;
            bus->first_io_address = address_word;
        }
    }
    if (value_word != NULL) {
        *value_word = UINT16_C(0);
    }
    return TI_C54X_BUS_UNMAPPED;
}

static ti_c54x_bus_status
run_image_io_write(void *context, ti_c54x_io_word_address address_word,
                   uint16_t value_word)
{
    run_image_bus *bus = (run_image_bus *)context;

    (void)value_word;
    if (bus != NULL) {
        bus->io_calls++;
        if (!bus->have_io) {
            bus->have_io = true;
            bus->first_io_address = address_word;
        }
    }
    return TI_C54X_BUS_UNMAPPED;
}

static const char *
stop_reason_name(ti_c54x_stop_reason reason)
{
    switch (reason) {
    case TI_C54X_STOP_BUDGET_EXHAUSTED:
        return "BUDGET_EXHAUSTED";
    case TI_C54X_STOP_UNIMPLEMENTED_OPCODE:
        return "UNIMPLEMENTED_OPCODE";
    case TI_C54X_STOP_BUS_FAULT:
        return "BUS_FAULT";
    case TI_C54X_STOP_IDLE:
        return "IDLE";
    case TI_C54X_STOP_BREAKPOINT:
        return "BREAKPOINT";
    case TI_C54X_STOP_INVALID_STATE:
        return "INVALID_STATE";
    case TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS:
        return "UNSUPPORTED_PROGRAM_ADDRESS";
    case TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY:
        return "UNRESOLVED_PROGRAM_BOUNDARY";
    case TI_C54X_STOP_COUNTER_OVERFLOW:
        return "COUNTER_OVERFLOW";
    case TI_C54X_STOP_BUS_WAIT:
        return "BUS_WAIT";
    default:
        return "UNKNOWN";
    }
}

static const char *
bus_status_name(ti_c54x_bus_status status)
{
    switch (status) {
    case TI_C54X_BUS_OK:
        return "OK";
    case TI_C54X_BUS_UNMAPPED:
        return "UNMAPPED";
    case TI_C54X_BUS_FAULT:
        return "FAULT";
    case TI_C54X_BUS_NO_CALLBACK:
        return "NO_CALLBACK";
    default:
        return "UNKNOWN";
    }
}

static int
parse_u32(const char *text, uint32_t *out)
{
    char *end = NULL;
    unsigned long value;

    if (text == NULL || out == NULL || *text == '\0') {
        return -1;
    }
    value = strtoul(text, &end, 0);
    if (end == text || *end != '\0' || value > UINT32_C(0xFFFFFFFF)) {
        return -1;
    }
    *out = (uint32_t)value;
    return 0;
}

static int
parse_u64(const char *text, uint64_t *out)
{
    char *end = NULL;
    unsigned long long value;

    if (text == NULL || out == NULL || *text == '\0') {
        return -1;
    }
    value = strtoull(text, &end, 0);
    if (end == text || *end != '\0') {
        return -1;
    }
    *out = (uint64_t)value;
    return 0;
}

static uint16_t *
load_words(const char *path, bool big_endian, uint32_t *count_out)
{
    FILE *file = NULL;
    long size = 0L;
    unsigned char *bytes = NULL;
    uint16_t *words = NULL;
    uint32_t count = 0U;
    uint32_t i = 0U;

    if (path == NULL || count_out == NULL) {
        return NULL;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    if (size == 0L || (size % 2L) != 0L) {
        (void)fclose(file);
        return NULL;
    }
    count = (uint32_t)size / UINT32_C(2);
    bytes = (unsigned char *)malloc((size_t)size);
    words = (uint16_t *)malloc((size_t)size);
    if (bytes == NULL || words == NULL) {
        free(bytes);
        free(words);
        (void)fclose(file);
        return NULL;
    }
    if (fread(bytes, 1U, (size_t)size, file) != (size_t)size) {
        free(bytes);
        free(words);
        (void)fclose(file);
        return NULL;
    }
    (void)fclose(file);
    for (i = 0U; i < count; i++) {
        uint16_t high = bytes[(size_t)i * 2U];
        uint16_t low = bytes[(size_t)i * 2U + 1U];

        words[i] = big_endian ? (uint16_t)((uint16_t)(high << 8U) | low)
                              : (uint16_t)((uint16_t)(low << 8U) | high);
    }
    free(bytes);
    *count_out = count;
    return words;
}

static void
usage(const char *argv0)
{
    (void)fprintf(stderr,
                  "usage: %s --selftest\n"
                  "   or: %s <prog_image> <prog_base> <entry> <budget> "
                  "<be|le> [data_image data_base]\n"
                  "   [--mcu-upload <flash_image> <blocks>]\n"
                  "   [--port-peer]\n"
                  "   [--dump-prog <base> <count>]\n"
                  "   [--watch-data <base> <count>]\n"
                  "   [--warm-boot <budget>]\n"
                  "\n"
                  "Runs one program-word image from <entry> for <budget>\n"
                  "retirements and prints a stop report. Bases, entry,\n"
                  "and budget accept C integer syntax (0x hex or\n"
                  "decimal). Unmapped program/data words stop the run\n"
                  "explicitly; nothing is invented. The trailing\n"
                  "--mcu-upload form attaches the MCU upload peer\n"
                  "(1-64 flash blocks); without a data image the peer\n"
                  "owns a zeroed 0x800-0x2800 DATA window. A trailing\n"
                  "--port-peer attaches the port peer, which serves\n"
                  "IO-space legs (writes journaled, reads sticky)\n"
                  "instead of faulting them. A --dump-prog trio\n"
                  "anywhere prints runtime program words for\n"
                  "vector/ISR diagnosis. A --watch-data trio anywhere\n"
                  "journals served DATA reads/writes in its window\n"
                  "(first 32 per phase). A --warm-boot pair anywhere\n"
                  "runs a second phase after the first stop: the CPU\n"
                  "re-inits at 0xFF80 (silicon warm-reset entry)\n"
                  "while program, data, peers, and overlay survive.\n",
                  argv0, argv0);
}

/* One budgeted execution phase on a fresh CPU against a caller-wired
 * bus. The bus (program image, DATA window, peers, program overlay,
 * coverage) is caller-owned and survives across phases: only CPU
 * state resets, which is the warm-reset contract (silicon re-applies
 * register reset while host-owned memory preserves). Storage and CPU
 * must enter NULL and leave owned by the caller until phase_end. */
static int
phase_begin(const ti_c54x_bus *bus_desc, uint32_t entry,
            unsigned char **storage, ti_c54x_cpu **cpu)
{
    ti_c54x_config config;

    if (bus_desc == NULL || storage == NULL || cpu == NULL ||
        *storage != NULL || *cpu != NULL) {
        return -1;
    }
    config.reset_pc_word = (ti_c54x_program_word_address)entry;
    memset(&config.trace, 0, sizeof(config.trace));
    *storage = (unsigned char *)malloc(ti_c54x_cpu_size());
    if (*storage == NULL) {
        return -1;
    }
    if (ti_c54x_cpu_init(*storage, ti_c54x_cpu_size(), &config, bus_desc,
                         cpu) != TI_C54X_ERROR_OK) {
        free(*storage);
        *storage = NULL;
        return -1;
    }
    return 0;
}

static void
phase_end(unsigned char **storage, ti_c54x_cpu **cpu)
{
    if (cpu != NULL && *cpu != NULL) {
        (void)ti_c54x_cpu_deinit(*cpu);
        *cpu = NULL;
    }
    if (storage != NULL) {
        free(*storage);
        *storage = NULL;
    }
}

/* In-memory selftest: no files, three fixed vectors. */
static int
selftest(void)
{
    /* Vector 1: two NOPs retire under budget 2. */
    static const uint16_t kNops[] = {UINT16_C(0xF495),
                                     UINT16_C(0xF495)};
    /* Vector 2: empty program window faults the first fetch. */
    static const uint16_t kEmpty[] = {UINT16_C(0x0000)};
    run_image_bus bus;
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_bus bus_desc;
    ti_c54x_config config;
    unsigned char *storage = NULL;
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    int failures = 0;

    memset(&bus, 0, sizeof(bus));
    bus.program_words = kNops;
    bus.program_base = UINT32_C(0x0100);
    bus.program_count = UINT32_C(2);
    endpoint.context = &bus;
    endpoint.advance = run_image_advance;
    endpoint.cancel = run_image_cancel;
    bus_desc.context = &bus;
    bus_desc.program_read_word = run_image_program_read;
    bus_desc.program_write_word = NULL;
    bus_desc.data_endpoint = endpoint;
    bus_desc.io_read_word = run_image_io_read;
    bus_desc.io_write_word = run_image_io_write;
    config.reset_pc_word = UINT32_C(0x0100);
    memset(&config.trace, 0, sizeof(config.trace));
    storage = (unsigned char *)malloc(ti_c54x_cpu_size());
    if (storage == NULL) {
        (void)fprintf(stderr, "selftest: storage alloc failed\n");
        return 1;
    }
    if (ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                         &bus_desc, &cpu) != TI_C54X_ERROR_OK) {
        (void)fprintf(stderr, "selftest: init failed\n");
        free(storage);
        return 1;
    }
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    if (result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
        result.instructions_retired != UINT64_C(2) ||
        result.instructions_attempted != UINT64_C(2) ||
        bus.program_reads != UINT64_C(2)) {
        (void)fprintf(stderr,
                      "selftest: NOP vector failed (reason=%d "
                      "retired=%" PRIu64 " attempted=%" PRIu64
                      " reads=%" PRIu64 ")\n",
                      (int)result.reason,
                      result.instructions_retired,
                      result.instructions_attempted,
                      bus.program_reads);
        failures++;
    }
    if (ti_c54x_cpu_deinit(cpu) != TI_C54X_ERROR_OK) {
        (void)fprintf(stderr, "selftest: deinit failed\n");
        failures++;
    }
    free(storage);

    memset(&bus, 0, sizeof(bus));
    bus.program_words = kEmpty;
    bus.program_base = UINT32_C(0x0100);
    bus.program_count = UINT32_C(0);
    storage = (unsigned char *)malloc(ti_c54x_cpu_size());
    if (storage == NULL) {
        (void)fprintf(stderr, "selftest: storage alloc failed\n");
        return 1;
    }
    if (ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                         &bus_desc, &cpu) != TI_C54X_ERROR_OK) {
        (void)fprintf(stderr, "selftest: init failed\n");
        free(storage);
        return 1;
    }
    result = ti_c54x_cpu_run(cpu, UINT64_C(10));
    if (result.reason != TI_C54X_STOP_BUS_FAULT ||
        result.instructions_retired != UINT64_C(0) ||
        result.instructions_attempted != UINT64_C(1) ||
        bus.program_unmapped != UINT64_C(1)) {
        (void)fprintf(stderr,
                      "selftest: unmapped vector failed (reason=%d "
                      "retired=%" PRIu64 " attempted=%" PRIu64
                      " unmapped=%" PRIu64 ")\n",
                      (int)result.reason,
                      result.instructions_retired,
                      result.instructions_attempted,
                      bus.program_unmapped);
        failures++;
    }
    if (ti_c54x_cpu_deinit(cpu) != TI_C54X_ERROR_OK) {
        (void)fprintf(stderr, "selftest: deinit failed\n");
        failures++;
    }
    free(storage);

    /* Vector 3: a warm-boot phase boundary preserves host-owned
     * memory while re-running from a fresh CPU. Phase 1 stores
     * through the absolute ST form (P3-S70, no register inputs);
     * after the boundary phase 2 stores again. Both cells must
     * hold: preservation plus rerun. */
    {
        static const uint16_t kPhase1[] = {UINT16_C(0x76F8),
                                           UINT16_C(0x0800),
                                           UINT16_C(0x00A5)};
        static const uint16_t kPhase2[] = {UINT16_C(0x76F8),
                                           UINT16_C(0x0801),
                                           UINT16_C(0x005A)};
        static uint16_t ram[4] = {0};
        ti_c54x_run_result phase_result;

        memset(&bus, 0, sizeof(bus));
        bus.program_words = kPhase1;
        bus.program_base = UINT32_C(0x0100);
        bus.program_count = UINT32_C(3);
        bus.data_words = ram;
        bus.data_base = UINT32_C(0x0800);
        bus.data_count = UINT32_C(4);
        endpoint.context = &bus;
        endpoint.advance = run_image_advance;
        endpoint.cancel = run_image_cancel;
        bus_desc.context = &bus;
        bus_desc.program_read_word = run_image_program_read;
        bus_desc.program_write_word = NULL;
        bus_desc.data_endpoint = endpoint;
        bus_desc.io_read_word = run_image_io_read;
        bus_desc.io_write_word = run_image_io_write;
        storage = NULL;
        cpu = NULL;
        if (phase_begin(&bus_desc, UINT32_C(0x0100), &storage, &cpu) !=
            0) {
            (void)fprintf(stderr,
                          "selftest: warm-boot phase 1 failed\n");
            failures++;
        } else {
            phase_result = ti_c54x_cpu_run(cpu, UINT64_C(1));
            if (phase_result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
                phase_result.instructions_retired != UINT64_C(1) ||
                ram[0] != UINT16_C(0x00A5)) {
                (void)fprintf(stderr,
                              "selftest: warm-boot phase 1 failed\n");
                failures++;
            }
        }
        phase_end(&storage, &cpu);
        bus.program_words = kPhase2;
        bus.program_base = UINT32_C(0x0200);
        bus.program_count = UINT32_C(3);
        if (phase_begin(&bus_desc, UINT32_C(0x0200), &storage, &cpu) !=
            0) {
            (void)fprintf(stderr,
                          "selftest: warm-boot phase 2 failed\n");
            failures++;
        } else {
            phase_result = ti_c54x_cpu_run(cpu, UINT64_C(1));
            if (phase_result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
                phase_result.instructions_retired != UINT64_C(1) ||
                ram[0] != UINT16_C(0x00A5) ||
                ram[1] != UINT16_C(0x005A)) {
                (void)fprintf(stderr,
                              "selftest: warm-boot phase 2 failed\n");
                failures++;
            }
        }
        phase_end(&storage, &cpu);
    }

    /* Vector 4: the windowed DATA watch journals in-window
     * accesses with served values and ignores the rest. */
    {
        static const uint16_t kWatch[] = {UINT16_C(0x76F8),
                                          UINT16_C(0x0800),
                                          UINT16_C(0x00A5),
                                          UINT16_C(0x76F8),
                                          UINT16_C(0x0900),
                                          UINT16_C(0x005A)};
        static uint16_t watch_ram[0x0200] = {0};
        ti_c54x_run_result phase_result;

        memset(&bus, 0, sizeof(bus));
        bus.program_words = kWatch;
        bus.program_base = UINT32_C(0x0100);
        bus.program_count = UINT32_C(6);
        bus.data_words = watch_ram;
        bus.data_base = UINT32_C(0x0800);
        bus.data_count = UINT32_C(0x0200);
        bus.want_watch = true;
        bus.watch_base = UINT32_C(0x0800);
        bus.watch_count = UINT32_C(0x0100);
        endpoint.context = &bus;
        endpoint.advance = run_image_advance;
        endpoint.cancel = run_image_cancel;
        bus_desc.context = &bus;
        bus_desc.program_read_word = run_image_program_read;
        bus_desc.program_write_word = NULL;
        bus_desc.data_endpoint = endpoint;
        bus_desc.io_read_word = run_image_io_read;
        bus_desc.io_write_word = run_image_io_write;
        storage = NULL;
        cpu = NULL;
        if (phase_begin(&bus_desc, UINT32_C(0x0100), &storage, &cpu) !=
            0) {
            (void)fprintf(stderr, "selftest: data watch failed\n");
            failures++;
        } else {
            phase_result = ti_c54x_cpu_run(cpu, UINT64_C(2));
            if (phase_result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
                phase_result.instructions_retired != UINT64_C(2) ||
                bus.watch_total != UINT32_C(1) ||
                bus.watch_journaled != UINT32_C(1) ||
                bus.watch_overflow ||
                bus.watch_journal[0].address_word != UINT32_C(0x0800) ||
                bus.watch_journal[0].value_word != UINT16_C(0x00A5) ||
                !bus.watch_journal[0].is_write ||
                watch_ram[0x0100] != UINT16_C(0x005A)) {
                (void)fprintf(stderr, "selftest: data watch failed\n");
                failures++;
            }
        }
        phase_end(&storage, &cpu);
    }
    /* Vector 4b: the watch caps at 32 entries with overflow. */
    {
        uint16_t prog[35U * 3U];
        static uint16_t cap_ram[0x0200] = {0};
        ti_c54x_run_result cap_result;
        uint32_t k;

        for (k = 0U; k < 35U; k++) {
            prog[3U * k] = UINT16_C(0x76F8);
            prog[3U * k + 1U] = (uint16_t)(UINT32_C(0x0800) + k);
            prog[3U * k + 2U] = (uint16_t)k;
        }
        memset(&bus, 0, sizeof(bus));
        bus.program_words = prog;
        bus.program_base = UINT32_C(0x0100);
        bus.program_count = UINT32_C(105);
        bus.data_words = cap_ram;
        bus.data_base = UINT32_C(0x0800);
        bus.data_count = UINT32_C(0x0200);
        bus.want_watch = true;
        bus.watch_base = UINT32_C(0x0800);
        bus.watch_count = UINT32_C(0x0100);
        endpoint.context = &bus;
        endpoint.advance = run_image_advance;
        endpoint.cancel = run_image_cancel;
        bus_desc.context = &bus;
        bus_desc.program_read_word = run_image_program_read;
        bus_desc.program_write_word = NULL;
        bus_desc.data_endpoint = endpoint;
        bus_desc.io_read_word = run_image_io_read;
        bus_desc.io_write_word = run_image_io_write;
        storage = NULL;
        cpu = NULL;
        if (phase_begin(&bus_desc, UINT32_C(0x0100), &storage, &cpu) !=
            0) {
            (void)fprintf(stderr, "selftest: data watch cap failed\n");
            failures++;
        } else {
            cap_result = ti_c54x_cpu_run(cpu, UINT64_C(35));
            if (cap_result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
                cap_result.instructions_retired != UINT64_C(35) ||
                bus.watch_total != UINT32_C(35) ||
                bus.watch_journaled != UINT32_C(32) ||
                !bus.watch_overflow ||
                bus.watch_journal[0].value_word != UINT16_C(0) ||
                bus.watch_journal[31].address_word !=
                    UINT32_C(0x081F) ||
                bus.watch_journal[31].value_word != UINT16_C(31)) {
                (void)fprintf(stderr,
                              "selftest: data watch cap failed\n");
                failures++;
            }
        }
        phase_end(&storage, &cpu);
    }

    if (failures == 0) {
        (void)printf("selftest: PASS (4 vectors)\n");
    }
    return failures == 0 ? 0 : 1;
}

/* Silicon warm-reset entry: reset forces PC to the vector-table base.
 * Only the CPU re-inits there; host-owned memory preserves. */
enum { WARM_BOOT_ENTRY_WORD = 0xFF80 };

int
main(int argc, char **argv)
{
    run_image_bus bus;
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_bus bus_desc;
    uint16_t *program = NULL;
    /* done: frees the overlay scalars on every exit path. */
    memset(&bus, 0, sizeof(bus));
    uint16_t *data = NULL;
    uint16_t *flash = NULL;
    uint32_t program_count = 0U;
    uint32_t data_count = 0U;
    uint32_t flash_count = 0U;
    uint32_t prog_base = 0U;
    uint32_t entry = 0U;
    uint32_t data_base = 0U;
    uint64_t budget = 0U;
    uint64_t block_limit = 0U;
    const char *flash_path = NULL;
    bool big_endian = false;
    bool have_data = false;
    bool want_mcu_peer = false;
    bool want_port_peer = false;
    bool want_dump = false;
    bool want_warm_boot = false;
    uint64_t warm_budget = UINT64_C(0);
    bool want_watch = false;
    uint32_t watch_base = 0U;
    uint32_t watch_count = 0U;
    uint32_t dump_base = 0U;
    uint32_t dump_count = 0U;
    unsigned char *visited = NULL;
    uint16_t *ram = NULL;
    unsigned char *storage = NULL;
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    uint64_t visited_count = 0U;
    uint32_t i = 0U;
    uint32_t phase = 0U;
    char data_fault_text[16] = {0};
    char io_text[16] = {0};
    int rc = 1;

    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) {
        return selftest();
    }
    /* An optional --port-peer strips out anywhere, so the
     * 6/8/9/11 forms are untouched with or without the peer
     * and flag order against --dump-prog does not matter. */
    for (i = 1; i < (uint32_t)argc; i++) {
        if (strcmp(argv[i], "--port-peer") == 0) {
            uint32_t src = i + 1U;

            want_port_peer = true;
            for (; src < (uint32_t)argc; i++, src++) {
                argv[i] = argv[src];
            }
            argv[i] = NULL;
            argc--;
            break;
        }
    }
    /* An optional --dump-prog <base> <count> trio strips out
     * anywhere after the program, so the positional dispatch
     * below keeps its fixed indices. */
    for (i = 1; i < (uint32_t)argc; i++) {
        if (strcmp(argv[i], "--dump-prog") == 0) {
            unsigned long parsed;
            char *end = NULL;

            if (want_dump || i + 2 >= (uint32_t)argc) {
                usage(argv[0]);
                return 2;
            }
            parsed = strtoul(argv[i + 1], &end, 0);
            if (end == NULL || *end != '\0' ||
                parsed > UINT32_C(0xFFFF)) {
                usage(argv[0]);
                return 2;
            }
            dump_base = (uint32_t)parsed;
            end = NULL;
            parsed = strtoul(argv[i + 2], &end, 0);
            if (end == NULL || *end != '\0' || parsed == 0U ||
                parsed > UINT32_C(0x1000)) {
                usage(argv[0]);
                return 2;
            }
            dump_count = (uint32_t)parsed;
            want_dump = true;
            {
                uint32_t src = i + 3U;

                for (; src < (uint32_t)argc; i++, src++) {
                    argv[i] = argv[src];
                }
                argv[i] = NULL;
                argc -= 3;
            }
            break;
        }
    }
    /* An optional --warm-boot <budget> pair strips out anywhere, so
     * the positional dispatch below keeps its fixed indices. */
    for (i = 1; i < (uint32_t)argc; i++) {
        if (strcmp(argv[i], "--warm-boot") == 0) {
            if (want_warm_boot || i + 1 >= (uint32_t)argc ||
                parse_u64(argv[i + 1], &warm_budget) != 0 ||
                warm_budget == UINT64_C(0)) {
                usage(argv[0]);
                return 2;
            }
            want_warm_boot = true;
            {
                uint32_t src = i + 2U;

                for (; src < (uint32_t)argc; i++, src++) {
                    argv[i] = argv[src];
                }
                argv[i] = NULL;
                argc -= 2;
            }
            break;
        }
    }
    /* An optional --watch-data <base> <count> trio strips out
     * anywhere, so the positional dispatch below keeps its fixed
     * indices. The window names DATA word addresses. */
    for (i = 1; i < (uint32_t)argc; i++) {
        if (strcmp(argv[i], "--watch-data") == 0) {
            unsigned long parsed;
            char *end = NULL;

            if (want_watch || i + 2 >= (uint32_t)argc) {
                usage(argv[0]);
                return 2;
            }
            parsed = strtoul(argv[i + 1], &end, 0);
            if (end == NULL || *end != '\0' ||
                parsed > UINT32_C(0xFFFF)) {
                usage(argv[0]);
                return 2;
            }
            watch_base = (uint32_t)parsed;
            end = NULL;
            parsed = strtoul(argv[i + 2], &end, 0);
            if (end == NULL || *end != '\0' || parsed == 0U ||
                parsed > UINT32_C(0x10000) ||
                watch_base + (uint32_t)parsed > UINT32_C(0x10000)) {
                usage(argv[0]);
                return 2;
            }
            watch_count = (uint32_t)parsed;
            want_watch = true;
            {
                uint32_t src = i + 3U;

                for (; src < (uint32_t)argc; i++, src++) {
                    argv[i] = argv[src];
                }
                argv[i] = NULL;
                argc -= 3;
            }
            break;
        }
    }
    if (argc != 6 && argc != 8 && argc != 9 && argc != 11) {
        usage(argv[0]);
        return 2;
    }
    if (strcmp(argv[5], "be") == 0) {
        big_endian = true;
    } else if (strcmp(argv[5], "le") == 0) {
        big_endian = false;
    } else {
        usage(argv[0]);
        return 2;
    }
    if (parse_u32(argv[2], &prog_base) != 0 ||
        parse_u32(argv[3], &entry) != 0 ||
        parse_u64(argv[4], &budget) != 0 || budget == UINT64_C(0)) {
        usage(argv[0]);
        return 2;
    }
    program = load_words(argv[1], big_endian, &program_count);
    if (program == NULL) {
        (void)fprintf(stderr, "error: cannot load program image '%s'\n",
                      argv[1]);
        return 1;
    }
    if (argc == 8 || argc == 11) {
        if (parse_u32(argv[7], &data_base) != 0) {
            usage(argv[0]);
            goto done;
        }
        /* Data images carry raw bytes; word order matches program. */
        data = load_words(argv[6], big_endian, &data_count);
        if (data == NULL) {
            (void)fprintf(stderr,
                          "error: cannot load data image '%s'\n", argv[6]);
            goto done;
        }
        have_data = true;
    }
    if (argc == 9 || argc == 11) {
        const char *flag = (argc == 9) ? argv[6] : argv[8];

        flash_path = (argc == 9) ? argv[7] : argv[9];
        if (strcmp(flag, "--mcu-upload") != 0 ||
            parse_u64((argc == 9) ? argv[8] : argv[10],
                      &block_limit) != 0 ||
            block_limit == UINT64_C(0) ||
            block_limit > UINT64_C(64)) {
            usage(argv[0]);
            goto done;
        }
        want_mcu_peer = true;
        flash = load_words(flash_path, big_endian, &flash_count);
        if (flash == NULL) {
            (void)fprintf(stderr,
                          "error: cannot load flash image '%s'\n",
                          flash_path);
            goto done;
        }
    }

    visited =
        (unsigned char *)calloc((size_t)program_count, (size_t)1);
    if (visited == NULL) {
        (void)fprintf(stderr, "error: coverage alloc failed\n");
        goto done;
    }
    if (have_data) {
        ram = (uint16_t *)calloc((size_t)data_count,
                                 sizeof(uint16_t));
        if (ram == NULL) {
            (void)fprintf(stderr, "error: data alloc failed\n");
            goto done;
        }
        (void)memcpy(ram, data, (size_t)data_count * sizeof(uint16_t));
    }
    if (want_mcu_peer && !have_data) {
        /* Peer-owned zeroed OVLY window when no data image
         * is supplied. */
        ram = (uint16_t *)calloc((size_t)UINT32_C(0x2000),
                                 sizeof(uint16_t));
        if (ram == NULL) {
            (void)fprintf(stderr, "error: peer data alloc failed\n");
            goto done;
        }
        data_base = UINT32_C(0x0800);
        data_count = UINT32_C(0x2000);
        have_data = true;
    }
    memset(&bus, 0, sizeof(bus));
    bus.program_words = program;
    bus.program_base = prog_base;
    bus.program_count = program_count;
    bus.data_words = ram;
    bus.data_base = data_base;
    bus.data_count = have_data ? data_count : UINT32_C(0);
    bus.visited = visited;
    bus.want_watch = want_watch;
    bus.watch_base = watch_base;
    bus.watch_count = watch_count;
    bus.program_overlay =
        (uint16_t *)calloc((size_t)UINT32_C(0x10000), sizeof(uint16_t));
    bus.program_overlay_written =
        (unsigned char *)calloc((size_t)UINT32_C(0x10000), sizeof(unsigned char));
    if (bus.program_overlay == NULL || bus.program_overlay_written == NULL) {
        (void)fprintf(stderr, "error: program overlay alloc failed\n");
        goto done;
    }
    if (want_mcu_peer) {
        ti_c54x_mcu_upload_peer_status peer_status =
            ti_c54x_mcu_upload_peer_init(
                &bus.mcu_peer, bus.data_words, bus.data_base,
                bus.data_count, flash, flash_count,
                (uint32_t)(TI_C54X_MCU_UPLOAD_FLASH_BLOCK0_BUS -
                           TI_C54X_MCU_UPLOAD_FLASH_BUS_BASE) /
                    UINT32_C(2),
                (uint32_t)TI_C54X_MCU_UPLOAD_FLASH_STRIDE_BYTES /
                    UINT32_C(2),
                (uint32_t)block_limit);
        if (peer_status != TI_C54X_MCU_UPLOAD_PEER_OK) {
            (void)fprintf(stderr,
                          "error: mcu upload peer init failed (%d)\n",
                          (int)peer_status);
            goto done;
        }
        bus.have_mcu_peer = true;
    }
    if (want_port_peer) {
        if (ti_c54x_port_peer_init(&bus.port_peer) !=
            TI_C54X_PORT_PEER_OK) {
            (void)fprintf(stderr, "error: port peer init failed\n");
            goto done;
        }
        bus.have_port_peer = true;
    }
    endpoint.context = &bus;
    endpoint.advance = run_image_advance;
    endpoint.cancel = run_image_cancel;
    bus_desc.context = &bus;
    bus_desc.program_read_word = run_image_program_read;
    bus_desc.program_write_word = NULL;
    bus_desc.data_endpoint = endpoint;
    bus_desc.io_read_word = run_image_io_read;
    bus_desc.io_write_word = run_image_io_write;
    /* Warm-reset entry (P3-S90): silicon forces PC to the
     * vector-table base. Only the CPU re-inits there; the whole
     * bus (program, data, peers, overlay, coverage) survives. */
    for (phase = 1U;; phase++) {
        /* The watch journal reflects the current phase, like the
         * first-fault latches. */
        bus.watch_total = 0U;
        bus.watch_journaled = 0U;
        bus.watch_overflow = false;
        if (phase == 2U) {
            (void)printf("warm_boot phase=2 entry=0x%04" PRIX32
                         " budget=%" PRIu64 "\n",
                         (uint32_t)WARM_BOOT_ENTRY_WORD, warm_budget);
            entry = (uint32_t)WARM_BOOT_ENTRY_WORD;
            budget = warm_budget;
            bus.have_data_fault = false;
            bus.have_io = false;
        }
        if (phase_begin(&bus_desc, entry, &storage, &cpu) != 0) {
            (void)fprintf(stderr, "error: cpu init failed\n");
            goto done;
        }
        result = ti_c54x_cpu_run(cpu, budget);
        visited_count = 0U;
        for (i = 0U; i < program_count; i++) {
            visited_count += (uint64_t)visited[i];
        }
        if (bus.have_data_fault) {
            (void)snprintf(data_fault_text, sizeof(data_fault_text),
                           "0x%04" PRIX32, bus.first_data_fault);
        } else {
            (void)snprintf(data_fault_text, sizeof(data_fault_text), "none");
        }
        if (bus.have_io) {
            (void)snprintf(io_text, sizeof(io_text), "0x%04" PRIX32,
                           bus.first_io_address);
        } else {
            (void)snprintf(io_text, sizeof(io_text), "none");
        }
        (void)printf("stop=%s bus=%s attempted=%" PRIu64 " retired=%" PRIu64
                     " pc=0x%" PRIX32 " opcode=%s0x%04" PRIX16
                     " prog_reads=%" PRIu64 " prog_unmapped=%" PRIu64
                     " data_txn=%" PRIu64 " data_faults=%" PRIu64
                     " io_calls=%" PRIu64 " prog_covered=%" PRIu64
                     "/%" PRIu32 " prog_writes=%u first_data_fault=%s"
                     " first_io=%s\n",
                     stop_reason_name(result.reason),
                     bus_status_name(result.bus_status),
                     result.instructions_attempted,
                     result.instructions_retired, (uint32_t)result.pc_word,
                     result.opcode_valid ? "" : "invalid ",
                     result.opcode_valid ? result.opcode_word : UINT16_C(0),
                     bus.program_reads, bus.program_unmapped,
                     bus.data_transactions, bus.data_faults, bus.io_calls,
                     visited_count, program_count, bus.program_writes,
                     data_fault_text, io_text);
        if (bus.have_mcu_peer) {
            (void)printf("mcu_upload fills_a=%" PRIu32 " fills_b=%" PRIu32
                         " fills_done=%" PRIu32 "\n",
                         bus.mcu_peer.fills_a, bus.mcu_peer.fills_b,
                         bus.mcu_peer.fills_done);
        }
        if (bus.program_writes > 0U) {
            (void)printf("prog_overlay writes=%u last=0x%04" PRIX32 ":0x%04" PRIX16
                         "\n",
                         bus.program_writes, bus.last_program_write_address,
                         bus.last_program_write_value);
        }
        if (bus.have_port_peer) {
            (void)printf("port_peer writes=%" PRIu32 " reads=%" PRIu32
                         " journaled=%" PRIu32 " overflow=%u\n",
                         bus.port_peer.writes, bus.port_peer.reads,
                         bus.port_peer.journaled,
                         bus.port_peer.overflow ? 1U : 0U);
            for (i = 0U; i < bus.port_peer.journaled; i++) {
                (void)printf("port_write port=0x%04" PRIX16 " value=0x%04" PRIX16
                             "\n",
                             bus.port_peer.journal[i].port,
                             bus.port_peer.journal[i].value);
            }
        }
        {
            /* Interrupt-wait diagnosis (P3-S89): the mask/flag state
             * the stop parks in. */
            ti_c54x_state view = {0};
            char intm_text[8] = {0};
            char ifr_text[8] = {0};

            if (ti_c54x_cpu_inspect(cpu, &view) == TI_C54X_ERROR_OK) {
                if (view.intm_known) {
                    (void)snprintf(intm_text, sizeof(intm_text), "%u",
                                   view.intm_value);
                } else {
                    (void)snprintf(intm_text, sizeof(intm_text), "unknown");
                }
                if (view.ifr_known) {
                    (void)snprintf(ifr_text, sizeof(ifr_text), "0x%04" PRIX16,
                                   view.ifr_value);
                } else {
                    (void)snprintf(ifr_text, sizeof(ifr_text), "unknown");
                }
            } else {
                (void)snprintf(intm_text, sizeof(intm_text), "error");
                (void)snprintf(ifr_text, sizeof(ifr_text), "error");
            }
            (void)printf("irq intm=%s ifr=%s\n", intm_text, ifr_text);
        }
        if (want_watch) {
            (void)printf("data_watch base=0x%04" PRIX32 " count=%" PRIu32
                         " watched=%" PRIu32 " journaled=%" PRIu32
                         " overflow=%u\n",
                         watch_base, watch_count, bus.watch_total,
                         bus.watch_journaled,
                         bus.watch_overflow ? 1U : 0U);
            for (i = 0U; i < bus.watch_journaled; i++) {
                (void)printf("data_watch addr=0x%04" PRIX32 " %s"
                             " value=0x%04" PRIX16 "\n",
                             bus.watch_journal[i].address_word,
                             bus.watch_journal[i].is_write ? "W" : "R",
                             bus.watch_journal[i].value_word);
            }
        }
        phase_end(&storage, &cpu);
        if (phase == 1U && want_warm_boot) {
            continue;
        }
        /* A dump reflects the final phase only. */
        if (want_dump) {
            /* Runtime program dump (P3-S89): overlay-aware words
             * for vector/ISR diagnosis. */
            if (dump_base < prog_base ||
                dump_count > program_count ||
                dump_base - prog_base > program_count - dump_count ||
                dump_base > UINT32_C(0xFFFF) - dump_count) {
                (void)fprintf(stderr, "error: dump range outside image\n");
                goto done;
            }
            (void)printf("prog_dump base=0x%04" PRIX32 " count=%" PRIu32 "\n",
                         dump_base, dump_count);
            for (i = 0U; i < dump_count; i++) {
                uint32_t address = dump_base + i;
                uint16_t word =
                    bus.program_words[address - prog_base];

                /* Runtime fetch prefers overlay-written words. */
                if (bus.program_overlay_written != NULL &&
                    bus.program_overlay_written[address] !=
                        (unsigned char)0) {
                    word = bus.program_overlay[address];
                }
                (void)printf("0x%04" PRIX32 ": 0x%04" PRIX16 "\n", address,
                             word);
            }
        }
        rc = 0;
        break;
    }

done:
    if (cpu != NULL) {
        (void)ti_c54x_cpu_deinit(cpu);
    }
    free(storage);
    free(visited);
    free(ram);
    free(program);
    free(data);
    free(flash);
    free(bus.program_overlay);
    free(bus.program_overlay_written);
    return rc;
}
