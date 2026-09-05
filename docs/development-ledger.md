# ti-c54x development ledger

This is the append-only execution record for `docs/implementation-plan.md`. It
records completed work and decisions; the plan remains the current source for future
intent and phase ordering.

## Entry format

Each entry contains:

- date and stable work-piece ID;
- plan section/gate;
- implementation and ledger commit hashes;
- modules and files owned by the change;
- local validation commands and results;
- architecture, lifetime, provenance, or knob decisions;
- deviations from the plan; and
- explicit follow-up work.

Corrections are new entries referencing the original ID. Existing entries are not
rewritten after publication.

## 2026-09-05 — P0-W1 — Standalone repository charter

- Plan: repository charter, isolation, and provenance gate.
- Implementation commit: `3b162c6` (`docs: establish standalone C54x project charter`).
- Ledger commit: this ledger's initial commit.
- Ownership: repository policy and planning documents only.
- Validation: staged diff whitespace check; no code existed to build.
- Decisions: build and test independently from every emulator; no emulator
  integration before provenance/licensing and stable standalone contracts.
- Follow-up: complete the provenance audit and choose a distribution license.

## 2026-09-05 — P0-W2 — Module ownership hierarchy

- Plan: ownership and module-shape gate.
- Implementation commit: `e22944b` (`docs: define module ownership hierarchy`).
- Ledger commit: this ledger's initial commit.
- Ownership: required source/test directory hierarchy and module contracts.
- Validation: staged diff whitespace check; documentation-only change.
- Decisions: files mirror module ownership; review handwritten files near 500 lines
  and require an explicit split decision before 800 lines.
- Follow-up: keep initial CPU and arithmetic files in their owning directories.

## 2026-09-05 — P0-W3 — Diagnostic and knob governance

- Plan: diagnostic isolation and knob ownership/lifecycle gates.
- Implementation commit: `786791e` (`docs: govern diagnostics and configuration knobs`).
- Ledger commit: this ledger's initial commit.
- Ownership: `src/debug/` owns observation gating; `docs/knobs.md` owns the human
  registry surface.
- Validation: staged diff whitespace check; documentation-only change.
- Decisions: no environment access in the library; one per-instance `uint64_t`
  observation mask; separate typed `TEST` and `A/B` controls; mandatory
  `KEEP`/`TEMP`/`RETIRE` disposition.
- Follow-up: implement the compile-out macro, machine-checkable registry, and source
  boundary audit as a later standalone work piece.

## 2026-09-05 — P1-W1 — Standalone API and numeric foundation

- Plan: CPU/bus API shell, 40-bit arithmetic foundation, standalone build and CI.
- State: active; implementation commit pending.
- Intended ownership: `include/ti_c54x/`, `src/cpu/`, `src/foundation/`, build
  metadata, `tests/support/`, mirrored unit tests, and the minimal example.
- Required validation: core-local strict C11 build/tests, sanitizer build/tests, and
  standalone example; no emulator targets.
- Decisions: CTest plus an always-on dependency-free test harness; instruction-budget
  execution only; unimplemented opcodes trap without retiring.
- Follow-up: architecture review and commit once the complete local suite is green.

## 2026-09-05 — P0-W4 — Legacy interpreter provenance audit

- Plan: legacy lineage audit and distribution-license gate.
- Implementation commit: `ccd5de4` (`docs: pin legacy C54x provenance`).
- Ledger commit: this entry's commit.
- Ownership: provenance/evidence documentation only; no legacy source copied.
- Validation: local Git commit/blob verification plus full imported-base comparison
  against the pinned upstream source.
- Decisions: the interpreter base is `bbaranoff/qemu` commit `94c7def...`; the first
  local import is `404ee94...`; Osmocom is a firmware/mailbox input and is not the
  interpreter's author.
- Follow-up: owner/legal selection of the distribution license and permitted use of
  TI manuals, DSP images, GNU metadata, and generated vectors before integration or
  public distribution.

## 2026-09-06 — P1-W1 completion — Standalone CPU foundations

- Corrects the active-state entry for `P1-W1` above; that entry remains unchanged as
  the record of the work piece when it began.
- Plan: CPU/bus API shell, 40-bit arithmetic foundation, standalone build, and
  diagnostic isolation.
- Implementation commit: `4e26e19` (`P1-W1: establish standalone CPU foundations`).
- Ledger commit: this entry's commit.
- Ownership: public lifecycle/bus/state/trace contracts; `src/cpu/` orchestration;
  `src/foundation/` width-defined arithmetic; `src/debug/` copied observer state and
  gating; mirrored unit tests; dependency-free test support; package/build metadata;
  and the minimal standalone consumer.
- Validation: `make test` passed under strict C11 warnings; `make
  TI_C54X_ENABLE_SANITIZERS=1 test` passed under ASan/UBSan; the observer-disabled
  archive/object symbol inspection contained no observer callback or gate symbols;
  staged whitespace validation passed. CMake was unavailable in the local tool
  environment, so package/CTest verification remains assigned to remote CI.
- Decisions: program addresses use a 32-bit word-address carrier; data and I/O use
  distinct 16-bit word-address types; CPU storage is opaque and caller-owned;
  execution budgets count instructions; an unimplemented opcode traps without
  retiring or advancing PC; and arithmetic uses canonical unsigned 40-bit carriers
  to avoid undefined or implementation-defined signed behavior.
- Knobs: activated `TI_C54X_BUILD_TESTS`, `TI_C54X_BUILD_EXAMPLES`,
  `TI_C54X_ENABLE_SANITIZERS`, `TI_C54X_WARNINGS_AS_ERRORS`,
  `TI_C54X_ENABLE_OBSERVERS`, and the copied per-instance `trace.event_mask`. Their
  classes, owners, defaults, tests, dispositions, and exit criteria are registered
  in `docs/knobs.md`.
- Deviation: the observer compile-out seam joined this foundation commit because the
  initialization/configuration contract already exposed tracing. The planned
  machine-readable knob registry and source-boundary audit remain a separate work
  piece.
- Follow-up: confirm GCC/Clang CMake, CTest, installation, and downstream-consumer
  jobs in GitHub Actions; then implement the structural boundary/registry gate
  before address-generation work.

## 2026-09-06 — P1-W1 correction — CMake private-header include

- Corrects the validation state recorded by the `P1-W1` completion entry.
- Implementation commit: `7c997bb` (`fix: make CMake foundation build self-contained`).
- Ledger commit: this entry's commit.
- Evidence: GitHub Actions run `33977004640` configured successfully but all three
  jobs failed while compiling `src/foundation/arithmetic40.c`; its include spelling
  depended on the direct Makefile's `-Isrc` path, which the library CMake target did
  not expose.
- Fix: the implementation now includes its private header from the same owning
  directory. CI also uses `actions/checkout@v5` to match the runner's Node.js 24
  runtime.
- Validation: `make test` and staged whitespace validation passed locally after the
  correction. Remote CMake/CTest/package validation is pending the corrective push.
- Knobs and architecture: unchanged.
- Follow-up: require the replacement GitHub Actions run to pass before marking the
  standalone build/CI plan row complete.

## 2026-09-06 — P1-W1 correction — Observer test include ownership

- Further corrects the remote validation state of `P1-W1`.
- Implementation commit: `3eef0c1` (`fix: propagate observer test API headers`).
- Ledger commit: this entry's commit.
- Evidence: replacement GitHub Actions run `33977097573` built the core and the
  observer-enabled test library, then failed because that test library marked the
  public include tree `PRIVATE`; its linked observer test therefore could not include
  `ti_c54x/cpu.h`.
- Fix: the observer-enabled test variant now publishes only the public `include/`
  usage requirement and keeps `src/` private.
- Validation: `make test` and staged whitespace validation passed locally. Remote
  CMake/CTest/package validation remains pending the next push.
- Knobs and architecture: unchanged.
- Follow-up: require the replacement GitHub Actions run to pass before marking the
  standalone build/CI plan row complete.

## 2026-09-06 — P1-W1 verification — Standalone CI complete

- Plan: closes the standalone build/CI row for `P1-W1`.
- Evidence: GitHub Actions run `33977216829` passed all jobs: strict GCC and Clang
  configure/build/CTest, package installation, downstream `find_package` consumer
  builds, and the Clang ASan/UBSan build/test job.
- Plan state: standalone build and CI moved from `Active` to `Complete`.
- Knobs and architecture: unchanged.
- Follow-up: implement the machine-readable knob registry and structural source
  boundary audit before address-generation work.

## 2026-09-06 — P1-W2 — Structural quality gates

- Plan: machine-readable knob registry and source-boundary audit.
- State: active; implementation commit pending.
- Intended ownership: `config/` policy data, `tools/quality/` deterministic checks,
  focused checker tests under `tests/contract/quality/`, the human knob registry,
  and standalone build/CI wiring.
- Required validation: checker unit tests; repository-wide knob and source-boundary
  checks; strict core-local tests; sanitizer tests; and remote GCC/Clang CMake/CTest,
  installation, and consumer validation. No emulator target is involved.
- Decisions: use a versioned JSON registry and Python 3 standard-library checkers;
  keep knob-policy and source-boundary logic independently owned and testable; make
  both checks part of the default standalone test gate.
- Follow-up: architecture review the checker precision and failure fixtures, commit
  the coherent gate, then append its completed evidence record.

## 2026-09-06 — P1-W2 completion — Structural quality gates

- Corrects the active-state entry for `P1-W2` above; that entry remains unchanged as
  the record of the work piece when it began.
- Plan: machine-readable knob registry, source-boundary audit, and observer-disabled
  artifact proof.
- Implementation commit: `16719c0` (`P1-W2: enforce structural quality gates`).
- Ledger commit: this entry's commit.
- Ownership: `config/knobs.json` is the versioned policy source; `tools/quality/`
  owns separate schema, C lexical, source-policy, observer-policy, repository, and
  artifact checks; `tests/contract/quality/` owns negative-path fixtures; the
  checked-in package consumer lives under `tests/contract/consumer/`; Make, CMake,
  CTest, CI, and README changes own integration only.
- Validation: 62 Python quality fixtures passed; the live knob registry and source
  boundary checks passed; the observer-disabled static archive contained no defined
  or undefined observer symbols; `make test` passed under strict C11 warnings;
  `make TI_C54X_ENABLE_SANITIZERS=1 test` passed under ASan/UBSan; and `make
  PYTHON=/definitely/missing build` proved the compiler-only path does not invoke
  Python. Staged whitespace validation passed.
- Decisions: active observer event kinds and mask literals are bidirectionally tied
  to stable registry bits; planned knobs may not have call sites; retired knob and
  observer-bit identities retain ABI-major tombstones; A/B knobs require two named
  implementations, corpus, decision owner, and selection criterion; test C/H files
  obey the same ownership-contract and size policy as production C/H.
- File-shape review: the initial checkers exceeded or approached 500 lines, so C
  lexing, static boundary data, observer-specific proof, and knob schema validation
  were split into named owners. Every handwritten quality file is now below 500
  lines; the remaining shallow C declaration analysis is intentionally conservative
  and compiler/architecture review remains authoritative for exotic syntax.
- Knobs: no behavior switch was added. The machine registry now governs the seven
  existing active/planned knobs, three active observer bits, and empty knob/bit
  tombstone sets.
- Deviation: CMake is unavailable locally. GCC/Clang CMake, tests-disabled library
  configuration, CTest, installation, and checked-in consumer validation remain
  assigned to the post-push GitHub Actions run.
- Follow-up: require remote CI to pass, then pin the common-core architecture and TI
  evidence index before implementing address generation or opcode semantics.

## 2026-09-06 — P1-W2 verification — Remote structural gates

- Plan: verifies the completed source-boundary and knob-registry gates on the
  standalone CMake/CTest/package surfaces.
- Evidence: GitHub Actions run `33979302909` passed strict GCC and Clang direct-Make
  tests, tests-disabled CMake library builds, tests-enabled CMake/CTest (including
  all quality and artifact checks), installation, the checked-in downstream
  consumer, and the Clang ASan/UBSan job.
- Plan state: source-boundary audit and knob ownership registry remain `Complete`.
- Knobs and architecture: unchanged.
- Follow-up: establish the common-core target and evidence registry before adding
  architectural state, address generation, decode, or opcode semantics.
