### P2-W25: generic input collection and ROL/ROR vertical slice (planned)

State: implemented. P2-W25 is a two-step rotate-through-C slice. The CPU's
resource-driven input reads now sit behind a private generic collector, and ROL
and ROR consume that collector and the existing `ACCUMULATOR_WRITE` plus
`C_WRITE` effect path. No new effect, sequencer, or public state API was
introduced.

Step 1 delivered the generic input contract:

- Added a generic `STATUS_C` input-resource bit beside `ACCUMULATORS`. The copied
  `ti_c54x_op_input_snapshot` owns typed C value/knownness (`status_c` and
  `status_c_known`) alongside A/B raw40 values and knownness. Unrequested resources
  are canonical zero/false; requested A/B unknownness is data, not collector
  failure.
- Collection now lives in `src/cpu/input/collect.{c,h}`, mirrored by
  `tests/unit/cpu/input/test_collect.c`. The collector is transactional: it
  rejects unknown resource-mask bits and invalid requested backing pointers,
  reads only requested resources, reads C exactly once when requested, and
  leaves the caller's output unchanged on rejection. It owns no globals,
  environment reads, allocation, logging, or diagnostic callbacks.
- `src/cpu/cpu.c` now makes one generic collector call for every instruction. No
  opcode-identity branch is permitted, and CPU orchestration must shrink as the
  private collector takes ownership of input gathering.

Step 2 delivered ROL/ROR and their four forms:

- `ROL` is value `0xF491`, mask `0xFEFF`; `ROR` is value `0xF490`, mask
  `0xFEFF`. Each is one program word with S at bit 8, yielding exactly
  A-in-place and B-in-place forms. `src/isa/decode/` owns a generic
  `ACCUMULATOR_SOURCE` layout extracting S from word 0 bit 8; no rotate-named
  field parser is allowed.
- The catalog rows declare `ACCUMULATORS | STATUS_C` input resources. The
  pure handlers live under `src/isa/ops/rotate/` and consume the selected source
  plus pre-C. ROL shifts the selected low 32 bits left, inserts pre-C at bit 0,
  and writes old bit 31 to C. ROR shifts right, inserts pre-C at bit 31, and
  writes old bit 0 to C. Guard bits 39:32 are cleared in the destination;
  no other status field changes.
- Unknown selected source or unknown pre-C blocks retirement transactionally;
  unknownness in the unselected accumulator is allowed. The generic collector
  preserves both A/B knownness, and the existing accumulator-write sequencer
  validates and commits the destination plus C atomically. PC `0xffff` boundary
  rejection also rolls back all architectural output.
- Ownership mirrors the tree: `src/isa/catalog/` owns rows and resource bits,
  `src/isa/decode/` owns source extraction, `src/isa/ops/rotate/` owns pure
  semantics, `src/cpu/input/` owns collection, `src/sequencer/accumulator/`
  remains the effect owner, and `src/cpu/` only orchestrates generic collection
  and retirement. Tests mirror these owners under
  `tests/unit/cpu/input/`, `tests/unit/isa/decode/`,
  `tests/unit/isa/ops/rotate/`, and `tests/unit/cpu/`.

Catalog metadata and generated-lookup proof remain separately owned under
`tests/unit/isa/catalog/`; the decode test links only the decode-owned layout
requirement so normative ownership does not cross module boundaries.

The passing TDD vectors cover C=0 and C=1 for both lanes; low-32 inputs
`0x00000000`, `0x00000001`, `0x7fffffff`, `0x80000000`, and `0xffffffff`;
guard-only and mixed raw40 values; canonical input/decode rejection with output
nonmutation; all four CPU forms; non-C status preservation; unknown selected/C
rollback; allowed unselected unknownness; PC-`0xffff` rollback; and exactly one
attempt, one retirement, and PC+1 for admitted forms. Decode coverage is
exhaustive over all 65,536 first words and must classify exactly
`0xF491`, `0xF591`, `0xF490`, and `0xF590`.

TI `SPRU172C` §4 ROL (printed p. 4-143) and ROR (printed p. 4-145) are the
normative form/effect sources. Their descriptions support the low-32
rotate-through-pre-C behavior, old endpoint-bit-to-C result, and guard-bit
clearing; no summary one-cycle value is admitted as machine timing evidence.
GNU binutils 2.42 commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c`, is nonnormative encoding corroboration only and must not
be copied or vendored. Table 2-28 does not positively authorize ROL or ROR
repetition, so repeatability remains `UNRESOLVED`; no RPT/IRQ, memory, device,
emulator, or timing behavior is part of this work piece.

Catalog quality now reports nine declarative rows and 565 populated encoded
words; that count is not an instruction count. The mixed reset-runnable workload
remains four of nine rows. The collector proof, four-form decode, pure rotate
semantics, generic effect transaction, and CPU retirement proof all pass without
CPU opcode branching or CPU file growth. Repeatability remains unresolved from
the Table 2-28 boundary, and no machine-timing contract is claimed.
