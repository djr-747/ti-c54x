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
opcode inventory, and distribution license are not final. Provenance and licensing
review remain open; this repository makes no license-selection or compatibility
claim. See the [provenance review](docs/provenance.md), [architecture baseline](docs/architecture-baseline.md),
and [evidence index](docs/evidence-index.md). The machine-readable
[`config/evidence.json`](config/evidence.json) registry is checked by the standalone
Python quality gate.

Current snapshot: P3-S34 complete (S3t–S3x R-variant lock-ins through the
shared `MAC`/`MAS`/`MACA`/`MASA`/`MPY` rows; S3z `ADD` Smem syntaxes 1–3;
S4 `XC`; S10–S13 `STH` stores; S14–S26 `SUB`/`AND`/`OR`/`XOR`/`SUBB`/`SUBS`
Smem rows; Wave-5 `ADD`/`SUB` leaf fan-out with S27–S29 `#lk`/`Xmem`/src-ASM
rows; Wave-6 `MPY`/`MAC` `#lk` leaves with S30 rows; S31 `ADD`/`SUB` Smem
SHIFT rows; S32 `ADDM` RMW-ADD seam; S33 `MPY` syn3 / `MAC` syn4 Smem-`#lk`
rows; S34 `RND`/`SAT` accumulator rows with the `WRITE_0` overflow seam;
S35 `MPYR`/`MACR`/`MACAR`/`MASR`/`MASAR` shared-row bindings).

- 279 runnable catalog rows under 29,437 populated first-word lookup entries.
  Inventory coverage is 127/205 forms SUBSET-or-better (61.951%) and 40/205
  strict-complete (19.512%); lookup encodings are not instruction counts.
- The public CPU uses immutable direct lookup and trusted bindings on first
  execution; there is no seen-before promotion. Performance remains a release
  constraint: one-pass trusted packets, allocation-free opcode leaves, and
  dedicated observer-off CPU benchmarks. These are host-performance checks,
  not TI machine-cycle claims.
- P3 works the Nokia 5110 DSP-image census seam-first. Wave-1 tail control
  flow is done (branch/return families with delayed companions, RC/RCD,
  RETF, XC conditional execution). The S1 MMR/device routing seam unlocks the S7 `STM` immediate-MMR
  store. S2 activates single-instruction repeat (`RPT_K`/`RPT_LK`,
  `INELIGIBLE`): CPU progress plus pending latch, predecode IRQ arbitration,
  and a body-eligibility gate. S3a adds the pure multiply-accumulate datapath
  core (signed 16x16 product, FRCT shift, R rounding, OVM saturation, SMUL-0
  baseline) with the `STATUS_FRCT` input resource. S3b lands the `MAC` Smem
  row (`FORM-T2-04-01`) with `ELIGIBLE` classification and `RPT MAC`
  end-to-end replay through the retirement-tail HOLD wiring. S3c lands the
  `MPY` Smem row (`FORM-T2-03-01`) reusing the shared
  round/select/indirect/addr7 layout with STORE completion (TI MPY Examples
  1 and 4 retire through the leaf). S3d lands the `MPYA` Smem row
  (`FORM-T2-03-06`) with the new indirect/addr7 layout, fixed-B STORE of
  Smem times A-high, and the read-phase T-load seam (TI MPYA Example 1
  retires through the leaf). S3e lands the `MAS` Smem row
  (`FORM-T2-04-14`) reusing the shared layout with SUBTRACT completion
  and result-after-subtract rounding (TI MAS Examples 1 and 3 retire
  through the leaf). S3g lands the `MAC` dual `Xmem`,`Ymem` rows
  (`FORM-T2-04-04`, nine MOD0–MOD2 combinations) with a combined
  RSD/XAR/YAR layout, read-phase T load of the X-side word, and
  `(Xmem)`x`(Ymem)`+`(src)` completion (TI MAC Examples 4 and 6
  retire through the leaf with AR2–AR5 dual addressing). S3h lands
  the `MPY` dual `Xmem`,`Ymem` rows (`FORM-T2-03-03`, nine MOD0–MOD2
  combinations) reusing the shared dual layout with STORE overwrite
  and read-phase T load (TI MPY Example 2 retires through the leaf). S3i lands
  the `MAS` dual `Xmem`,`Ymem` rows (`FORM-T2-04-16`, nine MOD0–MOD2
  combinations) reusing the RSD/XAR/YAR layout with `(src)` minus
  `(Xmem)`x`(Ymem)` completion and read-phase T load (the TI MAS Example
  2/4-derived silicon vectors retire through the leaf; TI's printed
  accumulators equal the FRCT=0 computation, and a dedicated lane pins
  that value while FRCT=1 executes the SPRU131-shifted product). S3j lands
  the register-only `MPYA dst` row (`FORM-T2-03-07`, both D encodings
  under one masked row) reusing the MAX/MIN single-destination-bit layout
  with `(T)`x`(A(32-16))` STORE completion and no T update (TI MPYA
  Example 2 retires through the leaf with A and T preserved). S3k lands
  the `SQUR` Smem row (`FORM-T2-03-09`, 512 D/I/addr7 encodings under
  one masked row) with a new destination/indirect/addr7 layout and
  `(Smem)`x`(Smem)` STORE completion with read-phase T load (TI SQUR
  Example 1 retires through the leaf). S3l lands the register-only
  `SQUR A,dst` row (`FORM-T2-03-10`, both D encodings under one masked
  row) reusing the MAX/MIN single-destination-bit layout with
  `(A(32-16))` squared STORE completion and no T update (TI SQUR
  Example 2 retires through the leaf with A and T preserved). S3m lands
  the `SQURA Smem,src`/`SQURS Smem,src` accumulate-square rows
  (`FORM-T2-04-21`/`FORM-T2-04-22`, 512 S/I/addr7 encodings each under
  one masked row) sharing one leaf with ADD/SUBTRACT select through the
  multiply core with read-phase T load (TI SQURA/SQURS Example 1 vectors
  retire through the leaf; the `LD_T` 0x0800 near-miss lane moves to the
  new `SQURA` row as the recorded cross-slice update). S3n lands
  the `MPYU Smem,dst` unsigned-multiply row (`FORM-T2-03-08`, 512
  D/I/addr7 encodings under one masked row) reusing the
  destination/indirect/addr7 layout with a new unsigned-operands
  select in the shared multiply core, STORE completion of
  `uns(T)`x`uns(Smem)` with no rounding and no T load (TI MPYU
  Example 1 retires through the leaf with T preserved; the SQUR
  0x2400-range near-miss lanes move to the new row as the recorded
  cross-slice update). S3o lands the `MACA Smem` accumulate-A row
  (`FORM-T2-04-07`, 512 R/I/addr7 encodings under one masked row)
  with a new round/indirect/addr7 layout, read-phase T load, and
  `(B)` plus `(Smem)`x`A(32-16)` ADD completion with R rounding
  (TI MACA Example 1 and the MACAR rounding lane retire through
  the leaf; the SQUR-acc/STM near-miss lanes move to the new row
  as recorded cross-slice updates, and the R bit carries the
  `MACAR` variant on the shared row). S3p lands the register-only
  `MACA T,src` row (`FORM-T2-04-08`, 8 S/D/R encodings under one
  masked row) with a new source/destination/round layout and
  `(src)` plus `(T)`x`A(32-16)` ADD completion with R rounding
  (TI MACA Example 2 and the MACAR rounding lane retire through
  the leaf with T preserved; the R bit carries the `MACAR` T,src
  variant on the shared row). S3q lands the two-word `MACP Smem,Pmad`
  row (`FORM-T2-04-12`, 512 S/I/addr7 encodings under one masked row
  plus the verbatim pmad second word) with a new
  destination/indirect/addr7/pmad16 layout and the DATA+PROGRAM pair
  seam (Smem DATA read then absolute PROGRAM coefficient read with
  read-phase Smem T load), completing `(dst)` plus `(Smem)`x`(pmad)`
  into the S-selected accumulator with no rounding (the recomputed
  `0x4000`x`0x4000` oracle and the FRCT lane retire through the leaf
  and the CPU; the BC/CC bit-15 near-miss lanes move to the new row
  as recorded cross-slice updates). S3r lands the two-word `MACD Smem,Pmad`
  row (`FORM-T2-04-11`, 512 S/I/addr7 encodings under one masked row
  plus the verbatim pmad second word) on the shared
  destination/indirect/addr7/pmad16 layout and the DATA+PROGRAM pair
  seam, adding the delay sidecar: after both reads complete the pair
  root issues a third-ID DATA write of the Smem word to `Smem+1`
  (16-bit wraparound) before the Smem post-access commit (the
  recomputed `0x4000`x`0x4000` oracle with `mem[0101]` `4000` and the
  FRCT lane retire through the leaf and the CPU; the BCD/CCD bit-15
  near-miss lanes move to the new row as recorded cross-slice
  updates). S3s lands the nine one-word `MACSU Xmem,Ymem` dual rows
  (`FORM-T2-04-13`, `0xA800` plus X/Y mode bits under mask `0xFECC`,
  S/Xar/Yar riding the word) on the shared dual destination/XAR/YAR
  layout and the dual pair seam with read-phase X-side T load,
  completing `(src)` plus unsigned-`Xmem` times signed-`Ymem` into
  the S-selected accumulator with no rounding (the recomputed
  `0x4000`x`0x4000` oracle, the FRCT lane, and the mixed-sign lane
  retire through the leaf and the CPU; the mixed product rides a new
  unsigned-T-only multiply-core select with both-unsigned
  precedence). S3t–S3x lock the R variants in place (`MACR` dual
  through the shared `MAC` rows, `MASR` Smem and dual, `MACAR`
  Smem, `MASA` Smem and T rows, `MASAR` T, each with TI example
  vectors). S3y locks `MPYR` Smem through the shared `MPY_SMEM`
  row,
  S3z lands the `ADD` Smem syntax 1–3 rows
  (`ADD_SMEM`/`ADD_SMEM_TS`/`ADD_SMEM_SH16`), and S4 lands `XC`.
  S10–S13 land the `STH` stores (syntax 1, by-ASM, by-SHFT Xmem,
  Smem SHIFT with MOD0–MOD3/MOD4+ rows); S14–S26 land the `SUB`,
  `AND`, `OR`, `XOR`, `SUBB`, and `SUBS` Smem rows. Wave-5 fans
  out the six `ADD`/`SUB` leaves (`#lk`, `Xmem`, src-ASM) with
  S27–S29 rows binding `FORM-T2-01-07/08/05/10` and
  `FORM-T2-02-07/08/05/10`; Wave-6 plus S30 land the `MPY`/`MAC`
  `#lk` leaves and rows (`FORM-T2-03-05`, `FORM-T2-04-02`).
- Major remaining work is the `ADD`/`SUB` syntax-4 Smem SHIFT rows
  (S31, in progress), `ADDM`, `MPY` Smem,`#lk` syntax 3, `MAC`
  Smem,`#lk` syntax 4, `SUBC`, `ADDC`/`ADDS`/`DADD`, MMR/stack
  moves, bit/field ops, shifts/misc, the port seam, far/XPC
  addressing, profiles/devices, and evidenced machine timing.
  Emulator integration remains deferred.

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
families; decode coverage, sequencing, interrupt behavior, and timing are added
as their specifications become pinned. The dispatcher follows the mGBA pattern
of a declarative instruction definition expanded into generated wrappers and
dispatch tables, while keeping semantic implementations readable and
independently testable. The [architecture baseline](docs/architecture-baseline.md)
describes the current ownership contracts, while the [evidence index](docs/evidence-index.md)
maps requirements to their sources and tests. `config/evidence.json` is the
machine-checked counterpart; it is validated without emulator or firmware gates.

## External inputs

Tests that need a firmware or DSP image will accept an explicit path and skip when
one is not supplied. Images are never committed, downloaded, or embedded by the
build. This keeps a clean checkout fast and reproducible.
