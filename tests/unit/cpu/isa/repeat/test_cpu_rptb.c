/* Module contract
 * Owns: public-CPU RED proof for RPTB block repeat
 * (P3-S79, TI SPRU172C section RPTB[D] with Z=0):
 * `RPTB 0x105` with BRC=1 retires the four-NOP block
 * twice (RSA/REA programmed, BRAF set then cleared,
 * BRC drained), landing past the block end.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/mmr_file.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define RPTB_WORD0 UINT16_C(0xF072)
#define RPTB_END UINT16_C(0x0105)
#define RPTB_NOP UINT16_C(0xF495)

typedef struct rptb_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
} rptb_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    rptb_fixture *fixture = (rptb_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    return TI_C54X_BUS_OK;
}

static void
test_block_repeats_brc_plus_one_passes(void)
{
    /* RPTB 0x105 with BRC=1: block 0x102..0x105 runs
     * twice, then falls through to 0x106. */
    rptb_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    const ti_c54x_bus bus = {.context = &fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {UINT32_C(0x0100), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    uint16_t rsa = UINT16_C(0);
    uint16_t rea = UINT16_C(0);
    uint16_t brc = UINT16_C(0xBEEF);

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = RPTB_WORD0;
    fixture.words[1] = RPTB_END;
    fixture.words[2] = RPTB_NOP;
    fixture.words[3] = RPTB_NOP;
    fixture.words[4] = RPTB_NOP;
    fixture.words[5] = RPTB_NOP;
    fixture.words[6] = RPTB_NOP;
    fixture.word_count = 7U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_brc(&cpu->arch_state.mmr_file,
                                                 UINT16_C(1)) ==
                       TI_C54X_MMR_FILE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(10));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(10));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(10));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0107));
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_read_rsa(&cpu->arch_state.mmr_file,
                                                &rsa) ==
                       TI_C54X_MMR_FILE_OK);
    TI_C54X_TEST_CHECK(rsa == UINT16_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_read_rea(&cpu->arch_state.mmr_file,
                                                &rea) ==
                       TI_C54X_MMR_FILE_OK);
    TI_C54X_TEST_CHECK(rea == RPTB_END);
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_read_brc(&cpu->arch_state.mmr_file,
                                                &brc) ==
                       TI_C54X_MMR_FILE_OK);
    TI_C54X_TEST_CHECK(brc == UINT16_C(0));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        TI_C54X_ST1_BRAF_MASK) == UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_block_repeats_brc_plus_one_passes();
    return 0;
}
