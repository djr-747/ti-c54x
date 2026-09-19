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

#include "mcu_upload_peer.h"

typedef struct run_image_bus {
    const uint16_t *program_words;
    uint32_t program_base;
    uint32_t program_count;
    uint16_t *data_words;
    uint32_t data_base;
    uint32_t data_count;
    ti_c54x_mcu_upload_peer_state mcu_peer;
    bool have_mcu_peer;
    /* Writable program overlay: MVDP/WRITA-style PROGRAM writes
     * land here and are visible to the fetch callback, so the
     * boot can copy code into program space. Reads fall back
     * to the loaded image; never-written, never-loaded words
     * still fault explicitly. */
    uint16_t *program_overlay;
    unsigned char *program_overlay_written;
    unsigned program_writes;
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
                  "\n"
                  "Runs one program-word image from <entry> for <budget>\n"
                  "retirements and prints a stop report. Bases, entry,\n"
                  "and budget accept C integer syntax (0x hex or\n"
                  "decimal). Unmapped program/data words stop the run\n"
                  "explicitly; nothing is invented. The trailing\n"
                  "--mcu-upload form attaches the MCU upload peer\n"
                  "(1-64 flash blocks); without a data image the peer\n"
                  "owns a zeroed 0x800-0x2800 DATA window.\n",
                  argv0, argv0);
}

/* In-memory selftest: no files, two fixed vectors. */
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

    if (failures == 0) {
        (void)printf("selftest: PASS (2 vectors)\n");
    }
    return failures == 0 ? 0 : 1;
}

int
main(int argc, char **argv)
{
    run_image_bus bus;
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_bus bus_desc;
    ti_c54x_config config;
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
    unsigned char *visited = NULL;
    uint16_t *ram = NULL;
    unsigned char *storage = NULL;
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    uint64_t visited_count = 0U;
    uint32_t i = 0U;
    char data_fault_text[16] = {0};
    char io_text[16] = {0};
    int rc = 1;

    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) {
        return selftest();
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
    endpoint.context = &bus;
    endpoint.advance = run_image_advance;
    endpoint.cancel = run_image_cancel;
    bus_desc.context = &bus;
    bus_desc.program_read_word = run_image_program_read;
    bus_desc.program_write_word = NULL;
    bus_desc.data_endpoint = endpoint;
    bus_desc.io_read_word = run_image_io_read;
    bus_desc.io_write_word = run_image_io_write;
    config.reset_pc_word = (ti_c54x_program_word_address)entry;
    memset(&config.trace, 0, sizeof(config.trace));
    storage = (unsigned char *)malloc(ti_c54x_cpu_size());
    if (storage == NULL) {
        (void)fprintf(stderr, "error: cpu storage alloc failed\n");
        goto done;
    }
    if (ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                         &bus_desc, &cpu) != TI_C54X_ERROR_OK) {
        (void)fprintf(stderr, "error: cpu init failed\n");
        goto done;
    }
    result = ti_c54x_cpu_run(cpu, budget);
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
    rc = 0;

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
