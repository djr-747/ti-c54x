# ti-c54x

`ti-c54x` is a standalone C11 implementation of the TI C54x DSP foundations. It
is being developed as a small, test-first library that can eventually be vendored
by emulators and other tools without pulling in a phone model, firmware image, or
host application.

The repository is intentionally independent of the Nokia DCT3 emulator. The
emulator may provide optional black-box traces and externally supplied DSP images
for later validation, but it is not a build or test dependency. Integration is
deferred until the standalone contracts and provenance review are complete.

## Status

Foundations are under active development. The public API, supported C54x variant,
opcode inventory, and distribution license are not final. In particular, the
provenance and licensing review is still open; this repository does not currently
make a license-selection or compatibility claim. See the [provenance review](docs/provenance.md),
[architecture baseline](docs/architecture-baseline.md), and [evidence index](docs/evidence-index.md).
The machine-readable [`config/evidence.json`](config/evidence.json) registry is checked by
the standalone Python quality gate.

Current progress:

- Production decode and CPU execution now cover 22 declarative opcode rows:
  evidence-backed `NOP`; the three immediate status-field forms
  `LD #k9,DP`, `LD #k5,ASM`, and `LD #k3,ARP`; and `MAX`, `MIN`, `CMPL`, `ROL`,
  `ROR`, `SFTL`, `AND`, `OR`, `XOR`, `ROLTC`, `SFTC`, and `EXP`. The logical
  families add 384
  register-accumulator encodings with full-40-bit Boolean/shift semantics and
  preserve C and all other status. Six two-word immediate AND/OR/XOR forms now
  add 204 populated first-word encodings, for 1287 total; that is a
  lookup-population count, not an instruction count. Exact/masked
  catalog generation, complete instruction bundles, extension fetch, operand
  extraction, dispatch, PC staging, and retirement are covered by the standalone
  test surfaces. Repeatability and machine timing remain unresolved; no `cpu.c`,
  public API, memory, RPT/IRQ, or timing change was needed for the six immediate
  rows.
- P2-W23 now adds four exact MAX/MIN accumulator encodings (`0xF486`, `0xF586`,
  `0xF487`, `0xF587`) with signed full-40-bit selection, tie/C semantics,
  checked accumulator knownness, transactional sequencer commit, and public CPU
  retirement proof. The full standalone local gate is green; repeatability and
  machine timing remain unresolved.
- P2-W24 now generalizes the accumulator effect to explicit C `PRESERVE`/`WRITE`
  actions and proves all four CMPL source/destination forms with exhaustive
  decode, full-40-bit one's-complement semantics, and atomic CPU retirement.
  Repeatability and machine timing remain unresolved; no silicon timing claim is
  made.
- P2-W25 now provides a generic resource-driven input collector with
  `ACCUMULATORS | STATUS_C`, typed `status_c`/`status_c_known`, transactional
  unknownness handling, and no opcode-specific CPU branch. ROL/ROR use the
  existing accumulator-write plus C-write effect path; benchmark coverage
  remains the four reset-runnable NOP/LD rows out of 13.
- P2-W26 now proves SFTL's generic source/destination/shift5 layout, signed
  `-16..15` handling, low-32 shift semantics with explicit C output, guard
  clearing, and atomic retirement through existing generic effects. The full
  local P2-W26 gate is green. No `cpu.c` change, public API, memory,
  RPT/IRQ, or timing claim was added; reset-runnable coverage is four of 13
  rows.
- P2-W28A establishes typed TC status access, resource-driven TC collection, and
  an atomic value-owned C+TC status delta; the full local gate is green and
  [remote CI is green](https://github.com/djr-747/ti-c54x/actions/runs/34058489923).
  P2-W28B adds the
  ROLTC/SFTC handlers and CPU proofs through the existing generic path: targeted
  local checks, independent review, and the full `make test` gate are green.
  Catalog coverage is now
  15 rows/1081 populated encodings with reset-runnable coverage at 4/15; no
  `cpu.c`, public API, memory, RPT/IRQ, timing, knob, diagnostic, or environment
  change was added. The central generator schema was minimally extended for TC
  resource tokens. Repeatability remains unresolved; the distinct SFTC
  guard-only-plus-low32-zero behavior boundary is also unresolved.
- P2-W29 is implemented at `fedbd6a` and ledger-closed at `96d86ed`: all RED
  contracts, focused tests, two independent reviews, the full `make test` gate,
  and [remote
  CI](https://github.com/djr-747/ti-c54x/actions/runs/34062796872) are green. The
  slice adds a private, explicitly unknown 16-bit T register and candidate-only
  tagged effect transaction, then
  EXP_ACC/exp_acc through the existing accumulator-source path. State, T-effect,
  shared transaction, family, catalog/decode, CPU-proof, and orchestrator
  ownership is fixed; any architecture gap is reported rather than hidden in a
  family workaround. The plan excludes public T/MMR access, T input, OVA/OVB,
  OVM/SXM, memory, RPT/IRQ, parallel issue, timing, diagnostics, environment,
  allocation, and emulator integration. Catalog coverage is now 16 rows and
  1083 populated words, with reset-runnable coverage at 4/16; EXP repeatability
  remains unresolved.
- P2-W30 implements six two-word immediate AND/OR/XOR accumulator forms. The
  rows add 204 populated first words, taking the catalog to 22 rows/1287
  populated words while reset-runnable coverage remains 4/22. Existing
  extension fetch, accumulator input/effect, tagged transaction, and CPU
  retirement seams are reused; common computation and the three family leaves
  have explicit mirrored ownership. Focused strict and sanitizer tests, two
  independent reviews, and the full local test gate are green. Repeatability
  and timing remain unresolved, and any new shared seam is a reported GAP.
- P2-W31A implements the canonical CPU-owned `ti_c54x_arch_state`, private
  checked SXM read/knownness, and generic `STATUS_SXM` input collection. Focused
  strict and sanitizer tests, two independent reviews, the full standalone
  local gate, and remote CI are green; its clean same-catalog-22 performance
  record is captured at `66ef384`. No opcode/catalog changes, memory effects,
  or memory/public-ABI behavior are claimed; those remain separate P2-W31B+
  evidence freezes.
- P2-W31B1 implements the copied public DATA WORD16 endpoint, canonical
  BEGIN/RESUME/WAIT/READY/FAULT protocol, cancellation retries, commit-at-READY
  visibility, pure MOD2 address resolution, persistent instruction work, and
  CPU resume-before-fetch. Observer-disabled builds compile the new trace sites
  out; focused sanitizer checks, independent review, and the full standalone
  local gate are green. W31B2 is now the active next ISA slice: fixed-MOD2
  `LD *ARx+,dst` and `STL src,*ARx+` will use the proven generic memory path.
  Timing, repeatability, AR-boundary wrap, device/MMR/program/IO, dual/parallel,
  and posted writes remain out of scope.
- P2-W31B2 now registers the fixed-MOD2 `LD *ARx+,dst` and `STL src,*ARx+`
  catalog/decode rows and their pure generic MEMORY effects. Catalog coverage
  is now 24 rows and 1319 populated first words, with reset-runnable coverage
  still 4/24. The implementation and CPU end-to-end retirement are green;
  focused/full/sanitizer review closure and the immediate-READY benchmark
  follow-up remain pending. The open W31P hosted stability follow-up remains
  separate.
- P2-W31P remains open as the performance architecture freeze. It replaces
  repeated hot-path metadata/layout validation with one catalog-derived per-ID
  binding, one private value-owned trusted execution packet, one
  operand-extraction pass, and one PC plan while retaining checked forged-input
  APIs and transaction atomicity. Structural/correctness gates are green and
  optimized hosted cohorts are below 60 ns per retired instruction, but exact
  three-capture stability acceptance remains pending/deferred; this is advisory
  hosted throughput evidence, not a CI or DSP timing claim. The open hosted
  follow-up no longer blocks W31B2.
- The registry now records 130 evidence items, 197 requirements (166 implemented,
  8 planned, 23 unresolved), and 157 tests (149 passing, 8 planned). Hosted
  throughput history and its noise-qualified catalog-twenty-two baseline are
  summarized below; these measurements are not DSP or silicon timing claims.
- Immediate `RPT` layouts, effects, and repeat policy are proved but remain inert;
  unresolved RC/PC/IRQ details are not guessed.
- The standalone DSP foundation includes checked 40-bit arithmetic and addressing,
  explicit WORD16 requests and BIG/LITTLE conversion, a resumable reducer, and a
  host-owned transactional DATA endpoint with proven exactly-once READY
  publication. Device mapping, arbitration, and timing remain demand-driven work.
- Speculative foundation expansion is frozen. The active track is the
  evidence-backed P2-W31B2 memory vertical slice while the P2-W31P hosted
  stability follow-up remains open. Emulator integration remains deferred.
- `ti_c54x_state` may grow observation fields before API stability; these fields
  are not a raw snapshot or persistence ABI, and no stable persistence guarantee
  is made.
- Hosted performance history is implemented and covered by the full local and
  remote gates:
  observer-off public-bus throughput, stable `nop`/`ld-dp`/`ld-asm`/`ld-arp`
  scenarios, catalog-aware mixed workload, seven samples each preceded by 16
  unrecorded warmup windows, and clean-commit append-only JSONL. The initial
  clean baseline at `aa0afc0` records 15.62 MIPS for NOP and 11.39 MIPS for
  `mixed-current`. The catalog-six `a14d331` record retains 15.26 MIPS for NOP;
  the catalog-seven `7cad341`, catalog-nine `705ac7b`, catalog-ten `cd88596`,
  catalog-thirteen `ef87c94`, catalog-fifteen `7164ace`, and catalog-sixteen
  `fedbd6a` records remain as prior cohorts. The prior clean catalog-twenty-two
  record at `9c22a33` measured NOP 131.883 ns / 7.582 MIPS, LD-DP 233.294 ns /
  4.286 MIPS, LD-ASM 240.304 ns / 4.161 MIPS, LD-ARP 206.770 ns / 4.836 MIPS,
  and mixed-current 109.273 ns / 9.151 MIPS with reset-runnable coverage of
  4/22. The current clean same-catalog record at `66ef384` measures NOP 111.770
  ns / 8.947 MIPS, LD-DP 116.084 ns / 8.614 MIPS, LD-ASM 117.836 ns / 8.486
  MIPS, LD-ARP 159.492 ns / 6.270 MIPS, and mixed-current 170.433 ns / 5.867
  MIPS, with the same semantic hashes and binary hash prefix `b0c7d039`.
  The workloads/catalog are unchanged; recorded spreads and an identical-binary
  diagnostic show no confirmed regression or improvement. See [hosted
  performance history](docs/performance.md).

See the [implementation plan](docs/implementation-plan.md) and
[development ledger](docs/development-ledger.md) for detailed status and evidence.

## Build and test

The C-only compile/link path requires only a C11 compiler and `make`:
`make build` does not invoke CMake, Python, the emulator, firmware, or a DSP
image. The default `make test` gate additionally runs the repository's Python
3 standard-library quality checkers and therefore requires a `python3`-compatible
interpreter (override it with `PYTHON=/path/to/python`). CMake 3.21 or newer and
CTest are used for packaging, IDE/CI validation, and the same Python quality
checks when tests are enabled.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The equivalent fast path is:

```sh
make test
```

For the compiler-only path, use:

```sh
make build
```

Useful options are:

```text
TI_C54X_BUILD_TESTS=ON             standalone unit tests (default)
TI_C54X_BUILD_EXAMPLES=OFF        minimal consumer example (default while API settles)
TI_C54X_ENABLE_SANITIZERS=OFF     AddressSanitizer + UBSan where supported
TI_C54X_ENABLE_OBSERVERS=OFF      structured lifecycle observers (test/diagnostic builds)
TI_C54X_WARNINGS_AS_ERRORS=ON     strict warnings (default)
TI_C54X_BUILD_BENCHMARKS=OFF      hosted observer-off performance tooling (no architectural effect)
PYTHON=python3                     quality-check interpreter used by `make test`
```

For example:

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTI_C54X_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Observer callbacks receive borrowed, structured event values only. The core
copies the event mask and sink descriptor during initialization; the sink
context and callback must remain valid until deinitialization. Callbacks must
not re-enter or mutate the running CPU. Observer support is disabled in the
default library build and is exercised by a separate enabled test variant.

The install tree exports the CMake package target `ti_c54x::core`. A consumer
should link that target and include only the installed public headers; internal
implementation headers are not part of the vendor surface.

The current implementation lineage and evidence boundary are pinned in
[`docs/provenance.md`](docs/provenance.md); the distribution-license decision
remains open pending the documented provenance review.

## Design boundaries

The core owns architectural state and instruction semantics. A caller owns memory,
I/O, device timing, logging, and persistence through explicit interfaces. Runtime
state is instance-owned: there are no environment reads, file loads, mutable
process-wide emulator hooks, or hidden firmware paths in the execution path.

Requirements are recorded with evidence identifiers and are accompanied by
executable tests. Pure arithmetic and address helpers are tested before opcode
families; decode coverage, sequencing, interrupt behavior, and timing are added as
their specifications become pinned. The dispatcher follows the mGBA pattern of a
declarative instruction definition expanded into generated wrappers and dispatch
tables, while keeping semantic implementations readable and independently
testable. The [architecture baseline](docs/architecture-baseline.md) describes
the current ownership contracts, while the [evidence index](docs/evidence-index.md)
maps requirements to their sources and tests. `config/evidence.json` is the
machine-checked counterpart; it is validated without emulator or firmware gates.

## External inputs

Tests that need a firmware or DSP image will accept an explicit path and skip when
one is not supplied. Images are never committed, downloaded, or embedded by the
build. This keeps a clean checkout fast and reproducible.
