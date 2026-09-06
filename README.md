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

- Production decode and CPU execution now cover ten declarative one-word
  opcode rows: evidence-backed `NOP`; the three immediate status-field forms
  `LD #k9,DP`, `LD #k5,ASM`, and `LD #k3,ARP`; and `MAX`, `MIN`, `CMPL`, `ROL`,
  `ROR`, and `SFTL`. Those rows expand to their encoded accumulator forms. The
  catalog now has 693 populated first-word encodings; that is a lookup-population
  count, not 693 distinct instructions. Exact/masked
  catalog generation, complete instruction bundles, extension fetch, operand
  extraction, dispatch, PC staging, and retirement are green under the full
  local standalone gate; remote P2-W26 validation is pending.
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
  remains the four reset-runnable NOP/LD rows out of nine.
- P2-W26 now proves SFTL's generic source/destination/shift5 layout, signed
  `-16..15` handling, low-32 shift semantics with explicit C output, guard
  clearing, and atomic retirement through existing generic effects. The full
  local P2-W26 gate is green. No `cpu.c` change, public API, memory,
  RPT/IRQ, or timing claim was added; reset-runnable coverage remains four of
  ten rows.
- The registry now records 98 evidence items, 144 requirements (116 implemented,
  9 planned, 19 unresolved), and 105 tests (96 passing, 9 planned). P2-W26's
  full local validation is green; remote validation is pending. The latest clean
  catalog-nine hosted
  performance record is captured at `705ac7b`: NOP 61.839 ns / 16.171 MIPS,
  LD-DP 91.592 ns / 10.918 MIPS, LD-ASM 91.619 ns / 10.915 MIPS, LD-ARP
  96.653 ns / 10.346 MIPS, and mixed-current 87.562 ns / 11.420 MIPS with
  reset-runnable coverage of 4/9.
- Immediate `RPT` layouts, effects, and repeat policy are proved but remain inert;
  unresolved RC/PC/IRQ details are not guessed.
- The standalone DSP foundation includes checked 40-bit arithmetic and addressing,
  explicit WORD16 requests and BIG/LITTLE conversion, and a callback-free resumable
  transaction reducer. External memory visibility, arbitration, and timing remain
  demand-driven work.
- Speculative foundation expansion is frozen. The active track is end-to-end ISA
  vertical slices, adding infrastructure only when a concrete opcode family needs
  it. Emulator integration remains deferred.
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
  the catalog-seven `7cad341` record measured 14.481 MIPS for NOP and 11.162
  MIPS for `mixed-current`; the latest catalog-nine `705ac7b` record is listed
  above. Catalog-nine fixed-scenario deltas versus catalog-seven range from
  -10.45% to -0.20%, with within-capture spreads of 6.05% to 29.42%; these are
  hosted noise/trend observations—not confirmed improvements or regressions and
  not architectural/silicon timing claims. The changed mixed workload is not
  compared. See [hosted performance history](docs/performance.md).

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
