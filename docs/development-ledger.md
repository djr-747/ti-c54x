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
- deviations from the plan;
- the slice status flip (`(planned)` to `(implemented; local verification
  green)` in both the `docs/plan/<id>-<slug>.md` heading and the
  `docs/implementation-plan.md` slice link); and
- explicit follow-up work.

The ledger-only commit that appends the entry also carries the status flip; the
implementation commit carries code, tests, registry/evidence, and generated
files only. Corrections are new entries referencing the original ID. Existing
entries are not rewritten after publication.

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

## 2026-09-06 — P2-W0 — Common-core evidence baseline

- Plan: pin the first architecture target and make requirement/evidence/test links
  machine-checkable before adding ISA behavior.
- State: active; implementation commit pending.
- Intended ownership: a versioned evidence registry under `config/`, human scope and
  evidence documentation under `docs/`, independent schema/repository checks under
  `tools/quality/`, focused negative fixtures under `tests/contract/quality/`, and
  standalone Make/CMake/CTest/CI integration.
- Required validation: schema and repository fixtures; live evidence check; existing
  structural, strict C11, sanitizer, CMake/CTest, install, and consumer gates.
- Decisions: target a device-neutral common C54x CPU first; keep XPC, device memory
  maps, exact interrupt sources/vectors, peripherals, and machine-cycle claims in
  later profiles; cite official TI document metadata and narrow derived assertions
  without copying manual prose, figures, or opcode tables.
- Follow-up: review the evidence schema and baseline boundaries, commit and verify
  the gate, then begin requirement-backed register/address foundations.

## 2026-09-06 — P2-W0 completion — Evidence-backed common target

- Implementation commit: `5485e43` (`P2-W0: establish evidence-backed common target`).
- Plan result: the common-core target and evidence registry are `Complete`; address
  generation is now `Active`, while opcode decode remains blocked on the address
  foundation.
- Ownership: `config/evidence.json` owns stable target, source, evidence,
  requirement, test, runner, applicability, and tombstone records;
  `docs/architecture-baseline.md` and `docs/evidence-index.md` own the human review
  surfaces; the split `tools/quality/evidence_*.py` modules own schema, policy,
  documentation parity, and build-runner proofs; the mirrored contract tests live
  under `tests/contract/quality/`.
- Evidence boundary: the active target is the device-neutral
  `C54X_CORE_COMMON_V1`; the VC5402 remains a separate planned profile. Project
  contracts bind to the real common execution target but require project-only
  evidence and tests, so they cannot masquerade as TI-conformance claims. Profile
  sources and evidence bind to exactly one profile.
- Initial pinned behavior: documented reset fields, one-word NOP decode/semantics/
  sequential flow, and the CPL=0 direct-DP address formula are sourced from official
  TI references. SP-boundary direct-address wrap, XPC, device maps, vectors,
  peripherals, and machine-cycle timing remain explicitly unresolved or excluded.
- Gate hardening: a `PASSING` test must exist under the ownership-mirrored test tree
  and be reachable through the actual Make and CTest graphs. Adversarial fixtures
  reject arbitrary files, removed Make prerequisites/recipes, wrong CTest names or
  discovery roots, scope leakage, sibling-profile evidence, duplicate documentation
  rows, and identifier/tombstone reuse.
- Local validation: strict `make test` passed all C executables and 96 Python quality
  tests; the live evidence check passed with 2 targets, 7 sources, 11 requirements,
  and 9 tests; ASan/UBSan `make test` passed; observer-disabled artifact, knob, and
  source-boundary checks passed; and `make PYTHON=/definitely/missing build` proved
  the library build remains compiler-only.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control was added.
- Deviation: CMake is unavailable locally. CMake/CTest, install, consumer, GCC/Clang,
  and sanitizer package surfaces remain assigned to the post-push GitHub Actions
  run.
- Follow-up: verify the pushed commit in standalone CI, then implement the first
  evidence-backed address-owner slice without selecting unresolved CPL=1 behavior.

## 2026-09-06 — P2-W0 verification — Remote evidence gates

- Evidence: GitHub Actions run `33983312950` passed the pushed P2-W0 commits.
- Coverage: strict GCC and Clang direct-Make builds/tests, tests-disabled CMake
  library configuration, CMake/CTest, installation, the checked-in downstream
  consumer, and the Clang ASan/UBSan job all passed in the standalone repository.
- Plan state: the common-core target and evidence registry remain `Complete`;
  address generation remains `Active`.
- Knobs and architecture: unchanged.
- Follow-up: begin the requirement-backed CPL=0 direct-DP address slice while
  preserving CPL=1 SP-boundary behavior as unresolved.

## 2026-09-06 — P2-W1 — CPL=0 direct-DP address primitive

- Plan: implement the first address-owner primitive independently of opcode
  execution, CPU storage, and emulator integration.
- State: active; implementation commit pending.
- Evidence correction: the original §5.4 assertion overstates AR noninterference.
  Replace it with a new immutable evidence ID covering the documented 9-bit DP,
  7-bit `dma`, 16-bit data-word address, and unchanged DP/SP facts; tombstone the
  superseded ID rather than editing it in place. Keep CPL=1 boundary behavior
  unresolved.
- API decision: use a private header-only, branchless `static inline` primitive in
  `src/address/` with distinct DP9 and DMA7 value carriers. Inputs must already be
  valid decoded architectural fields; the module will neither normalize nor define
  behavior for wider C values. Return the existing 16-bit data-word address type.
- Ownership: `src/address/direct_dp.h` owns only CPL=0 direct-DP formation;
  `tests/unit/address/test_direct_dp.c` owns its exhaustive valid-domain proof;
  build files own standalone registration; evidence registry/docs own requirement,
  runner, correction, and tombstone parity.
- Required validation: all 512 × 128 valid DP/`dma` combinations against an
  independent arithmetic reference, explicit boundary vectors, unchanged by-value
  inputs, strict warnings, ASan/UBSan, live evidence/source/knob gates, and remote
  CMake/CTest/install/consumer CI.
- Knobs: none planned.
- Follow-up: independently review the implementation and generated machine code,
  then commit only if the primitive is pure, branchless, ownership-mirrored, and
  fully evidence-linked.

## 2026-09-06 — P2-W1 completion — Direct-DP formation proof

- Implementation commit: `b2fc22f` (`P2-W1: prove direct DP address formation`).
- Plan result: the first address-owner slice is implemented and passing; address
  generation remains `Active` because register effects, CPL=1, indirect modes,
  circular addressing, and ordered parallel effects are not complete.
- Ownership: new public `include/ti_c54x/address.h` owns only scalar program/data/I/O
  word-address carriers. Bus, state, and trace consume that neutral type contract;
  `src/address/direct_dp.h` privately owns the CPL=0 DP9:`dma7` formation primitive;
  its executable proof mirrors ownership under `tests/unit/address/`.
- API decision: the hot primitive is a branchless `static inline` function over two
  distinct by-value carriers. Valid 9-bit DP and 7-bit `dma` values are an explicit
  internal precondition. The helper does not validate, mask, normalize, or promise
  results for wider C values; decoder/state boundaries must establish those field
  invariants once rather than adding a branch to every address calculation.
- Evidence correction: superseded
  `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9` was removed from active records and retained as
  an ABI-0 tombstone pointing to immutable replacement `...-R1`. The corrected
  assertion covers only TI's documented field widths, data-word address formation,
  and DP/SP non-change statement; it explicitly does not claim AR effects or CPL=1
  overflow/wrap behavior.
- TDD boundary: `REQ-COMMON-DIRECT-DP-ADDRESS` now claims only the executable-proven
  valid-domain formation. DP/SP preservation was split into planned
  `REQ-COMMON-DIRECT-REGISTER-NONMUTATION` with its own future state-level test.
  CPL=1 SP-boundary behavior remains `UNRESOLVED`.
- Local validation: all 512 × 128 valid DP/`dma` pairs passed an independent
  multiply/add reference plus explicit corners and repeatability checks; strict
  `make test` and ASan/UBSan `make test` passed; all 96 Python quality tests passed;
  live evidence passed with 2 targets, 7 sources, 12 requirements, and 10 tests;
  source-boundary, knob, and observer-off artifact checks passed; and
  `make PYTHON=/definitely/missing build` remained compiler-only. Independent review
  confirmed shift/or branchless code at optimization.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control was added.
- Deviation: CMake/CTest are unavailable locally; package, install, consumer, and
  compiler-matrix validation remain assigned to the pushed standalone CI run.
- Follow-up: verify CI, then design the address-effect/state seam needed to prove
  DP/SP nonmutation before choosing another direct or indirect addressing behavior.

## 2026-09-06 — P2-W1 verification — Remote direct-DP gates

- Evidence: GitHub Actions run `33984658231` passed the pushed P2-W1 commits.
- Coverage: strict GCC and Clang direct-Make tests, tests-disabled CMake library
  builds, CMake/CTest, installation, the downstream package consumer, and Clang
  ASan/UBSan all passed in the standalone repository.
- Plan state: address generation remains `Active`; opcode decode remains blocked on
  the remaining address/effect foundations.
- Knobs and emulator integration: unchanged.
- Follow-up: specify the smallest explicit address-effect/state seam that can prove
  DP/SP preservation without introducing unresolved CPL=1 or indirect semantics.

## 2026-09-06 — P2-W2 — CMPT=0 single-indirect word addressing

- Plan: establish the first reusable address-effect seam with the documented
  CMPT=0 single-indirect MOD0–MOD3 word-address behaviors, independently of CPU
  state, opcode decoding, bus access, and emulator integration.
- State: active; implementation commit pending.
- Evidence boundary: pin SPRU131G §5.5.1, §5.5.2, Table 5-4/§5.5.3, §5.5.3.1,
  and §5.5.5 as separate immutable records. Claim only ARF-selected AR0–AR7,
  old-versus-updated effective-address order, one-word `+1`/`-1` updates, the
  MOD3 write-only restriction, and CMPT=0's documented ARP non-update. Do not
  claim pipeline timing, circular or bit-reversed modes, dual addressing, long
  operands, or boundary wrap behavior.
- API decision: a checked private planner returns a result by value containing a
  status, one data-word effective address, the selected AR index, and zero or one
  explicit AR update with pre/post phase and before/after values. A non-success
  result has a deterministic zero effect. The planner accepts only word access;
  long-width delta and span rules remain future evidence-backed work.
- Boundary decision: valid interior arithmetic is implemented without C overflow.
  `AR=0` for post-decrement and `AR=0xFFFF` for incrementing modes return an
  unresolved-boundary status because the pinned source does not yet prove the
  architectural edge result. MOD0 remains valid across the full 16-bit AR domain.
- Ownership: `src/address/address_effect.h` owns the private producer/consumer
  effect contract; `src/address/indirect_single.[ch]` owns checked MOD0–MOD3 word
  planning; `tests/unit/address/` mirrors those contracts; build files own only
  standalone registration; the evidence registry and its human mirrors own all
  claims and traceability.
- Required validation: exhaustive valid interior AR values for every supported
  mode/access combination; explicit endpoint, ordering, source-index, update,
  write-only, invalid-input, and deterministic-error-effect checks; strict Make
  tests, ASan/UBSan, evidence/knob/source-boundary gates, and post-push CMake,
  CTest, install, consumer, GCC/Clang, and sanitizer CI.
- Knobs: none planned.
- Follow-up: delegate implementation and an independent evidence/API review, then
  integrate and commit only if the effect seam is narrow enough for a later
  sequencer to apply without exposing CPU storage or bus policy.

## 2026-09-06 — P2-W2 completion — Single-indirect word effect proof

- Implementation commit: `3e1aefb` (`P2-W2: prove CMPT0 indirect word
  addressing`).
- Plan result: CMPT=0 single-indirect MOD0–MOD3 word-address planning is
  implemented and passing behind a private effect seam. Address generation
  remains `Active`: architectural state application, direct-address register
  preservation, long operands, remaining indirect modes, boundary behavior, and
  ordered parallel effects are not complete.
- Ownership: `src/address/address_effect.h` owns the by-value result and one-AR-
  update contract; `src/address/indirect_single.[ch]` owns checked word planning;
  `tests/unit/address/` separately owns the COMMON_TI semantic proof and the
  PROJECT_CONTRACT_ONLY rejection/status proof. The normal and observer-disabled
  libraries both compile the same address implementation.
- Architectural behavior: MOD0 uses the old AR with no update; MOD1 and MOD2 use
  the old AR and report one-word post-access decrement/increment; MOD3 write uses
  the incremented AR and reports a pre-access update. The already-decoded ARF
  index is carried consistently in the effective address and update. MOD3 read is
  rejected. No CPU storage, bus access, opcode decode, or scheduling behavior was
  introduced.
- Evidence boundary: five new immutable SPRU131G records pin §§5.5.1, 5.5.2,
  5.5.3/Table 5-4, 5.5.3.1, and 5.5.5 at printed pages 5-10 through 5-24. ARP state
  enforcement and non-writeback remain planned, edge wrap is unresolved, and long,
  circular, bit-reversed, dual-indirect, pipeline, and timing behavior remain
  unimplemented. A separate project evidence record owns checked status values and
  deterministic zero-valued effects on rejection.
- TDD proof: the COMMON_TI test covers 3,669,976 valid mode/access/index/AR
  combinations across every supported interior value, plus the MOD3 write-only
  restriction. The project-contract test covers high and negative invalid enum
  values, invalid AR indices, all deliberately unresolved word edges, status
  classification, and zero-valued error effects.
- Review correction: independent review found that the initial Make graph omitted
  the new address object from the observer-disabled archive. The archive list was
  corrected before commit; a forced rebuild and symbol inspection then proved that
  both library variants contain `ti_c54x_indirect_single_word`, while the observer-
  off artifact check remained green. No other review finding remained.
- Local validation: strict `make test` passed all C executables and 96 Python
  quality tests; live evidence passed with 2 targets, 7 sources, 20 requirements,
  and 14 tests; source-boundary, knob, and observer-off artifact checks passed;
  ASan/UBSan `make test` passed; a strict compiler analyzer review of the new
  implementation passed; and `make PYTHON=/definitely/missing build` proved that
  the standalone library/test build remains compiler-only.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control was added.
- Deviation: the local CMake executable remains unavailable. CMake/CTest, install,
  consumer, GCC/Clang, and sanitizer package surfaces remain assigned to the
  post-push standalone CI run.
- Follow-up: verify the pushed commits in CI, then define the private architectural
  register-state and sequencer-application seam needed to apply address effects and
  prove register preservation without exposing CPU storage.

## 2026-09-06 — P2-W2 verification — Remote indirect-address gates

- Evidence: GitHub Actions run `33986551986` passed the pushed P2-W2 implementation
  and ledger commits.
- Coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled CMake
  library configurations, CMake/CTest, installation, checked-in downstream consumer
  linkage, and the Clang ASan/UBSan job all passed in the standalone repository.
- Plan state: address generation remains `Active`; the effect description exists,
  but architectural state ownership and sequencer application must be established
  before opcode decode can consume it safely.
- Knobs and integration: unchanged. No emulator checkout, firmware image, hosted
  environment gate, or non-architectural control participated in the build or tests.
- Follow-up: begin the requirement-backed register-state/effect-application slice,
  preserving reset-unspecified fields explicitly and keeping bus transfer and opcode
  execution outside that work piece.

## 2026-09-06 — P2-W3 — Private architectural register-state foundation

- Plan: introduce the private fixed-layout architectural state that later CPU and
  sequencer owners will compose, and prove an evidence-backed common reset seed
  without changing the current public CPU or observation ABI.
- State: active; implementation commit pending.
- Work-piece split: P2-W3 owns state representation, reset seeding, and knownness
  only. P2-W4 will own transactional address-effect application and pre/post phase
  use. CPU embedding, opcode execution, and bus transfers remain later vertical
  slices.
- Representation decision: store PC, IFR, RC, SP, and AR0–AR7 as fixed-width
  scalars; store ST0, ST1, and PMST as raw 16-bit values with independent 16-bit
  known masks; use compact scalar/AR known masks for registers not packed into
  those status words. Use no C bitfields, heap allocation, mutable globals, CPU
  pointer, or diagnostic state.
- Reset decision: the TI common reset seed sets only values proven by SPRU131G,
  including PC=FF80h, IFR=0, RC=0, the complete documented ST0/ST1 fields, and
  the documented PMST fields. MP/MC remains pin-sampled and unknown; profile-
  conditional PMST bits remain unknown; SP and AR0–AR7 remain unknown. Physical
  zero storage is never sufficient to make a value architectural.
- Shell separation: `ti_c54x_config.reset_pc_word` remains an unchanged project
  bootstrap contract in `src/cpu/`; it is not relabeled as TI reset behavior and
  is not routed through the new state seed in this work piece. The private common
  PC remains 16-bit, while the public 32-bit program-word type remains a carrier-
  only integration boundary.
- Ownership: `src/state/register_layout.h` owns evidenced raw register bit masks;
  `src/state/arch_state.[ch]` owns private storage, knownness, reset, and narrow
  checked state access; `tests/unit/state/` separates COMMON_TI reset-value proof
  from PROJECT_CONTRACT_ONLY unknown/read/write behavior. Build files own only
  standalone registration; evidence registry/docs own traceability.
- Evidence plan: retain §6.9 as the reset-action authority and add exact immutable
  records for ST0/ST1 and PMST layouts, AR/SP widths, PC width/XPC exclusion, RC,
  and raw IFR scope. Do not promote the existing full sequencer reset requirement;
  the new state-seed requirement proves construction only.
- Required validation: exact reset values and known masks, repeated deterministic
  reset that clears stale state, explicit UNKNOWN reads that never return physical
  zero as known, checked AR/SP access, strict Make tests, ASan/UBSan, evidence/
  knob/source-boundary gates, compiler-only isolation, independent review, and
  post-push CMake/CTest/install/consumer/compiler CI.
- Knobs: none planned.
- Follow-up: delegate implementation and independent evidence/layout review, then
  commit only if the state remains private, compact, fully initialized, and usable
  by a later sequencer without cross-owner field mutation.

## 2026-09-06 — P2-W3 completion — Architectural register state

- Implementation commit: `2bb6d25` (`P2-W3: establish architectural register
  state`).
- Plan result: the architectural register-state foundation is `Complete`. The
  implementation provides private fixed-layout storage, explicit knownness, an
  evidence-backed common reset seed, and checked AR/SP access. CPU embedding,
  sequencer application, bus transfer, and opcode execution remain outside this
  work piece.
- Ownership: `src/state/register_layout.h` owns named ST0/ST1/PMST field masks;
  `src/state/arch_state.[ch]` owns storage, lifecycle, reset, and narrow checked
  access; `tests/unit/state/test_arch_state_reset.c` owns COMMON_TI reset proof;
  `tests/unit/state/test_arch_state_knownness.c` owns PROJECT_CONTRACT_ONLY state
  API behavior. Both normal and observer-disabled libraries compile the same state
  implementation.
- Reset behavior: common construction seeds PC=FF80h, IFR=0, RC=0, ST0=1800h,
  ST1=2900h, and PMST=FF80h. ST0 is fully known; ST1 deliberately leaves reserved
  bit 10 unknown; PMST marks only IPTR, OVLY, AVIS, DROM, and CLKOFF known. MP/MC,
  profile-dependent PMST bits, SP, and AR0–AR7 remain architecturally unknown even
  though their backing storage is deterministically cleared.
- State contract: unknown and invalid reads preserve the caller's output; invalid
  writes preserve the complete state byte-for-byte; successful AR/SP writes
  establish knownness; reset clears stale values and knownness deterministically.
  Null and out-of-range behavior is explicit and tested.
- Evidence boundary: seven focused SPRU131G records now pin register layouts and
  widths at their exact printed-page locators, and one project record owns the
  checked-state contract. Review corrected the PC-width locator so it claims only
  Table 6-2 on printed page 6-4; XPC begins on page 6-5 and is intentionally not
  claimed. SP/RC/IFR assertions were narrowed to their local evidence, while reset
  values continue to rely on the existing section 6.9 authority.
- Review correction: independent review also removed redundant composite reset
  constants that could drift from the named field masks. The final review was
  clean, with no public ABI, diagnostic, CPU, bus, opcode, or emulator coupling.
- Local validation: strict `make test` and ASan/UBSan `make test` passed all C
  executables and 96 Python quality tests; live evidence passed with 2 targets, 7
  sources, 22 requirements, and 16 tests; source-boundary passed for 29 files;
  knob and observer-disabled artifact checks passed; and
  `make PYTHON=/definitely/missing build` proved compiler-only standalone builds.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control was added.
- Deviation: the local CMake executable remains unavailable. CMake/CTest, install,
  downstream consumer, GCC/Clang, and sanitizer package surfaces are assigned to
  the post-push standalone CI run.
- Follow-up: P2-W4 should add a private transactional sequencer seam that validates
  an entire address effect before applying its pre- or post-access AR update, with
  rejection paths proven to leave architectural state unchanged.

## 2026-09-06 — P2-W3 verification — Remote architectural-state gates

- Evidence: GitHub Actions run `33988181708` passed the pushed P2-W3 implementation
  and completion-ledger commits.
- Coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled CMake
  library configurations, CMake/CTest, package installation, checked-in downstream
  consumer linkage, and the Clang ASan/UBSan job all passed in the standalone
  repository.
- Plan state: the architectural register-state foundation is `Complete`. Address
  generation remains `Active` until a transactional sequencer can validate and
  apply the existing address effects without partial state mutation.
- Knobs and integration: unchanged. No emulator checkout, firmware image, hosted
  environment gate, diagnostic control, or project-specific integration was needed
  to build or prove this slice.
- Follow-up: begin P2-W4 with rejection-first tests for whole-effect validation,
  pre/post phase matching, known source ARs, stale-before detection, and bytewise
  state preservation on every rejected application.

## 2026-09-06 — P2-W4 — Transactional address-effect application

- Plan: add the first private `src/sequencer/` seam, limited to validating a
  complete address result against architectural state and applying at most one
  auxiliary-register update in its declared phase.
- State: active; implementation commit pending.
- Ownership: `src/sequencer/address_effect_apply.[ch]` will own validation order,
  application status, phase matching, and the single commit point;
  `tests/unit/sequencer/` will separate COMMON_TI successful-effect application
  from PROJECT_CONTRACT_ONLY malformed/rejected-call behavior. Address arithmetic
  remains owned by `src/address/`, and register storage remains owned by
  `src/state/`.
- API decision: accept caller-owned state, a result borrowed for the duration of
  the call, and one exact requested phase. `NONE` validates a no-update effect;
  `PRE_ACCESS` and `POST_ACCESS` must match the effect exactly. The seam owns no
  CPU, bus, opcode, memory-transfer, diagnostic, or timing behavior.
- Transaction decision: validate pointers and phase, upstream result status,
  complete effect shape, phase agreement, effective-address relation, selected-AR
  knownness, and stale-before state before mutation. Successful AR effects perform
  one checked write; every rejection leaves the full state byte-for-byte unchanged.
- Extensibility decision: do not enforce increment/decrement magnitude in the
  sequencer. The address planner owns arithmetic, while future circular and
  bit-reversed modes may legitimately propose non-linear or unchanged values.
- Evidence plan: reuse the existing immutable SPRU131G indirect-address records for
  successful MOD0–MOD3 state effects. Add a narrow sequencer-owned COMMON_TI
  requirement/test and a separate project-contract evidence/requirement/test for
  validation statuses, deterministic precedence, and transactional rejection.
  Keep CMPT=0 ARP state planned because this slice neither represents nor mutates
  ARP.
- Required validation: all eight AR indices and every supported interior word
  value through planner-produced MOD0–MOD3 results; exact pre/post/no-update
  behavior; malformed enum/index/phase/payload/address relations; upstream
  failures with forged payloads; unknown and stale ARs; repeated application; and
  bytewise state preservation on every rejection. Run strict Make, ASan/UBSan,
  compiler-only isolation, evidence/knob/source-boundary/artifact gates,
  independent review, and post-push standalone CI.
- Knobs: none planned.
- Follow-up: delegate implementation and independent API/evidence review, then
  integrate only if application remains a narrow private state transition rather
  than growing into premature CPU or bus orchestration.

## 2026-09-06 — P2-W4 completion — Transactional address-effect application

- Implementation commit: `26011c8` (`P2-W4: apply address effects
  transactionally`).
- Plan result: transactional address-effect application is `Complete`. Address
  generation remains `Active` because direct-register effects, ARP/CMPT state,
  long operands, remaining indirect modes, and boundary behavior are not complete.
- Ownership: `src/sequencer/address_effect_apply.[ch]` owns validation precedence,
  exact phase matching, stale-state detection, and the single AR commit point;
  `tests/unit/sequencer/test_address_effect_apply.c` owns COMMON_TI successful
  transitions; its contract sibling owns project-only validation and rejection
  behavior. The module is compiled into both normal and observer-disabled archives.
- Transition contract: a `NONE` effect is validated only at phase `NONE` and leaves
  state unchanged. AR effects require a matching selected/update index, PRE or POST
  phase, phase-consistent effective address, a known selected AR, and an unchanged
  `value_before`; a successful call then performs one checked write. Upstream
  failure, malformed shape, phase mismatch, unknown AR, and stale AR are distinct.
- Transaction boundary: pointer/requested-phase validation precedes upstream result
  status; result status precedes intrinsic shape; shape precedes phase agreement;
  phase agreement precedes effective-address relation; and all precede state read
  and mutation. A non-OK result payload is never interpreted, and every rejection
  preserves the complete state byte-for-byte.
- Extensibility: the seam deliberately does not infer an address mode or enforce a
  `+1`/`-1` delta. Equal before/after updates are accepted when phase/address/state
  relations hold, preserving a route for future circular and bit-reversed modes.
  Bus access, opcode execution, timing, CPU lifecycle, diagnostics, and ARP/CMPT
  policy remain absent.
- TDD proof: the COMMON test independently asserts MOD0–MOD3 TI phase, address,
  before/after, and final-state expectations over 2,097,128 planner-produced valid
  index/value combinations and compares the entire resulting state. The project
  test covers nulls, negative/high enums, all non-OK and unknown result statuses
  with forged payloads, malformed NONE/AR shapes, index/address mismatches, exact
  status precedence, unknown/stale state, repeated application, equal before/after
  updates, and bytewise rejection non-mutation.
- Review correction: the initial COMMON test derived its expected phase/value from
  the planner payload and the initial project test covered unknown state only for a
  no-update result. Independent review and main audit converted the COMMON proof to
  independent per-mode expectations and whole-state comparison, added unknown
  update coverage, removed direct state-field inspection, and expanded non-OK and
  precedence coverage. No production defect remained.
- Local validation: strict `make test` and ASan/UBSan `make test` passed all C
  executables and 96 Python quality tests; live evidence passed with 2 targets, 7
  sources, 24 requirements, and 18 tests; source-boundary passed for 33 files;
  knob and observer-disabled artifact checks passed; and
  `make PYTHON=/definitely/missing build` proved compiler-only standalone builds.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control was added.
- Deviation: local CMake and Clang executables remain unavailable. CMake/CTest,
  install, downstream consumer, GCC/Clang, and Clang sanitizer package surfaces are
  assigned to post-push standalone CI.
- Follow-up: verify remote gates, then choose the next evidence-backed addressing
  foundation between direct no-update effects and the private status-field access
  needed to enforce CMPT=0/ARP=0 without exposing raw register layout.

## 2026-09-06 — P2-W4 verification — Remote transactional-application gates

- Initial evidence: GitHub Actions run `33989551444` passed direct Make but failed
  all normal CMake library builds because the new cross-owner private header could
  not resolve `address/address_effect.h` from the core target. The observer test
  archives already had the private source include root, exposing a real build-graph
  parity defect rather than a semantic failure.
- Correction commit: `4dab92c` (`build: add private source includes to core`) adds
  `${CMAKE_CURRENT_SOURCE_DIR}/src` as a `PRIVATE` include directory of the normal
  library target. It does not enter the public or install interface, so downstream
  consumers remain restricted to installed public headers.
- Verified evidence: GitHub Actions run `33989649434` passed after the correction.
  GCC and Clang strict direct-Make builds/tests, tests-disabled CMake library builds,
  CMake/CTest, package installation, checked-in downstream consumer linkage, and
  the Clang ASan/UBSan job all passed in the standalone repository.
- Plan state: transactional address-effect application remains `Complete`; address
  generation remains `Active` for the explicitly deferred modes and state policy.
- Knobs and integration: unchanged. No emulator checkout, firmware image, hosted
  environment gate, diagnostic control, or public private-header exposure was
  introduced by the correction.
- Follow-up: select P2-W5 from evidence and dependency shape, preferring the smallest
  foundation that closes a real architectural gap without coupling sequencing to
  the public CPU shell.

## 2026-09-06 — P2-W5 — CMPT=0/ARP=0 state policy

- Plan: close the existing standard single-indirect mode-state requirement by
  adding narrow knownness-aware ARP/CMPT field access, carrying the required state
  policy in the value-owned address effect, and enforcing it transactionally before
  any selected-AR read or write.
- State: active; implementation commit pending.
- Selection decision: choose mode-state policy before direct no-update effects or
  32-bit single-indirect addressing. Direct effects cannot yet use the AR-specific
  no-update carrier without redesign; 32-bit effects need a focused Table 5-4
  evidence record, width/span representation, and explicit two-word boundary
  policy. ARP/CMPT access is the immediate prerequisite already represented in the
  private register state.
- Ownership: `src/state/register_layout.h` will own ARP/CMPT field positions;
  `src/state/arch_state.[ch]` will own narrow checked reads/writes and knownness;
  `src/address/address_effect.h` will own the state-policy tag;
  `src/address/indirect_single.c` will attach the CMPT=0/ARP=0 policy; and
  `src/sequencer/address_effect_apply.[ch]` will validate the tagged policy before
  AR state. Mirrored state and sequencer tests own their respective proofs.
- Policy decision: use one private effect-carried enum with `NONE` and
  `CMPT0_ARP0`, not a CMPT0-specific application wrapper. Unknown enum values are
  malformed effects. `NONE` leaves unrelated future address families ungated;
  planner-produced CMPT=0 single-indirect effects always carry `CMPT0_ARP0`.
- State decision: expose only field-specific ARP (three-bit) and CMPT (one-bit)
  accessors. Reads distinguish unknown state without changing caller output; writes
  validate range, preserve unrelated raw bits and known masks, and establish only
  the written field's knownness. Do not add raw ST0/ST1 access, infer reserved-bit
  policy, or claim arbitrary data-memory status writes.
- Sequencer decision: after intrinsic effect/phase/address validation, establish
  that every required mode field is known, then require CMPT=0 and ARP=0. Report
  unknown mode state separately from a known constraint violation; never silently
  clear ARP or alter CMPT. Both precede selected-AR knownness/staleness and the one
  commit point.
- Evidence plan: reuse the exact ST0/ST1 layout/reset record and standard-indirect
  §§5.5.1/5.5.5 records; add a focused §4.1.1 record only for independently claimed
  field write surfaces, plus separate project records for checked field access and
  effect-policy/status behavior. Promote the existing
  `REQ-COMMON-INDIRECT-CMPT0-ARP-STATE` only when its sequencer test is green.
- Required validation: exhaustive ARP values 0..7 and CMPT values 0..1; reset-known
  reads; invalid/null/unknown calls and bytewise rejected-write preservation;
  unrelated raw value/known-mask preservation; policy-tag production and zero error
  effects; CMPT0/ARP0 MOD0–MOD3 success with ARP unchanged; nonzero and unknown mode
  state rejection before AR handling; generic-policy noninterference; deterministic
  status precedence; strict/sanitizer/compiler-only gates; independent review; and
  post-push GCC/Clang/CMake/install/consumer CI.
- Exclusions: CMPT=1 selection/writeback, direct DP/SP effects, 32-bit operands,
  boundary wrap, bus ordering, opcode decode, CPU integration, timing, diagnostics,
  raw status serialization, and device-specific behavior.
- Knobs: none planned.
- Follow-up: delegate TDD implementation and an independent field/policy evidence
  review, then integrate only if raw register layout remains state-owned and all
  failed policy checks are transactionally inert.

## 2026-09-06 — P2-W5 completion — Standard-indirect state policy

- Implementation commit: `3e46617` (`P2-W5: enforce standard indirect state
  policy`).
- Plan result: standard-indirect mode-state policy is `Complete`. Address
  generation remains `Active` because direct-register effects, long operands,
  remaining indirect modes, and boundary behavior remain deliberately deferred.
- Ownership: `src/state/register_layout.h` owns the ARP/CMPT positions;
  `src/state/arch_state.[ch]` owns checked field access and knownness;
  `src/address/address_effect.h` owns the declarative state-policy tag;
  `src/address/indirect_single.[ch]` owns policy production; and
  `src/sequencer/address_effect_apply.[ch]` owns policy enforcement and status
  precedence. COMMON_TI behavior tests and PROJECT contract tests mirror those
  owner boundaries under `tests/unit/`.
- State contract: ARP reads require all three field bits to be known and CMPT reads
  require its single field bit to be known. Checked writes accept only ARP 0..7 or
  CMPT 0..1, preserve unrelated raw bits and knownness, and mark only the owned
  field known. Null, unknown, and out-of-range operations preserve caller output
  and complete state as applicable; no raw status-register API was introduced.
- Effect contract: every successful CMPT=0 single-indirect plan carries
  `CMPT0_ARP0`; every failed result remains the all-zero `NONE` effect. The
  sequencer treats unknown policy tags as malformed, checks that all required mode
  fields are known before testing either value, distinguishes unknown mode state
  from known policy violation, and performs those checks before selected-AR reads.
  It never repairs CMPT or ARP, never writes ARP as an addressing side effect, and
  leaves generic `NONE` policy effects unaffected.
- TDD proof: the COMMON state test independently proves documented reset values,
  field positions, and every ARP/CMPT field value. The PROJECT state test proves
  checked-access null, unknown, range, output-preservation, unrelated-bit, and
  unrelated-knownness contracts. COMMON sequencer coverage checks MOD0–MOD3 over
  AR0..AR7 with independent expected addresses and exact whole-state outcomes;
  PROJECT coverage checks malformed policies, both mixed unknown/violating field
  orders, all-known violations, policy-before-stale precedence, generic-policy
  noninterference, and bytewise rejection non-mutation.
- Evidence: the registry now contains 2 targets, 7 sources, 27 requirements, and
  21 tests. The new TI record is narrowly scoped to documented ST0/ST1 field write
  surfaces; project records own checked-access knownness and sequencer-policy
  behavior. ARP auto-clear, CMPT=1 behavior, reserved bits, arbitrary data-memory
  status writes, direct effects, 32-bit addressing, and boundary behavior remain
  unclaimed.
- Review corrections: independent review replaced planner-derived COMMON
  expectations with per-mode literals, added both mixed unknown/nonzero
  precedence cases, synchronized module contracts, separated project-knownness
  claims from COMMON evidence, and narrowed reset wording to documented ARP/CMPT
  values. Final review found the production semantics clean.
- Local validation: strict `make test` and ASan/UBSan `make test` passed all C
  executables and 96 Python quality tests; source-boundary passed for 37 files;
  evidence, knob, and observer-disabled artifact checks passed; and
  `make PYTHON=/definitely/missing build` proved the compiler-only standalone
  path. Local CMake and Clang executables remain unavailable and are assigned to
  post-push standalone CI.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control was added.
- Follow-up: verify remote GCC/Clang/CMake/install/consumer/sanitizer gates, then
  select P2-W6 from evidence and dependency shape without growing the AR-specific
  effect seam into a generic monolith.

## 2026-09-06 — P2-W5 verification — Remote standard-indirect policy gates

- Verified evidence: GitHub Actions run `33991324850` passed for implementation
  commit `3e46617` and completion-ledger commit `f5e06e0`.
- Remote coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled
  CMake library builds, CMake/CTest, package installation, checked-in downstream
  consumer linkage, and the Clang ASan/UBSan job all passed in the standalone
  repository.
- Plan state: standard-indirect mode-state policy remains `Complete`; address
  generation remains `Active` only for the explicitly deferred effect families,
  modes, widths, and boundaries.
- Scope confirmation: the verified build did not add an emulator dependency,
  firmware image, environment lookup, diagnostic path, public private-header
  exposure, or new knob.
- Follow-up: choose and start P2-W6 from the evidence ledger and ownership graph.

## 2026-09-06 — P2-W6 — 32-bit single-indirect operand shape

- Plan: extend the proven CMPT=0 MOD0–MOD3 indirect slice to 32-bit data
  operands, representing both accessed word addresses and the evidenced two-word
  auxiliary-register delta before any opcode, bus, or CPU integration.
- State: active; implementation commit pending.
- Selection decision: choose 32-bit single-indirect effects before direct
  no-update effects. Long32 operands preserve the existing AR snapshot,
  CMPT0/ARP0 policy, and transactional writeback seam. A truthful direct effect
  still needs a separate DP/SP source-snapshot design; treating it as having no
  register dependency or borrowing indirect MOD0 would allow stale addresses or
  false AR reads.
- Terminology: use `long32` or `32-bit data operand`, never bare `long`, because
  the TI ISA also uses “long offset” for a distinct 16-bit `lk` operand.
- Operand-shape decision: replace the private effect's single scalar effective
  address with an owned descriptor containing width plus explicit first and
  second data-word addresses. Word16 has one canonical first address and no
  second address. Long32 records the first/MSW and second/LSW address explicitly:
  an even first address pairs with the next higher word and an odd first address
  pairs with the previous lower word. Pairing is data-address behavior, not a
  bus-order or timing claim.
- Planner decision: keep `ti_c54x_indirect_single_word` and add an unambiguous
  `ti_c54x_indirect_single_long32` entry in the same cohesive single-indirect
  owner. Share private mode logic rather than duplicate MOD0–MOD3 behavior. Long32
  uses delta two for MOD1–MOD3, retains NONE/POST/POST/PRE ordering, and retains
  the MOD3 write-only restriction.
- Boundary decision: preserve the unresolved unsigned-AR edge policy. MOD0 is
  valid for every first address because parity pairing stays in the 16-bit data
  domain; MOD1 rejects AR values below two; MOD2 and MOD3 reject values above
  `0xFFFD`. Do not infer architectural wrap from C integer conversion.
- Validation decision: the address owner defines canonical word16/long32 operand
  shapes. The sequencer validates that shape before mode state or AR state, then
  reuses the existing policy, stale-snapshot, phase, and one-commit rules. Every
  rejected descriptor or result remains bytewise nonmutating.
- Evidence plan: add an immutable §5.5.3/Table 5-4 record for the explicit
  16-bit-delta-one/32-bit-delta-two footnote and an immutable §5.8 record for
  first/MSW, second/LSW, and even-higher/odd-lower pairing. Keep timing, bus use,
  and write-cycle text outside the assertions. Add separate project evidence for
  canonical operand-shape validation.
- TDD plan: independently prove all 65,536 long32 parity pairs; MOD0 over every AR
  value and MOD1–MOD3 over their complete non-boundary domains for AR0–AR7 and
  supported access kinds; exact delta, phase, before/after, policy, and all-zero
  failure effects; malformed width/pair/update relations and status precedence;
  successful whole-state transitions; and bytewise nonmutation on every rejection.
  Existing word16 behavior must remain exhaustive and unchanged.
- Ownership: `src/address/` owns operand descriptors, parity pairing, long32
  planning, and intrinsic shape; `src/sequencer/` owns transactional application;
  mirrored tests own COMMON_TI behavior and separate PROJECT rejection contracts.
  Build/evidence documents own only registration and proof links.
- Exclusions: MOD4–MOD15, circular/bit-reversed/indexed/offset forms, CMPT=1,
  dual operands, memory-mapped registers, bus selection/order, two-cycle stores,
  opcode decode, CPU lifecycle, timing, device profiles, diagnostics, emulator
  integration, and any address-domain wrap claim.
- Knobs: none planned.
- Follow-up: delegate address, sequencer, and evidence/build ownership to separate
  Luna workers; integrate and independently review before the one implementation
  commit and ledger-only completion entry.

## 2026-09-06 — P2-W6 completion — 32-bit single-indirect operand shape

- Implementation: commit `c4dfbbc` completes the standalone long32 operand and
  indirect-effect slice. The implementation-plan row is `Complete`; the broader
  address-generation phase remains `Active` for the explicitly deferred families.
- Ownership: `src/address/data_operand.[ch]` owns canonical word16/long32 operand
  descriptors and parity pairing; `src/address/indirect_single.[ch]` owns pure
  MOD0–MOD3 plans; and `src/sequencer/address_effect_apply.[ch]` owns validation,
  state-policy enforcement, stale-snapshot rejection, and transactional commit.
  COMMON_TI behavior and PROJECT contracts remain split under matching
  `tests/unit/address/` and `tests/unit/sequencer/` hierarchies. The largest
  touched test owner is 388 lines after splitting state-transition contracts out
  of the application-contract file.
- Operand contract: a word16 operand has one canonical address. A long32 operand
  records the first/MSW and second/LSW addresses explicitly: even first addresses
  pair upward and odd first addresses pair downward. Validators borrow their
  input read-only only for the duration of the call, retain no pointer, and do not
  mutate the descriptor.
- Planner contract: long32 MOD0 is valid across the complete 16-bit AR domain and
  has no update; MOD1 uses post-decrement by two; MOD2 uses post-increment by two;
  and write-only MOD3 uses pre-increment by two. With wrap still unresolved, MOD1
  rejects AR values below two and MOD2/MOD3 reject values above `0xFFFD`. All
  successful effects carry the existing CMPT0/ARP0 state policy; all failures
  retain the exact all-zero effect.
- Application contract: canonical operand shape is checked before architectural
  mode or selected-AR state. PRE, POST, and NONE effects prove their first-address
  relationship independently, while the operand owner alone defines the
  parity-dependent second address. Invalid shape, unknown policy, unknown mode,
  policy violation, unknown selected AR, stale snapshot, and malformed phase or
  transition each preserve the complete state byte-for-byte.
- TDD proof: exhaustive tests cover all 65,536 long32 parity pairs, every selected
  AR, MOD0 over the complete address domain, MOD1–MOD3 over every admitted address,
  supported access kinds, exact delta/phase/policy/word addresses, and whole-state
  application. PROJECT tests cover invalid widths and pairings, validator
  nonmutation via snapshots and `memcmp`, status precedence, boundary refusals,
  canonical NONE effects, and canonical long32 PRE/POST transitions.
- Evidence: the live registry contains 2 targets, 7 sources, 31 requirements, and
  26 tests. TI-backed claims are limited to the documented two-word delta and
  first/MSW–second/LSW parity pairing. Project evidence owns descriptor validity,
  rejection precedence, and transaction mechanics; bus ordering, timing, and
  address wrap remain unclaimed.
- Review corrections: orchestration review rejected a false no-dependency direct
  design, removed PROJECT validation claims from COMMON tests, narrowed evidence
  language, fixed a non-stale stale-state vector, removed raw state-field
  reach-through, made long-effect assertions explicit, and split a near-monolithic
  contract test. Independent closure review then required and verified canonical
  PRE/POST successes, bytewise validator nonmutation, and precise borrowed-input
  lifetime documentation.
- Local validation: strict `make test` and ASan/UBSan `make test` passed every C
  executable and all 96 Python quality tests; source boundaries passed for 45
  files; evidence, knob, and observer-disabled artifact checks passed; and
  `make PYTHON=/definitely/missing build` proved the compiler-only standalone
  path. Local CMake and Clang remain assigned to post-push standalone CI.
- Knobs: unchanged. No `TEST`, `OBSERVE`, or `A/B` control, environment read,
  diagnostic hook, or emulator dependency was added.
- Follow-up: verify remote GCC/Clang/CMake/install/consumer/sanitizer gates, then
  design direct-address effects around truthful DP/SP source snapshots rather
  than reusing the selected-AR dependency shape.

## 2026-09-06 — P2-W6 verification — Remote long32 indirect gates

- Verified evidence: GitHub Actions run `33993459022` passed for implementation
  commit `c4dfbbc` and completion-ledger commit `08aacf0`.
- Remote coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled
  CMake library builds, CMake/CTest, package installation, checked-in downstream
  consumer linkage, and the Clang ASan/UBSan job all passed in the standalone
  repository.
- Plan state: the 32-bit single-indirect operand shape remains `Complete`; address
  generation remains `Active` only for explicitly deferred effect families,
  modes, and unresolved boundary behavior.
- Scope confirmation: the verified build added no emulator dependency, firmware
  image, environment lookup, diagnostic path, public private-header exposure, or
  new knob.
- Follow-up: begin the next evidence-led address slice with explicit ownership and
  source-snapshot semantics; do not encode a direct DP/SP dependency as an
  indirect AR dependency.

## 2026-09-06 — P2-W7 — Direct-address source snapshots

- Plan: turn the existing pure CPL=0 direct-address formation helper into a
  truthful WORD16 address-effect family, add only the evidenced non-overflow
  CPL=1 SP form, and prove application without DP/SP mutation or AR coupling.
- State: active; implementation commit pending.
- Selection decision: include both direct bases in one source-snapshot migration.
  The same TI section defines CPL selection, DP concatenation, positive SP offset,
  and DP/SP nonmutation. Implementing DP alone would leave a temporary tagged seam
  and force the same private effect migration again for SP.
- Width decision: direct effects are WORD16 only. Figure 5-4 names a 32-bit-read
  bus surface but §5.4 does not define direct long32 word pairing or ordering;
  the existing generic long32 descriptor is not evidence to infer that behavior.
- Boundary decision: CPL=0 concatenation covers the complete DP9×dma7 domain.
  CPL=1 accepts only sums representable without crossing `0xFFFF` and returns the
  existing unresolved-boundary status otherwise. No modulo wrap is inferred.
- Effect-shape decision: replace the private indirect-only selected-AR field with
  one tagged, value-owned source snapshot. AR sources retain index and before
  value; DP and SP sources retain their true base plus the copied seven-bit
  offset. Direct effects carry explicit CPL0/CPL1 policy, a canonical WORD16
  operand, and no update. Failed planners retain an exact all-zero effect.
- Application decision: the sequencer validates canonical operand, source, policy,
  update, phase, and address relationships before reading state. It then checks
  known CPL before policy value, reads only the tagged source through its owning
  state accessor, rejects stale snapshots distinctly, and returns success without
  mutation for direct effects. Existing indirect behavior remains unchanged.
- Ownership: `src/address/direct_dp.[ch]` owns DP9:dma7 effects;
  `src/address/direct_sp.[ch]` owns non-overflow SP+dma7 effects;
  `src/address/address_effect.h` owns the shared value types; `src/state/` owns
  checked DP/CPL access; and `src/sequencer/` owns transactional validation and
  application. COMMON_TI and PROJECT tests mirror those owner directories.
- TDD plan: exhaust the complete DP9×dma7 domain and every non-overflow SP×dma7
  pair; prove exact operands, source snapshots, CPL policies, and no updates;
  prove DP/CPL accessor widths, knownness, isolation, and rejected-call
  nonmutation; migrate all indirect fixtures to the tagged source; and cover
  malformed tags/fields/relations, unknown and stale source precedence, exact-zero
  failures, and bytewise state preservation.
- Evidence: reuse pinned TI `SPRU131G` Rev G §5.4, printed pages 5-7–5-9. Its
  assertion already covers dma width, CPL selection, DP concatenation, positive SP
  addition, and DP/SP nonmutation while explicitly excluding SP overflow/wrap.
  Project evidence will own snapshot validation, status precedence, and the C
  transaction seam.
- Exclusions: direct long32 pairing, SP overflow/wrap, AR effects, memory-mapped
  registers, bus selection/order, opcode decode, CPU lifecycle, timing,
  diagnostics, device profiles, and emulator integration.
- Knobs: none planned.
- Follow-up: delegate state, address, sequencer, and evidence/build ownership to
  Luna workers; integrate and independently review before the one implementation
  commit and ledger-only completion entry.

## 2026-09-06 — P2-W7 completion — Direct-address source snapshots

- Implementation commit: `c6eeb07` (`P2-W7: add direct address source snapshots`).
- Plan result: direct-address source snapshots moved from `Active` to `Complete`.
  The broader address-generation phase remains `Active` for explicitly deferred
  addressing families, modes, and unresolved boundary behavior.
- Ownership: `src/address/address_effect.h` now owns one tagged, value-owned source
  snapshot shared by indirect AR and direct DP/SP effects;
  `src/address/direct_dp.[ch]` owns checked CPL=0 DP9:dma7 WORD16 planning;
  `src/address/direct_sp.[ch]` owns checked non-overflow CPL=1 SP16+dma7 WORD16
  planning; `src/state/` owns knownness-aware DP/CPL access; and
  `src/sequencer/address_effect_apply.[ch]` owns source/policy validation, stale
  snapshot rejection, and transactional application. Tests remain under matching
  address, state, and sequencer owners.
- Effect contract: every successful direct result carries a canonical WORD16
  operand, its true copied DP9/dma7 or SP16/dma7 source, the exact CPL policy, and
  an all-zero no-update descriptor. Every rejected planner result retains an
  all-zero effect. Existing indirect effects migrated from an AR index to an AR
  index/value snapshot without changing their admitted arithmetic or mode policy.
- Boundary decision: CPL=0 exhausts all 512×128 valid field combinations. CPL=1
  exhausts every SP16×dma7 pair whose mathematical sum is at most `0xFFFF` and
  returns `UNRESOLVED_BOUNDARY` above it. No direct long32 word pairing or SP wrap
  behavior was inferred.
- State and application contract: DP and CPL have checked field-width,
  knownness, rejected-call nonmutation, and cross-field-isolation accessors. The
  sequencer validates intrinsic operand/source/update/policy shape before phase,
  mode, and source state; reports unknown mode, unknown source, and stale source
  separately; and preserves complete state on every rejection. Accepted direct
  effects read only CPL plus their tagged DP or SP and perform no address-register
  write.
- TDD proof: common tests exhaust the DP and admitted SP domains independently,
  then exhaust planner-produced application while leaving AR state unknown so an
  accidental AR dependency fails. Project tests cover malformed tags, fields,
  operand/source relations, policies, forbidden updates and direct long32 shape,
  non-OK-result precedence, unknown/stale states, exact-zero planner failures, and
  bytewise transactional state preservation.
- Evidence: the live registry now contains 2 targets, 7 sources, 34 immutable
  evidence records, 39 requirements, and 31 tests (27 `PASSING`, 4 `PLANNED`).
  COMMON_TI claims remain limited to TI `SPRU131G` Rev G section 5.4's direct
  selection, safe address formation, and DP/SP nonmutation. Project evidence owns
  checked rejection, source snapshots, status precedence, and transaction
  mechanics. SP wrap remains `UNRESOLVED`.
- Review corrections: orchestration review restored generic AR/NONE policy
  compatibility, removed a truncating SP-sum helper outside the admitted domain,
  made successful direct results fully zero-initialized, strengthened malformed
  source and precedence vectors, narrowed common claims to admitted WORD16 forms,
  and aligned module/evidence language with the shared application seam. An
  independent Luna closure review found no blockers. It recorded the neutral
  `dma7` carrier reuse from `direct_dp.h` as a possible future extraction only if
  the direct-address family grows; the dependency contains no state or logic.
- Local validation: strict `make test` and ASan/UBSan `make test` passed every C
  executable and all 96 Python quality tests; source boundaries passed for 52
  files; evidence, knob, and observer-disabled artifact checks passed; and
  `make PYTHON=/definitely/missing build` proved the compiler-only standalone
  path.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` control,
  environment read, diagnostic hook, firmware input, emulator dependency, global
  mutable state, or public private-header exposure was added.
- Follow-up: verify remote GCC/Clang/CMake/install/consumer/sanitizer gates, then
  select the next evidence-led address-foundation slice. Emulator integration
  remains deferred until provenance and standalone contracts are stable.

## 2026-09-06 — P2-W7 verification — Remote direct-source gates

- Verified evidence: GitHub Actions run `33995287457` passed for implementation
  commit `c6eeb07` and completion-ledger commit `71f1b15`.
- Remote coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled
  CMake library builds, CMake/CTest, package installation, checked-in downstream
  consumer linkage, and the Clang ASan/UBSan job all passed in the standalone
  repository.
- Plan state: direct-address source snapshots remain `Complete`; address generation
  remains `Active` only for explicitly deferred effect families, modes, and
  unresolved boundary behavior.
- Scope confirmation: the verified build added no emulator dependency, firmware
  image, environment lookup, diagnostic path, public private-header exposure, or
  non-architectural knob.
- Follow-up: choose the next requirement-and-evidence-backed address slice before
  opening opcode decode or emulator integration.

## 2026-09-06 — P2-W8 — AR0-indexed single-indirect effects

- Plan: implement only the CMPT=0 single-operand WORD16 MOD5/MOD6 forms so an
  address effect can truthfully snapshot two architectural register sources before
  circular, bit-reversed, or dual-operand work expands the model further.
- State: active; implementation commit pending.
- Selection decision: prefer AR0-indexed linear addressing over the smaller MOD13
  long-offset slice because MOD5/MOD6 establish the multi-source effect shape also
  needed by later DSP-specific bit-reversed and indexed-circular modes. MOD13
  remains a separate future slice with no invented coupling.
- Evidence: pin new narrow records from TI `SPRU131G` Rev G section 5.5.3/Table 5-4,
  printed page 5-13, and section 5.5.3.3, printed page 5-15. They define MOD5 as
  old `ARx` for the operand followed by `ARx - AR0`, MOD6 as old `ARx` followed by
  `ARx + AR0`, and describe AR0 as indexing any other auxiliary register. Existing
  section 5.5.1 and 5.5.5 records retain CMPT=0 ARF selection and ARP=0 policy.
- Alias decision: admit selected AR1–AR7 only. The primary text says AR0 operates on
  an “other” auxiliary register and does not define ARF=0 self-alias ordering;
  selected AR0 therefore returns a checked unsupported result until stronger
  evidence exists.
- Boundary decision: admit subtraction only when `ARx >= AR0` and addition only
  when the mathematical sum is at most `0xFFFF`. Rejected crossings use the
  existing unresolved-boundary status and an all-zero effect. Unsigned 16-bit ARAU
  storage does not by itself prove modulo wrap.
- Effect-shape decision: add one dedicated indexed-AR0 source arm containing a
  selected-AR index/value snapshot and a copied AR0 value snapshot. Keep the
  existing CMPT0/ARP0 state policy, canonical WORD16 operand at old ARx, POST_ACCESS
  update of only the selected AR, and one-update transaction boundary. Do not turn
  source dependencies into an untyped array.
- Application decision: validate the active indexed source arm and selected
  AR1–AR7 domain before state access; then enforce mode state, read and stale-check
  selected AR followed by AR0 through existing state accessors, and commit at most
  the one described ARx update. Any unknown/stale dependency or malformed effect
  preserves complete caller-owned state.
- Ownership: `src/address/indirect_indexed.[ch]` owns MOD5/MOD6 arithmetic and
  checked result construction; `src/address/address_effect.h` owns the new tagged
  source value; `src/sequencer/address_effect_apply.[ch]` owns dependency
  validation and transactional commit. Existing `src/state/` accessors remain the
  sole register-storage boundary. Matching tests stay under address and sequencer
  owners; evidence/build synchronization remains separate delegated ownership.
- TDD plan: cover both modes, both access kinds, all selected AR1–AR7 values,
  complete 16-bit ARx and AR0 dimensions against representative opposite operands,
  and every exact safe/unsafe boundary class without falsely claiming the roughly
  120-billion-case full Cartesian product. Prove old-AR effective addresses, exact
  POST updates, copied sources, all-zero failures, mode-before-source precedence,
  both dependency knownness/staleness paths, AR0 nonmutation, and bytewise
  transactional rejection.
- Exclusions: selected-AR0 alias behavior, address wrap, LONG32 indexed operands,
  MOD4/MOD7 reverse carry, MOD8–MOD11/MOD14 circular behavior, BK, long offsets,
  memory-mapped-register restrictions, dual operands, buses, opcode decode, timing,
  diagnostics, device profiles, and emulator integration.
- Knobs: none planned.
- Follow-up: delegate planner/tests, sequencer/tests, and evidence/build work to
  Luna agents with disjoint ownership; integrate under the main orchestrator and
  run one independent closure review before validation and commit.
- Completion commit: `e85a96c` (`P2-W8: add AR0-indexed address effects`).
- Plan result: AR0-indexed single-indirect effects moved from `Active` to
  `Complete`. Address generation remains `Active` for the deliberately deferred
  modes and boundary behavior listed above.
- Delivered behavior: the value-owned effect shape now carries distinct selected
  ARx and AR0 snapshots. MOD5 and MOD6 use the old selected AR as the WORD16
  operand and schedule exactly one POST_ACCESS selected-AR subtraction or
  addition. The sequencer checks CMPT=0/ARP=0, then selected AR, then AR0, and
  preserves the complete state object on every rejection. Selected AR0 and
  unsigned boundary crossings remain explicitly unsupported or unresolved; no
  wrap behavior was inferred.
- TDD proof: COMMON_TI tests cover both access kinds, every selected AR1–AR7,
  each mode's complete 16-bit selected-AR and AR0 dimensions against safe
  opposite operands, representative cross-source vectors, and exact admitted
  boundaries. Project-contract tests own invalid discriminants, selected-AR0,
  unsafe-boundary rejection, all-zero failed effects, copied-source lifetime,
  malformed effect shape and policy, precedence, both dependencies' unknown and
  stale paths, AR0 nonmutation, one selected-AR commit, and bytewise transactional
  rejection. This does not claim the full two-source Cartesian product.
- Evidence: the live registry contains 2 targets, 7 sources, 39 immutable
  evidence records, 44 requirements, and 35 tests (31 `PASSING`, 4 `PLANNED`).
  P2-W8 adds exactly four evidence records: two narrowly pinned TI primary
  records and two project contracts.
- Correction to the P2-W7 completion entry: its recorded evidence count of 34
  was one low. Repository history shows 31 records at `fc56b5a` and 35 from the
  P2-W7 implementation commit `c6eeb07` onward. P2-W8 adds four to that actual
  base, producing the current count of 39. The old entry remains unchanged under
  the append-only ledger policy.
- Review corrections: orchestration review separated COMMON_TI safe-domain
  claims from project-owned rejection policy, removed inactive-union reads from
  zero-effect tests, made source-kind validation explicit, strengthened
  dependency and precedence vectors, and made each mode's single-source sweeps
  genuinely complete. The independent Luna closure review caught an older
  invalid-enum sentinel that became the new indexed-source value; the regression
  test now uses the next unassigned value and passes for the intended reason.
- Local validation: final-tree strict `make test` and ASan/UBSan `make test`
  passed every C executable and all 96 Python quality tests. Source boundaries
  passed for 58 files; evidence, knob, and observer-disabled artifact checks
  passed. `make PYTHON=/definitely/missing build` proved the compiler-only
  standalone path. CMake is unavailable on this host, so its configure, CTest,
  install, and consumer paths remain assigned to remote CI.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` control,
  environment lookup, diagnostic hook, firmware input, emulator dependency,
  process-global mutable state, or public private-header exposure was added.
- Follow-up: push the implementation and ledger commits, verify the remote
  GCC/Clang/CMake/install/consumer/sanitizer gates, and record that run in a
  ledger-only verification commit.

## 2026-09-06 — P2-W10 remote verification — Reverse-carry WORD16 effects

- Verified commits: implementation
  `d0328f12b1bd461040170081e5acfb44b1e210b9` and completion ledger
  `0088b5a7f4dcae197b6a64c523447048e8308e4d` on `origin/main`.
- GitHub Actions: CI run
  `https://github.com/djr-747/ti-c54x/actions/runs/34000243852` completed
  successfully.
- Remote gates: GCC and Clang both passed the direct standalone Make build and
  tests, library-only CMake configure/build, full CMake configure/build/CTest,
  install, and downstream consumer package smoke test. The Clang sanitizer job
  passed configure, build, and CTest under the configured sanitizers.
- Result: no corrective implementation or evidence change was required. The
  reverse-carry work piece remains `Complete`; final propagation/wrap,
  selected-AR0 alias ordering, and LONG32 remain explicitly unresolved.
- Knobs and isolation: unchanged; remote validation introduced no core knob,
  environment lookup, diagnostic dependency, or emulator coupling.
- Follow-up: begin the separately owned BK architectural-state/accessor
  prerequisite, then split sequencer validation/dependency ownership before
  integrating circular address effects.

## 2026-09-06 — P2-W9 verification — Remote signed long-offset gates

- Verified evidence: GitHub Actions run `33998478157` passed for implementation
  commit `4d75222` and completion-ledger commit `31f364e`.
- Remote coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled
  CMake library builds, CMake/CTest, package installation, checked-in downstream
  consumer linkage, and the Clang ASan/UBSan job all passed in the standalone
  repository.
- Plan state: signed long-offset premodify remains `Complete`; address generation
  remains `Active` only for explicitly deferred modes and unresolved boundary
  behavior.
- Scope confirmation: the verified build added no emulator dependency, firmware
  image, environment lookup, diagnostic path, public private-header exposure, or
  non-architectural knob. The corrected split `SPRU172C` class 9B/page 3-21 and
  class 13B/page 3-29 evidence records are present in the verified tree.
- Follow-up: select the next requirement-and-evidence-backed address-foundation
  slice. Opcode decode and emulator integration remain blocked or deferred until
  the address/evidence foundation is sufficiently complete.

## 2026-09-06 — P2-W8 verification — Remote AR0-indexed gates

- Verified evidence: GitHub Actions run `33996579756` passed for implementation
  commit `e85a96c` and completion-ledger commit `5ce1003`.
- Remote coverage: GCC and Clang strict direct-Make builds/tests, tests-disabled
  CMake library builds, CMake/CTest, package installation, checked-in downstream
  consumer linkage, and the Clang ASan/UBSan job all passed in the standalone
  repository.
- Plan state: AR0-indexed single-indirect effects remain `Complete`; address
  generation remains `Active` only for explicitly deferred modes and unresolved
  boundary behavior.
- Scope confirmation: the verified build added no emulator dependency, firmware
  image, environment lookup, diagnostic path, public private-header exposure, or
  non-architectural knob. The P2-W7 evidence-count correction recorded above is
  also present in the verified ledger.
- Follow-up: select the next requirement-and-evidence-backed address-foundation
  slice. Opcode decode and emulator integration remain blocked or deferred until
  the address/evidence foundation is sufficiently complete.

## 2026-09-06 — P2-W9 — Signed long-offset premodify

- Plan: implement the CMPT=0 MOD13 `*+ARx(lk)` address effect for both WORD16
  and LONG32 data operands, proving signed pre-index arithmetic and a copied
  instruction-extension source before decode or program fetch is introduced.
- State: active; implementation commit pending.
- Selection decision: complete both operand widths in one cohesive MOD13 slice.
  TI `SPRU131G` Rev G section 5.5.3/Table 5-4 (printed page 5-14) explicitly
  defines a signed 16-bit `lk`, addition before use, replacement of ARx by the
  sum, and use of that sum as the operand address. Section 5.8 (printed pages
  5-28–5-29) supplies the existing long32 MSW/LSW pairing rule. TI `SPRU172C`
  Rev C classes 9B and 13B (printed pages 3-28–3-29) independently establish
  long-word read and write operands using long-offset indirect addressing.
- MOD12 decision: defer `*ARx(lk)`. The CPU reference calls its carrier a
  16-bit long offset but, unlike MOD13/MOD14, does not explicitly call it signed
  and, unlike MOD15, does not call it unsigned. Official negative-literal
  assembler/compiler examples corroborate signed intent but do not normatively
  define CPU arithmetic. Record the signedness gap as unresolved rather than
  applying MOD13 semantics by symmetry.
- Arithmetic decision: accept a raw copied `uint16_t long_offset_word` and
  sign-extend it explicitly into `int32_t`; never rely on implementation-defined
  unsigned-to-signed narrowing. Admit only mathematical sums in `0..0xFFFF` and
  return `UNRESOLVED_BOUNDARY` with an all-zero effect outside that range. The
  offset remains one data-word address displacement for both widths; it is not
  scaled by the MOD0–MOD3 one/two-word update rule.
- Effect-shape decision: add a dedicated value-owned AR-long-offset source arm
  containing the selected AR index/value, raw offset word, and MOD field. A
  successful effect carries CMPT0/ARP0, a canonical WORD16 or LONG32 operand at
  the computed sum, and exactly one selected-AR PRE_ACCESS update to that sum.
  The raw offset is immutable instruction input rather than architectural state,
  so it is shape-validated but never stale-checked.
- Validation decision: `src/address/indirect_long_offset.[ch]` owns signed
  evaluation, result construction, and intrinsic MOD13 effect validation. The
  sequencer calls that narrow address-owned validator before mode/source reads,
  then enforces CMPT=0/ARP=0, stale-checks the selected AR through state
  accessors, and commits at most the one described write. This avoids duplicating
  address arithmetic in the sequencer and keeps nonlinear future modes out of a
  generic arithmetic assumption.
- Ownership: `src/address/address_effect.h` owns the new tagged source value;
  `src/address/indirect_long_offset.[ch]` owns MOD13 planning and intrinsic
  validation; `src/sequencer/address_effect_apply.[ch]` owns validation order,
  architectural dependency checks, and transactional commit. Tests mirror the
  address and sequencer owners. Evidence/build synchronization is a separate
  bounded work stream.
- TDD plan: for both widths, both access kinds, and all selected AR0–AR7, exhaust
  the complete 16-bit AR dimension with zero offset and the complete raw offset
  dimension with AR=`0x8000`; add representative cross-source/parity vectors and
  exact zero, +1, -1, +32767, and -32768 safe/rejected boundaries. This is not a
  claim over the full AR-by-offset Cartesian product. Separately prove copied
  sources, canonical long32 pairing, exact-zero failed effects, malformed
  discriminants and source/update/operand relations, mode-before-source
  precedence, unknown/stale selected AR, one PRE_ACCESS commit, and bytewise
  transactional rejection.
- Exclusions: MOD12 until signedness is resolved; MOD4/MOD7 reverse carry;
  MOD8–MOD11/MOD14 circular behavior and BK/base state; MOD15 absolute long
  offset; CMPT=1/ARP update; address wrap; memory-mapped-register restrictions;
  repeat-single rejection; instruction extension fetch/PC accounting; buses,
  opcode decode, timing, devices, diagnostics, firmware, and emulator integration.
- Knobs: none planned.
- Follow-up: delegate address/tests, sequencer/tests, and evidence/build work to
  Luna agents with disjoint ownership; integrate under the main orchestrator,
  run an independent closure review, validate once, commit, and verify remotely.

## 2026-09-06 — P2-W9 completion — Signed long-offset premodify

- Plan state: signed long-offset premodify is `Complete`; address generation
  remains `Active` for the explicitly deferred modes and unresolved boundary
  behavior. Opcode decode remains blocked on address/evidence foundations, and
  emulator integration remains deferred.
- Implementation commit: `4d75222` (`P2-W9: add signed long-offset address
  effects`).
- Delivered behavior: CMPT=0 MOD13 `*+ARx(lk)` planners now copy the raw 16-bit
  extension, sign-extend it explicitly into `int32_t`, and construct WORD16 or
  LONG32 effects for READ and WRITE access. The checked in-domain sum is both
  the first operand address and one selected-AR PRE_ACCESS replacement; LONG32
  uses the existing canonical MSW/LSW pairing without scaling `lk`.
- Boundary decision: mathematical sums outside `0..0xFFFF` return
  `UNRESOLVED_BOUNDARY` with an all-zero effect. No wrap behavior is inferred.
  MOD12 remains an explicit `UNRESOLVED` signedness requirement because the
  pinned CPU-reference text does not classify that carrier as signed or
  unsigned.
- Ownership: `src/address/address_effect.h` owns the tagged copied source;
  `src/address/indirect_long_offset.[ch]` owns MOD13 arithmetic, planning, and
  intrinsic validation; `src/sequencer/address_effect_apply.[ch]` owns ordered
  state checks and the transactional commit. The four new tests mirror their
  address and sequencer owners. Build registration and evidence documentation
  remain in their existing owning files; no monolith was introduced.
- TDD proof: COMMON_TI coverage exercises every AR0–AR7 index, both operand
  widths, both access kinds, the complete selected-AR dimension at zero offset,
  the complete raw-offset dimension at AR=`0x8000`, representative combinations,
  LONG32 parity, and exact signed boundaries. Project-contract coverage proves
  invalid-argument precedence, exact unresolved crossings, copied-source
  lifetime, all-zero failed effects, malformed intrinsic shapes, phase and
  mode/source precedence, unknown and stale selected AR state, one PRE_ACCESS
  write, and bytewise transactional rejection. It does not claim the full
  AR-by-offset Cartesian product.
- Evidence: the live registry contains 2 targets, 7 sources, 45 immutable
  evidence records, 50 requirements, and 39 tests (35 `PASSING`, 4 `PLANNED`).
  The new records separate architectural assertions from project rejection and
  transaction contracts.
- Correction to the committed P2-W9 kickoff entry: its `SPRU172C` locator
  incorrectly grouped classes 9B and 13B as printed pages 3-28–3-29. The precise
  locations are class 9B on printed page 3-21 for long-offset long-word reads and
  class 13B on printed page 3-29 for writes. The registry and current evidence
  documents use separate records for those locators; the old entry remains
  unchanged under the append-only ledger policy.
- Review corrections: orchestration review kept safe-domain architectural proof
  separate from project-owned boundary rejection, removed implementation-defined
  signed narrowing from production and test oracles, strengthened exact-boundary
  vectors, compared the complete architectural state after successful apply,
  and made invalid-enum sentinels durable. Independent Luna closure review found
  and verified the corrected split `SPRU172C` locators and reported no remaining
  blockers; targeted GCC analyzer and sanitizer checks were clean.
- Local validation: final-tree strict `make test` passed every C executable and
  all 96 Python quality tests. ASan/UBSan `make test` also passed. Evidence,
  source-boundary (64 files), knob-registry, observer-disabled artifact, and
  whitespace checks passed. `make PYTHON=/definitely/missing build` proved the
  compiler-only standalone path. CMake is unavailable on this host, so its
  configure, CTest, install, consumer, and sanitizer paths remain assigned to
  remote CI.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` control,
  environment lookup, diagnostic hook, firmware input, emulator dependency,
  process-global mutable state, or public private-header exposure was added.
- Follow-up: push the implementation and ledger commits, verify the remote
  GCC/Clang/CMake/install/consumer/sanitizer gates, and record that run in a
  ledger-only verification commit.

## 2026-09-06 — P2-W10 — Reverse-carry arithmetic and WORD16 effects

- Plan: implement a constrained CMPT=0 MOD4/MOD7 reverse-carry foundation and
  WORD16 address-effect slice. This deliberately prioritizes a DSP-specific
  addressing primitive over the simpler MOD15 absolute-offset form.
- State: active; implementation commit pending.
- Selection decision: independent Luna audits compared reverse carry, circular
  addressing, and MOD15. Reverse carry is next because it exercises nonlinear
  ARAU behavior with no new architectural register prerequisite. Circular
  MOD8–MOD11/MOD14 remains the next larger family but first needs a separately
  owned BK state/accessor work piece and circular-mapping proof. MOD15 is
  evidence-ready but offers less foundation leverage.
- Primary evidence: TI `SPRU131G` Rev G section 5.5.2 (printed p. 5-11)
  defines ARAU arithmetic as unsigned 16-bit. Section 5.5.3/Table 5-4 (printed
  p. 5-13) defines MOD4 `*ARx-0B` and MOD7 `*ARx+0B`: the old ARx addresses the
  operand and ARx is updated after access by reverse-carry subtraction/addition
  of AR0. Section 5.5.3.5 (printed pp. 5-18–5-19) requires AR0=`2^(N-1)` for an
  FFT size `2^N`, states that carry propagates left-to-right rather than the
  normal direction, and supplies the `0x60,0x68,0x64,0x6c,0x62,0x6a,0x66,0x6e`
  worked sequence plus the four-bit table. Section 5.5.1/Table 5-3 (printed pp.
  5-10–5-11) supplies CMPT=0/ARP=0 selection. Section 5.5.5/Table 5-9 (printed
  p. 5-23) proves `*AR0+0B` and `*AR0-0B` syntax is legal but does not define
  the read/update ordering when the selected pointer and index alias.
- Arithmetic boundary: `src/foundation/reverse_carry.[ch]` will own a portable
  unsigned 16-bit MSB-to-LSB add/subtract primitive. It returns a value only
  when no carry or borrow propagates past bit 0; an escaping propagation is an
  explicit non-OK result with no usable wrapped candidate. Production and test
  code must not use signed shifts, implementation-defined narrowing, host
  bit-field layout, or a modulo shortcut that silently chooses wrap behavior.
- Planner boundary: `src/address/indirect_reverse_carry.[ch]` will accept only
  WORD16 READ/WRITE, MOD4/MOD7, a selected AR1–AR7 snapshot, and a nonzero
  one-hot AR0 snapshot. The old selected AR is the operand address; a successful
  plan carries one selected-AR POST_ACCESS update and the existing CMPT0/ARP0
  policy. Invalid discriminants and non-one-hot AR0 are project-contract
  rejections; final propagation returns `UNRESOLVED_BOUNDARY` with an all-zero
  effect.
- Alias and width decision: selected AR0 is not declared illegal—official syntax
  explicitly permits it—but its same-register source/update ordering is
  unresolved, so this slice returns an explicit unsupported result and records
  a no-test architectural requirement. LONG32 is also deferred because no
  primary source was found that directly binds MOD4/MOD7 to its width behavior;
  no update scaling is inferred from the MOD1–MOD3-only footnote.
- Effect and sequencer shape: `src/address/address_effect.h` will own a distinct
  reverse-carry source arm containing the selected index/value, copied AR0, and
  MOD field. The address-owned intrinsic validator will prove the operand and
  update relation. `src/sequencer/address_effect_apply.[ch]` will enforce that
  shape before mode/source reads, then read selected AR followed by AR0 and
  commit at most one POST_ACCESS selected-AR write without recomputing
  arithmetic.
- File-shape decision: keep reverse-carry arithmetic and planning in separate
  modules with mirrored foundation/address tests. The sequencer source is 366
  lines before this work and may remain whole only while it stays below the
  500-line review threshold. Before circular-effect integration, split intrinsic
  validation/dependency work into a named sequencer-owned module rather than
  allowing `address_effect_apply.c` to become a monolith.
- TDD plan: COMMON_TI tests reproduce the exact TI worked sequence and inverse
  safe MOD4 transitions, prove old-address/POST_ACCESS behavior for READ and
  WRITE across AR1–AR7, and preserve CMPT0/ARP0. Project-contract tests use an
  independent bit-by-bit oracle over every 16-bit left operand and every
  representable one-hot AR0 for both operations, including explicit escaping
  results; separately prove invalid/rejection precedence, exact-zero failed
  values/effects, copied-source lifetime, malformed shape, ordered unknown/stale
  dependencies, and bytewise transactional rejection. This does not claim the
  full two-input Cartesian product or unresolved wrap/alias semantics.
- Exclusions: carry/borrow discard and reverse-counter wrap, selected-AR0 alias
  ordering, LONG32, dual operands, CMPT=1/ARP update, circular/BK state, MMR
  routing, buses, opcode decode, timing/hazards, repeat behavior, devices,
  diagnostics, firmware, and emulator integration.
- Knobs: none planned.
- Follow-up: commit this kickoff decision, delegate foundation/address,
  sequencer, and evidence/build work to Luna agents with disjoint file ownership,
  integrate under the main orchestrator, run an independent closure review,
  validate once, commit, push, and verify remotely.

## 2026-09-06 — P2-W10 completion — Reverse-carry WORD16 effects

- State: complete in implementation commit
  `d0328f12b1bd461040170081e5acfb44b1e210b9`.
- Outcome: added a pure unsigned 16-bit reverse-carry foundation, a constrained
  CMPT=0 MOD4/MOD7 WORD16 address planner and intrinsic validator, a distinct
  value-owned reverse-carry source arm, and transactional sequencer application.
  Valid effects preserve the old selected-AR operand address and commit exactly
  one POST_ACCESS selected-AR update after ordered selected-AR then AR0 state
  checks.
- Ownership: `src/foundation/reverse_carry.[ch]` owns only reverse-carry
  arithmetic; `src/address/indirect_reverse_carry.[ch]` owns mode validation,
  planning, and effect-shape validation; `src/address/address_effect.h` owns the
  copied source type; and `src/sequencer/address_effect_apply.[ch]` owns ordered
  state validation and commit. Six new tests mirror the foundation, address,
  and sequencer owners. Build and evidence changes remain in their existing
  owning files.
- TDD proof: COMMON_TI tests reproduce TI's complete worked MOD7 sequence and
  safe MOD4 inverse transitions, then exercise both access kinds and AR1-AR7
  through planner and sequencer boundaries. Project-contract tests independently
  map reverse-carry through bit reversal and ordinary unsigned arithmetic across
  every 16-bit left operand, all 16 one-hot right operands, and both operations.
  They also prove invalid-input precedence, whole-register and FFT-window escape
  rejection, exact-zero failed values/effects, copied-source lifetime, intrinsic
  malformation rejection, phase/mode/source precedence, selected-AR-before-AR0
  dependency ordering, and bytewise transactional rejection.
- Evidence: the live registry now contains 2 targets, 7 sources, 51 immutable
  evidence records, 60 requirements, and 45 tests (41 `PASSING`, 4 `PLANNED`).
  TI records own only the documented old-address/update order, bit-15-to-bit-0
  propagation, power-of-two AR0 use, and legal selected-AR0 syntax. Project
  records separately own validation, deterministic status, zero-result, and
  transaction behavior.
- Explicit nonclaims: final carry/borrow discard and reverse-counter wrap,
  selected-AR0 source/update ordering, LONG32 applicability or scaling, dual
  operands, CMPT=1/ARP update, circular/BK state, MMR routing, buses, opcode
  decode, timing/hazards, repeat behavior, devices, diagnostics, firmware, and
  emulator integration remain unresolved or deferred.
- Review corrections: orchestration review moved boundary and invalid-operation
  assertions out of the COMMON_TI arithmetic test, replaced a production-shaped
  ripple oracle with an independent bit-reversal oracle, added FFT-window escape
  vectors, corrected the evidence direction to bit 15/MSB toward bit 0/LSB,
  corrected the exact project anchor, and removed an unsupported planner-width
  claim. Sequencer tests construct unknown state only through state-owner APIs.
  Independent Luna closure review reported no remaining blockers and a clean GCC
  analyzer pass.
- File-shape decision: `src/sequencer/address_effect_apply.c` is 411 lines, below
  the 500-line review threshold. It remains cohesive for this slice, but must be
  split into named sequencer-owned validation/dependency modules before circular
  effects are integrated.
- Local validation: final-tree strict `make test` passed every registered C test
  and all 96 Python quality tests. ASan/UBSan `make test` also passed. Evidence
  (2 targets, 7 sources, 60 requirements, 45 tests), source-boundary (74 files),
  knob-registry (7 knobs), observer-disabled artifact, and whitespace checks
  passed. `make PYTHON=/definitely/missing build` proved the compiler-only
  standalone path. CMake is unavailable on this host, so configure, CTest,
  install, consumer, and sanitizer variants remain assigned to remote CI.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob,
  environment lookup, diagnostic hook, file I/O, firmware input, emulator
  dependency, process-global mutable state, or public private-header exposure
  was added.
- Follow-up: push the implementation and ledger commits, verify the remote
  GCC/Clang/CMake/install/consumer/sanitizer gates, and record that run in a
  ledger-only verification commit.

## 2026-09-06 — P2-W11 — BK architectural state prerequisite

- Plan: add the private 16-bit circular-buffer size register as explicitly
  owned architectural state with checked knownness-aware accessors. This is a
  prerequisite only; circular address arithmetic remains a later slice.
- State: active; implementation commit pending.
- Primary evidence: TI `SPRU131G` Rev G section 3.3.4.8 (printed p. 3-28)
  defines BK as the 16-bit circular-buffer size register used by the ARAUs.
  Section 3.3.4/Table 3-3 (printed p. 3-27) maps BK to CPU MMR data-memory
  address `0x19`; that mapping will be recorded but not routed in this slice.
  Section 5.5.3.4 (printed pp. 5-15–5-16) is reserved as future circular-math
  authority. Section 6.9's reset list (printed p. 6-25) does not prescribe BK,
  so no architectural reset value is inferred.
- State boundary: `src/state/arch_state.h` continues to own the private aggregate
  and gains copied `uint16_t` BK storage. `src/state/register_layout.h` owns its
  scalar-knownness bit. New `src/state/block_size_register.[ch]` owns only BK
  reads and writes; circular planners and sequencers must use that interface and
  must not touch the field or knownness mask directly.
- Reset decision: common reset clears physical storage to zero through the
  existing deterministic aggregate reset, but BK remains architecturally
  unknown. That zero-plus-unknown representation is a project contract, not a
  TI reset claim. Unknown and invalid reads preserve caller output; writes copy
  every `uint16_t` value exactly and mark BK known; rejected calls preserve the
  complete state byte-for-byte.
- TDD plan: a COMMON_TI state-owner test exhausts all 65,536 values through the
  checked write/read seam to prove exact 16-bit storage. A separate
  PROJECT_CONTRACT_ONLY test proves reset unknownness, output preservation,
  zero/one/maximum round trips, sole-field/knownness mutation, NULL rejection,
  reset clearing of stale storage and knownness, and bytewise nonmutation on
  every rejected call.
- Evidence plan: add one TI BK-width record, one TI MMR-location record, one TI
  circular-definition record for the future boundary, and one project accessor
  contract. Implement only the BK-width and project-access requirements. Keep
  MMR routing and circular BK consumption as explicit planned requirements with
  no passing-test claim.
- Ownership and build plan: mirror `src/state/block_size_register.[ch]` with
  `tests/unit/state/test_block_size_register.c` and
  `test_block_size_register_contract.c`; register the source in normal and
  observer-disabled artifacts and both tests in direct Make and CMake/CTest.
  No public header or ABI changes.
- Sequencer decision: the independent audit found
  `src/sequencer/address_effect_apply.c` currently mixes pure shape validation,
  mode checks, dependency reads, and commit orchestration at 411 lines. P2-W12
  will extract a private zero-on-error validation/commit-description seam as a
  separate atomic refactor after BK and before circular integration.
- Exclusions: BK reset value, raw MMR instruction semantics, bus or data-page
  routing, circular EFB/EOB/base/index arithmetic, BK=0 behavior, MOD8–MOD11 and
  MOD14, dual operands, LONG32 circular scaling, pipeline conflicts, timing,
  device profiles, opcode decode, diagnostics, firmware, and emulator
  integration.
- Knobs: none planned.
- Ledger correction: the published `P2-W10 remote verification` entry was
  inserted before the P2-W9/P2-W10 chronological block rather than at physical
  end-of-file. Its recorded run `34000243852` is valid and green; the subsequent
  ledger-only commit also passed final CI run `34000306631`. Under the append-only
  rule, the old entry remains untouched and this note corrects only its ordering.
- Follow-up: commit this kickoff decision, delegate state/tests and
  evidence/build/docs to Luna agents with disjoint ownership, integrate under
  the main orchestrator, run an independent closure review, validate once,
  commit, push, and verify remotely.

## 2026-09-06 — P2-W11 completion — BK architectural state prerequisite

- State: complete in implementation commit
  `9dcc7d9d84c1c93cc35c27ead3a582b3df4e4a95`.
- Outcome: the private architectural aggregate now carries an exact 16-bit BK
  value and a dedicated scalar-knownness bit. The state-owned
  `ti_c54x_arch_state_read_bk` and `ti_c54x_arch_state_write_bk` seam reports
  existing OK/UNKNOWN/INVALID_ARGUMENT statuses, preserves rejected outputs and
  state, and accepts every `uint16_t` value without truncation.
- Reset boundary: deterministic common-state construction physically clears BK
  storage but does not mark it known. This zero-plus-unknown representation is
  explicitly project policy because the pinned common TI reset list prescribes
  no BK value; it is not presented as a hardware reset assertion.
- Ownership: `src/state/arch_state.[ch]` owns aggregate storage and reset,
  `src/state/register_layout.h` owns the knownness-bit position, and
  `src/state/block_size_register.[ch]` owns only checked BK access. Mirrored
  tests live under `tests/unit/state/`. No circular planner, sequencer, bus,
  MMR, public header, or emulator module reaches into this state directly.
- TDD proof: the COMMON_TI test round-trips all 65,536 representable BK values
  through the checked state seam. The PROJECT_CONTRACT_ONLY test proves reset
  unknownness and physical representation, caller-output preservation, exact
  zero/one/maximum values, sole BK/knownness mutation, stale-state reset
  clearing, NULL precedence, and bytewise nonmutation on rejection.
- Evidence: the live registry now contains 2 targets, 7 sources, 55 immutable
  evidence records, 64 requirements, and 49 tests (43 `PASSING`, 6 `PLANNED`).
  TI records separately pin BK width, CPU MMR address `0x0019`, and BK's future
  circular-size role. Only the width requirement is implemented here; MMR
  routing and circular BK dependency remain planned with null runners.
- Review corrections: orchestration standardized the symbols on the existing
  `ti_c54x_arch_state_read_*`/`write_*` naming convention, narrowed reset wording
  from an initialization claim to an evidence-bound unknown value, added the
  direct `stddef.h` dependency in the contract test, and rewrote planned
  requirements as target behavior rather than process notes. The evidence gate
  caught a forbidden timing token in a planned circular requirement; the claim
  was narrowed to BK dependency and passed on rerun. Independent Luna closure
  review and GCC analyzer checks found no remaining blockers.
- File shape: the new owner is 41 C lines plus a 39-line private header;
  `arch_state.c` remains 237 lines. All remain well below review thresholds.
- Local validation: final-tree strict `make test` passed every registered C test
  and all 96 Python quality tests. ASan/UBSan `make test` also passed. Evidence
  (2 targets, 7 sources, 64 requirements, 49 tests), source-boundary (78 files),
  knob-registry (7 knobs), observer-disabled artifact, GCC analyzer, and
  whitespace checks passed. `make PYTHON=/definitely/missing build` proved the
  compiler-only standalone path. CMake is unavailable locally, so configure,
  CTest, install, consumer, and sanitizer variants remain assigned to remote CI.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob,
  environment lookup, diagnostic hook, file I/O, firmware input, emulator
  dependency, process-global mutable state, or public ABI surface was added.
- Follow-up: push and verify P2-W11 remotely, then execute P2-W12 as an atomic
  sequencer validation/dependency split before implementing circular effects.

## 2026-09-06 — P2-W11 remote verification — BK architectural state

- Verified commits: implementation
  `9dcc7d9d84c1c93cc35c27ead3a582b3df4e4a95` and completion ledger
  `b57194e83f33e3c8ba70ad31e5b6b2e2570176d6` on `origin/main`.
- GitHub Actions: CI run
  `https://github.com/djr-747/ti-c54x/actions/runs/34001357808` completed
  successfully.
- Remote gates: GCC and Clang both passed the direct standalone Make build and
  tests, library-only CMake configure/build, full CMake configure/build/CTest,
  install, and downstream consumer package smoke test. The Clang sanitizer job
  passed configure, build, and CTest under the configured sanitizers.
- Result: no corrective implementation, evidence, state-ownership, or build
  change was required. BK state remains a private prerequisite only; MMR
  routing and circular BK consumption remain explicitly planned.
- Knobs and isolation: unchanged; remote validation introduced no `TEST`,
  `OBSERVE`, or `A/B` knob, environment lookup, diagnostic dependency, firmware
  input, public ABI change, or emulator coupling.
- Follow-up: execute P2-W12 as an atomic behavior-preserving split of sequencer
  validation/dependency ownership before implementing circular effects.

## 2026-09-06 — P2-W12 kickoff — Sequencer address-effect decomposition

- State: active; this is a behavior-preserving architecture work piece, not a
  new addressing-mode implementation.
- Objective: remove read-only validation and architectural dependency checking
  from `src/sequencer/address_effect_apply.c` before circular addressing adds
  BK as another explicit dependency. Preserve the complete externally observed
  status, precedence, read-order, and transactional-mutation contract.
- Ownership: new private `src/sequencer/address_effect_validate.[ch]` owns
  intrinsic effect validation, requested-phase validation, ordered mode/source
  reads, and production of a caller-owned commit descriptor. The existing
  `address_effect_apply.[ch]` remains the public-private application boundary
  and becomes the sole owner of the final AR write. Neither module owns address
  arithmetic, planner policy, state storage, buses, opcode decode, diagnostics,
  or public ABI.
- Seam: `ti_c54x_sequencer_validate_address_effect` borrows immutable state and
  result objects, receives the requested phase, and fills a discriminated
  `{target, ar_index, value_after}` commit descriptor only after complete
  validation. The descriptor is zeroed on entry and remains zero for every
  rejection. The validator never mutates state; `NONE` and an AR0 write remain
  distinguishable by the target discriminator.
- Preserved precedence: invalid pointers or requested phase; non-OK planner
  result; address-owner intrinsic validators; generic operand, source, update,
  and policy shape; requested-phase mismatch; update/effective-address
  relation; mode-state reads; source-dependency reads; commit description.
  CMPT precedes ARP, the selected AR precedes copied AR0 for indexed and
  reverse-carry effects, and CPL precedes DP or SP for direct effects.
- TDD proof: add a mirrored
  `tests/unit/sequencer/test_address_effect_validate_contract.c` project test
  that poisons the output descriptor before rejection cases, proves exact
  NONE/PRE/POST descriptors including a valid AR0 target, proves bytewise state
  nonmutation, and exercises the existing status/read-order matrix directly.
  Every existing apply, direct, indexed, long-offset, reverse-carry, operand,
  and state-policy test remains a regression gate.
- Evidence: add one immutable project-contract record, one implemented
  sequencer requirement, and one passing test record for the private validation
  seam. No new TI architectural claim is made by this refactor.
- File-shape gate: the extracted validator is one cohesive read-only owner and
  must remain below the 500-line review threshold. If circular support would
  approach that threshold, split intrinsic shape validation from ordered state
  dependencies before adding it; do not regrow an all-purpose sequencer file.
- Build/isolation: register the new source in standalone CMake, direct Make,
  and observer-disabled artifacts, and the new test in Make and CTest. Add no
  emulator, firmware, host, environment, allocation, file, logging, mutable
  global, diagnostic, or installed-header dependency.
- Knobs: none planned; the `TEST`/`OBSERVE`/`A/B` registry is unchanged.
- Follow-up: commit this kickoff decision, delegate disjoint implementation and
  proof/build ownership to Luna workers, integrate and independently review the
  final tree, run the local gates once, then commit and verify remotely.

## 2026-09-06 — P2-W12 completion — Sequencer address-effect decomposition

- State: complete in implementation commit
  `cebd507d221e50ee6667f65594c77da5c5b17abc`.
- Outcome: `ti_c54x_sequencer_validate_address_effect` is now a private,
  read-only transaction-preparation seam. It validates one borrowed planner
  result against borrowed architectural state and returns either a fully
  zeroed NONE descriptor or one discriminated selected-AR commit descriptor.
  `ti_c54x_sequencer_apply_address_result` is reduced to orchestration and the
  sole final AR write.
- Preserved behavior: argument and requested-phase rejection precedes non-OK
  result rejection; address-owned MOD13/reverse validators precede generic
  effect checks; intrinsic source/update/policy shape precedes phase mismatch;
  phase mismatch precedes the update/address relation; mode state precedes
  source state. CMPT and ARP, selected AR and copied AR0, and CPL and DP/SP keep
  their established read/status order. Direct effects still produce no commit,
  and an AR0 write remains distinct from NONE through the target discriminator.
- Transaction boundary: the validator clears the complete caller-owned commit
  object before every path that can return an error and never mutates state.
  The apply layer consumes only a successful descriptor, performs no duplicate
  validation or dependency read, and permits at most one checked AR write.
- Ownership and file shape: `address_effect_validate.c` is the cohesive
  read-only owner at 394 lines with a 45-line private header;
  `address_effect_apply.c` is 41 lines. The initial combined proof reached 499
  lines, so integration split it by responsibility into a 226-line validation/
  descriptor contract and a 246-line dependency-order contract rather than
  accepting a near-threshold test monolith. Future circular growth must split
  intrinsic shape from state dependencies before the validator approaches 500
  lines.
- TDD proof: `test_address_effect_validate_contract.c` proves null and invalid
  inputs, rejected planner results, malformed operand/source/update/policy
  shapes, phase precedence, full zero-on-error output, exact NONE/PRE/POST
  descriptors including AR0, and bytewise state nonmutation.
  `test_address_effect_dependency_contract.c` independently reaches and proves
  CMPT/ARP, selected-AR-before-AR0, and CPL-before-DP/SP dependency outcomes,
  including unknown and stale sources. Every pre-existing sequencer and address
  application test remains unchanged as behavior-preservation coverage.
- Evidence: one immutable project-contract record and one implemented
  sequencer requirement link both new proof owners. The registry now contains
  2 targets, 7 sources, 56 immutable evidence records, 65 requirements, and 51
  tests (45 `PASSING`, 6 `PLANNED`). No TI architectural behavior was added by
  this refactor.
- Review corrections: integration made descriptor clearing padding-safe,
  corrected the new header's mandatory module-contract marker, removed
  misleading public-boundary wording, split the 499-line test, repaired the
  Make target layout required by static runner proof, and fixed the dependency
  helper to request each effect's actual phase so its vectors reach state reads.
  A final independent Luna review found no remaining blocker and identified a
  stale current-count paragraph, which was corrected in the architecture
  baseline along with the new validation/apply ownership description.
- Local validation: final-tree strict `make -B test` passed every registered C
  test and all 96 Python quality tests. ASan/UBSan `make -B test` also passed.
  Source boundaries passed for 82 files; evidence passed at 65 requirements and
  51 tests; knob and observer-disabled artifact gates passed. GCC `-fanalyzer`
  accepted both sequencer implementation files, and
  `make -B PYTHON=/definitely/missing build` proved the standalone
  compiler-only path. CMake configure, CTest, install, consumer, and sanitizer
  variants remain assigned to remote CI.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob,
  environment lookup, diagnostic hook, file I/O, firmware input, emulator
  dependency, mutable process global, allocator, or public ABI surface was
  added.
- Remote context: the P2-W12 kickoff commit passed GitHub Actions run
  `34001721915`. Push and verify the implementation and this completion ledger
  next, then begin the evidence-first circular-addressing work piece against
  the explicit BK dependency.

## 2026-09-06 — P2-W12 remote verification — Sequencer decomposition

- Verified commits: implementation
  `cebd507d221e50ee6667f65594c77da5c5b17abc` and completion ledger
  `8816b117b4d74cd130ac91c31b4b53abc7c9a183` on `origin/main`.
- GitHub Actions: CI run
  `https://github.com/djr-747/ti-c54x/actions/runs/34002761612` completed
  successfully.
- Remote gates: GCC and Clang passed direct standalone Make build/tests,
  library-only CMake configure/build, full CMake configure/build/CTest,
  installation, and downstream consumer package smoke tests. The Clang
  sanitizer job passed configure, build, and CTest.
- Result: no corrective implementation, evidence, ownership, packaging, or
  test change was required. The private validator remains read-only and the
  41-line apply owner remains the only architectural mutation boundary.
- Knobs and isolation: unchanged; remote validation introduced no `TEST`,
  `OBSERVE`, or `A/B` knob, environment lookup, diagnostic dependency,
  firmware input, public ABI change, or emulator coupling.
- Follow-up: begin P2-W13 with an evidence-first circular-addressing boundary
  and BK dependency plan before implementing any arithmetic or effects.

## 2026-09-06 — P2-W13 kickoff — Sequencer validation subownership

- State: active; this is a behavior-preserving prerequisite for circular
  addressing, not a new architectural mode.
- Trigger: P2-W12 reduced the mutation owner to 41 lines but left the cohesive
  read-only validator at 394 lines. Adding a circular source arm, intrinsic
  shape, selected-AR/BK dependency order, and statuses there would approach the
  500-line review threshold and recreate mixed ownership.
- Ownership: new private `src/sequencer/address_effect_intrinsic.[ch]` owns
  result-status, address-owner validator dispatch, generic operand/source/
  update/policy shape, requested-phase mismatch, effective-address relation,
  and production of a candidate commit. New private
  `src/sequencer/address_effect_dependencies.[ch]` owns mode and source state
  reads only. `address_effect_validate.c` owns argument checks, zero-on-error
  orchestration, and publication of the candidate after dependencies succeed;
  `address_effect_apply.c` remains the sole mutation owner.
- Seam contract: the top-level validator clears caller output, rejects invalid
  state/result/output pointers and requested phase, asks the intrinsic owner to
  fill a local candidate, asks the dependency owner to validate borrowed state
  against the already-validated effect, and copies the candidate out only on
  success. The dependency seam is private and requires an intrinsically valid
  effect; it never repairs or mutates state.
- Preserved precedence and order: invalid arguments/requested phase; non-OK
  result; address-specific and generic shape; policy mapping; phase mismatch;
  update/address relation; CMPT then ARP; selected AR then copied AR0; CPL then
  DP/SP. Every existing status value, direct NONE outcome, AR0 discriminator,
  and single-AR commit must remain unchanged.
- TDD: no new behavior or conformance claim is introduced. The two P2-W12 seam
  tests and all existing direct, indexed, long-offset, reverse-carry, operand,
  mode-policy, state, and transactional application tests remain the executable
  proof. The split is accepted only if the full final suite is unchanged and
  both new implementation owners remain well below the review threshold.
- Build/isolation: register both sources in standalone CMake, direct Make, and
  observer-disabled artifacts. Add no installed header, source-kind, status,
  arithmetic, firmware, emulator, environment, diagnostic, allocator, logging,
  mutable global, or knob.
- Circular boundary for P2-W14: the official TI audit confirmed MOD8/MOD10 as
  fixed-step circular post-updates and the printed algorithm partitions
  `0 <= index + step < BK`, `index + step >= BK`, and
  `index + step < 0`; BK zero means no circular modification. P2-W13 adopts none
  of those semantics. Their exact evidence records, safe domain, source shape,
  BK read order, and tests belong to the next work piece.
- Knobs: none planned; the registry remains unchanged.
- Follow-up: commit this kickoff decision, delegate disjoint core extraction
  and build ownership to Luna workers, integrate under the main orchestrator,
  independently review exact behavior preservation, validate once, then commit
  and verify remotely before circular implementation begins.

## 2026-09-06 — P2-W13 completion — Sequencer validation subownership

- State: complete in implementation commit
  `55549dbf465c29bec7c2d3dcdebafc2dfcd9d6d4`.
- Outcome: the prior 394-line read-only validator is now three explicit private
  owners. `address_effect_intrinsic.[ch]` owns state-free effect shape, policy,
  phase, relation, and candidate-commit construction;
  `address_effect_dependencies.[ch]` owns ordered architectural reads; and the
  58-line `address_effect_validate.c` owns only argument/result orchestration,
  zero-on-error handling, and publication after both delegated checks succeed.
  The 41-line `address_effect_apply.c` remains the sole AR mutation owner.
- Preserved behavior: invalid arguments and requested phases still precede
  rejected planner results; address-specific and generic intrinsic validation
  precedes phase and effective-address relations; dependency reads remain CMPT
  then ARP, selected AR then copied AR0, and CPL then DP/SP. NONE and AR0 remain
  distinguishable through the commit target, and no failure publishes a partial
  candidate or mutates architectural state.
- Kickoff correction: the kickoff entry assigned planner-result status checking
  to the intrinsic owner. Integration kept it in the top-level validator because
  status belongs to the result container, while the intrinsic seam deliberately
  accepts only the contained effect. This changes no status or precedence and
  keeps the state-free owner independent of planner-result packaging.
- Ownership and shape: handwritten implementations are 240 lines for intrinsic
  validation, 184 for dependency reads, 58 for orchestration, and 41 for apply.
  Both standalone CMake and direct Make source lists, including the observer-off
  artifact, name the two new owners explicitly. No installed header or public ABI
  was added.
- TDD and evidence: this is a behavior-preserving refactor, so the existing
  validation-descriptor and dependency-order contract tests remain its direct
  executable proof; all existing address and sequencer tests remain regression
  coverage. No new architectural claim was introduced, and the registry remains
  2 targets, 7 sources, 56 immutable evidence records, 65 requirements, and 51
  tests (45 `PASSING`, 6 `PLANNED`).
- Independent review: a final read-only Luna audit found no behavioral or build
  blocker and verified the exact rejection/read order, padding-safe output
  clearing, delayed candidate publication, AR0 discriminator, sole-write
  boundary, build wiring, and unchanged evidence/knob obligations.
- Local validation: strict `make -B test` passed all C tests and all 96 Python
  quality tests; ASan/UBSan `make -B test` passed; source boundaries passed for
  86 files; evidence and knob checks passed; GCC `-fanalyzer` accepted all four
  sequencer owners; the observer-disabled artifact check passed; and
  `make -B PYTHON=/definitely/missing build` proved the compiler-only path.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob,
  environment read, diagnostic hook, firmware input, emulator dependency,
  allocator, file I/O, logger, or mutable process global was added.
- Related timing decision: planning commit
  `3b6c7dbd634c922015fdc756b683c555dea6d907` separates instruction budgets from
  evidenced machine-cycle accounting and forbids handler-local magic costs.
  Resumable exact-cycle execution remains a planned sequencer/API contract, not
  implemented behavior in P2-W13.
- Remote context: the P2-W13 kickoff passed GitHub Actions run `34003154833`.
  Push and verify the implementation and this completion ledger next, then begin
  the evidence-first circular MOD8/MOD10 WORD16 work piece.

## 2026-09-06 — P2-W13 remote verification — Validation subownership

- Verified commits: implementation
  `55549dbf465c29bec7c2d3dcdebafc2dfcd9d6d4` and completion ledger
  `c11b8d7d10b57a17fd59caa517d4afad35166905` on `origin/main`.
- GitHub Actions: CI run
  `https://github.com/djr-747/ti-c54x/actions/runs/34003761573` completed
  successfully.
- Remote gates: GCC and Clang passed direct standalone Make build/tests,
  library-only CMake configure/build, full CMake configure/build/CTest,
  installation, and downstream consumer package smoke tests. The Clang
  sanitizer job passed configure, build, and CTest.
- Result: no corrective implementation, test, evidence, ownership, packaging,
  or isolation change was required. The intrinsic, dependency, orchestration,
  and sole-mutation boundaries remain independently named and green.
- Knobs and isolation: unchanged; remote verification introduced no `TEST`,
  `OBSERVE`, or `A/B` knob, environment read, diagnostic dependency, firmware
  input, public ABI, emulator coupling, or device-specific behavior.
- Follow-up: make exact cycle-bounded resumability explicit in the timing plan,
  then start P2-W14 with evidence and failing tests before circular arithmetic.

## 2026-09-06 — Timing-plan refinement — Exact-cycle resumability

- State: architecture decision recorded in planning commit
  `fb043f61ee7e184a7069b2e207872d73e85067ba`; no timed execution behavior is
  implemented or claimed by this entry.
- Decision: the future timed surface has explicit machine-cycle units. One tick
  advances exactly one DSP machine cycle, a cycle-budget run never overshoots,
  and an instruction-step convenience continues until one retirement or an
  explicit stop. Existing instruction-budget entry points are never silently
  reinterpreted as cycle budgets.
- Continuation boundary: a multi-cycle instruction is a private per-instance
  sequencer transaction. Budget exhaustion preserves its decoded identity,
  phase, timing/resource class, prepared effects, and pending bus transaction;
  the next tick resumes it. Register/status/PC effects commit once at retirement,
  and evidenced bus effects complete once at their named phase.
- Failure discipline: unknown timing never defaults to one cycle; checked
  `uint64_t` accounting cannot silently wrap; a timed bus profile must expose an
  allocation-free READY/WAIT transaction; and interrupt sampling remains an
  explicit unresolved boundary until primary evidence pins it.
- Integration boundary: an emulator adapter, not the standalone core, owns the
  reduced rational ARM-to-DSP ratio and integer phase accumulator. The common
  core receives only whole DSP-cycle budgets and contains no ARM tick, host
  clock, PLL, or fractional-cycle state.
- Independent review: a read-only Luna audit checked the existing public CPU and
  synchronous bus surfaces and confirmed that exact-cycle resumability belongs
  in the plan now, while concrete APIs, pending-state layout, interrupt phases,
  wait protocol, and instruction costs must wait for opcode and primary timing
  evidence.
- Validation: all 96 Python quality tests, evidence/knob/source-boundary checks,
  and the observer-disabled artifact check passed after the planning edit.
- Knobs and isolation: unchanged. The decision adds no knob, environment read,
  diagnostic hook, code, ABI, emulator dependency, or timing value.
- Follow-up: implement P2-W14 circular MOD8/MOD10 WORD16 semantics without any
  timing claim; start timed execution only after the opcode inventory can carry
  explicit evidenced or unknown timing metadata.

## 2026-09-06 — P2-W14 kickoff — Circular MOD8/MOD10 WORD16 effects

- State: active; evidence and tests precede implementation.
- Scope: common C54x, CMPT=0, one WORD16 data operand, AR0–AR7, and fixed-step
  circular MOD8/MOD10 only. MOD8 addresses with the old AR and requests a
  circular post-decrement by one; MOD10 addresses with the old AR and requests
  a circular post-increment by one. Read and write operands share this address
  behavior.
- Evidence boundary: official `SPRU131G` Table 5-4 owns the two modification
  forms and WORD16 step magnitude. Section 5.5.3.4 owns BK as buffer size, the
  strict `2^N > BK` alignment, EFB/index formation, the three signed-step
  partitions, the initial-in-buffer precondition, and the BK=0 no-modification
  rule. The registry receives narrow immutable records before implementation.
- BK=0 decision: the source explicitly describes the result as equivalent to
  `ARx+0`; the planner therefore returns the old AR operand and a NONE update,
  while retaining the selected-AR and BK snapshots so the sequencer still
  proves that the planned inputs are current. It does not perform a same-value
  architectural write merely to represent an absent modification.
- Positive-BK domain: `N` is the least integer for which `2^N > BK`; EFB clears
  the low `N` selected-AR bits; the admitted initial index is `0 <= index < BK`.
  An interior fixed step stays unchanged, an upper crossing subtracts BK, and a
  negative crossing adds BK. Intermediate arithmetic uses a wider signed type,
  and the `N=16` mask case must avoid undefined shifts. An initial AR outside
  the documented buffer is rejected as unresolved rather than assigned invented
  hardware behavior.
- Ownership: new private `src/address/indirect_circular.[ch]` owns pure scalar
  planning, circular arithmetic, and intrinsic effect validation. The value-
  owned address-source union gains a circular arm copying selected AR, MOD, and
  BK. The sequencer intrinsic owner only dispatches and recognizes that arm;
  the dependency owner reads CMPT then ARP, selected AR, then BK. The apply owner
  remains the sole possible selected-AR writer, and BK is never mutated.
- TDD shape: keep architectural vectors, planner-contract rejection cases,
  common application behavior, and transactional/dependency contract proof in
  four separately owned test files. Cover BK zero, size one, power-of-two and
  non-power-of-two sizes, aligned bases, lower/upper wrap, interior movement,
  `N=16`, both accesses, all AR selectors, malformed effects, unknown/stale AR
  and BK, read precedence, exact one-write success, and bytewise nonmutation on
  rejection.
- Explicit exclusions: out-of-buffer hardware behavior, LONG32 circular width,
  MOD9/MOD11 AR0 steps, MOD14 signed offset, dual operands, CMPT=1, MMR routing,
  opcode decode, instruction fetch/repeat, interrupts, bus transfers, pipeline,
  and machine-cycle timing remain unresolved or deferred. No ordinary 16-bit
  wrap rule is inferred from circular arithmetic.
- Build/isolation: register the private source and four tests in direct Make,
  CMake/CTest, and the observer-disabled artifact without adding an installed
  header, emulator, firmware image, environment read, file I/O, allocator,
  logger, mutable process global, or device profile.
- Knobs: none planned; the `TEST`/`OBSERVE`/`A/B` registry is unchanged.
- Delegation: the main agent owns the decisions, integration, review, gates,
  and commits. Luna workers own bounded evidence registration, test-first proof,
  address-module implementation, sequencer integration, and independent review;
  workers do not commit.

## 2026-09-06 — Architecture-plan refinement — Shared-memory gate

- State: structural requirement recorded in planning commit
  `8629f8a2fd1178fa547c44c3b415b9ce513370d3`; no memory-mutation behavior is
  implemented or claimed by this entry.
- Decision: memory is a first-class transaction owned across bus and sequencer
  boundaries before any load/store handler may mutate it. The core exchanges
  typed architectural words and never owns or aliases caller byte-backed RAM.
- Ordering and suspension: handlers declare complete value-owned memory effects;
  the sequencer resolves evidenced parallel read/write order; and an explicit
  begin/poll/complete transaction survives waits and budget exhaustion without
  reissuing reads or duplicating writes. Architectural effects remain staged
  around that transaction under a separately tested fault/visibility contract.
- Shared ownership: an external bus/emulator adapter arbitrates DSP CPU, ARM CPU,
  DMA, and device masters deterministically. Host thread timing cannot determine
  emulated order, and the common core contains no ARM identity, shared-RAM
  pointer, device map, host mutex, or platform arbitration.
- Endianness: the common C54x core is word-native and host-endian-neutral. Each
  byte-oriented boundary explicitly owns its conversion: image encoding, physical
  byte-lane order, and snapshot/wire format are not interchangeable. Multiword
  values follow evidenced DSP word order and never rely on host `uint32_t`
  layout, casts, packed structs, union punning, or native byte order.
- Nokia integration note: the current DCT3 tree identifies its ARM/flash, DSP
  images, HPI, and ordinary mailbox words as big-endian, while some protocol
  fields have separately evidenced little-endian encoding. Those are future
  Nokia-profile adapter facts and do not become a global common-core flag.
- Proof obligation: a common test bus must cover request shape, READY/WAIT/fault,
  exactly-once issue/completion, word composition, explicit BE/LE adapters,
  overlapping parallel accesses, and deterministic independent masters before a
  memory-writing opcode slice can pass.
- Validation: all 96 Python quality tests and evidence parity passed after the
  plan edit. P2-W14 remains register-address arithmetic only and does not cross
  this gate.
- Knobs and isolation: unchanged. No knob, environment read, code, public ABI,
  emulator dependency, memory map, raw pointer, timing value, or device behavior
  was added.

## 2026-09-06 — P2-W14 complete — Circular MOD8/MOD10 WORD16 effects

- State: complete in implementation commit
  `2c9dfb8d7c954438d6166542f03b3a93d1aba540`.
- Delivered scope: common C54x, CMPT=0, one WORD16 data operand, AR0–AR7,
  read/write access, known BK, and fixed-step circular MOD8/MOD10. The planner
  emits the old AR as the operand address and a post-access decrement/increment
  effect computed from the documented EFB/index algorithm.
- Boundary behavior: BK=0 produces the canonical NONE update while retaining
  copied AR and BK dependency snapshots. Positive BK requires the initial index
  to be inside the documented buffer; an outside-buffer value returns the named
  unresolved-boundary status. The maximum BK (`0xffff`) exercises the safe
  `N=16` path without an overflowing shift.
- Ownership: `src/address/indirect_circular.[ch]` owns pure planning, circular
  arithmetic, and intrinsic validation. `src/address/address_effect.h` owns the
  copied circular source shape. The sequencer intrinsic and dependency modules
  recognize and validate that shape; the existing apply module remains the sole
  selected-AR writer and never writes BK. No handwritten file approaches the
  monolith review threshold.
- TDD evidence: four separately owned tests were written before implementation;
  the first compile failed because `address/indirect_circular.h` did not yet
  exist. Final vectors cover BK zero, size one, power-of-two and non-power-of-two
  sizes, interior movement, both wraps, aligned bases, max BK, read/write access,
  every AR selector, malformed effects, unknown and stale dependencies, ordered
  read failures, exact one-write success, and transactional nonmutation.
- Review corrections: replaced duplicated algorithm-shaped test oracles with
  explicit expected values, initialized unrelated AR state in positive
  sequencer cases, removed a producer-provenance expectation that a value-only
  generic effect cannot enforce, added the max-BK vector, and zeroed complete
  result objects so padding is deterministic as well as named fields.
- Registry: 2 targets, 7 sources, 60 evidence records, 74 requirements
  (57 implemented, 5 planned, 12 unresolved), and 54 tests (49 passing,
  5 planned). The five circular exclusions remain explicit: initial AR outside
  the buffer, LONG32 scaling, MOD9/MOD11 AR0 steps, MOD14 offset, and CMPT=1 or
  dual-operand behavior.
- Validation: the complete direct Make suite passed, including all C tests and
  96 Python quality tests; ASan/UBSan passed; a compiler-only build passed; GCC
  `-fanalyzer`, observer-disabled artifact checks, evidence checks, source-
  boundary checks, and `git diff --check` passed. Local CMake execution remains
  unavailable on this host and is delegated to standalone remote CI.
- Review: an independent Luna audit found no remaining blocker after the full-
  object zero-initialization correction. The algorithm, overflow discipline,
  dependency order (CMPT, ARP, selected AR, BK), transactional application,
  build wiring, and isolation boundary were all checked.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob, environment
  read, emulator dependency, firmware input, memory mutation, byte-order policy,
  opcode decode, or timing claim was added.
- Follow-up: verify both commits in standalone CI, then select the next bounded
  foundation slice. Shared-memory and exact-cycle execution remain plan-only
  gates and cannot be bypassed by a future memory-mutating opcode.

## 2026-09-06 — P2-W14 remote verification

- Verified implementation commit
  `2c9dfb8d7c954438d6166542f03b3a93d1aba540` and completion-ledger commit
  `701f29a5cf6727be549d56297f9aae198426bef3` in GitHub Actions run
  `34005806734`.
- The standalone matrix passed GCC and Clang direct-Make tests, library-only
  configuration/build, full CMake/CTest, install/package checks, downstream
  consumer smoke tests, and the Clang sanitizer build and tests.
- Result: P2-W14 is remotely proven on the repository's independent build
  surfaces. No emulator checkout, firmware image, environment gate, or external
  project build participated.

## 2026-09-06 — P2-W15 kickoff — Ownership-mirrored build manifests

- State: active; behavior-preserving structural prerequisite to the opcode tree.
- Trigger: `tests/CMakeLists.txt` has reached 800 handwritten lines and the root
  `Makefile` has reached 560. Adding the first ISA/decode owners to those flat
  inventories would work against the repository's explicit ownership hierarchy
  and monolith gate.
- Scope: keep root Make/CMake files responsible for global policy, orchestration,
  aggregate targets, packaging, and inclusion order. Move source registration to
  the matching `src/<owner>/` directories and unit-test registration to matching
  `tests/unit/<owner>/` directories. One small `tests/cmake/` helper may own
  repeated target setup without owning the target inventory.
- Compatibility: preserve every current source object, normal and observer-off
  archive, direct-Make test executable, CTest name, compiler option, sanitizer
  application, install/package surface, quality test, and minimal-consumer path.
  The work piece adds no runtime behavior and no architectural claim.
- Proof: compare pre/post target and CTest inventories, run the complete direct
  Make suite, and use remote GCC/Clang/CMake/sanitizer/install/consumer CI for the
  CMake path unavailable locally. Structural checks must continue to run from a
  clean standalone checkout.
- Ownership and delegation: one Luna worker owns the build-manifest refactor and
  no production C or public header. The main agent owns design decisions, review,
  validation, commits, and any correction. The worker must not commit or revert
  concurrent work.
- Knobs and isolation: no knob or default may change. No environment read,
  emulator input, firmware dependency, diagnostic behavior, generated opcode,
  semantic implementation, or timing claim belongs in this slice.
- Follow-up: P2-W16 starts the evidence-backed one-row NOP inventory/decoder,
  semantic no-effect, and sequential-PC seam. Unresolved address modes remain
  explicit semantic traps and no longer block decoder scaffolding.

## 2026-09-06 — P2-W15 complete — Ownership-mirrored build manifests

- State: complete in implementation commit
  `5a3123e95eacc2e43456c86f3a820d7d4358d775`.
- Delivered scope: root build files now own only policy and orchestration while
  each `src/<owner>/` and `tests/unit/<owner>/` directory owns its source or test
  inventory. The root Makefile fell from 560 to 130 lines, the root CMake file
  is 138 lines, and `tests/CMakeLists.txt` fell from 800 to 113 lines.
- Preserved surfaces: all 18 handwritten core sources remain registered in the
  same order for normal and observer-disabled libraries. All 47 C unit-test
  targets remain registered, and direct Make executes those targets plus the
  minimal downstream consumer for 48 executions.
- Structural proof: the new manifest graph follows only reachable nested Make
  and CMake manifests, requires exact owner-directory correspondence, rejects
  disconnected fragments and non-normalized traversal, and distinguishes macro
  registrations from direct special targets. The source-manifest checker proves
  exact membership, uniqueness, order parity, paired Make/CMake ownership, and
  that both normal and observer-disabled object sets derive from the same source
  inventory. Nested future owners such as `isa/decode` are covered.
- Maintainability: manifest parsing, evidence-runner policy, build-manifest
  checking, registry tests, and runner tests have separate owners; all remain
  below the 500-line review threshold. No production C file or public header
  changed.
- TDD and review: adversarial tests first exposed direct-target aggregate bypass,
  cross-owner registration, orphan fragments, path traversal, source-list drift,
  and a literal-target fallback through the macro path. Corrections reject each
  case while accepting correctly nested owners. An independent review found no
  remaining blocker; its relative-root helper observation is non-blocking because
  every repository caller resolves the root before use.
- Validation: the final standalone `make -B test` passed all 121 Python quality
  tests, rebuilt and passed every C unit test under strict warnings, built the
  observer-disabled archive, and passed the minimal consumer. Evidence and build
  manifest checks and `git diff --check` passed. CMake is unavailable locally;
  the independent CMake/CTest, install, consumer, compiler-matrix, and sanitizer
  proof remains assigned to remote CI.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob, environment
  read, emulator dependency, firmware input, runtime behavior, public API, opcode
  table, or timing claim was added.
- Follow-up: push the implementation and ledger commits, verify the standalone
  remote matrix, then begin P2-W16 with the evidence-backed NOP catalog, pure
  decoder, no-effect semantics, and checked sequential-PC retirement seam.

## 2026-09-06 — P2-W15 remote verification

- Verified implementation commit
  `5a3123e95eacc2e43456c86f3a820d7d4358d775` and completion-ledger commit
  `f611c3ee177825508d7957364f171bccacc66c55` in GitHub Actions run
  `34008390915`.
- The standalone matrix passed GCC and Clang direct-Make tests, library-only
  configuration/build, full CMake/CTest, install/package checks, downstream
  consumer smoke tests, and the Clang sanitizer build and tests.
- Result: P2-W15 is proven on every independent repository build surface. The
  ownership-mirrored manifests are ready to accept nested ISA modules without an
  emulator checkout, firmware image, environment gate, or external project build.

## 2026-09-06 — P2-W16 kickoff — NOP-seeded opcode architecture

- State: active kickoff; this entry records the evidence and contract boundary
  before production ISA behavior is implemented. The registry now contains 61
  evidence records, 75 requirements (57 implemented, 6 planned, 12 unresolved),
  and 55 tests (49 passing, 6 planned).
- Scope: add `src/isa/catalog/` as the sole declarative X-macro opcode inventory.
  Macro passes will derive opcode identity/metadata, a direct 16-bit first-word
  lookup, and a function-pointer semantic dispatch table. The only registered
  row is NOP `0xF495`, width one program word; unknown words classify explicitly.
  NOP emits an explicit no-opcode-owned-effect result. The private sequencer owns
  checked 16-bit PC `+1` planning and reports an unresolved boundary without
  mutation at `0xffff`.
- Evidence: `E-PROJECT-OPCODE-CATALOG-CONTRACT` is anchored to the P2-W16 section
  in `docs/implementation-plan.md`; the existing TI NOP and sequential-flow
  records remain the only architectural evidence and all three NOP requirements
  remain planned until their executable tests pass.
- Ownership and delegation: the main agent owns design decisions, integration,
  validation, and the atomic commit. One delegated worker owns catalog/decode,
  its quality contract, and mirrored tests; a second owns opcode effects,
  dispatch, and the NOP semantic test; a third owns only the sequential-PC seam
  and its mirrored test. The registry/documentation worker owns only the kickoff
  records and parity surfaces assigned for P2-W16. No worker may revert another
  worker's edits or create a commit outside the main-agent workflow.
- TDD sequence: first prove the catalog contract and unknown/known direct decode,
  then prove the NOP no-effect semantic result, then prove sequential-PC success
  and the `0xffff` unresolved-boundary transaction. Keep each owner on its own
  test path and link every passing test back to its requirement.
- Exclusions: no public CPU-shell integration (the shell carrier is 32-bit and
  high-carrier/XPC policy is unresolved), no machine-cycle or pipeline timing,
  memory/bus/device effects, observer behavior in opcode operations, or copied
  legacy/mGBA/external source, tables, comments, or tests.
- Follow-up: implement the bounded private ISA/sequencer seams, register their
  mirrored build/test manifests, then update the three NOP requirements and the
  catalog project requirement only when passing executable tests and evidence
  checks are present. Shared-memory and timing gates remain future work.

## 2026-09-06 — P2-W16 complete — NOP-seeded opcode architecture

- State: complete in implementation commit
  `f9cd7be4570f85fac2ef35835fac824462801f69`.
- Catalog and decode: `src/isa/catalog/` is the sole handwritten instruction
  inventory. Its initial `EXACT` NOP row expands through distinct X-macro passes
  into opcode identity, immutable metadata, a fixed 65,536-entry `uint16_t`
  first-word lookup, and the function-pointer handler table. Unregistered words
  map to the zero-valued UNKNOWN identity; decoder results preserve the fetched
  word and report a zero program-word count explicitly.
- Semantics and sequencing: NOP validates against borrowed catalog metadata and
  publishes the explicit `NONE` opcode effect only after success. It contains no
  copied encoding literal and owns no PC, state, bus, timing, or observer action.
  The sequencer separately stages a unit-bearing private 16-bit PC effect,
  commits it only against the matching frozen source, and reports the unresolved
  `0xffff` boundary without mutation.
- Ownership: catalog, decode, opcode effects, NOP semantics, handler dispatch,
  and sequential PC flow have separate private C/header owners with mirrored
  Make/CMake test manifests. The generic dispatch header does not include the
  NOP-family header. No public header or CPU-shell behavior changed.
- Structural proof: the catalog contract rejects malformed rows, reserved or
  duplicate IDs, duplicate exact words, duplicate handler bindings, invalid
  widths/ranges, missing macro consumers, and a second NOP encoding literal in
  handwritten ISA C/H. Decoder coverage exhaustively classifies all 65,536 first
  words. The source-manifest checker now recursively proves normalized nested
  aggregate reachability, exact leaf ownership, order parity, uniqueness,
  acyclicity, and Make/CMake hierarchy parity.
- Maintainability correction: recursive manifest parsing moved into the new
  362-line `source_manifest_graph.py`; the policy/orchestration CLI is 165 lines.
  No handwritten implementation, test, or quality module reaches the 500-line
  review threshold.
- TDD and review: initial red checks observed missing catalog/NOP/PC headers;
  review-driven red checks then exposed duplicate encoding ownership, the
  NOP-specific generic-header dependency, missing word-unit PC fields, catalog
  collision gaps, nested-manifest false failures, and the missing CMake PC-test
  registration. Each was corrected before promotion. An independent Luna audit
  found no remaining blocker or major defect.
- Registry: 2 targets, 7 sources, 61 evidence records, 75 requirements
  (61 implemented, 2 planned, 12 unresolved), and 55 tests (53 passing,
  2 planned). NOP decode, no-effect semantics, sequential PC flow, and the
  project catalog contract are now implemented.
- Validation: final standalone `make -B test` passed 137 Python quality tests,
  all 50 C unit targets, the 51-test direct execution set including the minimal
  consumer, evidence/knob/source/manifest checks, and the observer-disabled
  artifact proof under strict warnings. Local CMake remains unavailable; remote
  CI owns CMake/CTest, install, consumer, compiler-matrix, and sanitizer proof.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob,
  environment read, emulator/firmware dependency, allocation, logging, public
  ABI, machine-cycle value, XPC policy, memory effect, or device behavior was
  introduced.
- Follow-up: verify both commits remotely. Then plan the CPU-level
  fetch/decode/dispatch/retire transaction with a named checked bridge between
  the public 32-bit fetch carrier and private 16-bit common PC. Add a separately
  proven masked-pattern catalog form before the first operand-bearing opcode;
  never implement variable operand bits as ad hoc decoder branches.

## 2026-09-06 — P2-W16 remote verification

- Published implementation commit:
  `f9cd7be4570f85fac2ef35835fac824462801f69`.
- Published completion-ledger commit:
  `884657b4dccb553ac7a0bb03c8abab05fe021885`.
- GitHub Actions run
  [34010592627](https://github.com/djr-747/ti-c54x/actions/runs/34010592627)
  completed successfully for the published completion-ledger head.
- Remote proof passed the GCC and Clang direct-Make suites, library-only and
  full CMake configurations, CMake builds and CTest, installation, downstream
  package-consumer smoke tests, and the Clang sanitizer build/test job.
- Result: the P2-W16 implementation and completion record are published and
  independently green on every repository-owned remote validation surface.

## 2026-09-06 — P2-W17 kickoff — masked opcode catalog foundation

- State: active kickoff; this entry records the contract and evidence boundary
  before any masked production instruction or generator implementation lands.
  The registry now contains 2 targets, 7 sources, 62 immutable evidence records,
  77 requirements (61 implemented, 4 planned, 12 unresolved), and 57 tests (53
  passing, 4 planned).
- Scope: extend the sole `src/isa/catalog/` inventory with `EXACT` and `MASKED`
  canonical 16-bit `value`/`mask` rows. `MASKED` masks are nonzero and not
  `0xffff`; values cannot set bits outside their mask; widths are 1..255 program
  words; IDs and handler tokens are unique; every pairwise overlap is
  rejected deterministically with no priority. The existing exact NOP row,
  `0xF495` behavior, and API remain unchanged. No masked production row is
  registered in this work piece.
- Evidence: `E-PROJECT-MASKED-OPCODE-CATALOG-CONTRACT` is anchored to the P2-W17
  implementation-plan section and is explicitly `PROJECT_CONTRACT_ONLY`; it
  captures structure and build policy only. Existing TI field/encoding records
  remain architectural context and are not reclassified as proof of this local
  catalog contract.
- Generation/build contract: the catalog subsystem owns a deterministic generator
  at `tools/isa/catalog/generate_opcode_lookup.py` and a checked-in immutable
  65,536-entry `uint16_t` direct lookup derivative at
  `generated/isa/catalog/opcode_first_word.inc`.
  Normal Make, CMake, and consumer builds remain compiler-only and Python-free;
  quality checks regenerate and reject stale output. Runtime performs no
  initialization, allocation, or mutable global-table update.
- Ownership and exclusions: `src/isa/catalog/` owns schema and runtime lookup,
  `tools/isa/catalog/` owns generation, `generated/isa/catalog/` owns the
  derivative, and `tests/contract/quality/isa/catalog/` owns the two planned
  Python quality tests. Operand-field layouts/extraction and extension-word
  fetch are separate future work pieces before real operand semantics. CPU
  fetch/decode/dispatch/retire integration, timing, memory, bus, device,
  observer, XPC, and external code/table reuse are excluded.
- TDD sequence: red schema, invalid-mask/width, duplicate-ID/handler, overlap,
  stale-artifact, and generator checks first; implementation second; exhaustive
  65,536-word mapping and generated-artifact immutability proof third; then
  independent review and normal quality validation. Planned tests will use the
  existing Python quality runner (`make test`, CTest
  `ti_c54x.quality.python`) when implemented.
- Follow-up: implement the catalog schema and generator under the ownership
  hierarchy, add both planned tests, prove stale-output and compiler-only build
  boundaries, then record implementation and remote-verification ledger entries
  before planning operand-field or extension-word work.

## 2026-09-06 — P2-W17 complete — masked opcode catalog foundation

- State: complete in implementation commit
  `a11791745778edb4ac7619fc8681013a1c239e2d`.
- Schema and ownership: the immutable shared schema under `tools/isa/catalog/`
  parses and validates the sole production inventory in `src/isa/catalog/`.
  It normalizes exact rows to a full mask, accepts canonical partial nonzero
  masks, rejects invalid widths/ranges/types, duplicate identities or handlers,
  and every pairwise exact/masked overlap with a deterministic witness. The
  production catalog still contains only the exact NOP row; this work piece
  enables no operand-bearing instruction.
- Generation and runtime: the repository-owned generator exhaustively expands
  all 65,536 first words and emits a deterministic sparse symbolic initializer
  at `generated/isa/catalog/opcode_first_word.inc`. The C catalog includes that
  checked-in derivative inside an immutable 65,536-entry `uint16_t` lookup;
  unspecified entries remain zero/UNKNOWN and decode remains one indexed load.
  `--check` recomputes and compares in memory without writing. Normal Make,
  CMake, and downstream consumer builds do not invoke Python.
- TDD and review: red tests first exposed the missing shared schema, missing
  generator, hard-coded exact-only C consumers, and rejection of ownership-
  mirrored nested quality-test paths. Review then forced all direct-row and
  generator inputs through one validator and extended malformed-path rejection
  coverage. An independent Luna review found no blocker, major, or minor issue.
- Registry: 2 targets, 7 sources, 62 evidence records, 77 requirements
  (63 implemented, 2 planned, 12 unresolved), and 57 tests (55 passing,
  2 planned). The two masked-catalog project-contract requirements and their
  schema/generator tests are now implemented and passing; they make no TI
  architectural-conformance claim.
- Validation: `generate_opcode_lookup.py --check`, `git diff --check`, and the
  final standalone `make -B test` passed. The gate ran 160 Python quality tests,
  rebuilt and passed all 50 C unit targets and the 51-test direct execution set,
  and passed evidence, knob, source-boundary, manifest, observer-disabled, and
  minimal-consumer checks under strict warnings. Kickoff commit
  `ba79925834d43dac8b4dd7a4cc33a8aa2cf79357` was independently green in GitHub
  Actions run `34011441802`; the completed implementation still requires its
  own remote matrix.
- Knobs and isolation: unchanged. No `TEST`, `OBSERVE`, or `A/B` knob,
  environment read, diagnostic path, emulator/firmware dependency, public ABI,
  runtime initialization, allocation, timing claim, or external source/table
  copy was introduced.
- Follow-up: publish and verify the implementation and completion record, then
  add the private checked program-PC bridge and CPU NOP
  fetch/decode/dispatch/stage/retire transaction. Operand-field extraction and
  extension-word fetch remain separate owners before `RPT`. A future `RPT`
  kickoff must register TI interrupt/repeat evidence and a versioned GNU
  binutils transcript only as a secondary `TOOLCHAIN_ORACLE_ONLY` cross-check;
  no external opcode table or implementation becomes repository source.

## 2026-09-06 — P2-W17 remote verification

- Published implementation commit:
  `a11791745778edb4ac7619fc8681013a1c239e2d`.
- Published completion-ledger commit:
  `65160ea1d6af55b1ce9697cd7b203d00bf08fd27`.
- GitHub Actions run
  [34013236855](https://github.com/djr-747/ti-c54x/actions/runs/34013236855)
  completed successfully for the published completion-ledger head.
- The standalone matrix passed GCC and Clang direct-Make tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-
  consumer smoke tests, and the Clang sanitizer build and tests.
- Result: P2-W17 is published and independently proven across every repository-
  owned build surface. Exact and masked catalog infrastructure is ready for the
  separately owned CPU transaction and operand-field work without an emulator,
  firmware image, Python runtime dependency, environment gate, or external
  opcode table.

## 2026-09-06 — P2-W18 kickoff — private program-PC bridge and CPU NOP transaction

- State: active kickoff; no implementation or test files were changed by this
  record.
- Plan: connect the standalone public CPU shell to the existing exact NOP decode
  and generated dispatch seam through a private 16-bit program-PC bridge. The
  stable plan anchor is `P2-W18: private program-PC bridge and CPU NOP transaction
  (active kickoff)` in `docs/implementation-plan.md`.
- Ownership: `src/cpu/` will own instruction-budget orchestration, exact public
  carrier handling, one-word program fetch, decode/dispatch sequencing, stop and
  counter reporting, and retirement accounting. `src/sequencer/` will own the
  value-owned private PC stage/commit bridge. Planned tests mirror those owners
  under `tests/unit/cpu/` and `tests/unit/sequencer/`.
- Contract decisions: the attempt phase begins immediately before the program-bus
  read, so bus faults and unknown decoded words count one attempted instruction;
  zero budget and cumulative-counter preflight failure count zero. The exact
  32-bit public carrier is passed to the bus, including high-carrier unknown-word
  probes. A known decoded NOP above `0xffff` fails private bridge validation
  without truncation, XPC inference, PC commit, or retirement. `0xfffe -> 0xffff`
  is admitted, while a one-word successor from `0xffff` is an explicit unresolved
  boundary. `run(N)` stops after `N` retirements or the first stop, and cumulative
  attempted/retired overflow is an explicit planned no-wrap/no-mutation stop. The
  three new policy stops will be named
  `TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS`,
  `TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY`, and
  `TI_C54X_STOP_COUNTER_OVERFLOW`; malformed internal transactions retain
  `TI_C54X_STOP_INVALID_STATE`.
- Evidence boundary: existing TI records are used only for NOP word/width, no
  status effects, and sequential-flow context. Carrier handling, stop reasons,
  counter phases, bridge transactionality, and run orchestration are local
  `PROJECT_CONTRACT_ONLY` requirements under the new P2-W18 evidence anchor.
- Exclusions: operand layouts and extension fetch, timing/cycles, shared-memory
  mutation, interrupts/repeat, data-memory buses, observers/diagnostics, profiles,
  emulator adapters, and other ISA families remain separate work pieces.
- Build compatibility: observer behavior is excluded and unchanged, but the
  direct-Make observer-enabled test must link a full archive derived from the
  canonical source manifest once CPU orchestration imports ISA and sequencer
  owners; this mirrors the already-correct CMake variant and prevents dependency
  drift.
- Registry: current authoritative totals are 2 targets, 7 sources, 63 immutable
  evidence records, 91 requirements, and 61 tests (55 `PASSING`, 6 `PLANNED`).
- Validation: `python3 tools/quality/check_evidence.py` and `git diff --check`
  pass for the kickoff records. Implementation and full test validation remain
  pending the next work-piece commit.
- Follow-up: implement the bridge and CPU transaction with red tests first,
  preserving the existing public carrier and attempted-count behavior, then run
  the repository fast gate and record implementation/remote-verification commits.

## 2026-09-06 — P2-W18 complete — private program-PC bridge and CPU NOP transaction

- Implementation commit: `52c76f75ff78935989136e398f658e838ae7bc47`.
- Kickoff commit: `579e1e883cb5b65f36cde9978b895a58b332273a`; GitHub Actions run
  [34013927291](https://github.com/djr-747/ti-c54x/actions/runs/34013927291)
  passed.
- Implemented the public CPU one-word fetch/decode/generated-dispatch/private
  program-PC stage-and-commit path and NOP retirement. The transaction preserves
  the exact 32-bit bus carrier, reports explicit high-address, unresolved-boundary,
  and counter-overflow stops, uses retirement-budget `run` semantics, orders
  attempted/retired counters explicitly, and never partially commits state.
- The private bridge owns transactional 32-to-16 validation after decode/dispatch:
  `0xfffe -> 0xffff` is admitted, while a one-word instruction at `0xffff` has an
  unresolved successor. Invalid, stale, and malformed effects preserve caller
  state and the effect object.
- The direct-Make observer-on archive now derives from the canonical complete core
  source inventory. Observer-enabled successful NOP and re-entry tests prove the
  observer remains behaviorally neutral and fails closed.
- TDD reds recorded: missing bridge header/implementation; the old CPU returned
  unimplemented for NOP and lacked the new stop enums; observer builds had
  incomplete hand-linked sources and no canonical observer-on archive.
- Independent Luna review found no blocker or major issue. The minor local-counter
  form was closed by documenting local deltas as bounded by guarded cumulative
  counters; observer NOP coverage was added; the traceability nit was closed by
  linking the CPU-boundary requirement to the CPU contract test.
- Registry final: 2 targets, 7 sources, 63 evidence records, and 91 requirements
  (77 implemented, 2 planned, 12 unresolved); 61 tests (59 passing, 2 planned).
- Validation: `make -B test` passed; 161 Python quality tests and all 54
  direct-Make C unit targets passed, followed by the 55-test direct execution
  set including the minimal consumer. Strict warnings, evidence, knob,
  source-boundary, manifest, observer-on/off archive, and generated-artifact
  checks passed; `generate_opcode_lookup.py --check` and `git diff --check`
  passed.
- Knobs/isolation unchanged: no `TEST`/`OBSERVE`/`A/B` knob, environment read or
  `getenv`, logging/diagnostic hot path, emulator/firmware dependency, external
  code/table copy, timing/cycle claim, shared-memory mutation, IRQ/repeat
  behavior, data bus, extra-word fetch, XPC inference, allocation, or mutable
  process global was introduced.
- Follow-up: publish and verify the implementation and this completion record;
  next implement operand-field extraction and extension-word fetch, then begin
  evidence-first `RPT`/IRQ work. This entry makes no claim of full ISA coverage.

## 2026-09-06 — P2-W18 remote verification

- Published implementation commit:
  `52c76f75ff78935989136e398f658e838ae7bc47`.
- Published completion-ledger commit:
  `a4e70865804de3b5c8d3598ed65a545c8b19a91b`.
- GitHub Actions run
  [34015197239](https://github.com/djr-747/ti-c54x/actions/runs/34015197239)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make tests, library-only
  configure/build, full CMake/CTest, installation, downstream package-consumer
  tests, and Clang sanitizer configure/build/test.
- Result: P2-W18 is published and proven across repository-owned build surfaces;
  standalone isolation is retained. Next is operand-field extraction and
  extension-word fetch before evidence-first `RPT`/IRQ work. This makes no claim
  of full ISA coverage, timing, memory, or IRQ implementation.

## 2026-09-06 — P2-W19A kickoff — value-owned operand-field extraction

- State: active kickoff; this append-only record changes documentation and
  evidence planning only. No production, test, or build files were changed.
- Prior published state: P2-W18 remote-verification head
  `0fb32d1ea161f3d7a33c17de86e2a0d148a434a6`; its GitHub Actions run
  `34015295148` passed. This kickoff does not rewrite that history.
- Plan split: P2-W19 is divided into atomic P2-W19A extraction and P2-W19B
  extension-word fetch/width-aware program-PC span and commit. Both precede
  repeat work, including `RPT`. Bus reads remain owned by the planned
  `src/cpu/` instruction-fetch module; sequencing owns width-aware span/commit,
  not bus I/O.
- Contract: `src/isa/decode/operand_field.[ch]` will own a pure,
  allocation-free extractor over a borrowed caller-owned view of one or more
  16-bit program words and a borrowed immutable field layout; it retains no
  pointers.
  Pieces identify a source program-word index, source LSB, width 1..16, and
  destination/value LSB 0..63. Fields contain 1..N pieces and declare raw width
  1..64. Pieces stay within source words and declared destination width;
  destination pieces do not overlap and cover the declared value exactly;
  source slices do not overlap within a source word. Successful extraction
  publishes a caller-owned `uint64_t` raw value plus explicit width and supports
  first-word, extension-word, split/non-contiguous, and 64-bit mechanics without
  sign extension or semantic interpretation. Null/invalid views, layouts,
  outputs, indexes, widths, overlap, and gaps leave output byte-for-byte
  unchanged. The seam owns no bus, CPU, state, PC/effect, timing, memory,
  interrupt/repeat, observer/diagnostic, file/environment, heap, or mutable-global
  behavior and changes no production opcode inventory, catalog macro arity,
  layout binding, handler, public API, or fetch path.
- Ownership and proof: the implementation/header owner is
  `src/isa/decode/operand_field.[ch]`; the planned executable proof is
  `tests/unit/isa/decode/test_operand_field.c`, with owner-local Make/CMake
  registration after implementation. The test plan exhausts valid one-word
  positions and widths and covers representative multiword/non-contiguous/64-bit
  fields plus the complete invalid and transactional matrix. Synthetic project
  data proves mechanics only; no TI evidence or conformance claim is registered.
- Registry kickoff: added exactly one project evidence record
  `E-PROJECT-P2-W19A-OPERAND-FIELD-CONTRACT`, one planned requirement
  `REQ-PROJECT-OPERAND-FIELD-EXTRACTION`, and one planned test
  `T-PROJECT-OPERAND-FIELD-EXTRACTION`, with forward and reverse links. Expected
  totals are 2 targets, 7 sources, 64 immutable evidence records, 92 requirements
  (77 implemented, 3 planned, 12 unresolved), and 62 tests (59 passing,
  3 planned).
- Focused checks for this docs-only kickoff: evidence checker, registry quality
  unit coverage, and `git diff --check`; the full build/test gate remains owned by
  the later implementation work piece.

## 2026-09-06 — P2-W19A complete — value-owned operand-field extraction

- State: complete in implementation commit
  `27e1404c2d731d94c502356ac4bce84068da2ad2`.
- Kickoff commit: `72fdcd762c0e629cd8d7f457182cce674832b64d`; GitHub Actions run
  `34016034357` passed. This entry records completion only; remote verification
  remains a separate follow-up.
- Private decode-owned API: `ti_c54x_program_words_view` explicitly carries the
  borrowed `program_words` array and its `program_word_count` extent. The borrowed
  immutable layout carries its piece array and `piece_count` extent. Both arrays
  must remain alive and unchanged through the call; the caller owns the output
  `uint64_t` raw value and explicit width, and no input pointer is retained.
- Validation and behavior: source slices are 1..16 bits and fields are 1..64
  bits. The implementation proves extraction from first words and already-
  supplied extension words, multiword and non-contiguous assembly, piece-order
  independence, and full 64-bit values; exact destination coverage and
  non-overlap, per-word source non-overlap, source-index and bit-bound checks,
  and transactional preservation of output on rejection are explicit. No sign
  semantics are applied.
- TDD: the initial red exposed the missing `operand_field.h` after owner manifest
  registration. Green covered the focused normal build/test and the focused
  AddressSanitizer/UndefinedBehaviorSanitizer path.
- Review: independent Luna review found no blocker or major issue. It closed the
  explicit array-extent/lifetime wording, exact program-word naming, staged zero
  initialization, owner-manifest comments, and mixed-invalid precedence
  documentation. Main review also restored the already-published immutable
  evidence assertion and planned requirement claim verbatim; completion changes
  only their state/runner.
- Ownership/shape: the initial review covered 116 C source lines, 67 header lines,
  and 235 test lines. The implementation remains split along the decode owner and
  owner-local manifests; no monolith was introduced.
- Performance boundary: this is a checked general, heap-free descriptor mechanism.
  A future generated or specialized catalog/layout binding may validate layouts
  offline and avoid descriptor revalidation in the hot path, but no such binding
  is implemented or claimed here.
- Registry final: 2 targets, 7 sources, 64 evidence records, and 92 requirements
  (78 implemented, 2 planned, 12 unresolved); 62 tests (60 passing, 2 planned).
- Validation: `make -B test` passed with 161 Python quality tests, 55 C unit
  targets, and 56 direct execution tests including the minimal consumer (the
  prior 54/55 plus this target). Strict warnings, evidence, knob, source,
  manifest, observer-artifact, and generated-artifact checks passed;
  `generate_opcode_lookup.py --check` and `git diff --check` passed.
- Knobs/isolation unchanged: no knob, environment read or `getenv`, diagnostic or
  logging path, public API, catalog opcode/macro/handler/fetch behavior, bus,
  CPU/state/PC, timing, memory, IRQ/repeat, emulator dependency, external copy,
  mutable global, or allocator was introduced.
- Follow-up: publish and verify this implementation and completion record, then
  implement P2-W19B transactional extension fetch and width-aware PC handling
  before evidence-first `RPT` work.

## 2026-09-06 — P2-W19A remote verification

- Published implementation commit:
  `27e1404c2d731d94c502356ac4bce84068da2ad2`.
- Published completion-ledger commit:
  `fe5d46dba3e18d44ca5536fb7ee8eb47a63a0aab`.
- GitHub Actions run
  [34017109364](https://github.com/djr-747/ti-c54x/actions/runs/34017109364)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, the
  library-only configuration, full CMake/CTest, installation, downstream
  package-consumer smoke tests, and the Clang sanitizer configure/build/test
  path.
- Result: P2-W19A is published and proven across repository-owned build surfaces;
  standalone isolation and private decode ownership remain intact. The next
  slice is P2-W19B transactional extension-word fetch and width-aware program-PC
  handling before `RPT`. This verification makes no claim about extension fetch,
  broader ISA coverage, timing, memory, or IRQ behavior.

## 2026-09-06 — P2-W19B kickoff — transactional extension fetch and width-aware program PC

- State: active contract kickoff; implementation and completion commits are pending.
- Prior repository head: `63115639c67a167f5df31cfa410cc51914956f2c`; the P2-W19A
  remote verification at GitHub Actions run `34017216955` is green.
- Plan: freeze two owner-local, standalone seams before any multiword opcode or
  repeat integration. `src/cpu/instruction_fetch.[ch]` owns allocation-free
  extension collection after first-word fetch/decode, while
  `src/sequencer/sequential_pc.[ch]` and `program_pc_bridge.[ch]` own checked
  width-aware PC span and transactional application. No production opcode,
  handler, catalog row, or CPU count-two retire integration is introduced.
- Instruction-fetch contract: the by-value result has helper capacity exactly two
  program words (not an ISA-wide maximum), explicit count, status, and exact bus
  status. The input copies the first word and exact `uint32_t`
  `first_program_word_address` and borrows the bus descriptor and callback context
  only for the call, retaining neither.
  Count zero and widths above two are rejected before callbacks. Count one
  publishes the first word with zero callbacks and `TI_C54X_BUS_OK` because no bus
  was attempted. Null-bus, unsupported-width, and address-overflow results also
  report `TI_C54X_BUS_OK` because no bus transaction was attempted. Count two
  checks `first_program_word_address + 1` before exactly one callback at that exact
  carrier, admits high carriers without truncation, rejects `UINT32_MAX` overflow
  before I/O, maps a missing callback to a bus fault plus
  `TI_C54X_BUS_NO_CALLBACK`, preserves each callback failure's exact non-OK bus
  status, maps fetch status to bus fault, and publishes zero count/words with all
  named result fields initialized on every non-OK path without retry. No first-word
  fetch, counter, CPU/PC/state, common 16-bit/XPC, timing, memory, observer, or
  diagnostic behavior belongs to this helper.
- Program-PC contract: explicit widths 1..255 convert both operands to `uint32_t`
  before addition for 16-bit-int hosts; count zero and sums above `UINT16_MAX` are
  rejected without wrap or narrowing. Existing one-word boundaries remain exact
  (`0xfffe -> 0xffff` admitted and `0xffff` unresolved). The matrix also admits
  `0xfffd + 2 = 0xffff` and rejects `0xfffe + 2` as unresolved. Valid, stale,
  and malformed effects stay transactional. Public carriers above `0xffff` are
  rejected without XPC inference, and this seam performs no bus I/O. P2-W18 NOP
  orchestration, attempted/retired counters, observer behavior,
  one-fetch-per-attempt contract, and `last_fetched_word` policy are unchanged;
  their tests remain regression gates. RPT work will choose the later ordering of
  span validation, extension reads, operand extraction, and semantics.
- Planned tests: `T-PROJECT-INSTRUCTION-EXTENSION-FETCH` at
  `tests/unit/cpu/test_instruction_extension_fetch.c` proves count-one/no-read,
  exact low/high count-two carriers, overflow, missing callbacks, every bus-fault
  status, callback scribble, no retry, complete-bundle-only publication,
  deterministic failures, and no pointer retention. `T-PROJECT-SEQUENCER-MULTIWORD-PC`
  at `tests/unit/sequencer/test_program_pc_bridge_multiword.c` exhausts count zero,
  valid widths and boundaries 1..255, count-one regression, explicit two-word
  boundary cases, high carriers, malformed/stale effects, and no mutation. Both
  remain `PLANNED` with null runners until their owners implement them.
- Registry kickoff: added immutable evidence
  `E-PROJECT-P2-W19B-EXTENSION-FETCH-PC-CONTRACT`, planned requirements
  `REQ-PROJECT-INSTRUCTION-EXTENSION-FETCH` and
  `REQ-PROJECT-SEQUENCER-MULTIWORD-PC`, and planned tests
  `T-PROJECT-INSTRUCTION-EXTENSION-FETCH` and
  `T-PROJECT-SEQUENCER-MULTIWORD-PC`, with forward and reverse links. Expected
  totals are 2 targets, 7 sources, 65 immutable evidence records, 94 requirements
  (78 implemented, 4 planned, 12 unresolved), and 64 tests (60 passing, 4 planned).
- Scope/provenance: no TI evidence or conformance claim is added. Machine-cycle
  timing, memory effects, IRQ, repeat semantics, emulator integration, external
  code/tables, environment knobs, diagnostics, allocators, and mutable globals
  remain out of scope. Ownership follows the directory hierarchy and no monolith
  is introduced.
- Focused docs-only validation: run the evidence/schema and registry quality
  checks plus `git diff --check`; do not run the full build/test gate for this
  kickoff. The later implementation work owns the coherent `make -B test` gate.

## 2026-09-06 — P2-W19B completion — transactional extension fetch and width-aware program PC

- State: implemented and green. Implementation commit:
  `3e57e316bdac5cbb469cba2cc5d5060cc45d1dea` (`P2-W19B: add extension fetch and width-aware PC`).
- Plan: complete the two private seams required before multiword opcode or repeat
  integration: CPU-owned extension collection and sequencer-owned width-aware PC
  span/commit. The anchored plan heading remains `P2-W19B: transactional extension
  fetch and width-aware program PC (active kickoff)` so the immutable evidence
  locator remains stable.
- TDD red summaries: the CPU extension-fetch matrix was initially unavailable
  without its owner implementation and the multiword PC matrix was initially
  rejected by the one-word-only planner. The implementation then made the red
  cases executable: count-one/no-read, exact count-two carriers and failure
  publication on the fetch seam; exhaustive widths, boundaries, malformed/stale
  effects, and no-mutation cases on the PC seam.
- Ownership/API decisions: `src/cpu/instruction_fetch.[ch]` owns a by-value,
  allocation-free two-word helper result and retains no bus/context pointer;
  `src/sequencer/sequential_pc.[ch]` and `program_pc_bridge.[ch]` own checked
  `uint8_t` widths 1..255 and transactional 16-bit PC effects. The helper does
  not fetch the first word or mutate CPU state, and the bridge performs no bus
  I/O. No production opcode, catalog row, handler, or CPU count-two retirement
  integration was introduced.
- Independent review: Luna implementation review reported no blocker, major, or
  minor finding. The fetch collector is 74 source lines, sequential-PC owner is
  80, and public-carrier bridge remains 114; ownership is mirrored by the directory
  tree and no monolith was introduced.
- Registry final: 2 targets, 7 sources, 65 immutable evidence records, and 94
  requirements (80 `IMPLEMENTED`, 2 `PLANNED`, 12 `UNRESOLVED`); 64 tests (62
  `PASSING`, 2 `PLANNED`). The two W19B requirements are implemented and their
  tests are passing under `ti_c54x.cpu_instruction_extension_fetch` and
  `ti_c54x.program_pc_bridge_multiword`.
- Validation: focused normal and ASan/UBSan tests passed for both new seams;
  full main validation `make -B test` passed with 161 Python quality tests, 57 C
  unit targets, and 58 direct executions including the minimal consumer. The
  evidence checker, evidence registry/runner unit tests, and `git diff --check`
  also passed.
- Isolation: no environment reads or `getenv`, allocator, logging, diagnostic,
  knob, public API, `cpu.c`, or P2-W18 contract changes. No timing, TI
  conformance, repeat, IRQ, memory mutation, XPC, or emulator integration claim
  was added.
- Follow-up: publish the implementation and completion records, verify CI, then
  begin evidence-first RPT/IRQ semantics. RPT will decide the later orchestration
  ordering among span validation, extension reads, operand extraction, and
  semantics.

## 2026-09-06 — P2-W19B remote verification

- Published implementation commit:
  `3e57e316bdac5cbb469cba2cc5d5060cc45d1dea`.
- Published completion-ledger commit:
  `f81eb6eb4476d8350c638ecb985b66c28b3bb1d9`.
- GitHub Actions run
  [34018936988](https://github.com/djr-747/ti-c54x/actions/runs/34018936988)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: P2-W19B is published and proven across repository-owned build surfaces;
  standalone isolation, private fetch/sequencer ownership, and the unchanged
  P2-W18 CPU transaction remain intact. The next slice is evidence-first RPT/IRQ
  semantics. This verification makes no claim about production multiword CPU
  retirement, timing, memory mutation, XPC, or emulator integration.

## 2026-09-06 — P2-W20A kickoff — RPT evidence and instruction-transaction architecture

- State: active documentation/evidence contract; no production source, public
  header, test implementation, generated artifact, or build manifest changed.
- Primary evidence: new narrow records pin `SPRU172C` Table 2-16, the RPT entry,
  and the repeatability restrictions, plus `SPRU131G` single-repeat and request
  handling sections. The immediate encodings are one-word `0xEC00/0xFF00` with
  K in bits 7:0 and exact two-word `0xF070` plus a 16-bit extension. The operation
  loads RC with the operand, changes no status bits, and executes the following
  instruction operand+1 times, including 65536 bodies for `0xffff`.
- Request contract: after RPT decode, maskable requests and NMI are deferred until
  every body execution completes; literal RS/reset remains admissible. A request
  can remain represented in IFR while acknowledgement is deferred and clears only
  when taken. No source bit, vector, priority, or device wiring is selected.
- Shape decision: P2-W20 is split into owner-local TDD slices. W20B owns the
  canonical copied instruction bundle under `src/isa/decode/`; W20C owns immutable
  catalog layout/repeatability binding; W20D owns RPT rows and a value-owned
  `REPEAT_START` effect; W20E owns pure repeat cardinality/eligibility/admission
  under `src/sequencer/repeat/`; W20F is later generic CPU integration. `cpu.c`
  must never branch on the RPT identity, and handlers never mutate state or IRQ
  policy directly.
- Unresolved by design: exact RC decrement phase/final RC, repeated-body PC
  hold/reload/final PC, and arbitration of a request already pending before RPT
  decode. Those requirements have no tests and block CPU repeat retirement rather
  than being filled by convenience behavior. Machine-cycle and detailed pipeline
  behavior remain outside this work piece.
- Secondary oracle: GNU binutils 2.42 is pinned in `docs/provenance.md` at source
  commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e9`, with independent `ec20` and
  `f070 ffff` cross-check vectors. It is `TOOLCHAIN_ORACLE_ONLY`, is not registered
  as normative evidence, and no GPL code, table, comment, or fixture corpus is
  copied or vendored.
- Registry kickoff: 2 targets, 7 sources, 72 immutable evidence records, and 105
  requirements (80 `IMPLEMENTED`, 10 `PLANNED`, 15 `UNRESOLVED`); 72 tests (62
  `PASSING`, 10 `PLANNED`). Eight new planned tests mirror their decode, opcode,
  repeat, and interrupt owners.
- Knobs/isolation unchanged: no `TEST`, `OBSERVE`, or `A/B` knob, environment read,
  diagnostic path, allocator, mutable global, emulator dependency, timing API, or
  shared-memory mutation is added.
- Validation for this docs-only kickoff is limited to the evidence/schema and
  registry quality checks plus `git diff --check`; the next implementation work
  piece owns the coherent full standalone test gate.
- Follow-up: commit and remotely verify this kickoff, then begin P2-W20B with red
  tests for the canonical complete-instruction bundle and unchanged NOP behavior.

## 2026-09-06 — P2-W20A kickoff remote verification

- Published kickoff commit:
  `0b1f0825c631b39cdfa77c033f377b141166886f`.
- GitHub Actions run
  [34020296678](https://github.com/djr-747/ti-c54x/actions/runs/34020296678)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: the RPT evidence, phased owner hierarchy, planned proof inventory, and
  explicit unresolved boundaries are published and green. No production behavior
  changed. P2-W20B canonical instruction-bundle TDD is next.

## 2026-09-06 — P2-W20B kickoff — canonical complete-instruction bundle

- State: active. The stable contract is the P2-W20B subsection under the existing
  P2-W20A implementation-plan anchor. Production still registers only NOP.
- Decode ownership: move the copied two-word helper bundle type under
  `src/isa/decode/`, retain an explicit helper-capacity boundary, and add a
  transactional complete-instruction binder that validates first-word decode,
  catalog metadata/pattern, bundle count, and copied first word before publication.
- CPU/operation ownership: CPU extension fetch only populates the canonical value;
  semantic dispatch and NOP consume the completed value. NOP remains one callback,
  one attempted/retired instruction, one-word PC advance, and no opcode-owned
  effect. CPU code gains no opcode-ID branch.
- TDD: add the planned decode-owned bundle test first, retain extension-fetch,
  decode, dispatch, CPU transaction, counter, PC bridge, and observer regressions,
  then run focused normal/sanitizer proof before the single coherent full gate.
- Exclusions: no RPT catalog row, operand-layout binding, repeat effect/state, RC,
  IRQ, timing, memory mutation, public API, knob, diagnostic path, or emulator
  dependency is introduced.
- Follow-up: implement through explicitly split decode, CPU, and opcode owners;
  main reviews and commits after the full standalone gate.

## 2026-09-06 — P2-W20B completion — canonical complete-instruction bundle

- State: implemented and green. Implementation commit:
  `00700d5dd8bd72b711a5cb8ad1147efd5198a92c`
  (`P2-W20B: add canonical instruction bundle`).
- TDD result: the planned decode-owned bundle proof covers successful copied-value
  completion, no retained caller storage, every public validation dimension, and
  byte-preserving transactional failures. Existing extension-fetch, NOP dispatch,
  CPU transaction/counter, PC, and observer tests remain green.
- Ownership: `src/isa/decode/instruction_bundle.h` owns only the fixed-capacity
  value carrier; `opcode_decode.[ch]` owns checked completion; `src/cpu/` owns bus
  population and orchestration; `src/isa/ops/` consumes the complete value. The CPU
  pipeline is first fetch, first-word decode, extension fetch, completion, generic
  dispatch, width-aware PC commit, then retirement. It contains no opcode-ID branch.
- Compatibility: NOP remains the only production opcode and performs exactly one
  bus callback, one attempt, one retirement, and one-word PC advance. First-word
  decode and the public API are unchanged; no RPT behavior is claimed.
- Independent Luna review: no blocker, major, correctness, ownership, manifest,
  undefined-behavior, or padding issue. Some validation is intentionally repeated
  at completion, dispatch, and the NOP boundary; a generated trusted fast path may
  remove this only after broader ISA proof makes the invariant measurable.
- Size/shape: the new carrier is 38 lines; complete-decode source/header are 103/76
  lines; CPU is 335 lines; fetch is 54; NOP/dispatch are 46/75. No handwritten owner
  approaches the 500-line review threshold and the directory hierarchy mirrors
  responsibility.
- Registry final: 2 targets, 7 sources, 72 immutable evidence records, and 105
  requirements (81 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`); 72 tests (63
  `PASSING`, 9 `PLANNED`). `T-PROJECT-CANONICAL-INSTRUCTION-BUNDLE` runs as
  `ti_c54x.instruction_bundle` under both supported build systems.
- Validation: focused normal and ASan/UBSan owner tests passed; full main validation
  `make -B test` passed with 161 Python quality tests, 58 C unit targets, and 59
  direct executions including the minimal consumer. Evidence, source-boundary,
  manifest, knob, observer-off artifact, and `git diff --check` gates passed.
- Isolation: no environment read or `getenv`, allocator, logging, diagnostic, knob,
  mutable global, timing API, memory mutation, emulator dependency, or external
  source/table/comment/test was introduced.
- Follow-up: publish and remotely verify P2-W20B, then begin P2-W20C. Operand-layout
  and repeatability metadata must land without transiently registering an RPT row
  that decodes successfully but can only terminate the CPU as `INVALID_STATE`.

## 2026-09-06 — P2-W20B remote verification

- Published implementation commit:
  `00700d5dd8bd72b711a5cb8ad1147efd5198a92c`.
- Published completion-record commit:
  `1537f7cc25cda32a0ee910d7a25e2f4ced1e0f9e`.
- GitHub Actions run
  [34021405630](https://github.com/djr-747/ti-c54x/actions/runs/34021405630)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: the canonical complete-instruction bundle is published and green across
  every repository-owned build surface. NOP compatibility, standalone isolation,
  copied-value ownership, and generic CPU orchestration remain intact. P2-W20C
  catalog-bound operand-layout and repeatability metadata is next.

## 2026-09-06 — P2-W20C kickoff — catalog metadata and operand-layout ownership

- State: active. The stable contract is the P2-W20C implementation-plan subsection.
  Registry kickoff totals are 2 targets, 7 sources, 72 evidence records, 106
  requirements (81 implemented, 10 planned, 15 unresolved), and 73 tests (63
  passing, 10 planned).
- Decision: split W20C into two atomic proofs. W20C1 owns scalar catalog metadata
  tokens; W20C2 owns decode layout-set descriptors/resolution/extraction. This keeps
  catalog independent of decoder types and avoids a speculative single-operand
  carrier that would not scale across the ISA.
- W20C1: every sole-inventory opcode row must name a fully namespaced operand-layout
  identity and tri-state repeatability class. NOP uses layout `NONE` and explicit
  `UNRESOLVED` repeatability. Schema, C macro passes, immutable metadata, decoded
  values, dispatch, and NOP validation agree; the direct lookup does not change.
- W20C2: `src/isa/decode/` owns immutable layout sets, source-width validation, and
  copied extraction through the existing shared field extractor. The catalog owns
  scalar IDs only; handlers own no masks. Outputs remain transactional and no input
  pointer is retained.
- Staging rule: no production RPT row, handler, effect, or CPU path lands in W20C.
  Inert RPT layouts may be proven only under decode ownership. Catalog activation
  must be coherent with a typed `REPEAT_START` effect and generic consumer; a
  decode-success then CPU-`INVALID_STATE` interval is forbidden.
- Delegation/review: two independent Luna read-only audits converged on the scalar
  token/decode resolver split. A bounded Luna worker may own implementation files;
  the main agent retains architecture decisions, integration, validation, commits,
  publication, and the append-only completion record.
- Exclusions: no TI execution claim, RC/repeat-PC policy, body eligibility decision,
  interrupt gate, cycle model, memory mutation, public API, knob, environment read,
  diagnostic, allocator, mutable global, emulator dependency, or external source is
  introduced.
- Validation for this docs-only kickoff is limited to evidence/schema quality and
  `git diff --check`; the implementation work piece owns its coherent standalone
  full test gate.

## 2026-09-06 — P2-W20C kickoff remote verification

- Published kickoff commit:
  `bac48dc04e763ce0c879c8ecfd2bc201e010b75c`.
- GitHub Actions run
  [34021936812](https://github.com/djr-747/ti-c54x/actions/runs/34021936812)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: the W20C1/W20C2 ownership split, planned proof, and no-RPT-activation gate
  are published and green. W20C1 scalar catalog metadata TDD is next.

## 2026-09-06 — P2-W20C1 completion — scalar catalog metadata tokens

- State: implemented and green. Implementation commit:
  `d56a9cc28f183c8e348837e393458660e37091d3`
  (`P2-W20C1: add catalog metadata tokens`). P2-W20C remains active for W20C2.
- TDD red: before production/schema completion, 18 focused catalog tests produced
  3 failures and 10 errors because old four/five-field rows lacked mandatory layout
  and repeatability metadata and direct `OpcodeRow` values lacked the new fields.
- Catalog/schema result: exact and masked rows now require fully namespaced scalar
  layout and repeatability tokens. The frozen Python row schema validates arity,
  types, known values, reserved sentinels, duplicates, ranges, and overlap; all
  three C X-macro passes consume the same sole inventory. The generated direct
  lookup is byte-for-byte unchanged and still classifies only NOP.
- Runtime result: immutable opcode metadata and the copied decoded-instruction value
  carry `ti_c54x_operand_layout_id` and `ti_c54x_repeatability_class`. Completion,
  dispatch, and NOP validate ranges/equality transactionally. NOP is `NONE` plus
  `UNRESOLVED`; unresolved never means eligible and no repeat gate is claimed.
- Ownership: `src/isa/catalog/` owns token identity, row schema, and immutable
  metadata; `src/isa/decode/` only copies and validates the scalars; operations only
  consume the completed value. Mirrored C proof lives under
  `tests/unit/isa/catalog/`. No decoder descriptor pointer enters catalog metadata.
- Independent Luna review: no blocker, major, C11/undefined-behavior, schema,
  ownership, manifest, generated-artifact, or transactional finding. The main
  follow-up added explicit negative forged-enum cases and non-string schema inputs.
  Repeated hot-path validation remains intentional until a measured generated
  trusted path can replace it safely.
- Size/shape: catalog source/header are 71/70 lines, schema tooling is 310, complete
  decode source/header are 114/81, dispatch/NOP are 83/52, and the new catalog test
  is 189. No handwritten owner approaches the 500-line review threshold.
- Registry final: 2 targets, 7 sources, 72 evidence records, and 106 requirements
  (82 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`); 73 tests (64 `PASSING`, 9
  `PLANNED`). `T-PROJECT-CATALOG-METADATA-TOKENS` runs as
  `ti_c54x.opcode_metadata` in both supported build systems.
- Validation: focused normal and ASan/UBSan ISA tests passed; the coherent full
  `make -B test` gate passed 163 Python quality tests, 59 C unit targets, and 60
  direct executions including the minimal consumer. Generated lookup staleness,
  manifests, source boundaries, evidence, knobs, observer-off artifact, and
  `git diff --check` passed.
- Isolation: no RPT row/layout/effect, CPU/state/sequencer/public API change,
  environment read, diagnostic, observer, knob, allocation, mutable global, timing,
  shared memory, emulator dependency, or external implementation was introduced.
- Follow-up: publish and remotely verify W20C1, then implement W20C2 decode-owned
  immutable layout sets and copied extraction without activating RPT production
  decode.

## 2026-09-06 — P2-W20C1 remote verification

- Published implementation commit:
  `d56a9cc28f183c8e348837e393458660e37091d3`.
- Published completion-record commit:
  `e932963af34e20f131ffa4814f6db6de5fd3b912`.
- GitHub Actions run
  [34023016646](https://github.com/djr-747/ti-c54x/actions/runs/34023016646)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: W20C1 scalar catalog metadata is published and green across every owned
  build surface. The generated lookup and production NOP behavior remain unchanged;
  W20C2 decode-owned immutable layout sets are next.

## 2026-09-06 — P2-W20C2 kickoff — immutable operand-layout sets

- State: active under the existing P2-W20C plan and
  `REQ-PROJECT-CATALOG-OPERAND-LAYOUT-BINDING`; registry counts do not change.
- Decode ownership: add a dedicated layout-set resolver/extractor under
  `src/isa/decode/`. A set contains zero, one, or multiple operand-field layouts so
  later multi-operand instructions do not require a carrier redesign. Returned
  values may borrow only process-lifetime static descriptors and retain no caller
  pointer.
- Field ownership: extend `operand_field` with pure layout validation and reuse that
  exact rule path from extraction and layout-set binding. Coverage, source/destination
  overlap, bit widths, and source-word indices may not be reimplemented elsewhere.
- Initial identities: retain `NONE` and add inert RPT immediate-K and immediate-lk
  layout IDs/descriptors from already pinned TI evidence. They are directly provable
  through the layout owner but remain absent from the production opcode catalog and
  generated first-word lookup.
- TDD: the planned `tests/unit/isa/decode/test_operand_layout_binding.c` proves
  NONE, one/two-word extraction, multi-field set shape, every invalid token/index/
  width/layout path, transactional output, static lifetime, and catalog NOP binding.
  Existing operand-field, bundle, catalog, NOP, and CPU tests remain regressions.
- Exclusions: no production RPT row, handler/effect, CPU/sequencer/state behavior,
  repeat eligibility, RC/PC/IRQ policy, timing, memory mutation, public API, knob,
  environment read, diagnostic, allocator, mutable global, emulator dependency, or
  external implementation is introduced.
- Delegation: a bounded Luna worker owns the decode/layout implementation and tests;
  the main agent owns design, integration, independent review, full validation,
  commits, publication, and ledger completion.

## 2026-09-06 — P2-W20C2 kickoff remote verification

- Published kickoff commit:
  `c59d7d3d15b3cc11b9704ecbfe11dc5a6de1c6e8`.
- GitHub Actions run
  [34023175915](https://github.com/djr-747/ti-c54x/actions/runs/34023175915)
  completed successfully across GCC, Clang, sanitizer, CMake/CTest, install, and
  downstream-consumer surfaces.
- Result: the immutable layout-set contract and inert-RPT staging boundary are
  published and green. W20C2 decode implementation is active.

## 2026-09-06 — P2-W20C2 completion — immutable operand-layout sets

- State: implemented and green. Implementation commit:
  `4746a8c0a1074bdf3b9419721901a0f8f0808486`
  (`P2-W20C2: add immutable operand layouts`). P2-W20C is complete; P2-W20D is
  next.
- TDD red: after registering the new decode-owned proof in both build systems, its
  first focused build failed because `operand_layout.h` did not yet exist. The
  implementation then made the same proof green without adding a production opcode.
- Decode result: `operand_layout` owns process-lifetime immutable layout sets for
  `NONE`, inert one-word RPT immediate-K, and inert two-word RPT immediate-lk.
  Resolution checks scalar identity and explicit program-word width before publishing
  a borrowed-static view; extraction authenticates that view and returns copied raw
  values through the shared field extractor.
- Field result: `operand_field` now exposes one pure descriptor-validation seam.
  Resolution and extraction reuse its existing coverage, source/destination overlap,
  bit-width, and source-word-index rules rather than reimplementing masks. Layout
  sets support zero, one, or multiple fields and retain no caller storage.
- Catalog/completion result: the catalog adds only namespaced scalar identities.
  Complete-instruction binding resolves layout metadata generically, with no opcode-ID
  branch. NOP remains the sole production row; `0xEC00` and `0xF070` remain unknown,
  and the generated 65,536-entry first-word lookup is byte-for-byte unchanged.
- Proof: the new mirrored decode test covers `NONE`, static descriptor lifetime,
  multi-operand descriptor shape, K/lk minimum and maximum values, invalid IDs and
  widths, field/index failures, forged views, and byte-transactional failure output.
  Existing field, bundle, catalog, NOP, CPU, observer, and minimal-consumer tests
  remain regressions.
- Independent Luna review: clean, with no blocker or major finding across API shape,
  ownership direction, transactionality, manifests, generated artifacts, CPU/decode
  branching, or production-RPT isolation. The main follow-up tightened RPT wording
  and boundary/transactionality assertions.
- Size/shape: layout source/header are 203/78 lines, field source/header are 143/76,
  completion decode source is 124, schema tooling is 314, and the new layout proof
  is 254. No handwritten owner approaches the 500-line review threshold.
- Registry final: 2 targets, 7 sources, 72 evidence records, and 106 requirements
  (83 `IMPLEMENTED`, 8 `PLANNED`, 15 `UNRESOLVED`); 73 tests (65 `PASSING`, 8
  `PLANNED`). `T-PROJECT-CATALOG-OPERAND-LAYOUT-BINDING` runs as
  `ti_c54x.operand_layout_binding` in both supported build systems. The project
  contract remains scope-safe; the separate planned common RPT decode requirement
  retains the pinned TI immediate-layout evidence.
- Validation: focused normal and ASan/UBSan layout/field tests passed. The frozen
  full `make -B test` gate passed 163 Python quality tests, 60 C unit targets, and
  61 direct executions including the minimal consumer. Generated lookup staleness,
  manifests, source boundaries, evidence, knobs, observer-off artifact, and
  `git diff --check` passed.
- Isolation: no production RPT row, operation/effect, CPU/sequencer/state behavior,
  public API, RC/PC/IRQ policy, timing, memory mutation, knob, environment read,
  diagnostic, allocator, mutable global, emulator dependency, or external
  implementation was introduced.
- Follow-up: publish and remotely verify P2-W20C2. P2-W20D then owns a focused RPT
  operation that converts these proven copied operands into a discriminated,
  value-owned `REPEAT_START` effect without mutating state or creating an unsupported
  production CPU path.

## 2026-09-06 — P2-W20C2 remote verification

- Published implementation commit:
  `4746a8c0a1074bdf3b9419721901a0f8f0808486`.
- Published completion-record commit:
  `7cf5c3a69c07a50558fbd3d42f73c32002f8dcc2`.
- GitHub Actions run
  [34024069691](https://github.com/djr-747/ti-c54x/actions/runs/34024069691)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: P2-W20C is published and green across every repository-owned build
  surface. Catalog scalar metadata and decode-owned immutable layout sets are now
  complete; production decode still contains only NOP, and P2-W20D's pure typed RPT
  effect is next.

## 2026-09-06 — P2-W20D kickoff — typed immediate-RPT effect

- State: active under new `REQ-PROJECT-RPT-IMMEDIATE-EFFECT` and
  `T-PROJECT-RPT-IMMEDIATE-EFFECT`. Kickoff totals are 2 targets, 7 sources, 72
  evidence records, 107 requirements (83 implemented, 9 planned, 15 unresolved),
  and 74 tests (65 passing, 9 planned).
- Scope decision: W20D proves a project-owned effect boundary only. The existing
  common TI RPT decode and operation requirements remain planned because no
  production row, RC mutation, repeat sequencing, or repeatability classification
  is activated here.
- Input decision: the pure repeat-family builder consumes a borrowed copied
  operand-field value already extracted from W20C2's inert K/lk layouts. It does not
  accept a forged catalog-decoded instruction, retain caller storage, re-mask opcode
  words, or duplicate decoder validation.
- Effect decision: extend the private discriminated effect with `REPEAT_START` and
  one exact unsigned 16-bit operand value. Do not encode operand+1 body cardinality,
  RC phase/final state, PC flow, interrupt admission, timing, or state pointers in
  the opcode-owned value.
- Ownership: the first repeat opcode-family module lives under
  `src/isa/ops/repeat/`, with tests under the mirrored
  `tests/unit/isa/ops/repeat/` subtree and owner-local Make/CMake manifests. Parent
  ops manifests aggregate rather than flatten that subowner.
- TDD: first register the test so it fails for the missing module/effect, then prove
  layout-composed K/lk minimum, interior, and maximum values; exact copied payloads;
  explicit invalid-argument versus invalid-operand status; invalid widths/raw
  ranges; nulls; deterministic NOP compatibility; and byte-preserving failures.
  Existing production-decode tests remain the no-RPT gate.
- Independent Luna audits: both recommend the narrow typed effect and keeping the
  common TI operation planned until a generic consumer can apply it. The main
  architecture decision additionally avoids passing a catalog-decoded carrier with
  no valid RPT opcode identity.
- Exclusions: no catalog row, lookup regeneration, semantic dispatch, CPU change,
  RC/state mutation, body cardinality, repeat PC, eligibility, IRQ/NMI/IFR/reset
  policy, bus fetch, machine cycles, shared memory, profile, public API, observer,
  diagnostic, knob, environment read, allocator, mutable global, emulator
  dependency, or external implementation.
- Follow-up: publish and remotely verify this kickoff, then delegate the bounded
  repeat-family implementation to Luna under main-agent review and commit ownership.

## 2026-09-06 — P2-W20D kickoff remote verification

- Published kickoff commit:
  `ea14aa8e355a469333ff2a6c6adcb62be8fbb909`.
- GitHub Actions run
  [34024592265](https://github.com/djr-747/ti-c54x/actions/runs/34024592265)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: the scope-safe W20D project effect contract, repeat-family ownership tree,
  TDD matrix, and no-production-activation gate are published and green. The bounded
  Luna implementation is next.

## 2026-09-06 — P2-W20D completion — typed immediate-RPT effect

- State: implemented and green. Implementation commit:
  `f4a445ccd8d6bb54d791c86f12066409fc0c6cef`
  (`P2-W20D: add typed RPT effect`). The project effect requirement is complete;
  the common TI decode and operation requirements remain planned.
- TDD red: the new nested repeat-family test and manifests landed first; its focused
  build failed because the registered `op_rpt_immediate` object did not yet exist.
  The same test became green only after the owned effect and operation modules were
  implemented.
- Effect result: the private discriminated carrier now supports `NONE` and a named
  `REPEAT_START` payload with one copied `uint16_t` operand. No operand+1 cardinality,
  RC/state pointer, PC, interrupt, timing, memory, or bus field enters the value.
  NOP and generic dispatch stage the expanded effect from zeroed values.
- Operation result: `ti_c54x_op_rpt_immediate_from_field()` accepts only a borrowed
  copied operand-field value and caller-owned output. Nulls return
  `INVALID_ARGUMENT`; nonnull unsupported widths or raw values outside their 8/16-bit
  shape return the appended `INVALID_OPERAND` status. Every failure preserves output
  bytes, success copies the exact operand, and no pointer is retained.
- Ownership: the first opcode-family subowner lives under `src/isa/ops/repeat/` with
  a mirrored `tests/unit/isa/ops/repeat/` subtree. Parent Make/CMake manifests only
  aggregate the child manifests; the operation imports no catalog, decoded
  instruction, CPU, state, bus, observer, diagnostic, or host dependency.
- Proof: the project test composes W20C2 layout resolution/extraction for K and lk
  minimum, interior, and maximum vectors, then proves exact effect kind/payload,
  input nonmutation, copied lifetime, invalid widths/ranges, typed statuses, and
  byte-transactional failures. Existing NOP, decode, layout, CPU, and observer tests
  remain regressions.
- Independent Luna review: clean, with no blocker, major, minor, C11 object-model,
  ownership, manifest, transactionality, range, activation, or test-coverage
  finding. The main review named the payload types and removed cross-owner and
  bytewise-success-padding assertions from the repeat-family test.
- Size/shape: effect/status headers are 45/29 lines, repeat operation source/header
  are 42/33, and its focused proof is 162. No handwritten owner approaches the
  500-line review threshold.
- Registry final: 2 targets, 7 sources, 72 evidence records, and 107 requirements
  (84 `IMPLEMENTED`, 8 `PLANNED`, 15 `UNRESOLVED`); 74 tests (66 `PASSING`, 8
  `PLANNED`). `T-PROJECT-RPT-IMMEDIATE-EFFECT` runs as
  `ti_c54x.rpt_immediate_effect` in both supported build systems.
- Validation: focused normal and ASan/UBSan repeat-effect tests passed. The frozen
  full `make -B test` gate passed 163 Python quality tests, 61 C unit targets, and
  62 direct executions including the minimal consumer. Nested manifest parity,
  source boundaries, evidence, knobs, observer-off artifact, generated lookup
  staleness, and `git diff --check` passed.
- Isolation: no production RPT row, lookup change, dispatch-table registration, CPU
  behavior, RC/state mutation, repeat cardinality, repeat PC, body eligibility,
  IRQ/NMI/IFR/reset policy, bus fetch, machine cycles, shared memory, profile, public
  API, observer, diagnostic, knob, environment read, allocator, mutable global,
  emulator dependency, or external implementation was introduced.
- Follow-up: publish and remotely verify W20D, then begin P2-W20E as the pure
  `src/sequencer/repeat/` owner without activating production RPT decode prematurely.

## 2026-09-06 — P2-W20D Clang portability correction and remote verification

- Published implementation commit:
  `f4a445ccd8d6bb54d791c86f12066409fc0c6cef`.
- Published completion-record commit:
  `120b7100407ae6ec1f229361599891db6073280d`.
- The first completion run
  [34025437867](https://github.com/djr-747/ti-c54x/actions/runs/34025437867)
  exposed one Clang-only `-Wmissing-field-initializers` error in the CPU's local
  effect value after the effect carrier gained its payload union. GCC and the frozen
  local gate had accepted the partial aggregate initializer.
- Correction commit: `73434c65190118a1f8ac2d314abecbb0ab99c469`
  (`fix: initialize expanded CPU effect`). It replaces that partial initializer with
  a complete zero-initializer. This is behavior-preserving because
  `TI_C54X_OP_EFFECT_NONE` is explicitly zero and the CPU still rejects every
  non-`NONE` effect.
- Local correction validation: the frozen full `make -B test` gate passed 163 Python
  quality tests, 61 C unit targets, and 62 direct executions including the minimal
  consumer. Clang is not installed on the development host, so the exact compiler
  regression remained owned by the remote matrix.
- GitHub Actions run
  [34025576755](https://github.com/djr-747/ti-c54x/actions/runs/34025576755)
  completed successfully across GCC, Clang, and Clang sanitizers, including direct
  Make tests, library-only builds, CMake/CTest, installation, and downstream package
  consumption.
- Result: W20D is published and green on every repository-owned build surface. The
  typed immediate-RPT effect remains pure and inactive in production decode; P2-W20E
  may now begin under the dedicated `src/sequencer/repeat/` owner.

## 2026-09-06 — P2-W20E kickoff — pure single-repeat policy foundation

- State: active under three new project-contract requirements and tests for
  cardinality derivation, preclassified-body admission, and repeat-only request
  gating. Kickoff totals are 2 targets, 7 sources, 73 evidence records, 110
  requirements (84 implemented, 11 planned, 15 unresolved), and 77 tests (66
  passing, 11 planned).
- Scope decision: W20E remains a pure policy foundation. The existing common TI
  cardinality, body-eligibility, and IRQ-gate requirements remain planned because a
  computed count does not prove body retirement, a generic scalar decision does not
  prove concrete production classifications, and no active repeat window is yet
  integrated.
- Ownership: create `src/sequencer/repeat/` and the mirrored
  `tests/unit/sequencer/repeat/` subtree. Cardinality, body admission, and request
  gating are separate leaf modules; parent Make/CMake manifests aggregate the child
  owner rather than listing its sources and tests directly.
- Cardinality decision: accept the complete copied `uint16_t` operand domain, widen
  before adding one, and publish a caller-owned `uint32_t` count transactionally.
  The proof sweeps all 65,536 operands and explicitly checks one and 65536 endpoints.
- Eligibility decision: consume only the catalog's scalar tri-state class. Only
  `ELIGIBLE` admits; `INELIGIBLE` rejects; `UNRESOLVED` remains distinct and cannot
  silently admit. No opcode identity, width, operand, addressing form, or table
  lookup enters the policy.
- Request-gate decision: accept an explicit remaining-body count from zero through
  65536 plus maskable, NMI, or literal RS/reset class. The repeat gate blocks nothing
  when inactive, defers maskable and NMI while active, and never blocks literal
  reset. This is not general interrupt admission and owns no IFR state.
- TDD: register three planned project tests, then make each fail for its missing leaf
  module before implementation. Cover invalid pointers, forged enums, counts above
  65536, and transactional outputs in addition to the valid policy matrix. Existing
  common RPT tests remain planned.
- Independent Luna audits: both recommend the three-leaf owner shape and flag the
  existing common cardinality wording as too broad for a pure calculation. The main
  decision follows the conservative alternative: add project proofs now and leave
  integration-level TI conformance for W20F or later.
- Documentation correction: the evidence-index and architecture-baseline headline
  counts are updated from their stale P2-W19B values to the current kickoff totals;
  historical ledger counts remain unchanged.
- Exclusions: no production RPT row, generated lookup change, opcode-effect
  consumption, CPU/state/RC/PC mutation, body execution loop, IFR sampling/latching/
  clearing, device wiring, timing, bus, memory, observer, diagnostic, knob,
  environment read, allocator, mutable global, emulator dependency, or external
  implementation.
- Follow-up: publish and remotely verify the kickoff, then delegate the bounded
  three-leaf implementation to Luna under main-agent review and commit ownership.

## 2026-09-06 — P2-W20E kickoff remote verification

- Published kickoff commit:
  `86f08ac1c623490d97ff29572b68e29f995b89eb`.
- GitHub Actions run
  [34026125995](https://github.com/djr-747/ti-c54x/actions/runs/34026125995)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: the W20E evidence boundary, three project contracts, three-leaf ownership
  plan, TDD matrix, corrected registry counts, and deliberate no-common-conformance
  claim are published and green. The bounded Luna implementation is next.

## 2026-09-06 — P2-W20E completion — pure single-repeat policies

- State: implemented and green. Implementation commit:
  `d876f94dfb6ddbd08257f477457ca20ae4f34e68`
  (`P2-W20E: add pure repeat policies`). The three project policy requirements are
  complete; all existing common TI RPT/repeat requirements retain their prior
  planned or unresolved states.
- TDD red: the mirrored repeat tests and nested manifests were registered first;
  the focused Make build stopped because the required
  `sequencer/repeat/repeat_cardinality.o` implementation did not exist. The same
  three registered targets became green only after their leaf owners were added.
- Cardinality result: `repeat_cardinality` widens the copied `uint16_t` operand to
  `uint32_t` before adding one and publishes a caller-owned value only after
  validation. Its test exhausts all 65,536 operands and proves the one and 65536
  endpoints without exposing an execution loop, RC, or PC.
- Body-admission result: `repeat_body_eligibility` consumes only the catalog's
  scalar tri-state value. `ELIGIBLE` admits, `INELIGIBLE` rejects, and `UNRESOLVED`
  remains a distinct zero-valued non-admitting result. `COUNT` and negative forged
  values fail without changing the caller's decision; no opcode/table lookup or
  operand/address inference occurs.
- Request-gate result: `repeat_request_gate` accepts zero through 65536 remaining
  body executions and explicit maskable, NMI, or literal RS/reset classes. Zero
  reports that repeat does not block any valid class; active counts defer maskable
  and NMI while never blocking literal reset. Larger counts, invalid/forged kinds,
  and null output are explicit transactional failures. `NOT_BLOCKED` does not mean
  globally admitted or serviced.
- Ownership: source and tests live under mirrored `sequencer/repeat/` subtrees with
  child Make/CMake manifests. The parent sequencer owners add only one include or
  subdirectory aggregation line. Cardinality, eligibility, and request policy remain
  separate leaf modules rather than forming a repeat-control monolith.
- Review: independent Luna review reported no blocker, major, or minor finding for
  behavior, C11/Clang shape, enum-zero safety, transactionality, units/naming,
  ownership direction, build parity, source boundaries, or scope. After delegated
  scaffolding did not complete, the main agent integrated and reviewed the bounded
  implementation without expanding file ownership.
- Size/shape: cardinality source/header are 33/41 lines, body-eligibility source/
  header are 43/42, request-gate source/header are 53/53, and their focused tests are
  67, 76, and 123 lines. No handwritten owner approaches the 500-line review
  threshold.
- Registry final: 2 targets, 7 sources, 73 evidence records, and 110 requirements
  (87 `IMPLEMENTED`, 8 `PLANNED`, 15 `UNRESOLVED`); 77 tests (69 `PASSING`, 8
  `PLANNED`). The three new project tests run as
  `ti_c54x.repeat_cardinality_contract`,
  `ti_c54x.repeat_body_admission_contract`, and
  `ti_c54x.repeat_request_gate_contract` in both supported build systems.
- Validation: focused Make builds and direct executions passed after the registered
  RED. The frozen full `make -B test` gate passed 163 Python quality tests, 64 C
  unit targets, and 65 direct executions including the minimal consumer. Evidence,
  nested manifest reachability/parity, source boundaries (139 files), knobs,
  observer-off artifact, generated lookup staleness, and `git diff --check` passed.
- Isolation: no production RPT row, generated lookup change, opcode-effect
  consumption, CPU/state/RC/PC mutation, body execution loop, IFR sampling/latching/
  clearing, request acknowledgement, device wiring, timing, bus, memory, public API,
  observer, diagnostic, knob, environment read, allocator, mutable global, emulator
  dependency, or external implementation was introduced.
- Follow-up: publish and remotely verify W20E. Do not begin W20F CPU integration
  until evidence resolves RC progression, repeated-body PC flow, and predecode
  interrupt arbitration; the common TI rows remain the integration proof gates.

## 2026-09-06 — P2-W20E remote verification

- Published implementation commit:
  `d876f94dfb6ddbd08257f477457ca20ae4f34e68`.
- Published completion-record commit:
  `348fe0761842d7af57a9a219162e3daedb8ab339`.
- GitHub Actions run
  [34027010846](https://github.com/djr-747/ti-c54x/actions/runs/34027010846)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and the Clang sanitizer configure/build/test path.
- Result: W20E's three pure repeat-policy leaves are published and green on every
  repository-owned build surface. Production decode remains NOP-only, the common TI
  repeat requirements remain planned, and unresolved RC/PC/predecode-interrupt
  semantics continue to block W20F CPU integration.

## 2026-09-06 — P2-W20F evidence closure — CPU integration deferred

- State: bounded primary-source and toolchain-oracle review complete; no production
  source, public API, catalog row, generated artifact, test runner, or build manifest
  changed. W20F CPU integration is explicitly deferred rather than assigned inferred
  RC, PC, or interrupt behavior.
- TI evidence added: `SPRU172C` Rev C RPT printed page 4-146 states that software
  cannot access RC while single-repeat execution decrements it. The MVDK/MVKD
  entries on printed pages 4-108–4-114 make their repeated address increment depend
  on current `RC != 0` and describe an initial `RC+1` transfer count. These facts do
  not identify the decrement phase or final RC value.
- TI evidence boundary retained: `SPRU131G` Rev G §6.7 starts the interrupt barrier
  once RPT is decoded. Its generic request receipt, acknowledgement, latency, and
  pipeline sections do not specify whether an already-pending request wins before
  that decode boundary. Generic PC and block-repeat descriptions do not define the
  single-repeat PC hold/reload rule or final PC.
- Requirement decision: `REQ-COMMON-SINGLE-REPEAT-RC-PROGRESSION`,
  `REQ-COMMON-SINGLE-REPEAT-PC-FLOW`, and
  `REQ-COMMON-SINGLE-REPEAT-PREDECODE-IRQ-ARBITRATION` remain `UNRESOLVED` with no
  invented tests. The common integrated RPT requirements remain `PLANNED` and the
  project-only W20D/W20E proofs remain unchanged.
- Secondary oracle: official GNU binutils 2.42 at pinned sourceware commit
  `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e9` confirms only masks, word counts,
  operand ranges, repeat-marker, and nonrepeatability metadata. It offers no RC/PC/
  IFR/arbitration execution semantics and remains `TOOLCHAIN_ORACLE_ONLY`; no GPL
  implementation, table, comment, or fixture is copied.
- Registry after the evidence additions: 2 targets, 7 sources, 75 immutable evidence
  records, 110 requirements (87 `IMPLEMENTED`, 8 `PLANNED`, 15 `UNRESOLVED`), and
  77 tests (69 `PASSING`, 8 `PLANNED`).
- Next normative proof input: stronger TI evidence or a versioned original-hardware
  observation recording exact initial state, program words, request timing, visible
  RC/PC results, and provenance. An optional black-box differential vector may
  identify a hypothesis or disagreement but cannot close the common requirement by
  itself, and may not become a standalone build dependency.
- Follow-up: validate and publish this evidence closure, then advance the independent
  shared-memory transaction foundation before any memory-mutating opcode family.

## 2026-09-06 — P2-W20F evidence-closure remote verification

- Published evidence-closure commit:
  `62ece60e02c18b80bf8698542a1cc56bca3f823c`.
- GitHub Actions run
  [34027853902](https://github.com/djr-747/ti-c54x/actions/runs/34027853902)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and Clang sanitizer configure/build/test.
- Result: the narrowed RC evidence, unchanged unresolved requirements, W20F deferral,
  and provenance boundary are published and green on every repository-owned build
  surface. The active path now moves to P2-W21 shared-memory transaction foundations;
  production RPT decode remains inert.

## 2026-09-06 — P2-W21A kickoff — private WORD16 bus request

- State: active documentation/project-contract kickoff. No production source,
  public header, callback ABI, generated artifact, build manifest, or executable
  test changed in this commit.
- Ownership: the first implementation creates `src/bus/request/` and mirrored
  `tests/unit/bus/request/` leaf owners. Parent `src/bus/` and `tests/unit/bus/`
  manifests aggregate the leaf; root Make/CMake files aggregate only the bus owner.
- Request decision: one value-owned request represents exactly one architectural
  16-bit word and names PROGRAM, DATA, or IO plus READ or WRITE. PROGRAM preserves
  the exact `uint32_t` carrier. DATA and IO constructors also consume `uint32_t` so
  values above `UINT16_MAX` are rejected before narrowing. A read request has a
  canonical zero write value; a write request copies the exact `uint16_t` input.
- Status decision: construction reports only `OK`, `INVALID_SPACE`, or
  `INVALID_ADDRESS`, checking the space tag before its address domain. Failed
  construction zeroes every named request field; tests do not compare or assign a
  semantic value to C padding bytes.
- Shape decision: the WORD16 type owns transfer width, so no one-value width enum is
  added. The pure request owns no phase; P2-W21C binds a copied request to a named
  sequencer phase when it becomes a resumable transaction. Multiword operands will
  compose separately ordered word requests rather than imply atomic host `uint32_t`
  access.
- Shared-memory boundary: PROGRAM/DATA/IO remain the only architectural spaces.
  `SHARED` is not added as a space because sharing, byte lanes, mapping, and
  DSP/ARM/DMA/device arbitration belong to external adapters and profile owners.
- ABI decision: the current synchronous public callbacks and
  `ti_c54x_bus_status` remain unchanged. WAIT and pending-token behavior require the
  later private transaction seam; no incomplete lifecycle is retrofitted into the
  phase-one callback contract.
- TDD plan: register `T-PROJECT-BUS-WORD16-REQUEST-SHAPE` under the mirrored owner,
  then prove all six space/access combinations, program/data/IO address boundaries,
  pre-narrowing rejection, copied write values, canonical reads, invalid/forged
  discriminants, named-field-zero invalid results, and null validation. Source-
  boundary checks, rather than a dynamic callback test, prove the request owner has
  no callback or host-I/O surface.
- Registry kickoff: 2 targets, 7 sources, 76 immutable evidence records, 111
  requirements (87 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`), and 78 tests
  (69 `PASSING`, 9 `PLANNED`). The new requirement is explicitly
  `PROJECT_CONTRACT`; no TI bus, byte-lane, wait-state, atomicity, or device claim is
  inferred.
- Knobs/isolation: no `TEST`, `OBSERVE`, or `A/B` knob, environment read, observer,
  diagnostic, allocator, mutable global, host-memory pointer, firmware, emulator
  dependency, CPU behavior, opcode row, memory effect, or timing API is introduced.
- Follow-up: validate and publish the kickoff, then delegate the bounded W21A TDD
  implementation under main-agent review and commit ownership. P2-W21B owns byte
  order and P2-W21C owns the resumable exactly-once transaction lifecycle.

## 2026-09-06 — P2-W21A kickoff remote verification

- Published kickoff commit:
  `ce3a921319ce55dcdbff50f1f786e5c13420092e`.
- GitHub Actions run
  [34028395152](https://github.com/djr-747/ti-c54x/actions/runs/34028395152)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and Clang sanitizer configure/build/test.
- Result: the W21 owner split, private WORD16 request contract, unchanged synchronous
  public ABI, explicit no-width/no-phase decision, project-only requirement, and
  standalone isolation are published and green. The bounded Luna TDD implementation
  is active under the new `bus/request/` ownership boundary.

## 2026-09-06 — P2-W21A completion — private WORD16 bus request

- State: implemented and green. Implementation commit:
  `2f23aeea02042f3419623a585066c192c2f2a402`
  (`P2-W21A: add WORD16 bus requests`).
- TDD RED: owner-mirrored tests and manifests were registered first;
  `rtk make -B build-fast/test_bus_word16_request` failed because the required
  `build-fast/obj/bus/request/word16_request.o` did not exist. Implementation made
  that same target and direct executable pass.
- Request result: separate read and write constructors return a value-owned status
  plus request. PROGRAM preserves all `uint32_t` carriers through `UINT32_MAX`;
  DATA/IO reject `0x10000` and `UINT32_MAX` before narrowing. READ owns canonical
  zero write data; WRITE copies zero, interior, and `UINT16_MAX` values.
- Validation: status is exactly `OK`, `INVALID_SPACE`, or `INVALID_ADDRESS`; invalid
  space wins before address validation. Failed construction zeroes every named
  request field. The borrowed validator rejects null, zero/forged discriminants,
  wide DATA/IO carriers, and noncanonical reads, and named-field snapshots prove it
  mutates neither valid nor invalid input. No padding-byte claim is made.
- Ownership: implementation and test live under mirrored `bus/request/` leaf
  subtrees. Parent bus manifests aggregate the leaf; root Make/CMake and the CMake
  test root each add only the required bus aggregation line. No callback, phase,
  width enum, read result, lifecycle state, or unrelated bus policy entered W21A.
- Independent Luna review found no blocker or major behavioral, C11, range,
  precedence, transactionality, ownership, isolation, manifest, or future-W21C
  issue. Its one minor nonmutation-proof finding was closed with named-field
  snapshots before the implementation gate.
- Size/shape: implementation is 102 lines, private interface 77 lines, and focused
  proof 247 lines. No file approaches the 500-line review threshold.
- Registry final: 2 targets, 7 sources, 76 immutable evidence records, 111
  requirements (88 `IMPLEMENTED`, 8 `PLANNED`, 15 `UNRESOLVED`), and 78 tests
  (70 `PASSING`, 8 `PLANNED`). `T-PROJECT-BUS-WORD16-REQUEST-SHAPE` runs as
  `ti_c54x.bus_word16_request` in both supported build systems.
- Validation: focused normal and ASan/UBSan tests passed. The frozen full
  `make -B test` gate passed 163 Python quality tests, 65 C unit targets, and 66
  direct executions including the minimal consumer. Source boundaries (142 files),
  manifests, evidence, knobs, observer-disabled artifact, generated lookup
  staleness, and `git diff --check` passed. Local CMake remains unavailable because
  the installed executable is permission-denied; the remote matrix owns that proof.
- Isolation: no public ABI, CPU/state/sequencer/ISA behavior, callback invocation,
  read completion, byte-order rule, host memory, raw storage pointer, phase, wait,
  issue/completion state, timing, arbitration, map, opcode effect, observer,
  diagnostic, knob, environment read, allocator, mutable global, firmware, emulator
  dependency, or external implementation was introduced.
- Follow-up: publish and remotely verify the implementation and completion record,
  then begin P2-W21B as the separate explicit WORD16 byte-order codec owner.

## 2026-09-06 — P2-W21A remote verification

- Published implementation commit:
  `2f23aeea02042f3419623a585066c192c2f2a402`.
- Published completion-record commit:
  `73fce87b7d2e74a5e3dbfef405759da476710c49`.
- GitHub Actions run
  [34029176546](https://github.com/djr-747/ti-c54x/actions/runs/34029176546)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream package-consumer
  smoke tests, and Clang sanitizer configure/build/test.
- Result: P2-W21A is published and green on every repository-owned build surface.
  The private WORD16 request remains unused by CPU execution and the public
  synchronous bus ABI is unchanged. P2-W21B may now begin under the separate
  `bus/byte_order/` owner.

## 2026-09-06 — P2-W21B kickoff — explicit WORD16 byte order

- State: active documentation/project-contract kickoff. No production source,
  public API, request type, callback ABI, generated artifact, build manifest, or
  executable test changed in this commit.
- Ownership: create `src/bus/byte_order/word16_codec.[ch]` and the mirrored
  `tests/unit/bus/byte_order/test_word16_codec.c`. Child manifests remain with the
  new owner; existing parent bus manifests aggregate it, so no root manifest change
  is required.
- Codec decision: every call names zero-sentinel `INVALID`, `BIG`, or `LITTLE` byte
  order. BIG maps `0x1234` to `0x12,0x34`; LITTLE maps it to `0x34,0x12`. Encode
  returns two value-owned lanes and decode returns one copied word.
- Status/ownership: results report only `OK`, `INVALID_ARGUMENT`, or
  `INVALID_ORDER`. Decode borrows exactly two readable bytes until return and checks
  null before order validity; encode accepts only scalars. Failed results zero every
  named payload field without a padding-byte claim, and no pointer is retained.
- Host-neutrality: implementation is limited to fixed-width shifts, masks, and
  explicit scalar conversions. Pointer/type-punning casts, unions, packed structs,
  native-layout `memcpy`, host-order inference, and a global current-order value are
  outside the contract.
- Policy boundary: this codec does not state the byte order of the C54x, a Nokia
  device, program images, physical shared RAM, or snapshots/wire formats. Those are
  separate adapter/profile policies that must select BIG or LITTLE explicitly.
- TDD: exhaustively round-trip all 65,536 words in both orders; assert exact lane
  vectors for zero, `0x1234`, and `0xffff`; cover zero/forged orders, null decode
  with valid and invalid order, error payload zeroing, and borrowed-input
  nonmutation. No TI conformance test is claimed.
- Registry kickoff: 2 targets, 7 sources, 77 immutable evidence records, 112
  requirements (88 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`), and 79 tests
  (70 `PASSING`, 9 `PLANNED`). The new row is `PROJECT_CONTRACT_ONLY`.
- Knobs/isolation: no `TEST`, `OBSERVE`, or `A/B` knob, environment read, observer,
  diagnostic, allocator, mutable global, callback, host memory, transaction, wait,
  CPU/state/ISA behavior, memory effect, firmware, or emulator dependency is added.
- Follow-up: validate and publish the kickoff, then delegate the bounded W21B TDD
  implementation under main-agent review and commit ownership. P2-W21C remains the
  separate exactly-once resumable transaction owner.

## 2026-09-06 — P2-W21B kickoff remote verification

- Published kickoff commit:
  `e9c081482d35f176970d8089cbdc2cc0865ebc05`
  (`P2-W21B: define WORD16 byte-order contract`).
- Before publication, `make -B test` passed 163 Python quality tests, 65 C unit
  targets, and 66 direct executions including the minimal consumer. Source
  boundaries (142 files), manifests, evidence, knobs, the observer-disabled
  artifact, generated lookup staleness, and `git diff --check` passed.
- Independent Luna review found no blocker, major, or minor issue in registry
  parity, classification, ownership, isolation, host-endian neutrality,
  BIG/LITTLE semantics, validation precedence, exhaustive TDD scope, or the
  W21A/W21C boundaries.
- GitHub Actions run
  [34029694201](https://github.com/djr-747/ti-c54x/actions/runs/34029694201)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream
  package-consumer smoke tests, and Clang sanitizer configure/build/test.
- Result: the W21B project contract is published and green on every
  repository-owned build surface. Its bounded TDD implementation may begin under
  the private `bus/byte_order/` owner.

## 2026-09-06 — P2-W21B completion — explicit WORD16 byte order

- State: implemented and green. Implementation commit:
  `1ce1ba17aa67b2400c3cf59372737ac2127493a1`
  (`P2-W21B: add WORD16 byte-order codec`).
- TDD RED: the owner-mirrored test and manifests were registered first;
  `rtk make -B build-fast/test_bus_word16_codec` failed because the required
  `build-fast/obj/bus/byte_order/word16_codec.o` did not exist. Adding the private
  codec made the same focused target and direct executable pass.
- Codec result: zero-sentinel `INVALID`, `BIG`, and `LITTLE` orders select conversion
  per call. Encode owns exactly two byte lanes; decode borrows exactly two readable
  lanes until return and owns its copied WORD16 result. BIG maps `0x1234` to
  `0x12,0x34`, while LITTLE maps it to `0x34,0x12`.
- Validation: statuses are exactly `OK`, `INVALID_ARGUMENT`, and `INVALID_ORDER`.
  Decode checks null before order, zero and forged orders are rejected, and each
  failure zeroes every named payload field without making a padding-byte claim.
- Host neutrality: production conversion uses fixed-width unsigned scalar shifts
  and conversions only. No pointer cast, type punning, union, packed layout,
  `memcpy`, native-order inference, or current-order global was introduced.
- TDD GREEN: the focused proof exhausts all 65,536 WORD16 values under both orders.
  Independent external encode/decode vectors prevent coupled conversion bugs from
  cancelling; endpoints, error payloads, validation precedence, and borrowed-input
  nonmutation through BIG and LITTLE are also covered.
- Independent Luna review found no blocker or major production, C11, ownership,
  isolation, manifest, or host-order issue. Its one minor independent-decode-vector
  gap was closed before the implementation gate.
- Size/shape: implementation is 93 lines, private interface 58 lines, and focused
  proof 170 lines. Production and test leaves have nested Make/CMake manifests and
  no file approaches the 500-line review threshold.
- Registry final: 2 targets, 7 sources, 77 immutable evidence records, 112
  requirements (89 `IMPLEMENTED`, 8 `PLANNED`, 15 `UNRESOLVED`), and 79 tests
  (71 `PASSING`, 8 `PLANNED`). `T-PROJECT-BUS-WORD16-BYTE-ORDER-CODEC` runs as
  `ti_c54x.bus_word16_codec` in both supported build systems.
- Validation: focused normal and ASan/UBSan tests passed. The full `make -B test`
  gate passed 163 Python quality tests, 66 C unit targets, and 67 direct executions
  including the minimal consumer. Source boundaries (145 files), manifests,
  evidence, knobs, observer-disabled artifact, generated lookup staleness, and
  `git diff --check` passed. Local CMake remains unavailable because the installed
  executable is permission-denied; the remote matrix owns that proof.
- Isolation: no public API, request change, callback, host memory, transaction,
  phase, wait, arbitration, memory effect, CPU/state/ISA behavior, observer,
  diagnostic, knob, environment read, allocator, mutable global, firmware,
  emulator dependency, or TI/device byte-order claim was added.
- Follow-up: publish and remotely verify the implementation and completion record,
  then begin P2-W21C under the separate exactly-once resumable transaction owner.

## 2026-09-06 — P2-W21B remote verification

- Published implementation commit:
  `1ce1ba17aa67b2400c3cf59372737ac2127493a1`.
- Published completion-record commit:
  `407eb42a20bf33f2b859d09fa416820257323e65`.
- GitHub Actions run
  [34030415491](https://github.com/djr-747/ti-c54x/actions/runs/34030415491)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream
  package-consumer smoke tests, and Clang sanitizer configure/build/test.
- Result: P2-W21B is published and green on every repository-owned build surface.
  The private codec remains outside the public ABI and CPU execution path, and no
  physical byte order is implied. P2-W21C may now begin under the separate
  resumable exactly-once transaction owner.

## 2026-09-06 — P2-W21C kickoff — resumable WORD16 transaction protocol

- State: active documentation/project-contract kickoff. No production source,
  public ABI, CPU path, request type, codec, test, or build manifest changed.
- Ownership: the planned private owner is `src/bus/transaction/`, mirrored by
  `tests/unit/bus/transaction/`; planned implementation leaves are
  `word16_transaction.[ch]` and `word16_transaction_validate.[ch]`. Focused tests
  are `test_word16_transaction.c` for lifecycle behavior and
  `test_word16_transaction_contract.c` for adversarial validation. Parent bus and
  test manifests aggregate the child only in the later implementation piece.
- Correction recorded: the earlier plan phrase “named sequencer phase” is
  superseded. W21C has lifecycle-only states `IDLE=0`, `ISSUE_READY`,
  `IN_FLIGHT`, `COMPLETION_READY`, `COMPLETE`, and `FAULT`; it adds no
  `PRE_ACCESS`, `ACCESS`, `POST_ACCESS`, or other semantic phase field. Address
  PRE/POST belongs to address effects, while future `src/sequencer/memory/` owns
  instruction/memory-effect phase binding.
- Contract: caller-declared canonical `{0}` storage is the pending token and
  copies a fresh caller-generated nonzero `uint64_t transaction_id`, valid W21A
  request, staged read word, and staged `ti_c54x_bus_status`. The caller maintains
  one authoritative storage instance and must not copy it by value while active. A
  fresh ID is required while stale observations could exist; ID generation/wrap is
  external, and an ID cannot detect a copied state with the same ID or deliberate
  ID reuse.
  `prepare` copies a borrowed request only from IDLE; `begin` returns copied
  request/id and authorizes exactly one issue; repeated WAIT polling remains
  IN_FLIGHT; first READY/FAULT latches COMPLETION_READY; `complete` publishes one
  result; and reset only clears IDLE/COMPLETE/FAULT, never active work. There is
  no abort operation.
- Status and payload policy: exactly `OK`, `WAIT`, `FAULT`, `INVALID_ARGUMENT`,
  `INVALID_STATE`, and `STALE_TRANSACTION`. Null pointer, canonical state/legal
  lifecycle, zero ID, stale nonzero ID, then observation payload validation is the
  precedence. WAIT/READY observations carry `TI_C54X_BUS_OK`; FAULT accepts only
  known non-OK `UNMAPPED`, `FAULT`, or `NO_CALLBACK`. WAIT/FAULT and READY-write
  read payloads are zero; READY-read accepts any WORD16. Failed outputs and
  rejected transitions preserve named fields; no padding-byte claim is made.
- Exactly-once scope: this is protocol-only under caller ownership and unique-ID
  preconditions. A separate planned `src/bus/adapter/` requirement/test owns real
  callback/MMIO/RAM issue and completion counts and physical visibility across
  wait-state suspension. Deterministic multi-master arbitration remains a separate
  later proof. No fake adapter counter is included in W21C.
- TDD plan: lifecycle coverage includes all six space/access combinations, copied
  request/source mutation, begin once, repeated WAIT, READY read vectors, READY
  write zeroing, all public fault statuses, completion once, reset/reuse with a
  fresh ID, and stale old observations. Adversarial validation covers forged
  states/requests, nulls, invalid observation tags/status/payloads, active reset,
  precedence, named-field nonmutation, and zero failed issue/completion payloads.
- Registry kickoff: 2 targets, 7 sources, 78 immutable evidence records, 115
  requirements (89 `IMPLEMENTED`, 11 `PLANNED`, 15 `UNRESOLVED`), and 82 tests
  (71 `PASSING`, 11 `PLANNED`). Added evidence
  `E-PROJECT-P2-W21C-BUS-WORD16-TRANSACTION-CONTRACT`, requirements
  `REQ-PROJECT-BUS-WORD16-TRANSACTION-LIFECYCLE`,
  `REQ-PROJECT-BUS-WORD16-TRANSACTION-VALIDATION`, and
  `REQ-PROJECT-BUS-WORD16-EXTERNAL-EXACTLY-ONCE`, plus their owner-mirrored
  planned tests. All are `PROJECT_CONTRACT_ONLY`.
- Knobs/isolation: no `TEST`, `OBSERVE`, or `A/B` knob, environment read,
  observer, diagnostic, allocator, mutable global, host-memory pointer, public
  callback, CPU/state/ISA behavior, timing, device, firmware, or emulator
  dependency is introduced.
- Validation for this docs-only kickoff: run `tools/quality/check_evidence.py`
  and `git diff --check` only. The implementation work owns focused normal and
  sanitizer tests and the full standalone gate before its commit.

## 2026-09-06 — P2-W21C kickoff remote verification

- Published kickoff commit:
  `9d04a1dc6035bc0fa512679bf46c21f2e4634e59`.
- GitHub Actions run
  [34031597203](https://github.com/djr-747/ti-c54x/actions/runs/34031597203)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream
  package-consumer smoke tests, and Clang sanitizer configure/build/test.
- Result: the P2-W21C lifecycle-only transaction contract is published and green
  on every repository-owned build surface. Implementation may proceed within the
  private `src/bus/transaction/` owner; external issue/visibility proof and
  deterministic arbitration remain separately owned future work.

## 2026-09-06 — P2-W21C completion — resumable WORD16 transaction protocol

- Implementation commit: `a3edba970cb2143bfb9d3eb437b9882084352a0d`.
- State: P2-W21C is implemented and green. The private owner is
  `src/bus/transaction/`, mirrored by `tests/unit/bus/transaction/`; production
  leaves are `word16_transaction.[ch]` and `word16_transaction_validate.[ch]`.
  Lifecycle and adversarial validation remain separate owners and tests.
- TDD: the focused RED run preceded the missing production leaves; GREEN then
  passed the lifecycle and adversarial executables. Independent review found no
  major defect and the follow-up closed duplicate-terminal, stale-precedence,
  forged-shape, borrowed-observation, reset, and zero-result proof gaps.
- Contract result: the callback-free, allocation-free reducer proves lifecycle-only
  IDLE/ISSUE_READY/IN_FLIGHT/COMPLETION_READY/COMPLETE/FAULT states, one issue
  authorization, resumable WAIT, one terminal observation, canonical READY/FAULT
  completion, stale IDs, and no active-work cancellation. Exactly-once remains a
  protocol claim under caller ownership and unique-ID preconditions; this record
  does not claim actual callback/MMIO/RAM counts, visibility, arbitration, timing,
  memory effects, CPU/ISA behavior, or public ABI changes.
- Registry result: 2 targets, 7 sources, 78 immutable evidence records, 115
  requirements (91 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`), and 82 tests
  (73 `PASSING`, 9 `PLANNED`). The two implemented tests run as
  `ti_c54x.bus_word16_transaction` and
  `ti_c54x.bus_word16_transaction_contract`; the external adapter
  exactly-once requirement/test remains `PLANNED` with a null runner.
- Full gate: 163 Python quality tests, 68 C unit targets, and 69 direct
  executions including the minimal consumer passed; source boundaries (151
  files), manifests, evidence, knobs, observer-disabled, and generated checks
  were green. Focused normal and ASan/UBSan tests passed. The local CMake
  executable remains permission-denied; remote CMake owns that proof.
- Size/shape review: production files are 227/113 lines, headers 115/40,
  lifecycle test 453, and adversarial test 499. Tests remain separate by
  lifecycle versus adversarial ownership; future growth must split rather than
  exceed the review threshold.
- Knobs/integration: no `TEST`, `OBSERVE`, or `A/B` knob and no integration
  behavior was added. Follow-up publish/remote verification precedes the next ISA
  slice; external adapter/visibility work stays deferred until a concrete memory
  opcode requires it.
- Direction: speculative foundation expansion is now frozen. Work shifts to
  end-to-end ISA vertical slices; new infrastructure is added only when a
  concrete opcode family proves it is required, with related opcodes batched
  where evidence and ownership permit.

## 2026-09-06 — P2-W21C remote verification

- Published implementation commit:
  `a3edba970cb2143bfb9d3eb437b9882084352a0d`.
- Published completion-record commit:
  `ff43359e3eaa933979842f693a71d659223abdbb`.
- GitHub Actions run
  [34033399541](https://github.com/djr-747/ti-c54x/actions/runs/34033399541)
  completed successfully.
- The remote matrix passed GCC and Clang direct-Make builds/tests, library-only
  configuration/build, full CMake/CTest, installation, downstream
  package-consumer smoke tests, and Clang sanitizer configure/build/test.
- Result: P2-W21C is published and green on every repository-owned build surface.
  The standalone foundation baseline is now frozen against speculative expansion;
  the next active work is an end-to-end ISA vertical slice.

## 2026-09-06 — P2-W22 active — immediate status-field LD vertical slice

- State: active. P2-W22 is one atomic end-to-end ISA vertical slice, not a new
  foundation phase. It batches three one-word immediate forms: `LD #k9,DP`
  (`0xEA00/0xFE00`, unsigned 9-bit, `DP := k9`, repeatability `INELIGIBLE`),
  `LD #k5,ASM` (`0xED00/0xFFE0`, raw 5-bit field representing `-16..15`,
  `ASM := raw field`, repeatability `UNRESOLVED`), and `LD #k3,ARP`
  (`0xF4A0/0xFFF8`, 3-bit, `ARP := k3`, repeatability `INELIGIBLE`). All have
  one-word width and no other status-bit side effects.
- Evidence: TI SPRU172C Table 2-19 printed pages 2-14–2-15 and LD §4.70
  printed pages 4-70–4-71 pin forms/effects; §2.5/Table 2-28 printed pages
  2-19–2-21 pins DP/ARP nonrepeatability while ASM remains unresolved. Summary
  cycle columns are not adopted as timing evidence. GNU binutils tag
  `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`, `opcodes/tic54x-opc.c` lines
  316–319, is nonnormative corroboration only; no code, table, comment, or
  implementation is copied or vendored, and mismatches escalate to TI.
- Ownership: `src/isa/decode/` owns rows/fields, `src/isa/ops/load/` owns the
  semantic handlers, `src/state/status/` owns only the status-register subowner
  required by these forms, `src/sequencer/status/` owns transactional effect
  application, and `src/cpu/` owns state observation and atomic retirement.
  Planned tests mirror these owners at
  `tests/unit/isa/decode/test_ld_status_fields.c`,
  `tests/unit/isa/ops/load/test_ld_status_fields.c`,
  `tests/unit/state/status/test_status_registers.c`,
  `tests/unit/sequencer/status/test_status_field_effect_apply.c`, and
  `tests/unit/cpu/test_cpu_ld_status_fields.c`; all runners are null until code
  exists.
- Scope: no memory/bus transaction, timing, RPT/IRQ, accumulator, device, or
  emulator behavior is added. Status-bank splitting and generic effect commit
  exist only because these opcodes require them. No knobs are introduced.
- Registry plan: 2 targets, 7 sources, 82 immutable evidence records, 120
  requirements (91 `IMPLEMENTED`, 14 `PLANNED`, 15 `UNRESOLVED`), and 87 tests
  (73 `PASSING`, 14 `PLANNED`).

## 2026-09-06 — P2-W22 completion and remote verification

- Published implementation commit: `e89207eaff944b410f5f9c6245dc497cfdc942ce`.
  W22 is complete with four production ISA instructions: `NOP` plus `LD #k9,DP`,
  `LD #k5,ASM`, and `LD #k3,ARP`. Exhaustive operand vectors cover 512, 32, and
  8 values respectively.
- The generated direct first-word lookup contains 553 populated initializers in
  a fixed 65,536-entry `uint16_t` read-only table (128 KiB). It remains a
  replaceable baseline pending real-trace benchmarking; no performance claim is
  made from table size alone.
- Corrected ownership is explicit: `src/isa/catalog/` owns rows,
  `src/isa/decode/` owns extraction, `src/isa/ops/load/` owns semantic handlers,
  `src/state/status/` owns the
  status-register contract, `src/sequencer/status/` owns transactional effect
  application, and `src/cpu/` owns observation and atomic retirement. The
  earlier active-entry shorthand is superseded by this split. Independent review
  corrected the public observation wording (`ti_c54x_state` grows before API
  stability but is not a raw snapshot or persistence ABI), PC `0xffff` boundary
  rollback, split registry ownership, catalog-derived handler metadata, and a
  future-proof NOP uniqueness test.
- Local final `rtk make -B test` was green after the first run legitimately caught
  and fixed a stale NOP-only assumption: 163 Python quality tests, 73 C unit
  executables plus the minimal consumer, source boundaries (162 files), and
  manifests, knobs, generated artifacts, and observer-off checks all passed.
  Registry state is 2 targets, 7 sources, 82 evidence records, 122 requirements
  (98 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`), and 87 tests (78 `PASSING`,
  9 `PLANNED`).
- Remote CI run
  [34036019012](https://github.com/djr-747/ti-c54x/actions/runs/34036019012)
  passed GCC and Clang direct Make, CMake library-only/full builds, tests,
  installation, consumer smoke tests, and Clang ASan/UBSan.
- No knob, environment read, emulator, cycle/timing, memory, RPT/IRQ,
  accumulator, or device behavior was added. Foundation freeze remains in force;
  the next work is concrete ISA vertical batches.

## 2026-09-06 — hosted performance history completion and baseline

- Harness implementation commit:
  `c47a4dc9cacd6faa9afe5f07fe6e60676be72cb5`.
  GitHub Actions run
  [34038916643](https://github.com/djr-747/ti-c54x/actions/runs/34038916643)
  completed successfully.
- Stabilization commit:
  `aa0afc0c97e121724af0d895f4d4bf617f0e5610`. Full local `make -B test` was
  green: 172 Python checks, 73 C unit executables plus the minimal consumer,
  and knobs, boundaries, manifests, evidence, observer-disabled, and generated
  checks. GitHub Actions run
  [34039488241](https://github.com/djr-747/ti-c54x/actions/runs/34039488241)
  completed successfully.
- Initial baseline/history and documentation commit:
  `33a1d2553a5a5aad44006cc567f906d440c1b139`. Local `make test` was green
  after restoring the immutable evidence anchor. GitHub Actions run
  [34039763667](https://github.com/djr-747/ti-c54x/actions/runs/34039763667)
  completed successfully.
- Decision: hosted POSIX `CLOCK_MONOTONIC` observer-off end-to-end public-bus
  callback throughput only; no silicon timing claim. Each result uses the
  median of seven recorded samples, with every sample preceded by 16
  unrecorded warmup windows. Comparisons are advisory and use baseline/current
  `(max-min)/median` spread; no CI threshold is imposed. Fixed semantic-hash
  scenarios remain comparable across catalog growth, while `mixed-current`
  evolves with catalog count and hash.
- The first uncommitted baseline was discarded: same-binary reruns varied by
  7–23%. `CLOCK_THREAD_CPUTIME_ID` and best-of-seven were tested and rejected
  because they did not improve repeatability uniformly.
- Exact clean baseline from `aa0afc0`: `nop` 64.023 ns/instruction,
  15.619508 MIPS, 13.30% spread; `ld-dp` 93.924 ns, 10.646890 MIPS,
  2.08% spread; `ld-asm` 94.222 ns, 10.613212 MIPS, 8.60% spread;
  `ld-arp` 94.939 ns, 10.533079 MIPS, 5.08% spread; `mixed-current` 87.770
  ns, 11.393391 MIPS, 7.86% spread. Catalog=4, GCC `cc` 15.2, i7-1255U.
- `TI_C54X_BUILD_BENCHMARKS` remains TEST/KEEP, default OFF, owned by `bench/`,
  with no architectural effect. Before the first multiword catalog
  instruction, derive the `mixed-current` program-word span and retirement
  window under the unresolved 16-bit PC boundary.

## 2026-09-06 — P2-W23 MAX/MIN accumulator vertical slice completion and performance closure

- State: complete. The implementation landed as
  `a14d331a361eaad8881942b030f5b3bea8e69160`; its GitHub Actions run
  [34045099893](https://github.com/djr-747/ti-c54x/actions/runs/34045099893)
  is green. The performance/documentation follow-up landed as
  `ed1be7f4f5dce61a052494b2ecf7eab538ccfe45`; its GitHub Actions run
  [34045384864](https://github.com/djr-747/ti-c54x/actions/runs/34045384864)
  is green.
- TDD evidence is RED/GREEN: the planned focused tests were wired before
  production code, captured the expected failures, then passed after the
  vertical slice was implemented. Full local `make test` is green (176 Python
  tests plus all C and quality checks). The registry closes at 87 evidence
  items, 128 requirements (103 implemented, 9 planned, 16 unresolved), and
  92 tests (83 passing, 9 planned).
- MAX_ACC (`0xF486`/`0xF586`) and MIN_ACC (`0xF487`/`0xF587`) admit four
  concrete one-word encodings through two masked catalog rows, with the generic
  MAX/MIN layout owning D at word-0 bit 8.
  Their handlers perform signed full-40-bit A/B compare/select (ties select
  A), return a value-owned accumulator effect and C value, and leave
  the non-destination accumulator untouched. Catalog rows own declaration and
  input resources; decode owns generic metadata/layout extraction; compare
  owns pure semantics; accumulator state owns canonical raw-40 values and
  knownness; the accumulator sequencer validates and commits effects
  atomically; CPU owns lifecycle and PC-boundary retirement. Mirrored tests
  follow that ownership tree. Direct handlers share one input/effect contract,
  preserve output on malformed input, and do not carry source snapshots in
  effects.
- Catalog input-resource metadata is explicit: NOP and LD rows declare
  `NONE`, MAX/MIN rows declare `ACCUMULATORS`. The typed mask is copied into
  decoded metadata and validated generically; CPU reads an accumulator snapshot
  only when the mask requests it, and candidate accumulator copy/commit is
  confined to the generic accumulator-effect path. This removes unconditional
  accumulator work from NOP/LD hot paths and keeps the core independent of any
  emulator or firmware image.
- Performance closure is hosted POSIX observer-off measurement only (no silicon
  timing claim). Clean `a14d331` records NOP at 65.538 ns / 15.258 MIPS and LD
  at 97.485–97.982 ns / 10.206–10.258 MIPS, a fixed +2.37–3.79% delta versus
  `aa0afc0`; new spreads are 2.19–8.76%. The reset-runnable `mixed-current`
  sample is 89.986 ns / 11.113 MIPS over a 4/6 runnable subset and is not
  compared across the catalog change. The initial implementation regressed
  NOP/LD by 13–25% because it unconditionally copied accumulator state; the
  generic resource-mask correction removed that work. Two post-fix samples
  remained noisy (one within −3.7%..+3.4%, the other +11.7%..+13.7%), so the
  clean cohort is reported without claiming universal recovery.
- Fanout policy is explicit: use 2–3 family-local implementation lanes, then
  serialize shared integration; an architecture GAP stops and reports rather
  than widening ownership. IRQ, RPT, memory, and timing semantics remain
  serialized. The CMPL scout remains a GAP until a generic accumulator-effect
  C disposition (`PRESERVE`/`WRITE`) is specified; it must not introduce a CPU
  opcode-identity branch.
- `cpu.c` is at the 443-line review boundary. The next CPU growth must extract
  generic effect routing/retirement transaction logic rather than adding family
  branches.

## 2026-09-07 — P2-W24 CMPL accumulator vertical slice completion and performance closure

- State: complete. The evidence and architecture plan freeze landed as
  `f53cf8496ef842e0b288932bb5742f648a903bed`; its GitHub Actions run
  [34046263884](https://github.com/djr-747/ti-c54x/actions/runs/34046263884)
  is green. The implementation landed as
  `7cad341a7373a2a5737611848b985ed67c70dbcf`; its GitHub Actions run
  [34047741091](https://github.com/djr-747/ti-c54x/actions/runs/34047741091)
  is green. The performance/documentation follow-up landed as
  `7f3dcc66b00091578a37db2c2abf9ac50a973c41`; its GitHub Actions run
  [34048073006](https://github.com/djr-747/ti-c54x/actions/runs/34048073006)
  is green.
- TDD evidence is RED/GREEN: the accumulator-effect test first failed on
  missing `ACCUMULATOR_WRITE`/C-action symbols, and the CMPL decode test first
  failed on the missing CMPL opcode ID/layout. Focused decode, ops, sequencer,
  CPU, and MAX/MIN regression tests then passed. Full local `make test` is
  green (176 Python checks, source-boundary checks over 179 files, and all C,
  quality, generated, and manifest checks). The registry is at 91 evidence
  records, 133 requirements (107 implemented, 9 planned, 17 unresolved), and
  96 tests (87 passing, 9 planned). Remote GCC/Clang, sanitizer, install,
  consumer, and bench-smoke checks are green.
- The reusable generic effect is `ACCUMULATOR_WRITE` with an explicit C
  disposition (`PRESERVE` or `WRITE`); MAX/MIN use `WRITE`, while CMPL uses
  `PRESERVE`. CMPL belongs to `src/isa/ops/accumulator/`, uses the generic S/D
  layout, and admits exactly four one-word encodings (`F493`, `F593`, `F693`,
  `F793`). Its semantics are a full 40-bit ones complement, require knownness
  only for the selected source, and preserve all status bits byte-for-byte.
  There is no opcode-specific CPU branch; `cpu.c` remains 443 lines. CMPL
  repeatability and machine timing remain unresolved, and no memory, emulator,
  RPT, or IRQ claim is made.
- Performance closure is hosted POSIX observer-off measurement only (no silicon
  timing claim). Clean `7cad341` records NOP at 69.057 ns / 14.481 MIPS,
  LD-DP at 99.866 ns / 10.013 MIPS, LD-ASM at 100.477 ns / 9.953 MIPS, and
  LD-ARP at 96.845 ns / 10.326 MIPS. The reset-runnable `mixed-current`
  sample is 89.591 ns / 11.162 MIPS over a 4/7 runnable subset. Fixed deltas
  versus `a14d331` are +5.37%, +2.44%, +2.89%, and -1.16%, with spreads of
  7.64%, 63.19%, 9.49%, and 11.52%; this is a hosted-noise trend with no
  confirmed regression. No knob changes were made.
- Follow-up: perform a strict evidence-and-architecture FIT/GAP review before
  the next small ISA fanout. Shared catalog, decode, state, sequencer, and CPU
  surfaces remain serialized; family-local lanes may proceed only where the
  contract is already proven.

## 2026-09-07 — P2-W25 ROL/ROR rotate-through-C vertical slice closeout

- State: complete. The P2-W25 plan/evidence freeze landed as
  `c21038e4464820117e66c25c7e3ef1d037d432d9`; its GitHub Actions run
  [34049479239](https://github.com/djr-747/ti-c54x/actions/runs/34049479239)
  is green. The implementation landed as
  `705ac7b625a4ab7e4033c3133efbbd8418b80bfa`; its GitHub Actions run
  [34050726672](https://github.com/djr-747/ti-c54x/actions/runs/34050726672)
  is green. The performance/documentation follow-up landed as
  `f5a9d2b3a9e09de8772b43a67a4afe04de666273`; its GitHub Actions run
  [34050999306](https://github.com/djr-747/ti-c54x/actions/runs/34050999306)
  is green.
- TDD evidence is RED/GREEN: collector, catalog metadata, decode, rotate-op,
  and CPU tests first exercised absent collectors/resources, missing ROL/ROR
  rows/layouts, and unintegrated retirement behavior; the final local
  `make test` is green with 176 Python checks, all C and quality checks,
  source-boundary checks over 188 files, and manifests, knobs, evidence,
  generated, and observer-off checks green. The registry closes at 95 evidence
  records, 139 requirements (112 implemented, 9 planned, 18 unresolved), and
  101 tests (92 passing, 9 planned).
- Architecture: the private `src/cpu/input/` collector owns one generic,
  transactional resource-driven snapshot path. `STATUS_C` is a typed resource
  with named `status_c`/`status_c_known` fields, A/B knownness is preserved as
  data, unrequested inputs are canonical zero/false, and invalid masks or
  backing pointers reject without output mutation. CPU makes one generic
  collector call with no opcode branch; `cpu.c` shrank from 443 to 428 lines.
  Ownership-mirrored child manifests are in place. `ACCUMULATOR_SOURCE` owns
  S at word-0 bit 8, pure rotate semantics live under
  `src/isa/ops/rotate/`, and ROL/ROR use the existing
  `ACCUMULATOR_WRITE` + `C_WRITE` atomic effect path. Four encodings implement
  low-32 rotate-through-C, clear guard bits, preserve unrelated state, and
  enforce selected-source/pre-C unknown rollback, allowed unselected unknowns,
  PC-boundary rollback, and one-attempt/one-retirement/PC+1 behavior.
- No knobs changed: eight remain registered, including three observer bits.
  No public API, emulator, memory, RPT/IRQ, or machine-timing claim was added.
  ROL/ROR repeatability remains unresolved, stateful operations remain outside
  the reset-runnable benchmark subset, and the strict FIT/GAP fanout rule is
  retained: shared architecture gaps are reported back before family lanes
  continue.
- Performance closure is hosted POSIX observer-off throughput only (no silicon
  timing claim). Clean `705ac7b` records NOP at 61.839 ns / 16.171 MIPS,
  LD-DP at 91.592 ns / 10.918 MIPS, LD-ASM at 91.619 ns / 10.915 MIPS,
  LD-ARP at 96.653 ns / 10.346 MIPS, and `mixed-current` at 87.562 ns /
  11.420 MIPS over a 4/9 reset-runnable subset. Fixed deltas and spreads are
  recorded in `docs/performance.md`; the measurements are hosted noise/trend
  observations with no confirmed improvement, regression, or silicon claim.
- Follow-up: resolve ROL/ROR repeatability and timing only with stronger
  evidence; keep stateful operations out of the reset-runnable benchmark until
  their bootstrap contract exists; and preserve the serialized shared-surface
  review plus FIT/GAP reporting policy for the next ISA fanout.

## 2026-09-07 — P2-W26 SFTL shifted-accumulator vertical slice closeout

- State: complete. The P2-W26 plan/evidence freeze landed as
  `1263a4d6bcffcec0fe0a6e76a273929353c33097`; its GitHub Actions run
  [34052234254](https://github.com/djr-747/ti-c54x/actions/runs/34052234254)
  is green. The implementation landed as
  `cd88596ec2a02bb749e09bee698841cb3ce39dd2`; its GitHub Actions run
  [34053101536](https://github.com/djr-747/ti-c54x/actions/runs/34053101536)
  is green. The performance/documentation follow-up landed as
  `979d1d6e3be9c7b0f6861dab40b46807c489e26c`; its GitHub Actions run
  [34053379228](https://github.com/djr-747/ti-c54x/actions/runs/34053379228)
  is green. This ledger entry records the closeout.
- TDD evidence is RED/GREEN: the first red failures were limited to the missing
  SFTL opcode ID, operand layout/header, catalog/schema registration, and
  generated metadata; focused exhaustive tests then passed. Full local `make
  test` passed with 176 Python checks, all C and quality checks, source-boundary
  checks over 194 files, and manifests, evidence, knobs, generated, and
  observer-off checks green. Registry closes at 98 evidence records, 144
  requirements (116 implemented, 9 planned, 19 unresolved), and 105 tests
  (96 passing, 9 planned).
- SFTL registers `SFTL_ACC`/`sftl_acc` at `0xF0E0`/`0xFCE0`, one word, with
  128 encoded first words spanning source, destination, and signed-five-bit
  shift fields. The generic
  `TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout
  extracts source bit 9, destination bit 8, and raw shift bits 4:0; the
  handler sign-extends `-16..15`. A central review correction set the
  one-piece operand `value_lsb` to zero before GREEN.
- Semantics are pure and transactional: pre-C is ignored; q<0 shifts low32
  right and carries source bit n-1, q=0 preserves low32 and clears C, q>0
  shifts low32 left and carries source bit 32-q; guard bits 39:32 clear and
  only C changes. The existing `ACCUMULATORS` input to
  `ACCUMULATOR_WRITE` + `C_WRITE` effect path commits atomically. CPU
  orchestration is unchanged and has no opcode branch; unknown selected or
  noncanonical input rejects transactionally while unselected unknown state is
  allowed. Catalog quality is 10 rows / 693 populated lookup entries, not 693
  instructions, with reset-runnable coverage 4/10.
- No knobs changed (8 remain, including 3 observer bits); no public API,
  memory, RPT/IRQ, or machine-timing behavior was added. SFTL repeatability
  remains unresolved; no DSP timing claim is made.
- Performance closure is hosted POSIX observer-off throughput only. Clean
  `cd88596` records NOP 97.138 ns / 10.295 MIPS, LD-DP 102.519 ns /
  9.754 MIPS, LD-ASM 97.918 ns / 10.213 MIPS, LD-ARP 99.295 ns /
  10.071 MIPS, and `mixed-current` 91.728 ns / 10.902 MIPS over a 4/10
  reset-runnable subset. Fixed deltas versus catalog-nine are +57.08%,
  +11.93%, +6.88%, and +2.73%; within-capture spreads are 36.02%, 10.71%,
  32.95%, and 10.72%. Wide hosted noise/spreads and changed workload mean
  no confirmed improvement or regression; these are not DSP timing claims.
- Follow-up: only proceed with further family fanout after a strict FIT
  review; any architecture GAP returns to the central architecture owner
  before implementation, preserving serialized shared-surface review.

## 2026-09-07 — P2-W27 AND/OR/XOR register-only logical accumulator vertical slice closeout

- State: complete. The P2-W27 plan/evidence freeze landed as
  `9e3e4099f6a748a207821d18d4af37fba9d7196c`; its GitHub Actions run
  [34054743366](https://github.com/djr-747/ti-c54x/actions/runs/34054743366)
  is green. The implementation landed as
  `ef87c944ccf325617487df2946fafbfa0cfe17ba`; its run
  [34055843312](https://github.com/djr-747/ti-c54x/actions/runs/34055843312)
  is green. Performance/documentation landed as
  `dac89d503c201790598e8f7fe44502529558e095`; its run
  [34056286318](https://github.com/djr-747/ti-c54x/actions/runs/34056286318)
  is green.
- TDD fanout used explicit non-overlapping ops, catalog/decode, and CPU-proof
  lanes; RED failures were limited to missing headers, handlers, IDs, rows,
  and unsupported CPU words. Review added noncanonical-destination coverage,
  removed a tautological reset-runnable assertion, made the decode loop 128
  unique words, and corrected an over-strong unknown-carrier test. Full local
  `make test` is green: 176 Python checks, all C/quality checks,
  source-boundary checks over 200 files, and manifests/evidence/knobs/generated/
  observer-off checks. Remote GCC/Clang/sanitizer/install/consumer/bench are
  green.
- AND `F080/FCE0`, OR `F0A0/FCE0`, and XOR `F0C0/FCE0` add 3 rows and 384
  encoded first words; the catalog is 13 rows / 1077 entries / 4 reset-runnable.
  Existing S/D/shift5 layout, `ACCUMULATORS` input, generic
  `ACCUMULATOR_WRITE` with `C_PRESERVE`, and generic CPU retirement are used.
  Full-40 logical shifts combine with the pre-destination; C and all status
  are preserved, and knownness/canonicality plus `PC=ffff` remain transactional.
  `cpu.c` remains 428 lines. No public API/state/effect/memory/RPT/IRQ/timing,
  knob, environment-read, allocation, diagnostic, or emulator dependency changed.
- Registry closes at 105 evidence records, 149 requirements (120 implemented,
  9 planned, 20 unresolved), and 109 tests (100 passing, 9 planned).
  Repeatability and machine timing remain unresolved.
- Performance closure is hosted POSIX observer-off throughput only (no silicon
  timing claim). Clean `ef87c94` records NOP 121.420 ns / 8.236 MIPS,
  LD-DP 209.005 ns / 4.785 MIPS, LD-ASM 202.901 ns / 4.929 MIPS,
  LD-ARP 113.815 ns / 8.786 MIPS, and `mixed-current` 96.359 ns / 10.378
  MIPS over a 4/13 reset-runnable subset. Fixed deltas versus catalog-ten are
  +25.00%, +103.87%, +107.22%, and +14.62%, with spreads 51.13%, 10.86%,
  5.71%, and 117.11%. An immediate same-binary rerun normalized to 66.571,
  98.326, 98.038, 96.481, and 86.448 ns, corroborating host noise only; no
  confirmed regression or DSP timing claim is made.
- The FIT fanout policy proved useful: strict-fit families may parallelize,
  shared integration remains serialized, and any missing architecture seam is
  a hard GAP report. TC/OVM/SXM work remains a separate follow-up.

## 2026-09-07 — P2-W28A transactional TC status foundation closeout

- State: complete. The P2-W28A plan/evidence freeze landed as `5deed87`.
  The implementation landed as `8f6c3d2`; remote CI was pending at ledger
  entry time.
- TDD evidence is RED/GREEN: architecture tests first failed on the missing
  TC accessors, status resource, and transactional status delta; focused tests
  then passed. Three new tests were added. Full local `make test` is green
  (176 Python checks, all C checks, source-boundary checks over 203 files, and
  manifests, evidence, knobs, and observer-off checks). No `make guard` target
  exists. Independent review found no blocker.
- Architecture: private typed TC accessors and knownness are owned by state;
  `STATUS_TC` is collected through the resource-driven input path; and the
  private accumulator status delta supports `C` and `TC` with `PRESERVE`,
  `WRITE_0`, and `WRITE_1` actions. Existing MAX/MIN, ROL/ROR, SFTL, CMPL,
  and logical accumulator handlers migrated to the shared delta path. The
  foundation remained transactional and kept generic CPU orchestration.
- `cpu.c` remained 428 lines and `effect_apply` is 64 lines. No public
  API, OVA/B surface, memory, RPT/IRQ, timing, diagnostic, or environment
  changes were made. The registry closed at 110 evidence records, 162
  requirements (123 implemented, 17 planned, 22 unresolved), and 120 tests
  (103 passing, 17 planned).
- Follow-up: P2-W28B is next, subject to the same FIT/GAP review and serialized
  shared-surface integration policy.

## 2026-09-07 — P2-W28B ROLTC/SFTC rotate-through-C and conditional-shift vertical slice closeout

- State: complete. The P2-W28 plan/evidence freeze landed as `5deed87`; the
  P2-W28A implementation was `8f6c3d2` and its ledger closeout was `4e73e18`
  (CI run `34058489923` green). P2-W28B implementation landed as
  `7164ace58677dae5525e02c14655bbefc38e87f4`; its [GitHub Actions run
  34059815766](https://github.com/djr-747/ti-c54x/actions/runs/34059815766)
  is green. Performance/documentation landed as
  `70425ccca3977ca355850131c34fe5cf759a8ee8`; its [GitHub Actions run
  34060040089](https://github.com/djr-747/ti-c54x/actions/runs/34060040089)
  is green.
- TDD evidence is RED/GREEN: family RED first exposed missing opcode IDs,
  catalog/decode metadata, headers, handlers, and unsupported CPU retirement;
  GREEN added eight catalog, decode, pure-operation, and CPU tests. Full local
  `make test` is green with 176 Python checks, all C tests, source-boundary
  checks over 215 files, and evidence, build-manifest, knob, and observer-off
  checks green. Full-gate review caught and fixed stale catalog-count coupling
  in the shared Python contract; old logical-family total assertions were
  removed so each family test owns only its rows.
- ROLTC (`F492`/`FEFF`) selects A/B, rotates pre-TC into bit 0, writes old
  low32 bit 31 to C, clears guard bits, and preserves TC. SFTC
  (`F494`/`FEFF`) leaves low32 zero unchanged with TC=1; for nonzero values,
  sign-equal low32 shifts left with guards preserved and TC=0, while
  sign-different values remain unchanged with TC=1. C is preserved and SFTC
  does not read pre-TC. Both use the generic transactional path; `cpu.c`
  remains 428 lines.
- A minimal central tooling GAP was found and closed under RED/GREEN: the
  catalog schema resource allowlist gained `STATUS_TC` and
  `ACCUMULATORS_STATUS_TC`. No runtime architecture GAP was required. The
  catalog now has 15 rows / 1081 populated lookup entries / 4 reset-runnable
  rows. The registry closes at 110 evidence records, 162 requirements
  (131 implemented, 9 planned, 22 unresolved), and 120 tests (111 passing,
  9 planned).
- No knobs changed (8 total, including 3 observer bits); no public API,
  memory, RPT/IRQ, machine timing, diagnostics, environment, allocation, or
  emulator dependency was added. Residuals are repeatability, machine timing,
  and the SFTC guard-only plus low32-zero evidence boundary.
- Performance closure is hosted POSIX observer-off throughput only (no DSP
  timing claim). Clean `7164ace` medians/throughput are 109.060/9.169,
  182.474/5.480, 194.054/5.153, 111.847/8.941, and mixed 88.796/11.262.
  Fixed deltas versus `ef87c94` are -10.18%, -12.69%, -4.36%, and -1.73%,
  with noisy spreads 50.31%, 3.83%, 6.90%, and 112.44%; there is no confirmed
  change and no DSP timing claim.
- Follow-up: continue strict FIT fanout only for families that fit current
  seams; any architecture GAP returns centrally before shared edits.

## 2026-09-07 — P2-W29 EXP accumulator-to-T vertical slice closeout

- State: complete. The plan/evidence freeze landed as
  `463259c9c8602878cf15296043eedf68a3ab64fa`. Implementation landed as
  `fedbd6a342ad19be0e53d90d82ad8fe8e5ac74fc`; performance and documentation
  landed as `27cf0b3`. The implementation CI run
  [34062796872](https://github.com/djr-747/ti-c54x/actions/runs/34062796872)
  is green. The performance/documentation run
  [34063227898](https://github.com/djr-747/ti-c54x/actions/runs/34063227898)
  is green across GCC, Clang, Clang sanitizers, install/consumer, and hosted
  benchmark smoke coverage.
- TDD fanout used seven bounded RED contracts (T state, T effect, effect
  transaction, EXP metadata, EXP decode, EXP handler, and CPU EXP), followed by
  focused GREEN checks and two independent reviews. Full local `make test` was
  green after correcting the initial expected catalog-count mismatch (15/1081
  to 16/1083): 176 Python tests plus all C tests, source-boundary checks over
  230 files, and build-manifest, evidence, knob, and observer-off checks.
  The registry closes at 116 evidence items, 170 requirements (138 implemented,
  9 planned, 23 unresolved), and 127 tests (118 passing, 9 planned).
- Architecture: private `uint16_t` T state resets to physical zero while
  remaining architecturally unknown; the T effect is value-only; and the
  generic tagged candidate-only transaction routes `NONE`, status,
  accumulator, and T-register effects. CPU retirement is
  dispatch -> stage effect -> candidate PC -> commit transaction -> assign PC
  -> retire. EXP is the one-word `0xF48E` / mask `0xFEFF` accumulator-source
  form, uses the existing accumulator-source input, writes encoded values
  `-8..31` to T, and leaves the selected accumulator unchanged.
- No public T/MMR, memory, RPT/IRQ, timing, knob, diagnostic/environment,
  allocation, or emulator-integration surface changed. EXP repeatability remains
  unresolved. No knobs changed. No unplanned architecture GAP was found:
  shared changes were serialized by the orchestrator, and no family CPU
  workaround was introduced.
- Performance closure is hosted POSIX observer-off throughput only, with no DSP
  timing claim. Clean catalog-16 on the `fedbd6a` binary (`def3fe...`) records
  NOP 124.251 ns / 8.048 MIPS, DP 205.652 / 4.863, ASM 209.142 / 4.781,
  ARP 222.837 / 4.488, and mixed 206.376 / 4.846 over 4/16 reset-runnable
  rows. Fixed deltas versus catalog-15 are +13.93%, +12.70%, +7.78%, and
  +99.23%; capture spreads are 22.39%, 0.92%, 4.17%, and 8.71%. An identical
  binary dirty diagnostic measured NOP 68.561 / 14.586, DP 101.587 / 9.844,
  ASM 104.687 / 9.552, ARP 106.211 / 9.415, and mixed 96.438 / 10.369
  (same-binary changes -44.82% to -53.27%), so no confirmed regression is
  claimed.
- Follow-up: select only the next two or three ISA families after strict FIT
  review. Any need for shared architecture must stop the lane and produce a
  GAP report before central integration.

## 2026-09-07 — P2-W30 AND/OR/XOR two-word logical-immediate vertical slice closeout

- State: complete. The plan/evidence freeze landed as `569742a`. The
  implementation landed as `9c22a33`; its GitHub Actions run
  [34067048536](https://github.com/djr-747/ti-c54x/actions/runs/34067048536)
  is green. Performance/documentation landed as `86feb9f`; its run
  [34067341093](https://github.com/djr-747/ti-c54x/actions/runs/34067341093)
  is green. Both runs cover GCC, Clang, direct Make, CMake/CTest, installation,
  downstream consumer, benchmark, and sanitizer jobs.
- TDD evidence is RED/GREEN: eight focused strict tests passed, followed by
  eight ASan/UBSan tests after the final family-alias correction and two
  independent reviews. Full local `make test` is green with 176 Python quality
  unittests, all silent C tests, generator/evidence/source-boundary/build-
  manifest checks, and observer-off gates green. The registry closes at 122
  evidence records, 178 requirements (146 implemented, 9 planned, 23
  unresolved), and 135 tests (126 passing, 9 planned).
- Six width-two AND/OR/XOR immediate rows add 204 populated lookup words,
  bringing the catalog to 22 rows / 1287 populated words / 4 reset-runnable
  rows. Two generic decode layouts cover shifted and fixed-16 forms. A shared
  logical-immediate helper owns validation, extraction, zero-extension, and
  shift semantics; family leaves own operation names and aliases. The generic
  CPU path is unchanged: no `cpu.c` edits, public API, memory, timing,
  diagnostic, environment, or knob changes.
- Review correction: catalog-bound row aliases initially accepted a sibling
  family form. Exact-ID guards and cross-form transactional tests now reject
  that ambiguity without changing the generic dispatch path.
- Performance closure is hosted POSIX observer-off throughput only (no DSP
  timing claim). Clean `9c22a33` values are NOP 131.883 ns, LD-DP 233.294 ns,
  LD-ASM 240.304 ns, LD-ARP 206.770 ns, and mixed 109.273 ns. The identical
  binary normalized to 80.803, 114.568, 127.581, 118.947, and 108.974 ns;
  there is no confirmed regression. The new rows are not exercised by the
  reset-runnable benchmark subset, and the binary hash is identical across
  the comparison captures. No knobs changed.
- Follow-up: evidence-first fixed the next memory-bridge scope around MOD2
  `LD *ARx+,dst` and `STL src,*ARx+`. Record the required canonical
  architecture-state composition, SXM input, WAIT-capable transaction, and
  atomic AR/PC/effect commit. Keep the nonblocking performance note that
  generic operand extraction currently revalidates immutable layouts three to
  four times; measure and address it only with representative memory paths.

## 2026-09-07 — P2-W31A fixed-MOD2 memory prerequisites closeout

- State: complete. The plan/evidence freeze landed as `1e371f9`; its GitHub
  Actions run [34067894773](https://github.com/djr-747/ti-c54x/actions/runs/34067894773)
  is green. The implementation landed as `66ef384`; full standalone local
  `make test` is green and its run
  [34069533137](https://github.com/djr-747/ti-c54x/actions/runs/34069533137)
  is green. The performance record landed as `31d4eb9`; its CI run
  [34070099277](https://github.com/djr-747/ti-c54x/actions/runs/34070099277)
  completed with conclusion `success`.
- TDD evidence is RED/GREEN: the initial contracts failed for the three
  prerequisites, then focused strict and ASan/UBSan tests passed with two
  independent reviews. The first full gate caught five positional snapshot
  initializers in `tests/unit/isa/ops/logical/immediate/and/test_and.c`;
  they were converted to future-proof designated fields before the successful
  rerun. This standalone repository has no `make guard` target, so the
  authoritative local gate is `make test`.
- The exact three implemented requirements and passing tests are
  `REQ-PROJECT-CPU-ARCH-STATE-COMPOSITION` /
  `T-PROJECT-CPU-ARCH-STATE-COMPOSITION`,
  `REQ-PROJECT-SXM-CHECKED-ACCESS` / `T-PROJECT-SXM-FIELD`, and
  `REQ-PROJECT-OP-INPUT-SXM-RESOURCE` / `T-PROJECT-OP-INPUT-SXM`. The registry
  closes at 125 evidence records, 181 requirements (149 implemented, 9
  planned, 23 unresolved), and 138 tests (129 passing, 9 planned).
- Architecture ownership is now explicit: `src/cpu/` embeds the canonical
  `ti_c54x_arch_state` as the sole owner of PC, status, AR0–AR7, SP, DP, CPL,
  and CMPT; its private `pc_word` uses the 32-bit program-word carrier while
  the existing private program-PC bridge retains the common 16-bit validity
  contract.
  `src/state/status/` owns the private checked SXM read and knownness derived
  from ST1, and `src/cpu/input/` generically collects `STATUS_SXM` plus its
  knownness into operation input. Reset seeds common architectural state before
  applying the configured carrier; accumulators, T, counters, bus descriptors,
  and observers remain separate owners.
- No opcode or catalog rows, public memory ABI or effects, WAIT/resume behavior,
  shared-memory visibility, byte order, fault or wrap policy, timing,
  diagnostics, environment reads, or knobs changed. P2-W31A makes no memory
  instruction claim and leaves those seams to later evidence freezes.
- Performance closure is hosted POSIX observer-off throughput only. The clean
  same-catalog-22 `66ef384` record (captured by `31d4eb9`, binary hash prefix
  `b0c7d039`) measured NOP 111.770 ns / 8.947 MIPS, LD-DP 116.084 / 8.614,
  LD-ASM 117.836 / 8.486, LD-ARP 159.492 / 6.270, and `mixed-current`
  170.433 / 5.867. An identical-binary diagnostic measured 76.978, 119.232,
  158.135, 173.612, and 108.285 ns in that scenario order. The wide spreads
  and same-binary variation establish no confirmed regression or improvement;
  these are not DSP or silicon timing claims.
- Follow-up P2-W31B: freeze an additive data endpoint, WAIT/resume semantics,
  commit-at-READY shared visibility, and a persistent root instruction
  transaction; only then freeze and implement fixed-MOD2 `LD *ARx+,dst` /
  `STL src,*ARx+`.

## 2026-09-07 — P2-W31B1 transactional data-memory foundation closeout

- State: complete. The plan/evidence freeze landed as `7fbd6ec`; the
  implementation landed as `cbb428b`. Its GitHub Actions run
  [34077005133](https://github.com/djr-747/ti-c54x/actions/runs/34077005133)
  is green across the repository's full CI matrix.
- TDD evidence is RED/GREEN. Focused strict and ASan/UBSan bus, sequencer, and
  CPU tests passed, followed by an independent combined review. Full standalone
  local `make test` is green: 176 Python tests plus all C tests, evidence and
  build-manifest checks, observer-off checks, and source-boundary checks over
  273 files. This repository has no `make guard` target, so `make test` remains
  its authoritative local gate.
- The additive public endpoint contract owns copied, value-only requests,
  BEGIN/RESUME access, WAIT/READY/FAULT outcomes, cancellation, stable 64-bit
  transaction IDs, and a canonical absent endpoint. The private 16-bit bus
  adapter and reducer own exactly-once external access and terminal publication.
  Shared DATA-memory visibility occurs only at READY; WAIT never publishes
  architectural state.
- The sequencer owns a persistent root memory-instruction transaction, pure
  MOD2 address resolution, candidate AR/PC state, and phase-safe
  commit/cancel/reset behavior. The CPU services pending work before fetch,
  never refetches or redispatches a waiting instruction, emits no false STOP on
  WAIT, guards reset/deinit cancellation against re-entry, and preserves the
  monotonic transaction-ID sequence across reset.
- Review and validation corrected five structural hazards before closure:
  RESUME no longer reruns resolver/planner validation; transaction phase and
  reducer state are cross-checked; reset/cancel cannot raw-clear an active
  transaction; observer-off builds compile trace call sites out; and CPU
  cancellation rejects callback re-entry. CPU execution, pending-work, and
  trace ownership were split from lifecycle code, while large tests were split
  by behavior. The 482-line root transaction owner remains a cohesive stable
  boundary and must not grow during P2-W31B2.
- The registry closes at 128 evidence records, 191 requirements (155
  implemented, 13 planned, 23 unresolved), and 151 tests (137 passing, 14
  planned). No knobs were added or changed. No environment access,
  diagnostics, timing model, opcode implementation, repeat/IRQ policy, device
  model, MMR/program/I/O space, dual access, parallel issue, or posted-write
  claim was introduced.
- No meaningful hosted performance record is available yet: the existing
  reset-runnable benchmark scenarios do not exercise this new memory path, and
  this milestone adds no opcode rows. Capture the first relevant performance
  baseline after P2-W31B2 makes the path executable.
- Follow-up P2-W31B2: add catalog, decode, operation, and end-to-end proofs for
  `LD *ARx+,dst` and `STL src,*ARx+`. This should move the catalog from 22 to
  24 rows and from 1287 to 1319 populated first words. Broader data-memory
  families remain separate evidence-led slices rather than implicit claims of
  this foundation.

## 2026-09-07 — P2-W31P trusted hot-path architecture and baseline freeze

- State: active. The evidence, ownership, structural, and hosted-performance
  contract landed as `bc06ac86f9aaf4caa76cc2c83ed4fa4540a04889`.
  P2-W31B2 remains paused until W31P implementation and performance acceptance
  are green.
- The freeze selects one compact catalog first-word lookup, one immutable
  catalog-derived per-ID binding, one private value-owned trusted execution
  packet, one operand-extraction pass, and one precomputed PC plan. Checked
  decode, dispatch, and PC APIs remain at forged-input and failure boundaries;
  generic resource collection, atomic effects, observer isolation, and the
  persistent memory WAIT/RESUME transaction remain unchanged contracts.
- The committed `bench/audits/p2-w31p-baseline.json` records the exact W31B1
  source identity, O2/no-LTO and O3/LTO commands and flags, compiler/host
  identity, binary and semantic hashes, sample ranges, and medians. The
  triggering O2 medians range from 75.475 to 181.788 ns per instruction across
  the fixed scenarios; O3/LTO ranges from 31.476 to 51.368 ns. These are hosted
  implementation observations, not TI timing evidence or an acceptance set.
- Acceptance requires three consecutive same-binary, same-scenario-hash
  captures for each profile. Every fixed-scenario capture must be below 60 ns
  per retired instruction with range/median at most 0.15. Failures are retained
  as `INCONCLUSIVE`; there is no cross-host CI threshold, runtime knob, test
  instrumentation, environment read, or production diagnostic path.
- Validation: the complete standalone `make test` gate passed after the freeze
  was made coherent: 176 Python tests, all C executables, evidence/knob/source/
  manifest checks, and the observer-disabled artifact check. JSON parsing and
  `git diff --check` also passed. Validation corrected an unrelated NOP owner
  drift and two omitted evidence-index links before commit.
- Registry: 130 evidence records, 197 requirements (155 implemented, 19
  planned, 23 unresolved), and 157 tests (137 passing, 20 planned). No knob,
  production source, public ABI, ISA behavior, memory behavior, timing model,
  firmware, device, or emulator dependency changed.
- Follow-up: begin W31P RED with the six registered owner-mirrored tests. First
  prove the release-profile/audit contract and exhaustive trusted binding;
  then add packet, trusted dispatch, CPU path, and PC-plan tests before their
  production implementations.

## 2026-09-07 — P2-W31P trusted hot-path implementation

- State: implementation and structural evidence complete; hosted acceptance
  remains pending. The implementation landed as
  `f36c1e30c8812f14f7ce9de1397e169c624699bf`. W31B2 remains paused until both
  required three-capture hosted profile sets pass.
- The common CPU path now performs one catalog-derived binding selection and
  constructs one value-owned trusted execution packet carrying a prevalidated
  static operand plan and trusted semantic handler. Checked packet construction
  remains available for forged-input tests, while the CPU uses the private fast
  constructor without a second binding lookup or dynamic layout resolution.
- Trusted semantic leaves preserve the existing generic resource collection and
  atomic effect boundary. The sequencer's checked and trusted PC APIs share one
  planning implementation; trusted commit applies the already-proven plan rather
  than repeating arithmetic or validation. Existing memory WAIT/RESUME behavior
  and compile-time observer isolation remain intact, with focused regressions for
  same-run WAIT-to-READY continuation and observer-enabled trusted retirement.
- TDD evidence is GREEN. The exact committed tree passed standalone `make test`:
  186 Python tests, the full C executable suite, the evidence and knob registries,
  source-boundary checks over 285 files, build-manifest checks, and the
  observer-disabled artifact check. Focused trusted-path strict, ASan/UBSan, and
  observer-enabled builds also passed independent review. `git diff --check` and
  the release-performance contract's direct CLI regression passed.
- The registry now contains 130 evidence records, 197 requirements (161
  implemented, 13 planned, 23 unresolved), and 157 tests (143 passing, 14
  planned). No knob was added or changed. No public ABI, ISA inventory, timing
  model, device behavior, environment access, allocator, file I/O, logging, or
  emulator dependency was introduced.
- The release benchmark target now forces an isolated O3/LTO rebuild, and the
  acceptance checker verifies source/binary/scenario identity, audit coherence,
  fixed-scenario coverage, the 60 ns ceiling, and the 0.15 range/median stability
  limit. These are hosted implementation gates only, not TI machine-cycle claims.
- Follow-up: from this clean implementation commit, build one O2/no-LTO binary
  and one O3/LTO binary, retain three consecutive captures from each, and run the
  acceptance checker over each same-binary set. Record inconclusive sets rather
  than silently replacing them; close W31P only after both profiles pass.

## 2026-09-07 — P2-W31P first hosted acceptance attempt

- State: both first three-capture sets are `INCONCLUSIVE`; W31P acceptance remains
  open and W31B2 remains paused. The six clean capture records and their audit
  summary landed as `12105cdddd144c863be8806d18bdfa207d3ff297`.
- Set `P2-W31P-O2-0D4F333-SET1` used one O2/no-LTO observer-off binary
  (`e5bb3ea8bd86cda00fa5b98aac90958d62beb9f6857c1a6f86e616ba6e325362`).
  NOP medians were 45.956–47.526 ns, while the three status-load medians reached
  65.721 ns; the checker also found per-capture stability ratios as high as
  0.378515. The complete set is retained rather than averaged or discarded.
- Set `P2-W31P-O3-LTO-0D4F333-SET1` used one O3/LTO observer-off binary
  (`356e780749de36e7de1ae3bc2bc43b552128bd79b1f5bbb7aba593ec54857285`).
  Most fixed medians were below 60 ns, but the noisiest capture reached 62.579 ns
  for LD-ASM and 66.884 ns for LD-ARP; stability ratios reached 0.855695. This
  complete set is also retained as inconclusive evidence.
- Both sets were captured from clean `0d4f333f58208e7ed0d749aec8eec39898c1342b`
  on the same i7-1255U host with GCC 15.2.0 and exact frozen flags. These are
  hosted implementation observations only, not TI cycle or silicon claims. No
  knob, production diagnostic, runtime instrumentation, or benchmark semantic
  policy changed.
- Follow-up profiling attributes the largest O2 costs to full staged packet
  clearing/copying and nested staged PC planning, followed by status-effect
  construction. Implement and test bounded portable-C direct-fill trusted paths,
  preserving checked failure atomicity and ownership. Any replacement capture
  set must use a new ID and record the external-condition change; fixed P-core
  affinity with competing agent/build load removed is the first admissible host
  condition to evaluate after the optimization commit is clean.

## 2026-09-07 — P2-W31P trusted direct-fill optimization

- State: implementation complete and local gates green; clean hosted acceptance
  capture remains pending. The bounded optimization landed as
  `c07be534cc8313437463307a48f01f0702ba6ecf`.
- Profile-guided work removed three avoidable staging costs without changing the
  frozen architecture. Fast packet construction now stages only extracted
  operand values and direct-fills the value-owned packet after the sole fallible
  operation succeeds. Checked packet construction retains forged-input rejection
  and byte-for-byte failure atomicity, and the structural proof still sees one
  operand-extraction seam.
- Sequential-PC remains the sole arithmetic owner. Its shared arithmetic helper
  supports a private trusted bridge path that direct-fills the planned effect
  after carrier, width, and boundary checks, while checked planning retains full
  staged validation. Trusted DP/ASM/ARP semantic leaves now direct-fill a complete
  deterministic status-field effect; checked handlers retain staged construction.
- TDD evidence is GREEN. The exact combined tree passed standalone `make test`:
  186 Python tests, the full C suite, evidence/knob/source/build-manifest checks,
  and the observer-disabled artifact check. Focused trusted packet, trusted
  semantic dispatch, LD-status, and PC tests passed; the PC test also passed an
  ASan/UBSan build. `git diff --check` passed.
- A dirty-tree diagnostic O2/no-LTO rebuild, used only to judge optimization
  direction and never as acceptance evidence, measured fixed medians of 28.842,
  45.396, 42.564, and 40.125 ns for NOP, LD-DP, LD-ASM, and LD-ARP on its second
  run. This gives substantial margin versus the first inconclusive cohort, while
  scheduler/frequency outliers still require a clean stable capture set.
- No knob, public ABI, opcode inventory, observer behavior, memory WAIT path,
  effect atomicity, benchmark semantic policy, environment read, logging, file
  I/O, allocation, or timing-model claim changed.
- Follow-up: rebuild both frozen profiles once from this clean ledgered commit,
  run three consecutive captures per unchanged binary with the host otherwise
  quiescent, and evaluate them under new set IDs. Use fixed P-core affinity only
  as an explicitly recorded external-condition change if the unpinned sets remain
  scheduler-noisy; retain every inconclusive set.

## 2026-09-07 — P2-W31P optimized hosted acceptance attempt

- State: throughput target met in every fixed measurement, but both SET2 cohorts
  remain `INCONCLUSIVE` on stability; W31P acceptance and W31B2 remain paused.
  The twelve-profile-history update through SET2 landed as
  `78bd2607d496a6c3531bbfc6fc740c4d8ac53446`.
- `P2-W31P-O2-5613EB3-CPU2-SET2` used one clean O2/no-LTO binary and
  `P2-W31P-O3-LTO-5613EB3-CPU2-SET2` used one clean O3/LTO binary, both from
  `5613eb3dad890e091c86adfebddc1122f230f7d9`. All 24 fixed-scenario medians
  were below 60 ns: O2 ranged from 26.970 to 47.938 ns and O3 from 19.310 to
  30.221 ns.
- The exact checker failures are retained with all six raw records. O2 failed
  only three isolated stability checks (LD-DP at 0.300377 and 0.253300, LD-ASM
  at 0.428192). O3 remained broadly noisy, with fixed-scenario ratios between
  0.158979 and 0.773938. No capture was discarded, averaged, or promoted.
- The replacement condition was explicit CPU2 affinity with this thread's agent
  and build activity quiesced. Contemporaneous read-only `mpstat` and `ps`
  snapshots still showed substantial unrelated host load, including heavy work
  on CPU2's SMT sibling CPU3 and an external compiler consuming a full core.
  This explains deferring another acceptance attempt until a genuinely quiet
  pair is observed; it does not relax or change the frozen threshold.
- No knob, benchmark semantic, production instrumentation, governor, priority,
  public ABI, core behavior, or TI timing claim changed. Follow-up: use a new
  SET3 ID only after the external load condition materially changes, rebuild the
  same two frozen profiles from a clean commit, and retain/evaluate three new
  consecutive captures per unchanged binary.

## 2026-09-07 — P2-W31P warmed CPU1 acceptance attempt

- State: SET3 remains `INCONCLUSIVE` solely on isolated stability excursions;
  all fixed medians remain below target and W31P/W31B2 stay paused. The six raw
  records and audit summary landed as
  `699e36d2a6aac99587535eea2bf2f2b86c1b48b7`.
- The changed host condition used CPU1 affinity plus a two-second same-core
  `sha256sum /dev/zero` warm-up before every capture, from clean
  `f6713e19b9516e89015b1163fa95eea3f0da46e5`. O2 fixed medians ranged from
  25.409 to 42.935 ns; O3 fixed medians ranged from 20.189 to 29.807 ns.
- O2 capture 3 and O3 capture 1 each independently met every threshold and
  stability check. They were not cherry-picked: the required consecutive sets
  also contain isolated ratios of 0.481695 and 0.563306 in O2, and 0.159742 to
  0.368087 in O3, so both complete cohorts remain inconclusive and retained.
- The threshold, sample/window counts, scenario semantics, clean-tree rule, and
  exact-three policy were not changed. No governor, scheduler priority, knob,
  production code, instrumentation, or TI timing claim changed.
- Follow-up: attempt a new set only when the quiet-core observation is sustained;
  keep CPU affinity and warm-up explicit, use a new set ID, and never substitute
  the individually green records for a consecutive three-capture result.

## 2026-09-07 — P2-W31P weighted CPU1 acceptance attempt

- State: SET4 remains `INCONCLUSIVE` solely on four isolated stability checks;
  all fixed medians remain below target. The raw records and audit summary landed
  as `50396cc557ad60422ed49e1d5caf921dd321212b`.
- The changed host condition retained CPU1 affinity, extended the same-core
  warm-up to five seconds, and ran each capture in a transient user scope with
  `CPUWeight=10000`. O2 missed only capture 3 LD-ARP at 0.442131. O3 missed
  capture 1 NOP at 0.401510 and LD-DP in captures 2 and 3 at 0.198382 and
  0.429899. All other fixed checks passed.
- Every fixed median remained below 60 ns. The O2 cohort ranged from 25.877 to
  46.112 ns; O3 ranged from 19.371 to 33.315 ns. No record was discarded or
  promoted from another set, and no governor, realtime priority, benchmark
  policy, production code, knob, instrumentation, or TI timing claim changed.
- Follow-up: require a sustained idle-core observation before SET5, retain the
  same explicit affinity/warm-up/weight controls, use a new set ID, and continue
  treating any preempted capture as evidence rather than selectively retrying a
  single member.

## 2026-09-08 — P2-W31B2 activation with W31P left open

- State: current intent updated in
  `071de4ddc6413b8af9725ef4c1b6406d8868c639`. P2-W31P remains open with its
  structural and correctness gates green, every optimized fixed-scenario median
  below 60 ns, and exact three-capture hosted stability acceptance still
  pending/deferred. This is not a performance acceptance claim.
- Decision: the open hosted stability follow-up no longer blocks P2-W31B2.
  Fixed-MOD2 `LD *ARx+,dst` and `STL src,*ARx+` are now the active bounded ISA
  slice through the already-proven transactional memory path.
- The frozen W31P acceptance rule and append-only performance history are
  unchanged. Dedicated immediate-READY LD/STL scenarios must still meet the
  same hosted target before later memory-family fanout; WAIT-capable behavior
  remains a separate correctness contract.
- Validation: standalone `make quality-test` passed after restoring the exact
  immutable evidence-locator headings: 186 Python tests, evidence/knob/source
  boundary/build-manifest checks, and the observer-disabled artifact check.
- No production code, public ABI, opcode inventory, requirement/evidence state,
  knob, benchmark semantic, runtime behavior, timing model, or TI silicon claim
  changed. Follow-up begins with RED catalog/decode tests for the two pinned
  masked rows, then proceeds by owner through pure effects and CPU retirement.

## 2026-09-08 — P2-W31B2 fixed-MOD2 LD/STL implementation

- State: implementation commit
  `b99c638cab33a7ee326570bc509b8bf13e766705` completes the bounded
  fixed-MOD2 pair `LD *ARx+,dst` and `STL src,*ARx+`. The catalog now contains
  24 rows and 1,319 populated first words, with 4 of 24 rows reset-runnable.
  P2-W31P remains open, and the dedicated immediate-READY LD/STL hosted
  benchmark follow-up remains pending before later memory-family fanout.
- TDD evidence: catalog/decode RED first failed on the absent opcode IDs and
  operand layout; operation RED failed on absent LD/STL handler symbols; CPU
  RED reached the old generic execution path and stopped invalid-state because
  MEMORY effects had no endpoint. GREEN adds the pinned `0x1090/0xFEF8` LD and
  `0x8090/0xFEF8` STL rows, the shared accumulator/ARF operand layout, pure
  checked and trusted handlers, dispatch bindings, and generic CPU MEMORY
  routing through the existing transaction root.
- Semantics: LD reads the old auxiliary-register address, applies SXM
  zero/sign extension of WORD16 into the selected 40-bit accumulator, then
  postincrements the selected AR. STL writes the selected accumulator's low
  WORD16 to the old AR address, then postincrements it. Both preserve C/TC,
  require CMPT=0 and ARP=0, and reject AR=0xffff. The CPU owns fetch/attempt and
  retirement accounting exactly once across READY, WAIT/retry, and FAULT.
- Validation: focused catalog, decode, pure-operation, CPU ISA, and existing
  memory lifecycle/pending tests passed; 27 relevant Python contract tests
  passed; the eight focused C targets passed from a clean ASan/UBSan build; and
  the authoritative clean `make test` passed, including all 186 Python tests.
  Independent Luna reviews found no P0/P1 correctness blocker.
- Evidence accounting is now 166 IMPLEMENTED, 8 PLANNED, and 23 UNRESOLVED
  requirements; 149 PASSING and 8 PLANNED tests; 130 evidence records; and 7
  source records. Lower-risk follow-up coverage remains for malformed observer
  retry and cancel-failed retry at the concrete opcode level, counter overflow,
  and mixed multi-instruction memory sequences; existing generic transaction
  tests cover the underlying contracts.
- No knob, public ABI, environment read, logging, allocation, mutable global,
  machine-cycle API, repeat behavior, device/MMR behavior, timing model, or TI
  silicon claim changed.

## 2026-09-08 — P2-W32 SSBX/RSBX evidence activation

- State: evidence and ownership activation landed as
  `99243d2e3afa74c922a000db1cfcfe514c90fdae`. P2-W32 is the active
  non-memory ISA slice; P2-W31P hosted stability and the W31B2 immediate-READY
  benchmark follow-ups remain open and unchanged.
- TI `SPRU172C` evidence now pins one-word SSBX at `0xF5B0/0xFDF0`
  (printed p. 4-166) and RSBX at `0xF4B0/0xFDF0` (printed p. 4-151), with N at
  bit 9, SBIT at bits 3:0, no input resources, and repeatability INELIGIBLE.
  SSBX sets and RSBX clears the selected raw ST0/ST1 bit; every encoded SBIT,
  including ST1 reserved bit 10, is admitted without invented side effects.
- Five planned contracts separate catalog, decode, architectural handler
  effects, the project-owned transactional status-bit application seam, and
  project-owned generic CPU retirement. The expected completed catalog is 26
  rows and 1,383 populated first words, likely 6/26 reset-runnable.
- Review corrected draft SSBX/RSBX set-versus-clear prose before publication
  and separated the shared transaction and CPU-retirement requirements from TI
  architectural claims through the immutable P2-W32 project-contract record.
  Current registry totals are 133 evidence records, 202 requirements (166
  implemented, 13 planned, 23 unresolved), and 162 tests (149 passing, 13
  planned).
- Validation: the evidence checker passed with 2 targets, 7 sources, 202
  requirements, and 162 tests; the complete `make quality-test` gate passed all
  186 Python tests plus evidence, knob, source-boundary, build-manifest, and
  observer-disabled artifact checks.
- No production or generated source, executable test, public ABI, memory path,
  RPT/IRQ behavior, timing claim, knob, environment read, diagnostic path,
  allocator, or mutable global changed. TDD proceeds with RED owner-local tests
  before any catalog row or status-bit implementation is added.

## 2026-09-08 — P2-W32 encoding-population correction

- Correction commit `81cbbecb3f13ffb714bc9b559c5ce1944a53a0a3`
  fixes the activation prose from 16 to 32 encodings per row. Mask `0xFDF0`
  leaves N at bit 9 plus SBIT at bits 3:0 variable, so each of SSBX and RSBX
  covers 32 first words and the pair covers 64.
- The already-recorded expected completed totals of 26 catalog rows and 1,383
  populated first words remain correct. No encoding, evidence locator,
  requirement/test state, production source, executable test, or behavior
  changed. The standalone evidence checker remained green.

## 2026-09-08 — P2-W32 SSBX/RSBX implementation

- State: implementation commit
  `3d9bd1866a129ee620cd76b6e2d7f4bf2c450bb4` completes the bounded
  SSBX/RSBX status-bit vertical slice. The catalog now contains 26 rows and
  1,383 populated first words, with 6/26 rows reset-runnable. P2-W31P hosted
  stability and the W31B2 immediate-READY benchmark follow-ups remain open.
- TDD RED was captured independently by owner. Status tests failed on the
  absent checked ST0/ST1 bit writer, value-owned effect payload, application
  leaf, and root transaction route. Catalog/decode tests failed on the absent
  IDs and N/SBIT layout; operation tests failed on the absent status-family
  header; CPU proof failed at the absent catalog identities. No production row
  or behavior was added before those failures were recorded.
- GREEN registers SSBX `0xF5B0/0xFDF0` and RSBX
  `0xF4B0/0xFDF0`, with N at bit 9, SBIT at bits 3:0, one program word,
  repeatability INELIGIBLE, and no input resources. SSBX stages binary one and
  RSBX binary zero for the selected ST0/ST1 raw bit. The checked state leaf also
  marks exactly that bit known; every unrelated raw and knownness bit is
  preserved, including when ST1 reserved bit 10 is selected.
- The shared implementation adds one private `STATUS_BIT_WRITE` effect, an
  explicit state-layer target mapping, and a distinct candidate-only root
  transaction route. All payload validation precedes publication. Checked and
  trusted handlers remain pure and value-owned. Existing generic CPU dispatch,
  sequential-PC planning, counters, and retirement consume the effect without
  any production CPU change or opcode-identity branch.
- Review found no P0/P1 semantic blocker. Integration review corrected a RED
  near-miss that accidentally selected the valid sibling opcode, refreshed the
  three catalog-size quality fixtures, clarified the catalog module contract,
  and fixed one clean-build `-Wconversion` diagnostic by constructing the bit
  mask only after range validation.
- Validation: all focused state, status-effect, root-transaction,
  catalog/decode, pure-operation, CPU-retirement, and fixed-MOD2 regression
  targets passed. `make quality-test` passed all 186 Python tests and every
  registry, knob, source-boundary, manifest, and observer-off artifact check.
  Nine focused targets passed from a clean ASan/UBSan build. The authoritative
  clean `make test` completed successfully.
- Evidence accounting is now 133 evidence records; 202 requirements (171
  IMPLEMENTED, 8 PLANNED, 23 UNRESOLVED); 162 tests (154 PASSING, 8 PLANNED);
  and 7 source records.
- No public ABI, memory behavior, RPT/IRQ policy, machine-cycle API, timing or
  repeatability claim, knob, environment read, logging, diagnostic fanout,
  allocator, mutable global, device/MMR behavior, or emulator dependency was
  added. Selection of the next ISA slice remains evidence-first.

## 2026-09-08 — P2-W33 CMPR evidence activation

- State: evidence and ownership activation landed as
  `f23f54fce6bfb449fcebfec3b3902a6e61b1884d`. P2-W33 is the active
  bounded ISA slice; P2-W31P hosted stability and the W31B2 immediate-READY
  benchmark follow-ups remain open and unchanged.
- TI `SPRU172C` evidence pins one-word `CMPR CC,ARx` at
  `0xF4A8/0xFCF8`, with CC in bits 9:8 and ARx in bits 2:0, yielding 32
  encodings. The selected unsigned EQ/LT/GT/NEQ comparison tests ARx against
  AR0 and writes TC to one iff the condition is true. Table 2-28 explicitly
  classifies CMPR as nonrepeatable.
- Five planned contracts separate catalog, decode, generic copied
  `AUXILIARY_REGISTERS` input collection, pure comparison effects, and generic
  CPU retirement. The input snapshot carries all eight 16-bit AR values plus
  knownness; unknown AR0 or selected ARx rejects the instruction without a
  partial effect. The handler reuses `STATUS_BIT_WRITE` for ST0 bit 12.
- Candidate review rejected `LD #K,dst` because the registered TI evidence
  does not fully define short-immediate widening for every K under both SXM
  states. It also rejected fixed-MOD2 STH because saturation-on-store depends
  on the currently unmodeled and reset-unknown SST state. Neither gap was
  hidden behind an inferred semantic or configuration assumption.
- Expected completed coverage is 27 catalog rows and 1,415 populated first
  words, with reset-runnable coverage unchanged at 6/27. Activation registry
  totals are 136 evidence records; 207 requirements (171 implemented, 13
  planned, 23 unresolved); and 167 tests (154 passing, 13 planned).
- Validation: `make quality-test` passed all 186 Python tests plus evidence,
  knob, source-boundary, build-manifest, and observer-disabled artifact checks;
  `git diff --check` also passed.
- No production or generated source, executable test, public ABI, memory
  behavior, timing model, machine-cycle API, RPT/IRQ policy, knob, environment
  read, diagnostic path, allocator, mutable global, device behavior, or
  emulator dependency changed. TDD proceeds with owner-local RED tests before
  the catalog row or handler is added.

## 2026-09-08 — P2-W33 CMPR implementation

- State: implementation commit
  `a0789263818f908708ccfd20df6ee2b479b598ab` completes the bounded
  `CMPR CC,ARx` vertical slice. The catalog now contains 27 rows and 1,415
  populated first words, with reset-runnable coverage unchanged at 6/27.
  P2-W31P hosted stability and the W31B2 immediate-READY benchmark follow-ups
  remain open.
- TDD RED was captured independently by owner. Catalog/decode tests failed on
  the absent CMPR ID, CC/ARx layout, and auxiliary-register resource. Input
  tests failed on the absent copied raw16 values, knownness mask, and
  architectural source. Pure-operation and CPU tests failed on the absent
  handler and catalog identity. No production row or behavior preceded those
  failures.
- GREEN registers `CMPR_AR` at `0xF4A8/0xFCF8`, with CC in bits 9:8, ARx in
  bits 2:0, one program word, and repeatability INELIGIBLE. The pure checked
  and trusted handler compares the selected 16-bit AR against AR0 as unsigned
  for EQ, LT, GT, or NEQ and emits only a `STATUS_BIT_WRITE` for TC at ST0 bit
  12. ARx=AR0 aliases correctly; unknown AR0 or selected ARx rejects without
  publishing a partial effect.
- The shared prerequisite is one private `AUXILIARY_REGISTERS` resource. The
  collector copies all eight raw16 AR values and a knownness mask through
  checked architectural reads, canonicalizes unknown values to zero, and
  commits the snapshot only after every requested resource succeeds. The CPU
  supplies its existing instance-owned architectural state through a
  designated source initializer; unrelated opcodes skip the new collection
  branch, and no opcode-specific CPU path or new effect kind was added.
- Integration exposed two stale tests that asserted UNKNOWN for words newly
  admitted by CMPR: one LD-status near miss and one MAX/MIN invalid vector.
  Both were replaced with genuinely unassigned fixed-bit misses, followed by
  a catalog-wide search for other hard-coded CMPR-range UNKNOWN vectors. Older
  input-test source helpers were also zero-initialized after the private source
  and snapshot carriers grew.
- Validation: all focused catalog, decode, input, pure-operation, CPU, status
  bit, and effect-transaction targets passed under strict warnings. The same
  seven focused targets passed from a clean ASan/UBSan build. `make
  quality-test` passed all 186 Python tests plus evidence, knob,
  source-boundary, manifest, and observer-disabled artifact checks. Two
  independent Luna reviews found no P0/P1/P2 blocker. After the two stale
  vectors were corrected, the authoritative final standalone `make test`
  completed successfully.
- Size decision: `src/isa/decode/operand_layout.c` is 506 lines after the new
  two-field descriptor and emits the expected review warning. It remains one
  cohesive layout-data/resolver owner for this slice; future growth must
  review decomposition, and the file must split before approaching 800 lines.
- Evidence accounting is now 136 evidence records; 207 requirements (176
  IMPLEMENTED, 8 PLANNED, 23 UNRESOLVED); 167 tests (159 PASSING, 8 PLANNED);
  and 7 source records.
- No public ABI, memory behavior, RPT/IRQ policy, machine-cycle API, timing or
  performance claim, knob, environment read, logging, diagnostic fanout,
  allocator, mutable global, device/MMR behavior, or emulator dependency was
  added. Selection of the next ISA slice remains evidence-first.

## 2026-09-08 — P2-W34 fixed-MOD2 LDU evidence activation

- State: evidence and ownership activation landed as
  `4d77bdf53cbaf9cd3211e28cce0a2d658b8da0d5`. P2-W34 is the active
  bounded ISA slice. The P2-W31P hosted-stability and W31B immediate-READY
  benchmark captures remain open and, by current owner direction, do not block
  this evidence-fit reuse of the established fixed-MOD2 transaction path.
- TI `SPRU172C` printed p. 4-79 defines one-word LDU with opcode diagram
  `0001_001D_IAAAAAAA`, unsigned WORD16 load into destination bits 15:0,
  clearing of bits 39:16 regardless of SXM, and no status-bit effect. Composed
  with the registered standard-indirect MOD2 field, the selected row is
  `0x1290/0xFEF8`, with D at bit 8 and ARF at bits 2:0, for 16 first words.
- Four planned contracts separate catalog registration, value-owned decode,
  pure MEMORY effect/completion, and generic CPU retirement. The existing
  W31B contract supplies CMPT=0, ARP=0, old-AR transfer before postincrement,
  READY-only architectural commit, and checked rejection at AR=0xffff. LDU
  needs no SXM input or new input resource, effect kind, address kind, memory
  lifecycle, or opcode-specific CPU branch.
- Repeatability remains explicitly `UNRESOLVED`. LDU is absent from Table
  2-28 and fixed MOD2 is not the separately forbidden long-offset/absolute
  case, but absence from that negative list is not treated as positive
  eligibility evidence.
- Expected completed coverage is 28 catalog rows and 1,431 populated first
  words, with reset-runnable coverage unchanged at 6/28. Activation accounting
  is 139 evidence records; 212 requirements (176 IMPLEMENTED, 12 PLANNED,
  24 UNRESOLVED); and 171 tests (159 PASSING, 12 PLANNED).
- Validation: the evidence checker passed with 2 targets, 7 sources, 212
  requirements, and 171 tests. `make quality-test` passed all 186 Python tests
  plus evidence, knob, source-boundary, build-manifest, and observer-disabled
  artifact checks; `git diff --check` passed. The existing 506-line operand
  layout owner emitted its already-reviewed size warning and is unchanged.
- No production, generated, or executable-test file changed. No knob, public
  ABI, timing or machine-cycle claim, IRQ/RPT behavior, device/MMR policy,
  environment read, diagnostic path, allocator, mutable global, or emulator
  dependency was added. Implementation begins with owner-local RED tests.

## 2026-09-08 — P2-W34 fixed-MOD2 LDU implementation

- State: implementation commit
  `cb5afb72a183678492c00d95de0fd8443b73df9c` completes the bounded
  fixed-MOD2 `LDU *ARx+,dst` vertical slice. The catalog now contains 28 rows
  and 1,431 populated first words, with reset-runnable coverage unchanged at
  6/28. P2-W31P hosted stability and the W31B immediate-READY benchmark
  follow-ups remain open and nonblocking.
- TDD RED was captured in three independent owner lanes. Catalog/decode tests
  failed on the absent LDU identity; the pure-operation test failed on the
  absent handler contract; and the public-CPU test reached
  `STOP_UNIMPLEMENTED_OPCODE` instead of retirement. No production row or
  handler preceded those failures.
- GREEN registers `LDU_SMEM_MOD2` at `0x1290/0xFEF8`, with D in bit 8, ARF in
  bits 2:0, one program word, `ACCUMULATOR_ARF`, no input resources, and
  repeatability UNRESOLVED. The checked and trusted pure leaves stage the
  existing generic fixed-MOD2 WORD16 read. Completion zero-extends the returned
  word into the selected canonical 40-bit accumulator, preserves C and TC,
  and never reads or interprets SXM.
- Generic W31B transaction and CPU paths provide the old-AR request,
  postincrement only after READY, WAIT resume with the same transaction ID and
  no refetch, FAULT rollback, sequential PC retirement, dependency rejection,
  and checked AR=0xffff rejection. No opcode-specific CPU branch, new input
  resource, effect kind, address kind, or memory lifecycle was added.
- Integration corrected one RED test that forged an instruction by flipping
  ARF bit 0, which is a valid operand field; the rejection now flips fixed bit
  3. Catalog fixtures were advanced to 28 rows, 1,431 populated words, and 29
  generated identities, while the quality contract gained an explicit LDU row
  assertion and retained its CMPR assertion at the preceding row. A search
  found no pre-existing UNKNOWN vector in the newly claimed LDU words.
- Validation: all four focused catalog, decode, pure-operation, and public-CPU
  targets passed under strict warnings and again in a forced ASan/UBSan build.
  `make quality-test` passed all 186 Python tests plus evidence, knob,
  source-boundary, build-manifest, and observer-disabled artifact checks. Two
  independent Luna reviews found no remaining P0/P1/P2 finding. The
  authoritative clean, unsanitized standalone `make test` completed
  successfully.
- Size decision: the new production files are bounded leaf owners. The existing
  506-line `src/isa/decode/operand_layout.c` emitted its previously reviewed
  warning and did not change in this slice.
- Evidence accounting is now 139 evidence records; 212 requirements (180
  IMPLEMENTED, 8 PLANNED, 24 UNRESOLVED); 171 tests (163 PASSING, 8 PLANNED);
  and 7 source records. LDU repeatability remains unresolved by design.
- No public ABI, timing or performance claim, machine-cycle API, IRQ/RPT
  policy, device/MMR behavior, knob, environment read, logging, diagnostic
  fanout, allocator, mutable global, or emulator dependency was added.

  Selection of the next ISA slice remains evidence-first.

## 2026-09-08 — P2-W35 fixed-MOD2 LD-to-T evidence activation

- State: evidence and ownership activation landed as
  `3fcac0a611c01cd04ef67b83c7ad50186929306a`. P2-W35 is the active
  bounded ISA slice. P2-W31P hosted stability and the W31B immediate-READY
  benchmark follow-ups remain open and nonblocking.
- TI `SPRU172C` §4.70 printed pp. 4-70–4-71 defines one-word `LD Smem,T`
  with opcode diagram `0011_0000_IAAAAAAA`, execution `T = Smem`, and no
  status-bit effect. Composed with the registered standard-indirect MOD2 rule,
  the selected row is `0x3090/0xFFF8`: bit 8 is fixed zero, only ARF bits 2:0
  vary, and exactly eight first words are admitted. The `*AR3+` example reads
  data `0xFEDC` from old address `0x0300` before AR3 becomes `0x0301`.
- Review corrected the initial scout's proposed `0xFEF8` mask before evidence
  activation. That mask would have incorrectly treated fixed bit 8 as a D/T
  operand and admitted 16 words. The primary opcode diagram instead requires
  `0xFFF8` and eight ARF-only encodings.
- Four planned contracts separate catalog registration, a new private generic
  `ARF` operand layout, pure MEMORY completion to the existing
  `T_REGISTER_WRITE` effect, and generic CPU retirement. The W31B lifecycle
  continues to own old-AR transfer, postincrement after READY, WAIT resume,
  FAULT rollback, CMPT=0/ARP=0 validation, and checked AR=0xffff rejection.
- Repeatability remains explicitly UNRESOLVED. Table 2-28 excludes long-offset
  and absolute single-data forms and lists some LD destinations, but absence of
  LD-to-T from the forbidden list is not treated as positive eligibility.
- Expected completed coverage is 29 catalog rows and 1,439 populated first
  words, with reset-runnable coverage unchanged at 6/29. Activation accounting
  is 142 evidence records; 217 requirements (180 IMPLEMENTED, 12 PLANNED,
  25 UNRESOLVED); and 175 tests (163 PASSING, 12 PLANNED).
- Validation: `make quality-test` passed all 186 Python tests plus evidence,
  knob, source-boundary, build-manifest, and observer-disabled artifact checks;
  `git diff --check` passed. The existing 506-line operand-layout owner emitted
  its reviewed size warning; W35 requires an explicit no-split decision before
  adding the cohesive three-bit descriptor.
- No production, generated, or executable-test file changed. No public T/MMR
  API, new memory/address/effect kind, CPU opcode branch, timing or
  machine-cycle claim, IRQ/RPT behavior, device policy, knob, environment read,
  diagnostic path, allocator, mutable global, or emulator dependency was
  added. TDD begins with owner-local RED tests.

## 2026-09-08 — P2-W35 fixed-MOD2 LD-to-T implementation

- State: implementation commit
  `264c3f8047b9dd1860020c0a461a4ab8acbc3ce6` completes the bounded
  fixed-MOD2 `LD *ARx+,T` vertical slice. The catalog now contains 29 rows
  and 1,439 populated first words, with reset-runnable coverage unchanged at
  6/29. P2-W31P hosted stability and the W31B immediate-READY benchmark
  follow-ups remain open and nonblocking.
- TDD RED was captured in three independent owner lanes. Catalog/decode tests
  failed on the absent LD-to-T identity and ARF layout; the pure-operation test
  failed on the absent handler contract; and the public-CPU test reached
  `STOP_UNIMPLEMENTED_OPCODE` instead of retirement. No production row,
  layout, or handler preceded those failures.
- GREEN registers `LD_T_SMEM_MOD2` at `0x3090/0xFFF8`, with fixed bit 8,
  ARF in bits 2:0, one program word, the private generic `ARF` layout, no
  input resources, and repeatability UNRESOLVED. All eight words
  `0x3090..0x3097` are generated; the tests also prove that bit 8 is not an
  operand. The checked and trusted pure leaves stage the existing generic
  fixed-MOD2 WORD16 read, and completion writes the raw returned word through
  the existing `T_REGISTER_WRITE` effect without reading SXM or status input.
- Generic W31B transaction and CPU paths provide the old-AR request,
  postincrement only after READY, WAIT resume with the same transaction ID and
  no refetch, FAULT rollback, sequential PC retirement, dependency rejection,
  and checked AR=0xffff rejection. Tests cover all eight ARFs, raw values
  `0x0000`, `0x8000`, and `0xffff`, replacement of an unknown T value,
  CMPT/ARP/AR knownness failures, status and accumulator preservation, and
  transactional checked/trusted/completion rejection. No opcode-specific CPU
  branch or new resource, effect kind, address kind, or memory lifecycle was
  added.
- Integration standardized the exact catalog/handler identity as
  `LD_T_SMEM_MOD2`/`ld_t_smem` and removed one unused local from a RED trusted
  rejection test so strict `-Werror` remained meaningful. Catalog fixtures
  advanced to 29 rows, 1,439 populated words, and 30 generated identities; a
  search and focused near-miss tests found no stale UNKNOWN vector in the newly
  claimed range.
- Validation: all four focused catalog, decode, pure-operation, and public-CPU
  targets passed under strict warnings and again in a forced ASan/UBSan build.
  `make quality-test` passed all 186 Python tests plus evidence, knob,
  source-boundary, build-manifest, and observer-disabled artifact checks. Two
  independent Luna reviews found no P0/P1/P2 finding. After removing the
  sanitizer-instrumented build artifacts, the authoritative clean,
  unsanitized standalone `make test` completed successfully.
- Size decision: the new production files are bounded leaf owners. The private
  three-bit ARF descriptor leaves `src/isa/decode/operand_layout.c` at 518
  lines. It remains the cohesive layout-data/resolver owner, is well below the
  800-line split threshold, and the reviewed decision is not to split it in
  this slice.
- Evidence accounting is now 142 evidence records; 217 requirements (184
  IMPLEMENTED, 8 PLANNED, 25 UNRESOLVED); 175 tests (167 PASSING, 8 PLANNED);
  and 7 source records. LD-to-T repeatability remains unresolved by design.
- No public ABI, timing or performance claim, machine-cycle API, IRQ/RPT
  policy, device/MMR behavior, knob, environment read, logging, diagnostic
  fanout, allocator, mutable global, or emulator dependency was added.
  Selection of the next ISA slice remains evidence-first.

## 2026-09-08 — P2-W36 fixed-MOD2 immediate-store evidence activation

- State: evidence and ownership activation landed as
  `38ea7bb6bc906a36bf1ff3c280742958b746c852`. P2-W36 is the active
  bounded ISA slice. P2-W31P hosted stability and the W31B immediate-READY
  benchmark follow-ups remain open and nonblocking.
- TI `SPRU172C` Store T/TRN/immediate printed pp. 4-166–4-167 defines the
  two-word `ST #lk,Smem` syntax with first-word diagram
  `0111_0110_IAAAAAAA`, raw 16-bit `lk` in word 1, no status-bit effect, and
  a memory write of that immediate. Composed with the registered standard
  indirect MOD2 rule, the selected row is `0x7690/0xFFF8`: bit 8 is fixed,
  ARF occupies bits 2:0, and exactly eight first words are admitted.
- Four planned contracts separate catalog registration, a private two-field
  `ARF_EXTENSION16` layout, a pure generic MEMORY WRITE effect, and generic CPU
  retirement. Existing extension fetch and width-aware PC staging supply the
  two-word instruction; W31B supplies old-AR addressing, postincrement after
  READY, WAIT resume without refetch, FAULT rollback, CMPT=0/ARP=0 validation,
  and checked AR=0xffff rejection. The store uses `write_value_word=lk` and
  `complete=NULL`, with no input resource or new CPU/effect/address seam.
- Repeatability remains explicitly UNRESOLVED. Immediate ST is absent from
  Table 2-28, but absence from its negative list is not positive eligibility
  evidence; the separately forbidden long-offset and absolute forms do not
  decide this fixed-MOD2 form.
- Candidate review retained `LD Smem,DP/ASM` as a later strict fit. `ST T`
  needs a currently absent T input resource; STH needs saturation/OVM/guard
  policy; and the audited ABS, NEG, RND, SAT, NORM, SFTA, ADD, SUB, and control
  forms need shared OVM/OV/T, composite-effect, PC, SP, IRQ, or timing seams.
  Those gaps were not hidden behind family-local behavior.
- Expected completed coverage is 30 catalog rows and 1,447 populated first
  words, with reset-runnable coverage unchanged at 6/30. Activation accounting
  is 145 evidence records; 222 requirements (184 IMPLEMENTED, 12 PLANNED,
  26 UNRESOLVED); and 179 tests (167 PASSING, 12 PLANNED).
- Validation: the evidence checker passed with 2 targets, 7 sources, 222
  requirements, and 179 tests. `make quality-test` passed all 186 Python tests
  plus evidence, knob, source-boundary, build-manifest, and observer-disabled
  artifact checks; `git diff --check` passed. The current 518-line
  operand-layout owner emitted its reviewed size warning; W36 requires another
  explicit no-split decision after adding the cohesive two-field descriptor.
- No production, generated, or executable-test file changed. No public ABI,
  CPU opcode branch, new resource/effect/address kind, timing or machine-cycle
  claim, IRQ/RPT behavior, device/MMR policy, knob, environment read,
  diagnostic path, allocator, mutable global, or emulator dependency was
  added. TDD begins with owner-local RED tests.

## 2026-09-08 — P2-W36 fixed-MOD2 immediate-store implementation

- State: implementation commit
  `28e87ea0e7e1ae8ba9d4cc89e64aaac332ab1fea` completes the bounded
  two-word fixed-MOD2 `ST #lk,*ARx+` vertical slice. The catalog now contains
  30 rows and 1,447 populated first words, with reset-runnable coverage
  unchanged at 6/30. P2-W31P hosted stability and the W31B immediate-READY
  benchmark follow-ups remain open and nonblocking.
- TDD RED was captured in three independent owner lanes. Catalog/decode tests
  failed on the absent immediate-store identity and ARF-plus-extension layout;
  the pure-operation test failed on the absent handler header; and the
  public-CPU test reached `STOP_UNIMPLEMENTED_OPCODE`. No production row,
  layout, or handler preceded those failures.
- GREEN registers `ST_IMMEDIATE_SMEM_MOD2` at `0x7690/0xFFF8`, with fixed
  bit 8, ARF in bits 2:0, two program words, no input resources, and
  repeatability UNRESOLVED. The private `ARF_EXTENSION16` layout extracts ARF
  from first-word bits 2:0 and copies all 16 bits of word 1 as `lk`. The
  checked and trusted leaves stage the existing generic fixed-MOD2 WORD16
  WRITE with the exact raw immediate in `write_value_word` and
  `complete=NULL`; input values and knownness are not inspected.
- Generic extension fetch, sequential-PC, W31B transaction, and CPU paths
  provide first-word/extension fetch ordering, the old-AR write address,
  postincrement and PC+2 only after READY, WAIT resume with the same
  transaction ID and no refetch, data and extension FAULT rollback, dependency
  rejection, and checked AR=0xffff rejection. Tests cover all eight ARFs,
  immediate boundaries `0x0000`, `0x7fff`, `0x8000`, and `0xffff`, status,
  accumulator, and T preservation, and program boundaries at 0xfffd/0xfffe.
  No opcode-specific CPU or sequencer production path was added.
- GREEN exposed a hidden RED-fixture defect: the CPU test wrote typed CMPT=0
  and ARP=0, then replaced the raw status registers and undid that setup. The
  owner reordered raw status seeding before the typed writes. The 0xfffe
  width-two boundary assertion was also aligned with the established memory
  transaction route, which reports `STOP_INVALID_STATE` for the invalid
  candidate PC; 0xfffd retires at PC 0xffff. Neither correction changed
  production behavior or weakened the architectural assertions.
- Catalog fixtures advanced to 30 rows, 1,447 populated words, and 31
  generated identities. The generated lookup maps exactly
  `0x7690..0x7697`; collision and stale-UNKNOWN review was clean. Schema,
  GNU Make, and CMake manifests include the new private layout, handler, and
  four executable contracts.
- Validation: all four focused catalog, decode, pure-operation, and public-CPU
  targets passed under strict warnings and again in a forced ASan/UBSan build.
  `make quality-test` passed all 186 Python tests plus evidence, knob,
  source-boundary, build-manifest, and observer-disabled artifact checks. Two
  independent Luna reviews found no P0/P1/P2 finding. After clearing the
  expected sanitizer-object contamination, the authoritative clean,
  unsanitized standalone `make test` completed successfully.
- Size decision: the new production files are bounded leaf owners. The
  cohesive two-field descriptor leaves
  `src/isa/decode/operand_layout.c` at 535 lines. That file remains the single
  layout-data/resolver owner, is well below the 800-line split threshold, and
  the reviewed decision is not to split it in this slice.
- Evidence accounting is now 145 evidence records; 222 requirements (188
  IMPLEMENTED, 8 PLANNED, 26 UNRESOLVED); 179 tests (171 PASSING, 8 PLANNED);
  and 7 source records. Immediate-store repeatability remains unresolved by
  design.
- No public ABI, timing or performance claim, machine-cycle API, IRQ/RPT
  policy, device/MMR behavior, knob, environment read, logging, diagnostic
  fanout, allocator, mutable global, or emulator dependency was added.
  `LD Smem,DP/ASM` remains the next recorded strict-fit ISA candidate.

## 2026-09-08 — P2-W37 fixed-MOD2 memory-to-status evidence activation

- State: evidence and ownership activation landed as
  `6f8d6b086582e11d009e604093e169457a67b062`. P2-W37 is the active
  bounded ISA slice. P2-W31P hosted stability and the W31B immediate-READY
  benchmark follow-ups remain open and nonblocking.
- TI `SPRU172C` §4.70 printed pp. 4-69–4-71 defines exactly two memory-to-
  status forms: `LD Smem,DP` and `LD Smem,ASM`. It defines no
  `LD Smem,ARP`; ARP is immediate-only in this instruction entry. Composed
  with the registered standard-indirect MOD2 rule, the DP row is
  `0x4690/0xFFF8` and the ASM row is `0x3290/0xFFF8`. Bit 8 is fixed, ARF
  occupies bits 2:0, and each row admits exactly eight first words.
- DP consumes memory data bits 8:0 and ASM consumes bits 4:0. Both forms
  report no status effect beyond writing the named field. Table 2-28 makes
  `LD DP` repeatability INELIGIBLE; it does not positively establish ASM
  eligibility, so ASM repeatability remains UNRESOLVED. Summary cycle values
  are not adopted as a machine-timing contract.
- Four planned contracts separate the two catalog rows, existing ARF decode,
  pure MEMORY completions to the existing `STATUS_FIELD_WRITE` route, and
  generic CPU retirement. W31B supplies the old selected-AR address for the
  WORD16 read, postincrement only after READY, WAIT resume without refetch,
  FAULT rollback, CMPT=0/ARP=0 validation, and checked AR=0xffff rejection.
  No new layout, resource, effect, address kind, CPU branch, or public API is
  required.
- Candidate audit found no catalog collision or stale UNKNOWN vector in
  `0x3290..0x3297` or `0x4690..0x4697`. Using mask `0xFEF8` would incorrectly
  admit fixed-bit ranges `0x3390..0x3397` and `0x4790..0x4797`; the evidence
  and planned tests require `0xFFF8`.
- Expected completed coverage is 32 catalog rows and 1,463 populated first
  words, with reset-runnable coverage unchanged at 6/32. Activation accounting
  is 148 evidence records; 227 requirements (188 IMPLEMENTED, 12 PLANNED,
  27 UNRESOLVED); and 183 tests (171 PASSING, 12 PLANNED).
- Validation: the evidence checker passed with 2 targets, 7 sources, 227
  requirements, and 183 tests. `make quality-test` passed all 186 Python tests
  plus evidence, knob, source-boundary, build-manifest, and observer-disabled
  artifact checks; `git diff --check` passed. The 535-line operand-layout
  owner is unchanged and retains its reviewed no-split decision.
- No production, generated, or executable-test file changed. No public ABI,
  timing or machine-cycle claim, IRQ/RPT behavior, device/MMR policy, knob,
  environment read, diagnostic path, allocator, mutable global, or emulator
  dependency was added. TDD begins with owner-local RED tests.

## 2026-09-08 — P2-W37 fixed-MOD2 memory-to-status implementation

- State: implementation commit
  `05e8c4a96b251f5c191f7f2a590653bb94120072` completes the bounded
  fixed-MOD2 `LD Smem,DP` and `LD Smem,ASM` vertical slice. The catalog now
  contains 32 rows and 1,463 populated first words, with reset-runnable
  coverage unchanged at 6/32. P2-W31P hosted stability and the W31B
  immediate-READY benchmark follow-ups remain open and nonblocking.
- TDD RED was captured in three independent owner lanes. Catalog/decode tests
  failed on the absent DP/ASM identities; the pure-operation test failed on
  the absent handler header; and the public-CPU test reached
  `STOP_UNIMPLEMENTED_OPCODE`. No production row or handler preceded those
  failures.
- GREEN registers `LD_DP_SMEM_MOD2` at `0x4690/0xFFF8` and
  `LD_ASM_SMEM_MOD2` at `0x3290/0xFFF8`, with fixed bit 8, ARF in bits 2:0,
  one program word, no input resources, and exactly eight encodings each.
  DP repeatability is INELIGIBLE; ASM remains UNRESOLVED. No memory-to-ARP
  form was invented.
- The catalog schema requires unique handler tokens. Integration preserved
  that invariant with thin `ld_dp_smem` and `ld_asm_smem` checked/trusted
  bindings over one shared internal semantic core. Both stage the existing
  fixed-MOD2 WORD16 MEMORY read and complete through `STATUS_FIELD_WRITE`,
  masking the copied value to `0x01ff` for DP or `0x001f` for ASM.
- Generic W31B address and CPU paths provide the old selected-AR read address,
  postincrement and sequential PC only after READY, WAIT resume with the same
  transaction ID and no refetch, FAULT/no-endpoint rollback, CMPT/ARP/AR
  knownness rejection, and checked AR=0xffff rejection. Tests cover both
  forms, all eight ARFs, boundary read values, status-field masking and
  preservation, checked/trusted/dispatch parity, transactional forged-input
  rejection, and accumulator/T preservation. No opcode-specific CPU or
  sequencer production branch was added.
- The first authoritative clean full gate exposed two stale LDU UNKNOWN
  fixtures: `0x1290 ^ 0x2000` is the newly valid ASM base `0x3290`. The
  catalog and decode fixtures now use `0x2008`, yielding the adjacent still-
  unknown `0x3298`, so they retain their overaccept boundary without denying
  the evidenced W37 row. Focused LDU and W37 tests passed after the correction.
- Validation: all four focused catalog, decode, pure-operation, and public-CPU
  targets passed under strict warnings and again in a forced ASan/UBSan build.
  `make quality-test` passed all 186 Python tests plus evidence, knob,
  source-boundary, build-manifest, and observer-disabled artifact checks. Two
  independent Luna reviews found no functional finding; an independent CMake
  configure/build and four-test ctest run also passed. After the stale-fixture
  corrections, the authoritative clean, unsanitized standalone `make test`
  completed successfully.
- Size decision: the new shared production leaf is 283 lines. W37 adds no
  operand layout, so `src/isa/decode/operand_layout.c` remains unchanged at
  535 lines. It remains the cohesive layout-data/resolver owner below the
  800-line split threshold, and the reviewed decision remains not to split it.
- Evidence accounting is now 148 evidence records; 227 requirements (192
  IMPLEMENTED, 8 PLANNED, 27 UNRESOLVED); 183 tests (175 PASSING, 8 PLANNED);
  and 7 source records. ASM repeatability remains unresolved by design.
- No public ABI, timing or performance claim, machine-cycle API, new resource,
  effect kind or address kind, IRQ/RPT policy, device/MMR behavior, knob,
  environment read, logging, diagnostic fanout, allocator, mutable global, or
  emulator dependency was added. Selection of the next ISA slice remains
  evidence-first.

## 2026-09-08 — P2-W38 fixed-MOD2 LD Smem,16 evidence activation

- State: evidence and ownership activation landed as
  `cf0f2d4128b8bfa78f508b9fb7c16b444c3e5c66`. P2-W38 is the active
  bounded ISA slice. P2-W31P hosted stability and the W31B immediate-READY
  benchmark follow-ups remain open and nonblocking.
- TI `SPRU172C` §4.66 printed pp. 4-65–4-69 defines syntax 3
  `LD Smem,16,dst` with diagram `0100_010D_IAAAAAAA`. Composed with the
  registered standard-indirect MOD2 rule, the selected row is
  `0x4490/0xFEF8`: D occupies bit 8, ARF occupies bits 2:0, and the row admits
  exactly sixteen one-word encodings.
- The WORD16 operand is zero-extended when SXM=0 or sign-extended when SXM=1
  to canonical 40 bits, then shifted left 16 within the 40-bit accumulator.
  Frozen boundary vectors include `0x8000` producing `0x0080000000` or
  `0xff80000000`, and `0xffff` producing `0x00ffff0000` or `0xffffff0000`.
  The printed `FEDC`/SXM=1 example produces `0xfffedc0000`. OVM and OVdst
  apply only to the separate SHIFT/ASM syntaxes, so this fixed-16 form
  preserves C, TC, OV, and unrelated state.
- Four planned contracts separate catalog registration, reuse of the existing
  `ACCUMULATOR_ARF` decode layout, a pure SXM-dependent MEMORY completion to
  `ACCUMULATOR_WRITE`, and generic CPU retirement. Existing W31B paths supply
  the old selected-AR read address, postincrement only after READY, WAIT resume
  without refetch, FAULT rollback, CMPT=0/ARP=0 validation, checked AR=0xffff
  rejection, and sequential PC retirement. No new layout, resource, effect,
  address kind, state, CPU branch, public API, or source manifest is required.
- Repeatability remains UNRESOLVED. Table 2-28 absence is not positive
  eligibility evidence, and the separately forbidden long-offset or absolute
  forms do not decide this fixed-MOD2 form. No timing claim is adopted.
- Candidate review deferred STH and LDR because their guard/rounding policies
  remain recorded gaps; `LD #K,dst` lacks a frozen short-immediate widening
  rule; `ST T,Smem` needs a generic T input resource; and `LD Smem,TS,dst`
  needs that resource plus shift-count semantics. Those gaps were not hidden
  behind family-local behavior.
- Expected completed coverage is 33 catalog rows and 1,479 populated first
  words, with reset-runnable coverage unchanged at 6/33. Activation accounting
  is 151 evidence records; 232 requirements (192 IMPLEMENTED, 12 PLANNED,
  28 UNRESOLVED); 187 tests (175 PASSING, 12 PLANNED); and 7 sources.
- Validation: `make quality-test` passed all 186 Python tests plus evidence,
  knob, source-boundary, build-manifest, and observer-disabled artifact checks;
  `git diff --check` passed. The existing load leaf is 241 lines before W38;
  the 535-line operand-layout owner is unchanged and retains its reviewed
  no-split decision.
- No production, generated, or executable-test file changed. No public ABI,
  timing or machine-cycle claim, IRQ/RPT behavior, device/MMR policy, knob,
  environment read, diagnostic path, allocator, mutable global, or emulator
  dependency was added. TDD begins with owner-local RED tests.

## 2026-09-08 — P2-W38 fixed-MOD2 LD Smem,16 implementation

- State: implementation commit
  `27c8df3665e1c086be872f8539b92411407de376` completes the bounded
  fixed-MOD2 `LD Smem,16,dst` vertical slice. The catalog now contains 33
  rows and 1,479 populated first words, with reset-runnable coverage unchanged
  at 6/33. P2-W31P hosted stability and the W31B immediate-READY benchmark
  follow-ups remain open and nonblocking.
- TDD RED was captured in three independently owned lanes. Catalog/decode
  tests failed because the instruction identity was absent; pure-operation
  tests failed because the checked and trusted symbols were absent; and the
  public-CPU test reached `STOP_UNIMPLEMENTED_OPCODE`. No production row or
  handler preceded those failures.
- GREEN registers `LD_SMEM_SHIFT16_MOD2` at `0x4490/0xFEF8`, with D in bit 8,
  ARF in bits 2:0, one program word, `ACCUMULATOR_ARF`, `STATUS_SXM`, exactly
  sixteen encodings, and unresolved repeatability. The handler reads WORD16
  Smem, zero-extends it for SXM=0 or sign-extends it for SXM=1 to canonical
  40 bits, and then logically shifts left 16. Frozen vectors cover zero,
  `0x7fff`, `0x8000`, `0xffff`, and the printed `0xfedc`/SXM=1 example.
  C, TC, OV, and unrelated architectural state are preserved; OVM and OVdst
  do not apply to this fixed-16 form.
- The unique `ld_smem_shift16` checked/trusted binding extends the existing
  `op_ld_smem` leaf through shared validation, SXM widening, and value staging,
  with a separate static completion callback. Existing MEMORY read,
  ACCUMULATOR_WRITE, fixed-MOD2 addressing, and generic CPU WAIT/READY/FAULT
  and sequential-PC paths are reused. No opcode-specific CPU production branch
  or new layout, resource, effect, address, state, API, or source seam was
  introduced.
- Integration corrected four test assumptions exposed by the new row: bit 8
  is the D operand rather than an invalid XOR near-miss; `0x4690` is the valid
  W37 DP sibling so `0x4698` remains the explicit unknown neighbor; the stale
  W37 catalog ID count advanced from 33 to 34; and the aggregate fixed-MOD2
  memory-row count advanced from two to three.
- Validation: all four focused catalog, decode, pure-operation, and public-CPU
  targets passed under strict warnings. Those targets plus the earlier
  `test_ld_smem_ops` and `test_cpu_ld_smem` regressions passed in a forced
  ASan/UBSan build. `make quality-test` passed all 186 Python tests and the
  evidence, knob, source-boundary, build-manifest, and observer-disabled
  artifact checks. Independent Luna semantic and integration reviews found no
  remaining functional issue after the aggregate-count correction. An
  independent CMake configure/build and full CTest passed 178/178, followed by
  a successful authoritative clean standalone `make test`.
- Size decision: `src/isa/ops/load/op_ld_smem.c` is 357 lines and its header is
  44 lines, both below 500. W38 adds no layout code, so
  `src/isa/decode/operand_layout.c` remains 535 lines with its reviewed
  no-split decision in force.
- Evidence accounting is now 151 evidence records; 232 requirements (196
  IMPLEMENTED, 8 PLANNED, 28 UNRESOLVED); 187 tests (179 PASSING, 8 PLANNED);
  and 7 source records. Repeatability remains unresolved by design.
- No public ABI, timing or performance claim, machine-cycle API, IRQ/RPT
  policy, device/MMR behavior, knob, environment read, logging, diagnostic
  fanout, allocator, mutable global, or emulator dependency was added.

## 2026-09-08 — P2-W39 generic T input and fixed-MOD2 ST T activation

- State: evidence and ownership activation landed as
  `2a6790e2b219ef98fcd616570c37d7169a5f534b`. P2-W39 is the active bounded
  ISA slice. P2-W31P hosted stability and the W31B immediate-READY benchmark
  follow-ups remain open and nonblocking.
- TI `SPRU172C` Store T/TRN/immediate (printed pp. 4-166–4-168) defines
  one-word `ST T,Smem` with opcode diagram `1000_1100_IAAAAAAA`, execution
  `(T) -> Smem`, and no status-bit effects. Composing the registered standard
  indirect MOD2 `*ARx+` rule produces row `0x8C90/0xFFF8`: bit 8 is fixed,
  ARF occupies bits 2:0, and exactly eight first words are admitted. The
  printed decrement example independently confirms that the old T value is
  stored at the old selected-AR address before the address update.
- The architecture-fit audit selected the smallest missing shared prerequisite:
  a generic catalog-declared `T_REGISTER` input resource. It will copy raw16 T
  plus explicit knownness from the existing private T owner only when requested.
  Unknown T remains representable in the input snapshot and must cause the pure
  store handler to reject without publishing an effect or starting a bus write.
- The planned `st_t_smem` leaf reuses the existing ARF-only layout, generic
  MEMORY WORD16 WRITE with `complete=NULL`, fixed-MOD2 address resolution, and
  generic CPU WAIT/READY/FAULT and sequential-PC retirement. No new layout,
  effect, address, architectural-state, public-API, or CPU opcode branch seam is
  authorized. Repeatability remains UNRESOLVED because absence from Table 2-28
  is not positive eligibility evidence; no timing claim is adopted.
- Candidate review keeps STH and LDR deferred on saturation/guard/rounding
  policy, `LD #K,dst` on short-immediate widening, and `LD Smem,TS,dst` on the
  additional shift-count contract. Production RPT activation remains gated by
  unresolved RC progression, repeated-body PC flow, and predecode IRQ
  arbitration; no existing pure RPT proof was mistaken for executable CPU
  coverage.
- TDD ownership is frozen into five RED contracts: generic T input collection,
  catalog registration, decode through the existing ARF layout, pure store
  effect, and public-CPU retirement. Expected completed coverage is 34 catalog
  rows, 1,487 populated first words, enum ID_COUNT 35, four aggregate fixed-MOD2
  memory rows, and reset-runnable coverage unchanged at 6/34.
- Activation accounting is 154 evidence records; 238 requirements (196
  IMPLEMENTED, 13 PLANNED, 29 UNRESOLVED); 192 tests (179 PASSING, 13 PLANNED);
  and 7 sources. `make quality-test` passed all 186 Python tests plus evidence,
  knob, source-boundary, build-manifest, and observer-disabled artifact checks;
  `git diff --check` passed.
- No production, generated, or executable-test file changed. No public ABI,
  timing or machine-cycle claim, IRQ/RPT behavior, device/MMR policy, knob,
  environment read, diagnostic path, allocator, mutable global, or emulator
  dependency was added. Implementation begins with owner-local RED tests.

## 2026-09-08 — P2-W39 generic T input and fixed-MOD2 ST T implementation

- State: implementation commit
  `1449255b8e0c4217be77001b7b1e8ebe3230eb6a` completes the generic T-input
  prerequisite and bounded fixed-MOD2 `ST T,*ARx+` vertical slice. Catalog
  coverage is 34 rows and 1,487 populated first words, with reset-runnable
  coverage unchanged at 6/34. P2-W31P hosted stability and the W31B
  immediate-READY benchmark follow-ups remain open and nonblocking.
- TDD RED was captured in five independently owned lanes. The input test failed
  on the absent T resource, source, and snapshot fields; catalog/decode tests
  failed on the absent identity and resource; the pure-operation test failed on
  the absent header; and the public-CPU test could not reach an implemented
  opcode. No catalog row or production handler preceded those failures.
- GREEN adds catalog input-resource bit 32 as `T_REGISTER`, extends the copied
  opcode input with raw16 T plus explicit knownness, and borrows the existing
  private T owner only during collection. Known values `0x0000`, `0x7fff`,
  `0x8000`, and `0xffff` copy exactly. Reset-unknown T produces canonical zero
  with `known=false`; missing requested sources reject transactionally, while
  unrequested T is never required.
- The catalog registers `ST_T_SMEM_MOD2`/`st_t_smem` at `0x8C90/0xFFF8`, with
  fixed bit 8, ARF in bits 2:0, one program word, the existing ARF layout,
  `T_REGISTER` input, eight encodings, and unresolved repeatability. The
  generated lookup remains the sole immutable first-word derivative.
- Checked and trusted store leaves stage the existing generic DATA WORD16
  MEMORY WRITE with exact copied T, MOD2 plus selected ARF, and `complete=NULL`.
  Unknown T returns `UNKNOWN_INPUT` without publishing an effect. Existing
  memory sequencing uses the old selected-AR address, commits postincrement
  and PC+1 only with READY, resumes WAIT under the stable transaction without
  refetch, and rolls back on FAULT, missing endpoint, unknown address inputs,
  or the unresolved AR=`0xffff` boundary. No opcode-specific CPU branch was
  added.
- Integration generalized the aggregate fixed-MOD2 metadata proof because the
  new ARF-only row has eight encodings and fixed bit 8, unlike the three prior
  accumulator rows with sixteen encodings and variable D/S. The full gate also
  exposed two stale W38-era `ID_COUNT == 34` assertions; both now expect 35.
- Validation: all five focused targets and the existing input, STL, and
  immediate-store regressions passed under strict warnings. The W39 targets
  plus existing store operation/CPU regressions passed in a forced ASan/UBSan
  build. `make quality-test` passed all 186 Python tests and the evidence, knob,
  source-boundary, build-manifest, and observer-disabled artifact checks. An
  independent Luna semantic review was clean; integration review found only
  the two stale ID-count fixtures above. A fresh independent CMake build passed
  CTest 183/183, followed by a successful authoritative clean standalone
  `make test`.
- Size decision: the new pure store leaf is 158 lines and its header is 36;
  input collection is 149 lines and CPU execution is 448. W39 adds no operand
  layout, so the existing 535-line layout owner retains its reviewed no-split
  decision. All handwritten files remain below the 500-line review threshold
  except that already-reviewed layout owner.
- Evidence accounting is now 154 evidence records; 238 requirements (201
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 192 tests (184 PASSING, 8 PLANNED);
  and 7 source records. ST T repeatability remains unresolved by design.
- No public ABI, new effect/address/state/layout seam, timing or performance
  claim, machine-cycle API, IRQ/RPT policy, device/MMR behavior, knob,
  environment read, logging, diagnostic fanout, allocator, mutable global, or
  emulator dependency was added.

## 2026-09-08 — P2-W40 authoritative ISA inventory activation

- State: evidence and ownership activation landed as
  `a322d824e0b48432452a1beba01669ae647bfcaa`. P2-W40 is the active
  quality/inventory work piece; no production decode or execution behavior
  changed.
- Two independent Luna audits of TI SPRU172C §§2.1–2.4, Tables 2-1 through
  2-26 (printed pp. 2-2–2-18), converged after correcting wrapped-row
  undercounts on the exact vector
  `[13,13,10,22,6,15,5,5,5,6,5,6,5,2,6,5,5,7,21,14,4,2,4,2,5,12]`:
  205 syntax forms, 126 normalized mnemonic tokens, and 13 parallel bundles.
- The 205 forms are an authoritative SPRU172C summary/source-universe
  checkpoint, not yet an exact common-core denominator. Common/profile,
  optional-operand, and addressing scope remain explicit decisions;
  unresolved candidate forms stay in the provisional denominator and an
  exclusion requires evidence.
- The planned inventory is audit metadata under `config/isa/`, with schema,
  exact-count, catalog-binding, and report contracts. It records
  `COMPLETE`, `SUBSET`, and `UNIMPLEMENTED` coverage separately from
  `COMMON_CANDIDATE`, `PROFILE_ONLY`, `OUT_OF_SCOPE`, and `UNRESOLVED`
  scope. `opcode_catalog.def` remains the sole opcode value/mask authority.
- The current checkpoint is 34/205 = 16.585% catalog-family progress. Nine
  catalog rows are fixed-MOD2 subsets of broader Smem forms, leaving the
  strict complete-form lower bound at 25/205 = 12.195%. The historical
  1,487/65,536 lookup-population ratio is not ISA coverage and will not be
  reported as such.
- Activation accounting is 156 evidence records; 242 requirements (201
  IMPLEMENTED, 12 PLANNED, 29 UNRESOLVED); 196 tests (184 PASSING, 12
  PLANNED); and 7 sources.
- Validation: `make quality-test` passed all 186 Python tests plus evidence,
  knob, source-boundary, build-manifest, and observer-disabled artifact
  checks; the known 535-line operand-layout size-review warning remains
  unchanged. A subsequent evidence-parity check and `git diff --check` also
  passed.
- No runtime source, generated decode, public ABI, timing, machine-cycle,
  IRQ/RPT, device/MMR, knob, environment, diagnostic, allocation, mutable
  global, or emulator dependency changed. Implementation begins with the four
  owner-local RED quality contracts.

## 2026-09-08 — P2-W40 authoritative ISA inventory implementation

- State: implementation commit
  `d56610607a6d77b7f66a94919160d03519c01938` completes the bounded
  instruction-form inventory and coverage-reporting quality work piece. No
  production decoder, opcode, or execution behavior changed.
- TDD RED comprised four owner-local Python test modules. Discovery produced
  four expected import failures because the inventory schema and repository
  checker did not yet exist; production data/tooling followed that failure.
- GREEN adds 205 stable `FORM-T2-XX-YY` records keyed by exact Chapter 2 table
  and row ordinal. The manifest reproduces the pinned 26-table vector, 126
  normalized mnemonic tokens, and 13 parallel bundles without transcribing
  opcode values, masks, or a second decode table. The table/ordinal locator is
  the compact authoritative form identity; full syntax remains in the cited TI
  source.
- Every one of the 34 production catalog symbols binds exactly once. Twenty-
  five forms are `COMPLETE`; nine fixed-MOD2 Smem rows are `SUBSET`; 171 forms
  are explicitly `UNIMPLEMENTED`. Covered forms are `COMMON_CANDIDATE`; the
  other 171 remain scope `UNRESOLVED`, so unevidenced common/profile exclusions
  cannot improve the denominator.
- The pure schema validates exact record shape, stable global/table ordinals,
  form-derived table counts, mnemonic and parallel checkpoints, closed scope
  and coverage states, and bidirectional form/binding consistency. It forbids
  opcode-authority fields and accepts future uppercase partial-addressing
  tokens without requiring validator code changes.
- The repository checker parses `opcode_catalog.def` through the existing sole
  catalog parser and rejects missing or unknown bindings. The report counts
  distinct bound form IDs and complete form states, preventing aliases from
  inflating progress. Make and CTest now invoke the checker explicitly.
- The first semantic review found four future-correctness gaps: hard-coded
  34/25/9 validator counts, no form/binding coverage cross-check, no derived
  table-vector validation, and binding-count rather than distinct-form report
  arithmetic. A dedicated Luna hardening lane fixed each gap and added negative
  regression tests. A separate integration review required the explicit Make
  and CTest checker gates. No review finding remains open.
- Validation: 21 focused inventory tests and the direct CLI passed. The final
  `make quality-test` and authoritative standalone `make test` each passed all
  207 Python checks plus the evidence, knob, source-boundary, build-manifest,
  ISA-inventory, observer-disabled artifact, C unit, and minimal-example gates.
  A fresh CMake configure independently passed the Python discovery and new
  `ti_c54x.quality.isa_inventory` CTest entries (2/2). The known 535-line
  operand-layout size-review warning is unchanged; `git diff --check` passed.
- Evidence accounting is 156 evidence records; 242 requirements (205
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 196 tests (188 PASSING, 8 PLANNED);
  and 7 sources.
- Reported progress is intentionally provisional: 34/205 = 16.585%
  catalog-family coverage and 25/205 = 12.195% strict complete-form coverage.
  The 1,487 populated first words remain a catalog statistic, not an ISA
  denominator.
- No public ABI, runtime source, generated decode, timing, machine-cycle,
  IRQ/RPT, device/MMR, knob, environment, diagnostic, allocation, mutable
  global, or emulator dependency changed. The next evidence-fit ISA slice is
  fixed-MOD2 `LD Smem,TS,dst`; generic Smem expansion is the subsequent
  throughput multiplier for the nine partial forms.

## 2026-09-08 — P2-W41 fixed-MOD2 LD Smem,TS,dst activation

- State: evidence and ownership activation landed as
  `e23767a3ac010967e1b646f83cabe016aafbb725`. P2-W41 is the active ISA work
  piece; no production decode or execution behavior changed.
- TI SPRU172C §4.66 and SPRU131G §§2.3.3 and 4.4 pin the one-word form,
  signed TS direction/range, SXM widening, and shifter bounds. The project
  contract composes the existing fixed-MOD2 `*ARx+` row as value `0x1490`,
  mask `0xFEF8`, and defines raw T[5:0] `0x20..0x2f` as transactionally
  rejected `INVALID_OPERAND` because TI behavior outside the documented TS
  range is not inferred.
- The slice reuses the existing `ACCUMULATOR_ARF` layout, SXM and T resource
  bits, generic MEMORY/ACCUMULATOR_WRITE effects, fixed-MOD2 address resolver,
  and generic CPU retirement. Repeatability remains `UNRESOLVED`.
- Four owner-local RED contracts are planned for catalog, decode, pure load
  effects, and CPU retirement. Implementation must review the load-family file
  size before crossing the 500-line architecture threshold.
- Activation accounting is 161 evidence records; 246 requirements (205
  IMPLEMENTED, 12 PLANNED, 29 UNRESOLVED); 200 tests (188 PASSING, 12
  PLANNED); and 7 sources.
- Validation: `make quality-test` passed all 207 Python tests and every quality
  gate; `git diff --check` passed. The known 535-line operand-layout
  size-review warning remains unchanged.
- Expected closure is 35 catalog rows, 1,503 populated first words, inventory
  family coverage 35/205 = 17.073%, and unchanged strict complete-form lower
  bound 25/205 = 12.195%. Generic Smem MOD0–3 expansion remains the next
  throughput slice.

## 2026-09-08 — P2-W41 fixed-MOD2 LD Smem,TS,dst implementation

- State: implementation commit
  `cd5242bcb3a240fcfed00ddd148545ca94ce9387` completes the bounded W41
  vertical slice.
- Four owner-local RED contracts first failed on the absent opcode/resource
  identities and semantic handler, while the CPU contract stopped on the
  unknown instruction before retirement. Production changes followed those
  failures; no child agent created a commit.
- GREEN registers `LD_SMEM_TS_MOD2` at `0x1490/0xFEF8` with sixteen D/ARF
  encodings, the existing `ACCUMULATOR_ARF` layout, unresolved repeatability,
  and a named composite of the existing SXM and T input bits. The generated
  first-word lookup remains a deterministic derivative of the sole catalog.
- The checked and trusted load leaves validate copied SXM/T input, use only
  T[5:0], sign- or zero-extend the WORD16 under SXM, apply counts 0..31 as
  modulo-40 logical left shifts and counts -16..-1 as arithmetic right shifts,
  and preserve status state through the generic accumulator-write effect.
  High T bits are ignored. The project-unsupported low-six-bit zone 0x20..0x2f
  rejects transactionally before a DATA request and on completion revalidation.
- Generic fixed-MOD2 address resolution and CPU READY/WAIT/FAULT/sequential-PC
  retirement are reused without any CPU production change or opcode-specific
  branch. The read uses the old selected AR and commits postincrement only with
  the completed transaction.
- Inventory coverage advances to 35/205 = 17.073% catalog-family progress,
  comprising 25 complete forms and ten fixed-MOD2 subsets. Strict complete-form
  coverage remains 25/205 = 12.195%; the catalog has 1,503 populated first
  words and ID_COUNT 36. Populated-word ratios are not ISA coverage.
- The load implementation remains cohesive at 491 lines after explicit review,
  below the 500-line split trigger. Any further growth of that leaf requires a
  split rather than another in-place family addition.
- Validation: the four focused W41 tests and existing unshifted/shift16 load
  regressions passed under strict flags; focused CMake ASan/UBSan passed 4/4;
  `make quality-test` passed all 207 Python tests plus evidence, knob,
  source-boundary, manifest, inventory, and observer-off checks; and the final
  standalone `make test` passed. The known 535-line operand-layout review
  warning remains unchanged, and `git diff --check` passed.
- Independent Luna semantic and integration reviews are clean. Review-found
  evidence-state overreach, a missing composite-token schema case, completion
  invalid-count coverage, and one 171/170 documentation mismatch were all
  corrected before commit.
- Evidence accounting is 161 records; 246 requirements (209 IMPLEMENTED,
  8 PLANNED, 29 UNRESOLVED); 200 tests (192 PASSING, 8 PLANNED); and 7 sources.
  No new public ABI, resource bit, effect/address/layout/state kind, timing,
  machine-cycle, IRQ/RPT, device/MMR, knob, environment, diagnostic, allocator,
  mutable-global, or emulator dependency was added. Generic Smem MOD0–3
  expansion is the next throughput slice.

## 2026-09-08 — P2-W42A standard-indirect MOD0–3 seam activation

- State: evidence and ownership activation landed as
  `c06098b667285be40a27a7f49c8650ed1a771af0`. P2-W42A is the active generic
  memory-transaction work piece; the catalog and opcode handlers remain
  unchanged.
- The project contract extends the existing `SINGLE_INDIRECT_WORD16` seam
  across MOD0 through MOD3. Effect validation owns structural mode/access/
  width/callback/write-value shape, including rejection of MOD3 reads before
  address resolution or endpoint issue. Address resolution owns MOD0 `NONE`,
  MOD1/MOD2 `POST`, and write-only MOD3 `PRE` update metadata.
- `PRE` records effective-address ordering; it does not expose an architectural
  AR mutation before the root transaction reaches `READY`. `WAIT`, `FAULT`,
  and cancel preserve architecture, while `READY` commits any target AR update
  atomically with accepted program-counter and effect commits.
- Three owner-local RED contracts are planned for the memory effect, address
  resolver, and root memory-instruction transaction. W42A adds no public ABI,
  catalog row, opcode handler, effect/address/state kind, CPU opcode branch,
  timing, device/MMR, knob, diagnostic, allocation, or emulator dependency.
- Activation accounting is 162 evidence records; 249 requirements (209
  IMPLEMENTED, 11 PLANNED, 29 UNRESOLVED); 203 tests (192 PASSING, 11
  PLANNED); and 7 sources.
- Validation: the evidence checker passed with 249 requirements and 203 tests;
  `make quality-test` passed all 207 Python tests and every quality gate; and
  `git diff --check` passed. The known 535-line operand-layout size-review
  warning remains unchanged.
- W42B remains staged behind this seam. Its catalog design will use neutral
  symbols and explicit MOD-bearing layouts with masks `0xFEE0` and `0xFFE0`.
  The projected lookup change is 1,503 to 1,863 populated first words: 272 new
  legal encodings and 88 load MOD3 encodings that handlers must reject before
  a bus request. Inventory coverage remains 35/205 = 17.073% catalog-family
  and 25/205 = 12.195% strict complete-form until those forms are implemented.

## 2026-09-08 — P2-W42A standard-indirect MOD0–3 seam implementation

- State: implementation commit
  `ef099cf26972a08837d7bb0619da54ee6bc91791` completes the generic W42A
  memory-transaction seam. The production opcode catalog and handlers remain
  unchanged.
- Three owner-local RED contracts first failed at the existing fixed-MOD2
  gates: MOD0 effect validation, MOD0 address resolution, and MOD0 root
  transaction preparation. Production changes followed those failures; no
  child agent created a commit.
- The existing `SINGLE_INDIRECT_WORD16` effect now admits reads through MOD0,
  MOD1, and MOD2 and writes through MOD0 to MOD3. MOD3 reads remain
  structurally invalid before address resolution or endpoint issue. Existing
  access, write-value, completion-callback, ARF, kind, and enum-range shape
  validation remains allocation-free and value-owned.
- The generic resolver continues to use the existing checked single-indirect
  planner. MOD0 requests the old address and stages no AR commit; MOD1 and
  MOD2 request the old address with decrement/increment `POST` commits; MOD3
  writes request the incremented address with a `PRE` commit descriptor. Mode
  boundaries reject without producing candidates or mutating state.
- The root transaction validates those mode-specific candidates before issue.
  `WAIT`, `FAULT`, and cancel preserve AR, PC, and ordinary effects; `READY`
  commits the selected AR only when an AR target exists and publishes PC and
  effects under the existing atomic transaction policy. `PRE` remains ordering
  metadata and is not exposed as a live AR mutation during `WAIT`. The
  externally visible write-READY path still performs only infallible scalar
  publication after the endpoint accepts the write.
- `memory_instruction_transaction.c` is 523 lines. Independent review found
  the root transaction cohesive and explicitly retains it unsplit for this
  work piece. Candidate validation must split if growth approaches roughly
  700 lines and the file must be split before 800 lines.
- Validation: six focused new/existing effect, resolver, and transaction tests
  passed under strict flags; a fresh ASan/UBSan CMake build passed the three
  W42A CTests (3/3); `make quality-test` passed all 207 Python tests and every
  quality gate; and the authoritative standalone `make test` passed all C
  tests and the minimal example. Evidence, build-manifest, and diff checks are
  green. The known 535-line operand-layout warning and the reviewed 523-line
  transaction warning are the only size-review warnings.
- Independent Luna semantic and integration reviews found no blocking issue.
  Existing fixed-MOD2 handlers and CPU memory lifecycle/pending regressions
  remain green.
- Evidence accounting is 162 records; 249 requirements (212 IMPLEMENTED,
  8 PLANNED, 29 UNRESOLVED); 203 tests (195 PASSING, 8 PLANNED); and 7 sources.
  No public ABI, catalog identity, generated decode, opcode handler, CPU opcode
  branch, effect/address/state kind, timing, machine-cycle, IRQ/RPT,
  device/MMR, knob, environment, diagnostic, allocation, mutable-global, or
  emulator dependency was added.
- Inventory coverage therefore remains 35/205 = 17.073% catalog-family and
  25/205 = 12.195% strict complete-form. W42B is the next throughput slice:
  bind the ten existing partial Smem families to explicit MOD0–3 operand
  layouts and reuse this seam, including pre-bus rejection of load MOD3.

## 2026-09-08 — P2-W42B standard-indirect Smem expansion activation

- State: evidence and ownership activation landed as
  `65955fcdaf0a9ab2a377980e4ff568be1eca7884`. P2-W42B is the active bounded
  fan-out across the ten existing fixed-MOD2 Smem families.
- The catalog contract replaces those ten rows rather than adding identities.
  Neutral symbols drop `_MOD2`; five D/source rows use their exact MOD0 bases
  with mask `0xFEE0`, and five no-D rows use mask `0xFFE0`. `ID_COUNT` remains
  36. Three explicit layouts append decoded MOD bits [4:3] while preserving
  existing D/source, ARF, and extension operand indices; handlers may not parse
  raw instruction words.
- Seven load families accept MOD0 through MOD2 and reject MOD3 as
  `INVALID_OPERAND` before producing a MEMORY effect or endpoint request.
  Three store families accept MOD0 through MOD3 through the completed W42A
  transaction seam.
- The projected lookup grows from 1,503 to 1,863 populated first words. The ten
  broadened rows cover 480 words: 392 legal and 88 known-but-rejected load
  MOD3 encodings, an increment of 272 legal plus 88 rejected words.
- Inventory symbols become neutral, but all ten forms remain `SUBSET` with
  `STANDARD_INDIRECT_MOD0_MOD3`; other Smem addressing forms remain outside
  W42B. Coverage therefore stays 35/205 = 17.073% catalog-family and 25/205 =
  12.195% strict complete-form.
- Five owner-local RED contracts are planned for catalog metadata,
  decode/layout, load effects, store effects, and generic CPU integration.
  `op_ld_smem.c` is already 491 lines and must split before handler growth.
- Activation accounting is 163 evidence records; 254 requirements (212
  IMPLEMENTED, 13 PLANNED, 29 UNRESOLVED); 208 tests (195 PASSING, 13
  PLANNED); and 7 sources.
- Validation: `make quality-test` passed all 207 Python tests and every quality
  gate; the evidence checker passed with 254 requirements and 208 tests; and
  `git diff --check` passed. The reviewed 535-line operand-layout and 523-line
  root-transaction warnings remain unchanged.
- No public ABI, effect/address/state kind, opcode-specific CPU branch, timing,
  machine-cycle, IRQ/RPT, device/MMR, knob, environment, diagnostic,
  allocation, mutable-global, or emulator dependency is admitted.

## 2026-09-08 — P2-W42B standard-indirect Smem expansion implementation

- State: implementation commit
  `42a0b1814f7038f8617287a39bb91e0ff8d0921a` completes the bounded W42B
  fan-out across the ten existing Smem families.
- Five owner-local RED contracts first failed on the fixed-MOD2 catalog,
  layouts, handlers, and CPU behavior. Production changes followed those
  failures; no child agent created a commit.
- The ten catalog rows now have neutral symbols and explicit MOD-bearing
  layouts while retaining `ID_COUNT=36`. Handlers consume decoded MOD fields:
  all seven loads admit MOD0 through MOD2 and reject MOD3 as
  `INVALID_OPERAND` before a MEMORY effect or endpoint request, while all
  three stores admit MOD0 through MOD3 through the W42A transaction seam.
- The generated direct lookup contains 1,863 populated first words. The ten
  broadened rows cover 480 words: 392 legal and 88 known-but-rejected load
  MOD3 words, an increment of 272 legal plus 88 rejected encodings. The
  generated artifact remains derived from the sole catalog source.
- Inventory bindings remain `SUBSET` with
  `STANDARD_INDIRECT_MOD0_MOD3`; catalog-family progress remains 35/205 =
  17.073% and strict complete-form coverage remains 25/205 = 12.195% because
  other Smem addressing forms remain outside this slice.
- The load owner was split before growth: `op_ld_smem.c` is approximately 393
  lines and the shared `op_ld_smem_common.c` helper approximately 145 lines.
  Independent review retains the 577-line `operand_layout.c` as one cohesive
  descriptor table for this slice; it must split before further growth near
  800 lines. The reviewed 523-line root memory transaction is unchanged.
- Validation: all five focused W42B CTests passed in a fresh ASan/UBSan build;
  `make quality-test` passed all 207 Python tests and every evidence, knob,
  source-boundary, manifest, inventory, and observer-off gate; the
  authoritative standalone `make test` passed all C tests and the minimal
  example; generator provenance and `git diff --check` passed.
- Independent Luna semantic and integration reviews found no P0/P1/P2
  semantic defect. The final integration review also executed all five W42B
  binaries and confirmed catalog/generated counts, neutral-ID migration,
  handler transactionality, manifest registration, and evidence parity.
- Evidence accounting is 163 records; 254 requirements (217 IMPLEMENTED,
  8 PLANNED, 29 UNRESOLVED); 208 tests (200 PASSING, 8 PLANNED); and 7
  sources. No public ABI, effect/address/state kind, opcode-specific CPU
  branch, timing, machine-cycle, IRQ/RPT, device/MMR, knob, environment,
  diagnostic, allocation, mutable-global, or emulator dependency was added.

## 2026-09-08 — P2-W43A indexed WORD16 memory seam activation

- State: evidence and ownership activation landed as
  `152738bfe5b8c7e04dfc4d80e1b7b936026b6904`. P2-W43A is the next generic
  memory-transaction work piece; it adds no catalog row or opcode handler.
- The planned private `INDEXED_AR0_WORD16` address kind carries an explicit
  MOD5/MOD6 discriminator and admits WORD16 reads and writes. Resolution will
  reuse the existing indexed planner after reading CMPT, ARP, selected AR, and
  AR0 from private architectural state; no new operation-input resource or
  public CPU snapshot is introduced.
- MOD5 uses the old selected AR as the operand address and stages selected-AR
  subtraction by AR0 after access; MOD6 stages addition. Underflow, overflow,
  and selected-AR0 alias reject, AR0 never becomes a commit target, and no wrap
  behavior is claimed.
- Three owner-local RED contracts are planned for the generic MEMORY effect,
  indexed resolver, and root transaction. WAIT, FAULT, and cancel must preserve
  architecture; READY must validate both copied AR snapshots before publishing
  the selected-AR, PC, and effect commits atomically.
- The 523-line root memory transaction may not absorb unbounded mode-specific
  validation: W43A must split candidate validation before or with implementation
  if this seam would grow the monolith. The indexed resolver receives the same
  implementation-start size review.
- W43B remains blocked pending primary-source per-family MOD5/MOD6 encoding
  composition. Its possible 20-row, ID_COUNT 36→56, and lookup 1,863→2,103
  figures are projections only and are not catalog, evidence, or coverage
  claims.
- Activation accounting is 163 evidence records; 257 requirements (217
  IMPLEMENTED, 11 PLANNED, 29 UNRESOLVED); 211 tests (200 PASSING, 11
  PLANNED); and 7 sources. `make quality-test` passed all 207 Python tests and
  every quality gate; the evidence checker and `git diff --check` passed.
- No public ABI, catalog identity, handler, CPU opcode branch, timing,
  machine-cycle, IRQ/RPT, device/MMR, knob, environment, diagnostic,
  allocation, mutable-global, wrap, or emulator dependency is admitted.

## 2026-09-08 — P2-W43A indexed WORD16 memory seam implementation

- State: implementation commit
  `19119a3500b75741cf4ba27fb4fdd23f5c6296fe` completes the generic W43A
  indexed-memory prerequisite without adding a catalog row or handler.
- Three owner-local RED contracts first failed on the absent indexed MEMORY
  kind and typed mode field. Production changes followed those failures; no
  child agent created a commit.
- The private effect carrier now distinguishes `INDEXED_AR0_WORD16` and carries
  typed MOD5/MOD6 state alongside the unchanged MOD0–3 single-indirect field.
  Indexed reads and writes require AR1–AR7 and preserve existing access-specific
  completion/write-value shape validation.
- Resolution reads CMPT, ARP, selected AR, then AR0 and reuses the existing
  indexed planner. MOD5 and MOD6 request the old selected-AR address and stage
  one POST selected-AR update; alias, unknown state, underflow, and overflow
  reject with zero output. AR0 is never a commit target.
- Candidate validation was extracted from the root transaction into a focused
  private owner. Live re-resolution compares both copied ARx and AR0 snapshots,
  so WAIT/FAULT/cancel preserve state and resumed READY rejects stale inputs
  before atomically publishing the selected AR, PC, and effect commits.
- The root transaction shrank from 523 to 335 lines; the new candidate validator
  is 268 lines and the resolver 169. Only the previously reviewed 577-line
  operand-layout warning remains.
- Validation: the three focused tests passed in a fresh ASan/UBSan build (3/3);
  `make quality-test` passed all 207 Python tests and every quality gate; the
  authoritative post-closure `make test` passed all standalone binaries and the
  minimal example; build manifests, evidence, inventory, observer-off artifact,
  source boundaries, and `git diff --check` are green.
- Independent Luna semantic review found no P0/P1/P2 issue and confirmed typed
  validation, arithmetic boundaries, stale-snapshot handling, transaction
  atomicity, file ownership, and the split decision.
- Evidence accounting is 163 records; 257 requirements (220 IMPLEMENTED,
  8 PLANNED, 29 UNRESOLVED); 211 tests (203 PASSING, 8 PLANNED); and 7 sources.
  Inventory coverage remains 35/205 = 17.073% catalog-family and 25/205 =
  12.195% strict complete-form. W43B remains blocked on primary per-family
  MOD5/MOD6 encoding composition.
- No public ABI, catalog identity, opcode handler, CPU opcode branch, timing,
  machine-cycle, IRQ/RPT, device/MMR, knob, environment, diagnostic,
  allocation, mutable-global, wrap, or emulator dependency was added.

## 2026-09-08 — P2-W43B indexed Smem expansion activation

- State: evidence and ownership activation landed as
  `05646cf721edaca4a03ff8464b2c02b70d07a217`. Primary TI evidence now
  unblocks the bounded MOD5/MOD6 fan-out across the ten existing Smem forms.
- SPRU131G Figure 5-7/Table 5-3 fixes I bit 7, MOD bits 6:3, and ARF bits 2:0;
  Table 5-4 fixes MOD5/MOD6. The ten SPRU172C opcode diagrams compose those
  fields into 20 disjoint rows with exact `0xFEF8` or `0xFFF8` masks.
- The existing ten MOD0–3 identities remain. Twenty new `_MOD5`/`_MOD6`
  identities use unique handler tokens and thin checked/trusted wrappers over
  family cores; catalog handler-token uniqueness is not relaxed.
- Three appended private four-bit-MOD layouts preserve all existing operand
  indices. The W43A indexed MEMORY seam owns ARx/AR0 resolution and atomic
  retirement; handlers consume decoded modes and add no CPU opcode branch.
- The frozen catalog projection is ID_COUNT 36→56 and populated first-word
  lookup 1,863→2,103, exactly +240 legal words. Inventory evolves to explicit
  multi-binding under `STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6` while deduplicated
  coverage remains 35/205 = 17.073% family and 25/205 = 12.195% strict.
- Six owner-local RED lanes are planned for catalog, decode, load effects,
  store effects, CPU integration, and inventory multi-binding quality.
- Activation accounting is 165 evidence records; 263 requirements (220
  IMPLEMENTED, 14 PLANNED, 29 UNRESOLVED); 217 tests (203 PASSING, 14
  PLANNED); and 7 sources. `make quality-test`, the evidence checker, inventory
  checker, manifests, source boundaries, observer-off check, and
  `git diff --check` passed.
- No public ABI, new effect/address/state kind, opcode-specific CPU branch,
  timing, machine-cycle, IRQ/RPT, device/MMR, knob, environment, diagnostic,
  allocation, mutable-global, wrap, or emulator dependency is admitted.

## 2026-09-08 — P2-W43B indexed Smem expansion implementation

- State: implementation commit
  `e3d135883f8ee45c19abccb4f5be0cabfd901636` completes the bounded MOD5/MOD6
  fan-out across the ten existing indexed Smem families.
- The catalog adds twenty private MOD5/MOD6 rows, taking `ID_COUNT` to 56 and
  the populated first-word lookup to 2,103. Inventory carries 55 catalog
  bindings; deduplicated coverage remains 35/205 = 17.073% catalog-family and
  25/205 = 12.195% strict complete-form coverage.
- All six owner-local RED lanes are GREEN: catalog, decode/layout, indexed
  load effects, indexed store effects, CPU integration, and inventory
  multi-binding. The implementation retained the W43A indexed MEMORY seam,
  used pure TS prevalidation before the endpoint, and corrected stale W42B and
  legacy `ID_COUNT` regression fixtures. No new ABI, effect/address/state
  kind, CPU branch, timing, or ISA scope was added.
- File-size review retains `src/isa/decode/operand_layout.c` at 620 lines as
  cohesive, with a required split before 800 lines. The active owners remain
  below the review threshold: `op_ld_smem.c` 393 lines,
  `op_ld_smem_common.c` 308, `op_ld_smem_indexed.c` 317, and
  `op_store_indexed_common.c` 243.
- Independent semantic and integration reviews were clean. A fresh ASan/UBSan
  configure/build passed all five focused CTests (5/5) without diagnostics;
  the inventory Python lane passed 3/3. The authoritative `rtk make test`
  gate passed all C tests plus quality, evidence, inventory, and observer-off
  checks, including 210 Python tests.
- Closure accounting is 165 evidence records; 263 requirements (226
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); and 217 tests (209 PASSING, 8
  PLANNED). Follow-up work remains evidence-first; no timing, device/MMR,
  IRQ/RPT, wrap, public-ABI, or broader scope claim is implied.

## 2026-09-08 — P2-W44A generic circular WORD16 MEMORY seam activation

- State: evidence and ownership activation landed in commit
  `c073a2bbcbcfdeeea394f915e0b42997b5447f3f`. W44A is an evidence-first
  generic circular WORD16 MEMORY seam; no catalog row or opcode handler is
  activated.
- One immutable project FIT/ownership record and three planned project
  requirements/tests freeze a private typed `CIRCULAR_WORD16` MOD8/MOD10
  specification for READ and WRITE. The operation-layer spec carries copied
  ARF plus typed circular mode; the resolved address effect carries copied
  AR/BK snapshots. Selected AR0–AR7 are admitted.
- Dependency order is CMPT, ARP, selected AR, then BK. The existing circular
  planner/application contracts own unknown/stale rejection, BK=0 valid
  `NONE`/no-update behavior, and the positive-BK wrap domain. WAIT, READY,
  FAULT, and cancel remain transactional; READY commits selected AR, PC, and
  MEMORY effect atomically.
- W44B remains non-activated, projected at `ID_COUNT=76`, 2,343 populated
  first-word lookup entries, and 75 bindings. No catalog, public ABI, timing,
  MMR/device, IRQ/RPT, or broader ISA scope is included in W44A.
- Quality validation covered 210 Python tests, and an independent semantic /
  integration review found no P0/P1/P2 issue. Closure accounting is 166
  evidence records; 266 requirements (226 IMPLEMENTED, 11 PLANNED, 29
  UNRESOLVED); and 220 tests (209 PASSING, 11 PLANNED). Follow-up remains
  evidence-first; central-file size review and split decisions precede any
  implementation growth.

## 2026-09-08 — P2-W44A generic circular WORD16 MEMORY seam implementation

- State: implementation commit
  `eca273bd805cc91efb37e6d7c4dde75d9d8f18f2` completes the bounded W44A
  generic circular WORD16 MEMORY seam without activating catalog rows or
  opcode handlers.
- All three owner-local RED lanes are GREEN: circular memory-effect shape,
  circular address resolution, and circular memory transaction. The operation
  layer carries only copied ARF and typed MOD8/MOD10 mode; the resolved address
  effect carries copied AR/BK snapshots. Selected AR0–AR7 are admitted, with
  strict CMPT, ARP, selected AR, then BK dependency order. BK=0 is valid
  `NONE`/no-update; BK>0 stages the documented POST-access wrap update.
  Unknown or stale live-state snapshots reject, and WAIT, READY, FAULT, and
  cancel remain transactional with atomic READY publication of selected AR,
  sequential PC, and MEMORY effect. The existing root transaction source is
  unchanged.
- The structural module-contract correction keeps ownership boundaries explicit
  for the memory effect, resolver, and candidate validator; no public ABI,
  catalog identity, timing, MMR/device, IRQ/RPT, or broader ISA scope was
  added. Current owner sizes are `memory_effect.c` 78 lines,
  `address_resolution.c` 222 lines,
  `memory_instruction_candidate_validate.c` 316 lines, and
  `memory_instruction_transaction.c` 335 lines unchanged. No split is needed;
  review the candidate validator before it reaches 500 lines.
- Catalog and inventory are unchanged at `ID_COUNT=56`, lookup 2,103, and 55
  bindings; coverage remains 35/205 = 17.073% catalog-family and 25/205 =
  12.195% strict complete-form.
- Fresh ASan/UBSan validation passed all eleven focused CTests (11/11) without
  diagnostics. Independent semantic and integration review found no P0/P1/P2
  issue. The authoritative `rtk make test` gate passed all C tests, quality
  checks, evidence/inventory/observer checks, and all 210 Python tests.
- Closure accounting is 166 evidence records; 266 requirements (229
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); and 220 tests (212 PASSING, 8
  PLANNED). W44B remains the next evidence-first work: a projected twenty-row
  MOD8/MOD10 fan-out at `ID_COUNT=76`, lookup 2,343, and 75 bindings.

## 2026-09-08 — P2-W44B circular MOD8/MOD10 per-family composition activation

- State: evidence and ownership activation landed in commit
  `75da0812324312d1b88a4be269ae5028400a0478`. W44B is an evidence-first
  twenty-row MOD8/MOD10 fan-out over the ten existing Smem families; no catalog
  implementation or handler is activated by this entry.
- Two immutable records were added: one TI_PRIMARY composition synthesis from
  Figure 5-7/Table 5-3, Table 5-4, and the ten registered SPRU172C diagrams;
  one PROJECT_CONTRACT_ONLY fit/ownership freeze. The exact rows are:
  `LD_SMEM_MOD8/10` `0x10C0/0x10D0` mask `0xFEF8`,
  `LD_SMEM_TS_MOD8/10` `0x14C0/0x14D0` mask `0xFEF8`,
  `STL_SMEM_MOD8/10` `0x80C0/0x80D0` mask `0xFEF8`,
  `LDU_SMEM_MOD8/10` `0x12C0/0x12D0` mask `0xFEF8`,
  `LD_T_SMEM_MOD8/10` `0x30C0/0x30D0` mask `0xFFF8`,
  `ST_IMMEDIATE_SMEM_MOD8/10` `0x76C0/0x76D0` mask `0xFFF8`,
  `LD_DP_SMEM_MOD8/10` `0x46C0/0x46D0` mask `0xFFF8`,
  `LD_ASM_SMEM_MOD8/10` `0x32C0/0x32D0` mask `0xFFF8`,
  `LD_SMEM_SHIFT16_MOD8/10` `0x44C0/0x44D0` mask `0xFEF8`, and
  `ST_T_SMEM_MOD8/10` `0x8CC0/0x8CD0` mask `0xFFF8`; tokens use `_mod8` and
  `_mod10` suffixes.
- Six project requirements/tests are planned for catalog, decode, seven load
  effects, three store effects, CPU integration, and inventory multi-binding.
  The projection is `ID_COUNT=76`, lookup 2,343, and 75 bindings. Inventory
  remains audit-only under
  `STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6_MOD8_MOD10`; family and strict
  coverage remain 35/205 = 17.073% and 25/205 = 12.195%.
- W44B reuses the W44A CIRCULAR_WORD16 effect/resolver/transaction seam and
  the existing `ACCUMULATOR_ARF_MOD4`, `ARF_MOD4`, and
  `ARF_EXTENSION16_MOD4` layouts. CMPT=0 resolution admits AR0–AR7 in strict
  CMPT→ARP→selected AR→BK order; BK=0 is valid `NONE`/no-update and BK>0 is
  the positive-BK POST-access wrap update. No `operand_layout.c` edit is
  planned. A new status-load circular file and new store circular common
  helper are preferred splits; catalog and inventory edits remain serialized.
- Unsupported scope remains MOD9/MOD11, MOD14, CMPT=1/dual-indirect, LONG32
  circular scaling, unselected boundary policy, timing/repeatability, BK
  MMR/device routing, public ABI, new architectural kinds, IRQ/RPT, and other
  families. Quality baseline is 210 Python tests; independent semantic and
  catalog-collision review is clean for the bounded activation.
- Activation accounting is 168 evidence records; 272 requirements (229
  IMPLEMENTED, 14 PLANNED, 29 UNRESOLVED); 226 tests (212 PASSING, 14
  PLANNED); and 7 sources. Follow-up remains evidence-first until all six RED
  lanes are implemented and their focused runners are GREEN.

## 2026-09-08 — P2-W44B circular MOD8/MOD10 per-family composition implementation

- State: implementation commit
  `2e925b8ea0bc7702511f5ac0aa65632e2fc7a54c` completes the bounded W44B
  twenty-row MOD8/MOD10 fan-out over the ten existing Smem families.
- RED replay against activation `75da0812324312d1b88a4be269ae5028400a0478`
  reproduced missing MOD8/MOD10 IDs and handlers, a CPU behavioral failure,
  and the inventory mismatch `55 != 75`. GREEN implementation scope is exactly
  twenty rows, `ID_COUNT=76`, 2,343 populated lookup entries, and 75 catalog
  bindings. The ten-family inventory uses the combined
  `STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6_MOD8_MOD10` token without changing
  family or strict coverage (35/205 = 17.073% and 25/205 = 12.195%).
- Circular typed WORD16 semantics remain bounded to CMPT=0, selected AR0–AR7,
  and strict CMPT→ARP→selected AR→BK dependency order. BK=0 is valid
  `NONE`/no-update; BK>0 stages the positive-BK POST-access wrap update. The
  checked and trusted paths reuse the W44A effect/resolver/transaction seams,
  and CPU retirement remains generic and transactional.
- Corrected manifest ordering and module-contract labels, plus legacy
  catalog/decode `0x0040` circular-sibling fixtures and evidence-index
  aggregate counts. The common load and LD_T implementation is 472 lines and
  must not grow without a split; the original status-load implementation is
  482 lines and the circular path is split; `operand_layout.c` remains 620
  lines under the prior split-before-800 decision.
- Targeted W44B validation is GREEN across catalog, decode, load effects,
  store effects, CPU integration, and inventory (five focused C lanes plus the
  Python inventory lane). Fresh ASan/UBSan validation passed 9/9 focused tests
  without diagnostics. Semantic review found no P0–P3 issue (12/12 review
  checks) and quality review passed 26/26. Integration review is clean after
  one stale documentation-count finding was corrected. `quality-test` passed
  213 Python tests; evidence, inventory, manifests, source boundaries, and
  observer-off checks are green, and the final authoritative `rtk make test`
  gate is green.
- Closure accounting is 168 evidence records; 272 requirements (235
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 226 tests (218 PASSING, 8
  PLANNED); and 7 sources. Unsupported scope remains MOD9/MOD11, MOD14,
  CMPT=1/dual-indirect, LONG32 circular scaling, unselected boundary policy,
  timing/repeatability, BK MMR/device routing, public ABI, new architectural
  kinds, IRQ/RPT, and broader family expansion. Next work remains
  evidence-first: reverse-carry MOD4/MOD7 activation/audit.

## 2026-09-08 — P2-W45A generic reverse-carry WORD16 MEMORY seam activation

- State: activation commit
  `24c5afb80e3641b34aff267b66c11cb615749ec6` records the evidence-first W45A
  ownership slice. Existing reverse-carry arithmetic, planner, effect, and
  application contracts are complete; the missing seams are the generic
  MEMORY kind, address resolution, and candidate validation.
- W45A freezes typed MOD4/MOD7 `REVERSE_CARRY_WORD16` behavior with ARF 1..7;
  AR0 aliasing remains unresolved and is rejected. Dependency order is
  CMPT→ARP→selected AR→AR0, with known nonzero one-hot AR0. The old selected
  AR is the operand address and only that selected AR receives the POST update.
  Boundary propagation that escapes the documented domain rejects; no BK,
  modulo, or wrap inference is added.
- Transactionality requires exact live snapshots, WAIT/FAULT/cancel and stale
  rejection, no refetch, and atomic READY publication. Three planned RED
  runners are registered at
  `tests/unit/isa/ops/memory/test_reverse_carry_word16_memory_effect.c`,
  `tests/unit/sequencer/memory/test_reverse_carry_word16_address_resolution.c`,
  and
  `tests/unit/sequencer/instruction/test_reverse_carry_word16_memory_transaction.c`.
  No root transaction, CPU, catalog/opcode, public ABI, timing, device, or RPT
  change is activated.
- W45B is audited but not activated: twenty projected MOD4/MOD7 rows at
  `ID_COUNT=96`, lookup 2,583, and 95 bindings, with coverage unchanged.
  Activation accounting is 169 evidence records; 275 requirements (235
  IMPLEMENTED, 11 PLANNED, 29 UNRESOLVED); 229 tests (218 PASSING, 11
  PLANNED); and 7 sources.
- Activation `rtk make test` and evidence/diff checks are green. Follow-up
  remains evidence-first until the three W45A RED lanes are implemented.

## 2026-09-08 — P2-W45A generic reverse-carry WORD16 MEMORY seam implementation

- State: implementation commit
  `c2b4920e006f465c6ecfabb28ed864735700c3d8` completes the bounded W45A
  reverse-carry MEMORY seam.
- Three REDs were replayed: effect compilation failed on the absent enum/typed
  field, a valid resolver case was rejected, and transaction prepare rejected.
  GREEN adds the private kind and typed field with canonical validation,
  ordered CMPT→ARP→selected AR→AR0 resolution, one-hot nonzero AR0, ARF 1..7
  admission with AR0 alias rejection, old-address POST update to selected AR
  only, boundary rejection, and candidate exact source/live/commit handling.
  No BK, root transaction, CPU, catalog/opcode, public ABI, timing, device, or
  RPT changes were made.
- Review fixes include the exact “Does not own” contract token, resolver
  nonzero sentinel and canonical-zero handling, rejection of forged
  `address_commit.value_after` without endpoint/state change, and the
  `address_resolution.h` module contract.
- Combined three focused runners are GREEN. Fresh sanitizer validation passed
  15/15 without diagnostics. Semantic review found no production defects after
  P1/P2 test fixes; integration review passed 18/18 with no runtime findings
  after the header-contract correction. `quality-test` passed 213 Python tests;
  evidence, manifests, source boundaries, observer-off, and inventory checks
  are green; the final authoritative `rtk make test` gate is green.
- Current sizes are `memory_effect.c` 95 lines, `memory_effect.h` 76,
  `address_resolution.c` 293, `memory_instruction_candidate_validate.c` 354,
  the related headers with contracts, and tests at 219/411/426 lines.
  Closure accounting is 169 evidence records; 275 requirements (238
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 229 tests (221 PASSING, 8
  PLANNED); and 7 sources.
- W45B remains audited but not activated: twenty projected MOD4/MOD7 rows at
  `ID_COUNT=96`, lookup 2,583, and 95 bindings; AR0 remains excluded and
  coverage is unchanged. `git diff --check` and append-only numstat are clean;
  no commit was created by this ledger update.

## 2026-09-08 — P2-W45B reverse-carry MOD4/MOD7 per-family composition activation

- State: activation commit
  `2251fb03dfa9dad9d365555897bf94ce4ab98c77` records the evidence-first
  twenty-row reverse-carry fan-out over the ten existing Smem families.
- The exact rows and masks are: `LD_SMEM_MOD4/7` `0x10A0/0x10B8`
  `0xFEF8`; `LD_SMEM_TS_MOD4/7` `0x14A0/0x14B8` `0xFEF8`;
  `STL_SMEM_MOD4/7` `0x80A0/0x80B8` `0xFEF8`; `LDU_SMEM_MOD4/7`
  `0x12A0/0x12B8` `0xFEF8`; `LD_SMEM_SHIFT16_MOD4/7` `0x44A0/0x44B8`
  `0xFEF8`; `LD_T_SMEM_MOD4/7` `0x30A0/0x30B8` `0xFFF8`;
  `ST_IMMEDIATE_SMEM_MOD4/7` `0x76A0/0x76B8` `0xFFF8` (two words);
  `LD_DP_SMEM_MOD4/7` `0x46A0/0x46B8` `0xFFF8`; `LD_ASM_SMEM_MOD4/7`
  `0x32A0/0x32B8` `0xFFF8`; and `ST_T_SMEM_MOD4/7` `0x8CA0/0x8CB8`
  `0xFFF8`. Handler tokens use `_mod4`/`_mod7` suffixes.
- W45B reuses the typed W45A reverse-carry seam: CMPT=0/ARP=0, known
  nonzero one-hot AR0, old selected-AR address, and POST update to selected AR
  only. ARF1..7 executes; ARF0 decodes but is transactionally rejected before
  endpoint issue because alias semantics are unresolved. No BK or wrap
  inference is admitted. Inventory uses
  `STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`; projected
  rows are 95 with `ID_COUNT=96`, lookup 2,583, and 95 bindings. Coverage is
  unchanged at 35/205 family and 25/205 strict.
- Six RED paths are planned for catalog, decode, seven load effects, three
  store effects, CPU integration, and inventory multi-binding. No operand
  layout, root transaction, CPU branch, public ABI, timing, device, or RPT
  change is admitted. Existing load/store family files are near their size
  threshold, so implementation should prefer new reverse-carry owner files;
  central catalog and inventory edits remain serialized.
- Activation accounting is 170 evidence records; 281 requirements (238
  IMPLEMENTED, 14 PLANNED, 29 UNRESOLVED); 235 tests (221 PASSING, 14
  PLANNED); and 7 sources. Evidence validation and the full authoritative
  `rtk make test` gate are green. Follow-up remains evidence-first.

## 2026-09-08 — P2-W45B reverse-carry MOD4/MOD7 per-family composition implementation

- State: implementation commit
  `a0c7a6450d3eb7f92b93016d0112a0e174a0703b` completes the bounded W45B
  reverse-carry fan-out across ten existing Smem families and twenty exact
  MOD4/MOD7 rows. The catalog has 95 rows, `ID_COUNT=96`, lookup 2,583, and
  95 bindings; inventory uses
  `STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`. Coverage is
  unchanged at 35/205 family (17.073%) and 25/205 strict (12.195%).
- W45B reuses the typed W45A reverse-carry seam with ARF1..7 execution only;
  ARF0 encodings decode but are rejected transactionally before endpoint issue
  because alias semantics remain unresolved. CMPT=0/ARP=0, known nonzero
  one-hot AR0, old selected-AR address, and POST update to the selected AR are
  preserved. No BK or wrap inference is admitted; no CPU opcode branch,
  public ABI, timing, device, or RPT change is made.
- The implementation avoids growing near-threshold family files by using
  reverse-carry-specific modules. It reuses the W45A typed seam and fixes
  store AR_MOD4 field extraction, stale-now-valid near-miss fixtures, the load
  completion NULL contract, trusted operand source parity, and test coverage
  gaps. No knobs were added or changed.
- Validation is GREEN: authoritative standalone `rtk make test` after final
  fixes and evidence closure; Python quality 215 OK; exact catalog/decode/
  inventory/manifest checks green; fresh post-fix ASan/UBSan validation passed
  5/5 W45B CTests without diagnostics; independent integration review found no
  P0/P1/P2 issues; and source boundaries pass with only the pre-existing
  `operand_layout.c` 620-line review warning. All new files remain under 500
  lines, with the CPU test at 498 lines; `git diff --check` is clean.
- Closure accounting is 170 evidence records; 281 requirements (244
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 235 tests (227 PASSING, 8
  PLANNED); and 7 sources. Follow-up remains evidence-first: preserve
  unresolved ARF0 alias semantics and perform an evidence/FIT audit before
  activating the next ISA slice.

## 2026-09-08 — P2-W46A/W46B atomic logical memory activation

- State: activation commit
  `475c4bc26d060c79b5ed2bb3e877ac656ada1973` freezes the next
  architecture-forcing slice. W46A owns a private allocation-free WORD16
  same-address read-modify-write transaction inside the generic MEMORY route;
  W46B composes ANDM, ORM, and XORM over neutral MOD0–MOD3 rows.
- The selected hot-path shape is one compact AND/OR/XOR transform descriptor,
  one copied operand, one address resolution, one READ, one WRITE at the
  cached old address, and one transform. Resume validation compares copied
  dependencies directly rather than rerunning address arithmetic. No indirect
  transform callback, new effect kind, opcode-specific CPU branch, allocation,
  public ABI, or runtime knob is admitted.
- Architectural AR/PC publication occurs only after WRITE READY. WAIT, FAULT,
  cancel, stale-state rejection, no-refetch, and fresh per-phase transaction
  IDs are required. This contract does not promise physical exclusion against
  an external bus master between the two endpoint requests.
- W46B freezes three two-word rows: ANDM `0x6880/0xFFE0`, ORM
  `0x6980/0xFFE0`, and XORM `0x6A80/0xFFE0`. MOD0/1/2 are admitted under the
  existing checked boundary policy; MOD3 decodes but must reject before the
  endpoint because the form requires a read and MOD3 is write-only. Projected
  catalog state is 98 runnable rows, `ID_COUNT=99`, lookup 2,679, and 98
  bindings; inventory becomes 38/205 family (18.537%) while strict coverage
  remains 25/205 (12.195%).
- Performance is a core acceptance axis. Deterministic structural tests must
  prove the bounded execution shape, and a separate observer-off RMW benchmark
  will establish its own baseline-relative cohort without changing the frozen
  five-scenario history or reusing its 60 ns target. Results remain hosted
  implementation-throughput evidence, not TI timing.
- Activation accounting is 175 evidence records; 290 requirements (244
  IMPLEMENTED, 17 PLANNED, 29 UNRESOLVED); 244 tests (227 PASSING, 17
  PLANNED); and 7 sources. `rtk make test` is GREEN, including 215 Python
  quality tests and the evidence registry; source boundaries report only the
  pre-existing `operand_layout.c` 620-line review warning. Registry-policy
  review corrected multi-owner paths and classified the TI-backed catalog and
  effect requirements as COMMON architectural claims before commit.

## 2026-09-08 — P2-W46A generic WORD16 read-modify-write implementation

- State: implementation commit
  `898d1f7985124a128acdd61238c620073d5f80e7` completes the private W46A
  same-address WORD16 read-modify-write seam inside the existing MEMORY
  carrier and CPU route.
- The carrier owns a compact value-based AND/OR/XOR operation and copied
  operand. The sequencer resolves the address once, issues one READ, applies
  the transform exactly once, resets the reducer, obtains a fresh transaction
  ID, and issues one WRITE to the cached address. AR and PC publish only after
  WRITE READY. WAIT, FAULT, cancellation, reset, ID exhaustion, no-refetch,
  and stale-state/candidate rejection are covered in both phases. This is
  core-architectural atomicity only; no external-bus-master exclusion is
  claimed.
- Review hardened exact candidate shape, including MOD0 canonical zero update
  fields, and added hostile read/write callback mutation checks. Resume
  validation compares copied CMPT, ARP, selected AR, AR0, BK, PC, operation,
  and operand snapshots directly without replanning the address. No callback
  transform, new effect kind, opcode-specific CPU branch, allocation, public
  ABI, environment read, or knob was added.
- Performance remains an acceptance axis: the MEMORY and root effect carriers
  remain 40/48 bytes on 64-bit and 36/40 bytes on 32-bit; six structural tests
  prove allocation-free bounded dispatch, no address replan, one paired READ/
  WRITE advance path, and no opcode branch in the CPU route. W46B retains the
  separate observer-off executable RMW benchmark so measured throughput can
  govern any hot-path refinement.
- Validation is GREEN: the nine affected memory effect/transaction CTests and
  six structural hot-path tests pass; fresh ASan/UBSan validation passed 8/8
  focused CTests without diagnostics; evidence validation reports 290
  requirements and 244 tests; and the final authoritative `rtk make test`
  gate passes, including 221 Python quality tests. Source boundaries pass with
  reviewed warnings for pre-existing `operand_layout.c` and the cohesive
  620-line RMW lifecycle test; the latter must split before 800 lines.
- Closure accounting is 175 evidence records; 290 requirements (247
  IMPLEMENTED, 14 PLANNED, 29 UNRESOLVED); 244 tests (230 PASSING, 14
  PLANNED); and 7 sources. Catalog rows and ISA coverage are unchanged at 95
  rows, 2,583 lookup entries, 95 bindings, 35/205 family (17.073%), and 25/205
  strict (12.195%). W46B ANDM/ORM/XORM composition is next.

## 2026-09-08 — P2-W46B atomic logical memory semantic/framework implementation

- State: implementation commit
  `58aabfb22507f53672cab7511a1b3a20b4ba8efd` composes ANDM, ORM, and XORM
  over the private W46A WORD16 read-modify-write seam. The three two-word
  MOD0–MOD3 rows use bases `0x6880`, `0x6980`, and `0x6A80` with mask
  `0xFFE0`, the existing `ARF_EXTENSION16_MOD` layout, and no input or status
  resources. Catalog state is 98 rows, `ID_COUNT=99`, 2,679 populated
  first-word lookup entries, and 98 bindings; inventory is 38/205 family
  (18.537%) and 25/205 strict (12.195%).
- Checked leaves validate metadata and layout before extraction. Trusted leaves
  consume the already copied operand packet in one pass and do not repeat
  catalog lookup, layout resolution, or raw-word extraction. MOD0/1/2 stage the
  generic READ carrier with a compact AND/OR/XOR descriptor; MOD3 rejects
  before endpoint publication because the instruction requires a read and the
  current MOD3 boundary is write-only.
- CPU tests cover all three operations in MOD0/1/2, exact paired request shape,
  same cached address, fresh transaction IDs, WAIT resume without refetch,
  read/write faults, reset cancellation in both phases, stale-state guards,
  AR/PC/counter publication, status and knownness preservation, and the MOD3
  zero-endpoint boundary. No opcode-specific CPU branch, public ABI, CPU
  production change, allocator, environment read, or knob was added.
- Performance remains a core acceptance axis. A separate observer-off RMW
  executable, strict capture tool, same-cohort comparator, and seven-test
  quality contract are present without altering the frozen five-scenario
  history. The smoke cohort retired 320,000 instructions per scenario with
  valid READ/WRITE order, IDs, address, and transforms; semantic hash is
  `3b39187c40a4fd43`. Observed medians were approximately 401 ns/instruction
  for ANDM, 428 for ORM, and 430 for XORM. These are advisory hosted results,
  not TI timing. A clean baseline artifact/comparison remains explicitly
  planned, so W46B is not yet fully closed.
- Validation is GREEN: authoritative `rtk make test` passed with 231 Python
  quality tests and one expected clean-baseline skip; focused sanitizer/catalog
  validation passed 25/25; evidence and manifests pass; benchmark smoke passes;
  and `git diff --check` is clean. Independent semantic review found no
  production P0/P1 issue. Benchmark review exposed and drove fixes for observer
  self-labeling, source/cohort identity, dirty-baseline admission, and strict ID
  validation. All new handwritten files remain below 500 lines. Source
  boundaries pass for 413 files with reviewed 620-line warnings only for the
  pre-existing operand-layout source and cohesive W46A lifecycle test.
- Closure accounting is 175 evidence records; 290 requirements (252
  IMPLEMENTED, 9 PLANNED, 29 UNRESOLVED); 244 tests (235 PASSING, 9 PLANNED);
  and 7 sources. The immediate follow-up is a clean W46B baseline/evidence
  closure, then the architecture-forcing P2-W47A LONG32 pair transaction seam.

## 2026-09-08 — P2-W46B observer-off RMW performance closure

- State: performance-evidence commit
  `bcb3555e92864ce2de4580419bbae21d630b27cb` records the clean dedicated
  RMW cohort at `bench/audits/p2-w46b-rmw-baseline.json` and closes W46B's
  final planned requirement/test. The capture identifies source, binary,
  compiler, build flags, host, observer-off profile, VCS state, exact roster,
  and semantic hash `3b39187c40a4fd43`.
- The clean baseline retired 320,000 instructions for each MOD0 ANDM/ORM/XORM
  scenario and proved two program-word reads plus exactly one DATA READ and
  one same-address DATA WRITE per retirement, strict request order, fresh
  transaction IDs, and correct transforms. Recorded hosted medians are
  620.863, 558.865, and 559.790 ns/instruction respectively; timing spread is
  retained in the artifact and the values remain advisory rather than TI
  machine-cycle claims or absolute pass/fail thresholds.
- `rtk make bench-rmw-compare` accepted a subsequent capture as the same
  source/profile/observer/host/compiler/roster/semantic cohort and emitted
  per-scenario deltas. The focused benchmark contract passed 7/7. The final
  authoritative `rtk make test` gate passed all 231 Python quality tests with
  no skips, plus the compiled C suite, evidence, manifest, inventory, source
  boundary, and observer-off artifact checks.
- Closure accounting is 175 evidence records; 290 requirements (253
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 244 tests (236 PASSING, 8 PLANNED);
  and 7 sources. W46B is fully closed. P2-W47A is next: an allocation-free
  private LONG32 pair transaction seam that preserves the one-word endpoint
  ABI and makes request order, WAIT/FAULT/cancel, visibility, and hot-path
  costs explicit before any DLD/DST opcode composition.

## 2026-09-08 — P2-W47A generic LONG32 pair transaction activation

- State: activation commit
  `00db85dc3b10c1d509d4b062ee74d5fa05048927` freezes a private
  sequencer-only LONG32 pair transaction before any opcode composition. It
  reuses the existing canonical `{first=MSW, second=LSW}` data operand and one
  private WORD16 reducer without changing the catalog, opcode handlers, CPU,
  public ABI, or frozen WORD16 MEMORY carrier.
- The project contract selects deterministic first/MSW then second/LSW issue,
  copied descriptors and values, and two caller-supplied fresh nonzero strictly
  increasing IDs. WAIT retains the active request and ID. A first-word fault
  cannot issue the second word; a second-word fault publishes no pair read
  result or later architectural commit. Cancel targets only the active word,
  and failed cancellation must preserve exact reducer state.
- Reads become available only after both requests are READY. With the existing
  one-word endpoint ABI, writes are externally visible per word: a completed
  first write is not rolled back if the second faults or is cancelled, while
  any later architectural state commit remains deferred. No external-master
  exclusion, rollback, all-or-none physical write, device, timing, or TI bus
  ordering claim is admitted. Existing TI records remain authoritative only
  for LONG32 word pairing and address delta; lifecycle policy is explicitly a
  project contract.
- Four RED lanes are registered for pair shape, read lifecycle, write
  lifecycle, and structural hot-path performance. The implementation must be
  allocation-free and fixed-size, issue exactly two requests without replanning,
  freeze supported-ABI sizes, and stay below file-size review boundaries. An
  executable opcode benchmark is deferred to W47B, where an evidenced LONG32
  opcode can exercise the full CPU path.
- Activation accounting is 176 evidence records; 294 requirements (253
  IMPLEMENTED, 12 PLANNED, 29 UNRESOLVED); 248 tests (236 PASSING, 12 PLANNED);
  and 7 sources. Evidence validation and the authoritative `rtk make test` gate
  are green with all 231 Python quality tests passing. ISA counts remain 98
  catalog rows, 2,679 lookup entries, 38/205 family, and 25/205 strict.

## 2026-09-08 — P2-W47A generic LONG32 pair transaction implementation

- State: implementation commit
  `9ef10773d40b2cc06669f5916083733d734060a9` completes the private
  allocation-free LONG32 pair reducer. It copies one canonical data-operand
  request, reuses one WORD16 child transaction, and issues exactly first/MSW
  then second/LSW using two supplied fresh nonzero increasing IDs. No catalog,
  opcode handler, CPU route, public ABI, or MEMORY effect-carrier change is
  included; ISA counts remain 98 rows, 2,679 lookup entries, 38/205 family,
  and 25/205 strict.
- The reducer preserves the active request and ID through WAIT, rejects stale
  observations and failed cancellation transactionally, prevents second-word
  issue after a first fault, clears staged read values on either fault, and
  publishes a read pair only after both words are READY. Reset is terminal-only.
  Writes preserve the existing one-word endpoint contract: each successful
  word may become externally visible independently, a first READY is not
  rolled back if the second faults or is cancelled, and the reducer makes no
  rollback, external-master exclusion, or physical pair-atomicity claim.
- Lifecycle and canonical validation are separate owner files at 325 and 198
  lines. Canonical checks compare copied child fields directly; request
  construction occurs only twice, at first preparation and the first-to-second
  transition. This removed repeated constructor work from begin/poll/complete/
  cancel validation. All new handwritten files remain below 500 lines.
- Performance is an executable acceptance axis. The fixed request/transaction
  sizes are 32/88 bytes on native LP64 and 32/80 on supported ILP32. The 6/6
  quality lane compiles and runs both ABI probes, freezes those sizes, proves
  one child reducer, copied request identity, exactly two constructor sites,
  no address replan, no allocation/I/O/callback state, and no crossing into
  public, opcode, CPU, or effect-carrier surfaces. A full opcode benchmark is
  intentionally deferred to W47B.
- Validation is GREEN: three focused shape/read/write CTests pass under fresh
  ASan/UBSan; the hardened API matrix covers NULL arguments, duplicate begin,
  stale/invalid observations, cancel/reset, and post-terminal preservation;
  independent review found no P0/P1 defect after correcting the test's physical
  write-visibility model. The authoritative `rtk make test` gate passes the
  full C suite and 237 Python quality tests, with evidence, manifests, source
  boundaries, inventory, and observer-off artifact checks green.
- Closure accounting is 176 evidence records; 294 requirements (257
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 248 tests (240 PASSING, 8 PLANNED);
  and 7 sources. The next forcing-function slice is W47B: evidence-first
  selection of exact DLD/DST encodings and composition through this seam,
  including an observer-off full-CPU opcode benchmark.

## 2026-09-08 — P2-W47B DLD/DST LONG32 MOD13 activation

- State: activation commit
  `e12dd2bb5a842d3234c2d6f2d95083c820c0ec80` freezes the two-word CMPT=0
  MOD13 rows `DLD_LONG32_MOD13` at `0x56E8/0xFEF8` and
  `DST_LONG32_MOD13` at `0x4EE8/0xFEF8`. Each row admits sixteen first words,
  carries D/S at bit 8, ARF at bits 2:0, MOD13 at bits 6:3, and a copied
  extension-word `lk`. Production catalog and inventory state remain
  unchanged until the rows and their tests exist.
- The activation deliberately chooses long-offset forms instead of the easier
  one-word forms. This forces extension fetch, a four-field decode layout,
  checked LONG32 MOD13 resolution, pair-backed root execution, pre-access AR
  publication, and full WAIT/FAULT/cancel behavior through one generic CPU
  route. DLD consumes SXM; its documented C16 lane descriptions are required
  to converge under C16×SXM vectors, so no behaviorally inert C16 hot-path
  input is admitted.
- The evidence audit corrected the initial DST assumption before code was
  written. PMST.SST applies saturation-on-store to DST, so DST must collect
  copied accumulator, SXM, and SST inputs and implement the full raw-40/signed-
  40 saturation boundaries. Unknown SST or SXM rejects before endpoint issue.
  SST is therefore a typed state/input-collection forcing seam, not a hidden
  default or an SST=0 restriction.
- Performance is part of the exit contract: the private MEMORY carrier must
  retain its supported-ABI size, pair/source values remain copied and fixed-
  size, no address replan/refetch/source recomputation is allowed, static
  completions own no context or diagnostic work, and closure requires an
  observer-off full-CPU DLD/DST benchmark. The root uses one discriminated
  child union and new helpers must keep handwritten owners below review
  thresholds. No knob was added or changed.
- Activation accounting is 181 evidence records; 304 requirements (257
  IMPLEMENTED, 18 PLANNED, 29 UNRESOLVED); 258 tests (240 PASSING, 18
  PLANNED); and 7 sources. The projected implemented catalog is 100 rows,
  `ID_COUNT=101`, 2,711 lookup entries, and 100 bindings. Family coverage
  becomes 40/205 (19.512%); strict complete-form coverage remains 25/205
  (12.195%) because each MOD13 row is only a subset of its full Lmem form.
  The authoritative `rtk make test` gate passed all 237 Python quality tests,
  the compiled C suite, evidence/inventory/manifests, source boundaries, and
  observer-disabled artifact checks; only the two already reviewed 620-line
  warnings remain.

## 2026-09-08 — P2-W47B1 generic LONG32 MEMORY MOD13 root

- State: implementation commit
  `6a1a6f2774d0188285399f4e457dcd7eb23ef029` completes the fixed-size generic
  LONG32 MEMORY carrier, checked CMPT=0 MOD13 resolver, and discriminated root
  transaction over the W47A pair reducer. Catalog rows, DLD/DST semantics, SST
  collection, CPU opcode tests, inventory activation, and the opcode benchmark
  remain W47B2; ISA counts therefore remain 98 rows, 2,679 lookup entries,
  38/205 family coverage, and 25/205 strict coverage.
- The root copies both write words and the read completion contract, resolves
  MOD13 once, reserves two consecutive nonzero transaction IDs, and reports the
  active child separately from the sequence watermark. WAIT resumes the same
  request. First/second faults, malformed observations, failed cancellation,
  candidate edits, live-state edits, and watermark edits cannot publish a
  partial read or commit AR/PC. A pair-owned staged-abandon transition makes
  pre-second and post-second READY rejection terminal and resettable without an
  endpoint callback; partial physical write visibility remains explicit.
- Performance decisions are structural and tested: the effect/address carriers
  stay 40/24 bytes on LP64 and 36/24 on ILP32; the root is 336/308 bytes; direct
  endpoint calls avoid double-reducing the embedded WORD16 child; and no address
  replan, allocator, I/O, environment, formatting, or callback context was
  added. Existing WORD16 instructions do not call the LONG32 cold preparation
  helper. No knob was added or changed.
- File review closes the 494-line root dispatcher to further W47B growth; new
  LONG32 lifecycle logic lives in its 419-line owner. The 782-line failure
  matrix remains one cohesive transition-system test below the 800-line split
  boundary and is closed to further growth; W47B2 tests belong in their opcode,
  CPU, and quality owners.
- Validation is GREEN: the focused compatibility matrix is 10/10; fresh
  ASan/UBSan is 10/10; a supported `-m32` compile probe freezes the ILP32 sizes;
  and the authoritative clean `rtk make test` gate passes the compiled C suite
  and all 237 Python quality tests. The first Make run exposed duplicate stale
  local archive members (`*.c.o` and `*.o`); after removing only repository
  build artifacts, the fresh archive contained one member per source and the
  full gate passed. Source-boundary warnings are limited to the two previously
  reviewed 620-line files and this reviewed 782-line lifecycle test.
- Closure accounting is 181 evidence records; 304 requirements (259
  IMPLEMENTED, 16 PLANNED, 29 UNRESOLVED); 258 tests (242 PASSING, 16 PLANNED);
  and 7 sources. W47B2 is next: typed SST input knownness, DLD/DST semantics and
  catalog/decode bindings, full CPU lifecycle tests, inventory activation, and
  the observer-off full-CPU benchmark.

## 2026-09-08 — P2-W47B2A typed PMST.SST input seam

- State: implementation commit
  `d21b88451a078ffcc2e077725f91aa2452ccb6a7` adds typed PMST.SST read/write
  access and knownness-aware input collection. Common reset deliberately leaves
  SST unknown; a checked typed write establishes its value and knownness. SST
  and SXM remain independently selectable catalog resources, and invalid
  collection leaves caller-owned output unchanged.
- The copied opcode-input snapshot remains 48 bytes on both supported LP64 and
  ILP32 layouts by using existing padding. The input-resource mask remains an
  8-bit value with SST assigned bit 6. No allocator, environment access, I/O,
  logging, observer work, opcode dispatch branch, public ABI, or mutable global
  state was added. No knob was added or changed.
- The SST requirement was narrowed at implementation from its activation-time
  combined wording to the state/input seam it actually owns. Rejection of
  unknown required SXM/SST before any endpoint request remains explicitly owned
  by the planned DLD/DST semantic and CPU lifecycle requirements; this commit
  does not claim those opcode behaviors.
- Validation is GREEN: focused status and input collection tests pass; the new
  collector contract passes fresh ASan/UBSan; an ILP32 compile probe confirms
  the 48-byte snapshot; and the authoritative clean `rtk make test` gate passes
  the compiled C suite and all 237 Python quality tests. Evidence, manifests,
  inventory, source boundaries, and the observer-disabled artifact check pass.
  Warnings remain limited to the three previously reviewed 620/782/620-line
  files. No GitHub or other hosted runner was invoked for this work piece.
- Accounting is 181 evidence records; 304 requirements (260 IMPLEMENTED, 15
  PLANNED, 29 UNRESOLVED); 258 tests (243 PASSING, 15 PLANNED); and 7 sources.
  Catalog counts remain 98 rows, `ID_COUNT=99`, 2,679 lookup entries, and 98
  bindings; family/strict coverage remains 38/205 and 25/205. W47B2B next adds
  the two catalog rows, four-field layout, and pure checked/trusted DLD/DST
  semantics without duplicate trusted-path extraction or resume-time source
  recomputation.

## 2026-09-09 — P2-W47B2B DLD/DST LONG32 MOD13 semantics

- State: implementation commit
  `4aa69eb42193e0b4498e44e9142253479a02715f` activates the exact
  `DLD_LONG32_MOD13` and `DST_LONG32_MOD13` rows at `0x56E8/0xFEF8` and
  `0x4EE8/0xFEF8`. Each row has sixteen first words, one copied extension word,
  the shared D/S–ARF–`lk`–MOD13 layout, and an ineligible repeat class. The
  generated direct lookup now contains 2,711 populated entries.
- Checked and trusted semantic leaves are separate. The checked path validates
  complete decode and extracts fields transactionally; the normal CPU trusted
  path consumes the four values already extracted into its canonical packet and
  does not resolve or extract the layout again. Dispatch remains a static
  catalog-derived function table with no opcode switch, learning pass, cache,
  allocation, observer fan-out, environment access, formatting, or I/O.
- DLD stages one generic LONG32 MOD13 read and publishes an accumulator effect
  only after copied MSW/LSW completion; SXM=0 clears guard bits and SXM=1 sign
  extends bit 31 while C/TC remain preserved. DST selects a copied full raw-40
  source, validates SXM/SST knownness, applies the pinned unsigned/signed clamp
  once before issue, and stages copied MSW then LSW values with no completion
  callback or source mutation. Resume-time source reads and recomputation remain
  absent.
- Inventory binds the rows to `FORM-T2-19-01` and `FORM-T2-20-01` as
  `LONG_OFFSET_MOD13_CMPT0` subsets. The current snapshot is 100 catalog rows,
  `ID_COUNT=101`, 2,711 lookup entries, 100 bindings, 40/205 family coverage
  (19.512%), and unchanged 25/205 strict coverage (12.195%).
- File review retains `src/isa/decode/operand_layout.c` as the cohesive central
  immutable layout registry at 637 lines, but closes it to another layout
  addition until its static family definitions are split into owned helpers.
  W47B2C requires no new layout. The two new semantic owners are 219 lines each
  and their focused tests are 230/225 lines. No knob was added or changed.
- Validation is GREEN: the four focused catalog/decode/DLD/DST C executables
  pass; 25 focused catalog/inventory Python tests pass; focused DLD/DST
  ASan/UBSan passes; generated lookup staleness checking passes; and the
  authoritative `rtk make test` gate passes the compiled C suite and all 238
  Python quality tests. Evidence, manifests, inventory, source boundaries, and
  the observer-disabled artifact pass. Two initial gate failures were stale
  aggregate count assertions and strict module-contract comment spelling; both
  were corrected before the clean green gate. No GitHub or other hosted runner
  was invoked.
- Accounting is 181 evidence records; 304 requirements (264 IMPLEMENTED, 11
  PLANNED, 29 UNRESOLVED); 258 tests (247 PASSING, 11 PLANNED); and 7 sources.
  W47B2C next proves full CPU READY/WAIT/FAULT behavior, deferred AR/PC and DLD
  publication, DST partial physical write visibility, unknown SXM/SST rejection
  before endpoint issue, C16 independence, and observer-off opcode performance.

## 2026-09-09 — P2-W47B2C DLD/DST CPU and performance closure

- State: implementation commit
  `d7db000f696337ed1182def9a4e599d90c6b1824` closes W47B with full public-CPU
  DLD/DST LONG32 MOD13 lifecycle tests and a dedicated observer-off opcode
  benchmark. The evidence registry now marks the two CPU lifecycle contracts
  and the hot-path performance contract implemented/passing. Catalog and ISA
  inventory remain 100 rows, `ID_COUNT=101`, 2,711 lookup entries, 100
  bindings, 40/205 family coverage, and 25/205 strict coverage.
- DLD proves two-word fetch, D=0/1, SXM=0/1, C16=0/1/unknown independence,
  unknown-SXM rejection before endpoint issue, even/odd LONG32 word pairing,
  first/second WAIT identity, no resume refetch, first/second FAULT, deferred
  accumulator/AR/PC publication, and fresh paired IDs. DST proves copied
  full-40-bit sources, both accumulators, the SST×SXM saturation boundary
  matrix, unknown source/SXM/SST rejection, the same WAIT/FAULT/ID/PC/AR
  lifecycle, and the existing per-word physical visibility rule when the
  second write faults.
- Performance remains a first-class acceptance axis. The 478-line hosted
  benchmark runs the exact two-word rows through the public CPU with observers
  compiled out, checking 80,000 program reads and 80,000 ordered DATA requests
  around 40,000 retirements per scenario. The 417-line capture owner validates
  the exact schema, semantic hash `4ccf84d27d3fd6b7`, host/compiler/build
  provenance, transaction ordering, PC progression, and timing arithmetic.
  The 246-line quality owner freezes single trusted-packet extraction, static
  dispatch, hot-path host-side-effect exclusion, and the established LP64/ILP32
  carrier sizes. No machine-dependent threshold, baseline history, or TI
  machine-cycle claim was added.
- Clean-tree capture at the implementation hash on the local i7-1255U with cc
  15.2, portable `-O2`/no-LTO, and observers off measured median DLD at
  501.186100 ns/instruction (1,995,266.828031 instructions/s) and DST at
  472.164275 ns/instruction (2,117,906.950923 instructions/s). Each instruction
  includes two program-word callbacks and two DATA endpoint callbacks; the
  record reports `vcs_state=clean`, IDs 470001..480000, and correct semantic
  values. These figures are advisory host measurements, not acceptance
  thresholds.
- Validation is GREEN: both focused CPU executables pass normally and under a
  fresh ASan/UBSan CTest matrix (2/2); the focused performance contract passes
  7/7; Make and CMake benchmark smoke paths pass; and the authoritative
  `rtk make test` gate passes the compiled C suite and all 245 Python quality
  tests. Evidence, manifests, knobs, inventory, source boundaries, and the
  observer-disabled artifact are green. Review warnings remain the three
  previously reviewed 637/782/620-line files. The new CPU tests are 434/437
  lines and all new owners remain below 500 lines. No knob was added or
  changed, and no GitHub or other hosted runner was invoked for this work piece.
- Closure accounting is 181 evidence records; 304 requirements (267
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 258 tests (250 PASSING, 8 PLANNED);
  and 7 sources. The next architecture-forcing direction is W20F repeat/IRQ
  evidence closure: resolve RC progression, repeated-body PC flow, and
  predecode pending-request arbitration before activating runtime RPT. Its
  implementation must retain prevalidated execution without per-repeat
  allocation, refetch, or operand re-extraction.

## 2026-09-09 — P2-W48A observable RC progression lifecycle

- State: implementation commit
  `148c811c77c250ca60c88e764258b79957a86827` adds checked typed RC access and
  a fixed-size repeat-progress reducer under the existing state and repeat
  owners. It consumes the value-owned `REPEAT_START` effect, loads exact RC,
  widens before forming the operand-plus-one remaining-body count, and mutates
  RC only after the caller reports a successfully retired body.
- Evidence correction: the earlier W20F conclusion remains immutable but is
  narrowed by three new TI SPRU172C Rev C records. READA printed
  pp. 4-136–4-137 and WRITA pp. 4-196–4-197 perform their transfer, increment
  PAR, and write `RC-1` while RC is nonzero, then perform the terminal transfer
  at RC zero without another RC update. FIRS p. 4-59 independently corroborates
  the nonzero decrement loop. Together with RPT's load and operand-plus-one
  count, these passages prove operation-level RC progression and final zero;
  they do not prove the exact internal decrement ordering.
- The progress reducer validates canonical IDLE/ACTIVE/COMPLETE phase shape,
  count bounds, RC knownness, and the exact `RC == remaining-1` invariant before
  publication. Unknown, stale, malformed, unsupported-effect, active-restart,
  and extra-completion paths preserve architectural state and progress. A new
  repeat may overwrite RC after a completed repeat, including after an
  intervening known RC state write through the typed owner.
- Performance is an explicit design constraint. The body-retirement hot path
  performs one typed RC read, one non-failing typed RC write, and one small
  progress update; review removed an initial whole-architectural-state copy
  from every body. Only the cold repeat-start path stages full state. `run.c`,
  the CPU instance, public ABI, catalog, trusted packet, and current benchmark
  binaries are unchanged, so this slice adds zero branches and zero carrier
  growth to normal CPU execution. No allocator, I/O, callback, environment,
  observer, diagnostic, mutable global, or knob was added.
- Validation is GREEN: the three focused Make executables pass; the agent's
  independent CMake build and exact CTest selection pass 3/3; the evidence
  checker passes at 2 targets, 7 sources, 307 requirements, and 261 tests; and
  the authoritative `rtk make test` gate passes the compiled C suite plus all
  245 Python quality tests. Manifests, knobs, inventory, source boundaries, and
  the observer-disabled artifact pass. Warnings remain limited to the three
  previously reviewed 637/782/620-line files. No GitHub or other hosted runner
  was invoked, and no RED commit was pushed.
- Closure accounting is 185 immutable evidence records; 307 requirements (270
  IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 261 tests (253 PASSING, 8 PLANNED);
  and 7 sources. Catalog/ISA counts remain 100 rows, `ID_COUNT=101`, 2,711
  lookup entries, 100 bindings, 40/205 family coverage, and 25/205 strict
  coverage. The existing exact-phase RC requirement remains UNRESOLVED; the new
  narrower observable-progression requirement is IMPLEMENTED. Repeated-body PC
  flow and predecode request arbitration still gate runtime RPT activation.

## 2026-09-09 — P2-W48B repeat-gated pending IFR lifecycle

- State: implementation commit
  `d06f12dddbcd81a2926318022eb35c2bd535a0f4` adds checked typed IFR access and
  one fixed-size, caller-owned maskable pending-request reducer under the new
  interrupt sequencer owner. Receipt validates one caller-selected IFR bit, sets
  it before consulting the existing repeat gate, and preserves both the bit and
  latch while an active repeat defers take. Explicit normal take at remaining
  count zero clears only the matching bit and latch.
- Evidence correction: new TI SPRU131G §6.10.1 printed pp. 6-27–6-28 evidence
  records all IFR clear paths. Normal interrupt take is not exclusive: reset,
  write-one-to-IFR, and `INTR` also clear flags. The live common pending-request
  requirement is narrowed to a maskable request received after the repeat gate
  is active. Exact bit assignment, NMI representation, priority selection,
  interrupt entry, and already-pending predecode arbitration remain outside it.
  Earlier immutable evidence and ledger entries were not rewritten.
- The state owner provides knownness-aware IFR reads, full writes, and exact-one-
  bit set/clear operations. The reducer uses the existing repeat request gate,
  rejects noncanonical EMPTY/LATCHED values, duplicate receipt, zero/multi-bit
  masks, counts above 65,536, unknown validation state, and stale latches without
  partial publication. IFR-write-one and `INTR` clear matching pending state;
  authoritative reset establishes known-zero IFR and an empty latch even when
  prior IFR storage is unknown. Unrelated IFR flags and architectural fields are
  preserved.
- Performance remains an acceptance constraint. This slice changes no CPU,
  `run.c`, catalog, trusted packet, public ABI, or benchmark binary, adding zero
  branches and zero carrier growth to current instruction execution. All work is
  fixed-size and allocation-free with no callback, environment, I/O, logging,
  observer, diagnostic, or mutable-global path. No knob was added or changed.
- Validation is GREEN: focused Make and CMake/CTest execution passes all three
  new state/interrupt tests; independent semantic review found no production
  defect; and the authoritative `rtk make test` gate passes the compiled C suite
  plus all 245 Python quality tests. Live evidence passes at 2 targets, 7 sources,
  309 requirements, and 263 tests. Manifests, knobs, inventory, source boundaries,
  and the observer-disabled artifact pass. Warnings remain limited to the three
  previously reviewed 637/782/620-line files; every W48B source and test is below
  500 lines. No GitHub or other hosted runner was invoked, no RED commit was
  pushed, and the implementation plus ledger commits are pushed together only
  after local GREEN.
- Closure accounting is 187 immutable evidence records; 309 requirements (273
  IMPLEMENTED, 7 PLANNED, 29 UNRESOLVED); 263 tests (256 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts remain 100 rows, `ID_COUNT=101`, 2,711 lookup
  entries, 100 bindings, 40/205 family coverage, and 25/205 strict coverage.
  Repeated-body PC flow and predecode request arbitration still gate runtime RPT
  activation; those evidence problems are next rather than inferred in code.

## 2026-09-09 — P2-W48C DLD repeatability correction

- State: implementation commit
  `b0144d16d4de6d559c26210c356a67031f025094` corrects only
  `DLD_LONG32_MOD13` repeatability from `INELIGIBLE` to `UNRESOLVED`. Its
  checked semantic shape validator and the DLD/DST metadata/inventory tests now
  preserve the exact asymmetry; `DST_LONG32_MOD13` remains `INELIGIBLE`.
- Evidence decision: a complete primary-source audit of TI SPRU172C
  §2.5/Table 2-28, SPRU131G §6.7/Table 6-18, and the DLD instruction entry
  found no DLD prohibition or positive authorization. The negative tables do
  not say that every omitted instruction is repeatable, and the timing-class
  tables include separately nonrepeatable instructions. DLD therefore cannot
  truthfully be either `INELIGIBLE` or `ELIGIBLE`; `UNRESOLVED` is the only
  supported state. Existing immutable evidence was not rewritten.
- The catalog now contains 84 `UNRESOLVED`, 16 directly evidenced
  `INELIGIBLE`, and zero `ELIGIBLE` rows. A new contract proves DLD maps to the
  distinct non-admitting unresolved decision, DST maps to reject-ineligible,
  and both retain canonical trusted bindings and non-null handlers. No runtime
  repeat body or trusted repeat cache is activated without positive evidence.
- Performance and execution behavior are unchanged. The correction changes no
  opcode value/mask, layout, resource, handler output, lookup entry, inventory
  binding, CPU route, packet/effect/input carrier, bus transaction, observer,
  public ABI, or benchmark binary. It adds no hot-path operation, allocation,
  I/O, environment access, mutable global, or knob.
- Validation is GREEN: the focused Make C tests pass; focused CMake/CTest passes
  5/5; affected inventory/catalog Python tests pass 4/4; and the authoritative
  `rtk make test` gate passes the compiled C suite plus all 245 Python quality
  tests. Live evidence passes at 2 targets, 7 sources, 310 requirements, and 264
  tests. Manifests, inventory, source boundaries, knobs, and the
  observer-disabled artifact pass. Only the three previously reviewed
  637/782/620-line warnings remain. No hosted runner was invoked, no RED commit
  was pushed, and implementation plus ledger commits are pushed together only
  after local GREEN.
- Closure accounting is 188 immutable evidence records; 310 requirements (274
  IMPLEMENTED, 7 PLANNED, 29 UNRESOLVED); 264 tests (257 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts remain 100 rows, `ID_COUNT=101`, 2,711
  lookup entries, 100 bindings, 40/205 family coverage, and 25/205 strict
  coverage. The next architecture-forcing implementation is the evidenced
  register ADD/SUB ALU/status slice; runtime RPT remains evidence-gated.

## 2026-09-09 — P2-W49A bit-32 ADD/SUB overflow foundation

- State: implementation commit
  `725f99ca89c1e28d2091949c4bf91413797a8e9a` adds checked modulo-2^40 ADD/SUB
  primitives, bit-32 carry/no-borrow, signed-32 overflow direction, typed
  OVM/OVA/OVB access, copied OVM input, and atomic sticky destination-overflow
  effects. No ADD/SUB opcode row or opcode-specific CPU branch is activated.
- Evidence decision: TI SPRU131G §4.1 and Tables 4-1/4-2 define bit-32 C,
  destination-specific sticky OVA/OVB, and OVM behavior; §4.2 defines the
  40-bit ALU and signed-32 saturation endpoints. TI SPRU172C NEG printed
  pp. 4-119–4-120 confirms both lower-32 and signed-40 extreme overflow cases.
  The implementation keeps modulo result, bit-32 carry/borrow, overflow
  direction, and OVM storage policy as separate facts rather than inferring one
  from another.
- Architecture: the input-resource carrier widens to `uint16_t` before OVM
  occupies bit `0x0080`, retaining invalid/future resource space. The copied
  input snapshot stays 48 bytes on LP64 and ILP32; metadata/decoded/trusted
  packet sizes remain 24/20/104 on LP64 and 24/20/76 on supported ILP32. The
  outer effect remains 48/40 bytes. All new paths are fixed-size,
  allocation-free, instance-owned, and free of I/O, environment, logging,
  observer, callback, and mutable-global behavior. No knob was added or changed.
- Performance: six interleaved same-host observer-off `-O2` benchmark pairs
  compared the frozen W48C binary with W49A. Median deltas were -1.34% NOP,
  -2.96% LD-DP, -1.21% LD-ASM, +1.23% LD-ARP, and -3.62% mixed-current; all
  semantic hashes were unchanged. The result is no material current-path
  regression, not a machine-timing claim or permanent host threshold.
- Validation is GREEN: the four focused strict Make executables pass; focused
  CMake/CTest passes 4/4; the supported `-m32` assertion probe passes; semantic
  review found no production defect after its coverage findings were closed;
  the evidence registry passes at 2 targets, 7 sources, 314 requirements, and
  269 tests; and the authoritative `rtk make test` gate passes the complete C
  suite plus all 246 Python quality tests. Manifests, inventory, source
  boundaries, knobs, and the observer-disabled artifact pass. Only the three
  previously reviewed 637/782/620-line warnings remain. No GitHub or other
  hosted runner was invoked, and no RED commit was pushed.
- Closure accounting is 192 immutable evidence records; 314 requirements (278
  IMPLEMENTED, 7 PLANNED, 29 UNRESOLVED); 269 tests (262 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts remain 100 rows, `ID_COUNT=101`, 2,711
  lookup entries, 100 bindings, 40/205 family coverage, and 25/205 strict
  coverage. W49B next composes the separately evidenced register-only ADD/SUB
  syntax-9 forms and measures their trusted public-CPU path.

## 2026-09-09 — P2-W49B register ADD/SUB shift forms

- State: implementation commit
  `5cbacda4076a43616be014ba0f528d7e95228e73` activates the one-word
  `ADD src,SHIFT,dst` and `SUB src,SHIFT,dst` syntax-9 rows at
  `0xF400/0xFCE0` and `0xF420/0xFCE0`. Checked and trusted pure semantics
  compose the W49A modulo-2^40, bit-32 carry/no-borrow, signed-32 overflow,
  OVM saturation, and destination-sticky OVA/OVB facts. Source/destination
  aliases are read before publication, and the generic CPU route gains no
  opcode-specific branch.
- Evidence and architecture decisions: the signed five-bit shift covers
  -16 through +15 over the canonical 40-bit source; right-shift fill follows
  SXM and left shifts truncate to 40 bits. OVM selects raw modulo storage or
  the evidenced signed-32 clamp without changing the separately derived carry
  fact. Repeatability remains `UNRESOLVED`; no repeat admission is inferred.
  The trusted leaf consumes constructor-proven operands without repeating ID,
  layout, mask, resource, or operand-shape validation, while architectural
  input knownness and transactional output behavior remain enforced.
- Performance is part of closure. Six alternating observer-off W49A/current
  benchmark pairs preserved every fixed semantic hash and 800,000-retirement
  count. Paired-delta medians were +3.51% NOP, +4.79% LD-DP, +0.19% LD-ASM,
  and +0.70% LD-ARP, within the wider same-host spread and with no material
  generic-path regression. The dedicated public-CPU benchmark measured
  70.569 ns/instruction for ADD and 68.553 ns/instruction for SUB with semantic
  hash `dd671e98fa2ca1dd`; these are host characterizations, not TI cycle claims.
  No benchmark history was written.
- Validation is GREEN: focused checked/trusted semantics, decode, metadata,
  CPU, inventory, and performance contracts pass; focused CMake/CTest passes
  5/5; generator and evidence checks pass; and the authoritative
  `rtk make test` gate passes the compiled C suite plus all 253 Python quality
  tests. Manifests, knobs, inventory, source boundaries, and the
  observer-disabled artifact pass. Only the three previously reviewed
  637/782/620-line warnings remain. No GitHub or other hosted runner was
  invoked, and no RED commit was pushed.
- Closure accounting is 197 immutable evidence records; 321 requirements (284
  IMPLEMENTED, 7 PLANNED, 30 UNRESOLVED); 274 tests (267 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts are now 102 rows, `ID_COUNT=103`, 2,967
  lookup entries, 102 trusted bindings, 42/205 family coverage, and 27/205
  strict coverage. No knob was added or changed. The next architecture-forcing
  ISA slice is evidence-first fixed-MOD2 `ADDC`/`SUBB`: it must carry incoming
  carry/borrow semantics through the existing one-read memory completion path
  without regressing the trusted generic loop; compound arithmetic RMW and
  dual-memory effects remain subsequent larger seams.

## 2026-09-09 — P2-W50A dual-WORD16 MOD0 read foundation

- State: implementation commit
  `501cf73ad81f6b39b173ee0aaa455f7590561384` adds the generic dual-indirect
  MOD0/MOD0 WORD16 read foundation without activating an opcode row. Checked
  address planning snapshots old AR2--AR5 values with no AR update; the memory
  effect carries copied X/Y selectors and one static completion callback; and
  the root serializes two canonical DATA reads X then Y over one endpoint before
  publishing one ordinary effect and sequential PC retirement.
- Evidence and architecture decisions: TI SPRU131G section 5.5.4 establishes
  the Xmod/Xar/Ymod/Yar fields, AR2--AR5 selector mapping, old-AR MOD0 behavior,
  and architectural X-D/Y-C bus roles. The public endpoint serialization is a
  deterministic project lifecycle contract, not a TI timing or physical
  dual-bus claim. MOD0 uses `STATE_POLICY_NONE`, does not consume CMPT or ARP,
  and produces no address commit. Full dual resolution is prepare-local; the
  persistent candidate retains only two canonical requests and size-neutral
  immutable selector/address shadows for resume-time forgery detection.
- Performance is a closure constraint. Adversarial review first exposed a
  416/392-byte persistent-root expansion; the design was changed before commit
  so the root remains frozen at 336/308 bytes on LP64/ILP32. The compact dual
  reducer is 80/72 bytes; address spec, memory effect, and outer effect remain
  24, 40/36, and 48/40 bytes. The common WORD16 route stays local and only
  paired forms use the cold generic dispatcher. W50A proves observer-disabled
  structure only; hosted nanosecond measurement is deferred to W50B activation.
- TDD and review: RED contracts covered the absent address, effect, reducer,
  root, and hot-path seams. Later RED regressions proved non-DATA pair requests
  were being normalized into DATA reads, paired snapshot plus live-AR mutation
  could forge Y, forged child identity could reach cancel, and a cleared
  `child_kind == 0` root was rejected by CPU reset. DATA-only validation,
  immutable address shadows, pre-callback canonical cancel validation, and the
  IDLE zero-sentinel reset rule close those cases. X/Y WAIT identity, READY,
  FAULT, malformed/stale observations, cancellation failure, ID exhaustion,
  aliasing, atomic completion, and no-update behavior are executable.
- Validation is GREEN: focused Make executables and focused CMake/CTest 5/5
  pass; dual and legacy LONG32 ABI contracts pass 14/14; evidence, manifests,
  inventory, source boundaries, knobs, observer-disabled structure, and diff
  checks pass; and the corrected authoritative `rtk make test` gate passes the
  complete compiled C suite plus all 260 Python quality tests. Reviewed size
  warnings are 637-line operand-layout owner, 521-line common root, 782-line
  LONG32 failure matrix, 769-line dual lifecycle matrix, and 620-line RMW
  matrix; the two W50A files remain cohesive below 800 and future cases split.
  No GitHub or other hosted runner was invoked, no RED commit was pushed, and
  no benchmark history was written.
- Closure accounting is 199 immutable evidence records; 327 requirements (290
  IMPLEMENTED, 7 PLANNED, 30 UNRESOLVED); 280 tests (273 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts remain 102 rows, `ID_COUNT=103`, 2,967
  lookup entries, 102 trusted bindings, 42/205 family coverage, and 27/205
  strict coverage. No knob was added or changed. W50B next activates the
  evidenced dual-memory ADD/SUB rows through this seam and adds observer-off
  public-CPU performance evidence.

## 2026-09-09 — P2-W50B dual-memory ADD/SUB MOD0 activation

- State: implementation commit
  `05e3ee68d5076c07e9bea356a041cdf1ad1e0451` activates the one-word
  syntax-6 `ADD Xmem,Ymem,dst` and `SUB Xmem,Ymem,dst` MOD0/MOD0 subsets as
  `ADD_DUAL_MOD0` at `0xA000/0xFECC` and `SUB_DUAL_MOD0` at
  `0xA200/0xFECC`. The decode-owned D/Xar/Yar layout maps raw X/Y selectors
  to AR2--AR5, and catalog-derived trusted bindings compose the existing W50A
  dual-read transaction without an opcode-specific CPU branch.
- Evidence and semantic decisions: TI SPRU172C and SPRU131G support independent
  SXM extension of both WORD16 inputs, fixed left shift 16, modulo-40 ADD/SUB,
  directional bit-32 C behavior, OVM signed-32 saturation, and sticky
  destination OVA/OVB. Repeatability remains `UNRESOLVED`; omission from the
  negative repeat tables is not positive eligibility evidence. Both inventory
  forms remain `SUBSET` under `DUAL_INDIRECT_MOD0_MOD0` because non-MOD0 modes
  are not claimed by this slice.
- CPU behavior is transactional: each retirement performs exactly one X read
  followed by one Y read, preserves WAIT identity without refetch or replan,
  and publishes accumulator/status/PC state only after both reads are READY.
  X or Y FAULT leaves architectural state unchanged. Checked APIs retain forged
  input validation; the production CPU uses the trusted binding on the first
  execution and performs no seen-before promotion.
- Performance remains a closure axis. The observer-off four-scenario benchmark
  retires 320,000 instructions per scenario with 640,000 ordered DATA reads and
  semantic hash `b40823092c0aa605`. Hosted samples varied materially across
  system load; one recorded characterization measured 1,749.8--1,811.9
  ns/instruction and a later smoke measured approximately 799.9--1,423.3
  ns/instruction. These values are advisory host observations, not a threshold
  or TI machine-cycle claim. The generic two-read path remains a visible
  optimization target, and no benchmark history artifact was written.
- Validation is GREEN: focused metadata, decode, checked/trusted semantics, CPU
  READY/WAIT/FAULT, inventory, and performance contracts pass; focused CTest is
  4/4; benchmark smoke passes; evidence parity passes at 2 targets, 7 sources,
  335 requirements, and 286 tests; and the authoritative `rtk make test` gate
  passes the complete compiled C suite plus all 268 Python quality tests.
  Manifests, inventory, source boundaries, knobs, observer-disabled structure,
  and diff checks pass. Independent review found no correctness blocker. No
  GitHub or other hosted runner was invoked, and no RED commit was pushed.
- Closure accounting is 203 immutable evidence records; 335 requirements (297
  IMPLEMENTED, 7 PLANNED, 31 UNRESOLVED); 286 tests (279 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts are 104 rows, `ID_COUNT=105`, 3,031 lookup
  entries, 104 trusted bindings, 44/205 family coverage, and 27/205 strict
  coverage. No knob was added or changed. The 499-line benchmark is retained as
  a cohesive fixed roster and future scenarios must split; the 665-line
  descriptor owner remains below 800 with future growth requiring an explicit
  split. W51 next implements coordinated non-MOD0 dual addressing/update modes,
  including same-AR Xmod precedence and atomic two-AR commit, while preserving
  trusted-path and two-read performance constraints.

## 2026-09-09 — P2-W51A dual-indirect MOD0–MOD2 planning checkpoint

- State: implementation commit
  `d039dee00f481cbf03a18c6451841d3932a69893` extends the pure dual-indirect
  WORD16 address planner and memory-address resolution seam to modes 0, 1, and
  2. Both operands retain their frozen old addresses; modes 1 and 2 produce
  value-owned post-access decrement/increment commit candidates. No opcode row,
  CPU path, root transaction, catalog projection, or persistent carrier is
  activated or widened in this checkpoint.
- Evidence and architecture decisions: TI SPRU131G section 5.5.4/Table 5-8
  defines Xmod/Xar/Ymod/Yar, direct AR2--AR5 selection, and mode 0/1/2 effects;
  TI SPRU179C ADD Example 4 confirms Xmod precedence when X and Y alias one AR.
  These modes use `STATE_POLICY_NONE` and do not consume CMPT or ARP. Aliased
  operands read the same frozen value and stage at most the X-owned update;
  distinct selectors retain independent candidates. Mode 1 at `0x0000` and
  mode 2 at `0xffff` are transactionally `UNRESOLVED_BOUNDARY`. MOD3 remains
  deferred at its AR0/BK indexed/circular and CMPT1 boundary.
- TDD proceeded RED to GREEN: the new planner matrix initially failed because
  modes 1 and 2 were unsupported, then the resolution contract initially
  failed because it admitted only mode 0. Focused strict Make tests and focused
  CMake/CTest pass after implementation. Coverage includes all nine mode-0/1/2
  pairs, frozen read order, post-update candidates, same-AR precedence,
  transactional boundaries, CMPT/ARP independence, and the unchanged 32-byte
  requests-only persistent snapshot.
- Performance remains a closure constraint. The observer-off W50B public-CPU
  smoke preserves semantic hash `b40823092c0aa605` and exactly two DATA reads
  per retirement. Dirty-tree host samples measured 733.988--882.744
  ns/instruction; this is volatile characterization only, with no history,
  threshold, regression claim, or TI machine-cycle claim. The production CPU
  and trusted first-execution route are unchanged.
- Validation is GREEN: the authoritative `rtk make test` gate passes the full
  compiled C suite plus all 268 Python quality tests; evidence parity passes at
  2 targets, 7 sources, 337 requirements, and 287 tests. Manifests, inventory,
  source boundaries, knobs, observer-disabled structure, and diff checks pass.
  Reviewed warnings remain the existing 665-line operand-layout owner, 521-line
  root transaction, and 782/769/620-line test matrices. No knob was added or
  changed. No GitHub or other hosted runner was invoked, and no RED commit was
  pushed.
- Closure accounting is 206 immutable evidence records; 337 requirements (299
  IMPLEMENTED, 7 PLANNED, 31 UNRESOLVED); 287 tests (280 PASSING, 7 PLANNED);
  and 7 sources. Catalog/ISA counts remain 104 rows, `ID_COUNT=105`, 3,031
  lookup entries, 104 trusted bindings, 44/205 family coverage, and 27/205
  strict coverage.
- Exact resume point: W51B extends the persistent dual snapshot with both
  prevalidated commit descriptors, validates snapshot and live-state identity,
  adds immutable mode/selector drift shadows, and atomically publishes distinct
  AR commits only after both reads and ordinary-effect staging succeed. A
  same-AR pair commits once; WAIT and FAULT remain non-publishing; root sizes
  stay frozen at 336/308 bytes. Broader dual-mode opcode activation follows
  only after that root transaction seam is green.

## 2026-09-09 — P2-W51B dual-memory root commit/publication seam

- State: implementation commit
  `c431a6130585494c288ab09097f7a280248b98c7` implements the W51B root
  transaction seam left planned by the previous session. The persistent dual
  snapshot grows from 32 to 48 bytes with exactly two value-owned
  post-access commit descriptors; the dual effect carrier, root prepare
  snapshot, and both candidate validators admit MOD0/MOD1/MOD2 with MOD3
  still deferred; copied mode/selector/address shadows plus live AR/PC
  identity are validated before endpoint issue and at every resume boundary;
  distinct-AR updates publish atomically only after both DATA reads are
  READY and ordinary-effect staging succeeds; a same-selector alias commits
  the one X-owned update; WAIT, FAULT, cancellation, and other nonterminal
  paths remain non-publishing. No opcode row, CPU branch, public ABI,
  catalog, or inventory change is admitted.
- Evidence and architecture decisions: W51A remains the source for MOD0-2
  old-address behavior, AR2--AR5 mapping, and Xmod precedence. Mode-drift
  detection reuses the frozen commits as the mode shadow: the validator
  re-derives the expected commit (zeroed no-update for MOD0/aliased-Y, exact
  non-wrapping decrement/increment for MOD1/MOD2) from the copied spec and
  frozen request address, so no new root bytes were needed and LP64/ILP32
  root sizes stay frozen at 336/308 bytes (the widened snapshot still fits
  the candidate union; both structs hold no pointers). The READY commit
  (ordinary staging plus atomic AR/PC publication) split into the new
  `memory_instruction_dual_commit` owner after the dual hot-path quality
  bound failed at 514 lines; `memory_instruction_dual.c` is back to 448
  lines with orchestration only. The W50-era carrier test now proves
  MOD1/MOD2 valid with MOD3 still rejected.
- Performance is part of closure. The observer-off W50B dual ADD/SUB smoke
  preserves semantic hash `b40823092c0aa605` with exactly two DATA reads per
  retirement across 320,000 retirements per scenario; one dirty-tree sample
  measured 766.5--840.7 ns/instruction, within the earlier volatile host
  spread, with no threshold, regression claim, TI-cycle claim, or history
  artifact. The split commit call sits on the retire path only; MOD0
  retirements stage zeroed commits and publish no AR writes.
- TDD and review: the inherited RED contract first failed on the 32-vs-48
  snapshot size, then went green; the widening exposed two pinned MOD0-only
  expectations (`test_dual_address_resolution` 32-byte static assert,
  `test_dual_word16_memory_effect` mode-1 rejection) that were updated to
  the W51B contract, and the 500-line quality bound forced the commit-owner
  split. Validation is GREEN: focused Make executables pass; focused
  CMake/CTest with ASan/UBSan passes 17/17 dual-adjacent tests plus the full
  CTest suite except the quality.python m32/ILP32 probes below; the full
  compiled C suite passes 258/258 including `minimal`; evidence, manifests,
  inventory, source boundaries, knobs, and observer-disabled structure pass.
  Three Python quality probes (`dual m32 carriers`, `long32 m32 sizes`,
  `dld_dst ilp32 sizes`) fail with SIGSYS under this sandbox; a clean-HEAD
  worktree reproduces two of them identically, and all three die in the
  `-m32` probe binary itself, so they are pre-existing environmental
  failures unrelated to this slice. (LSan also fatals under sandbox ptrace,
  so sanitizer runs use `LSAN_OPTIONS=detect_leaks=0` with ASan/UBSan
  active.) No GitHub or other hosted runner was invoked, and nothing was
  pushed.
- Closure accounting is 208 immutable evidence records; 338 requirements
  (300 IMPLEMENTED, 7 PLANNED, 31 UNRESOLVED); 288 tests (281 PASSING, 7
  PLANNED); and 7 sources. Catalog/ISA counts remain 104 rows,
  `ID_COUNT=105`, 3,031 lookup entries, 104 trusted bindings,
  44/205 family coverage, and 27/205 strict coverage. No knob was added or
  changed. The next architecture-forcing seam is broader dual-mode opcode
  activation through this root; `memory_instruction_dual.c` at 448 lines
  stays under review if it approaches 500 again.

## 2026-09-09 — P2-W51C dual ADD/SUB MOD0–MOD2 activation

- State: implementation commit
  `48ef75dde535993bd94f439d2046c76065125758` activates the sixteen
  non-MOD0/MOD0 syntax-6 dual-memory ADD/SUB rows through the W51B root.
  Each row fixes one (Xmode, Ymode) combination at
  `0xA000|(m<<6)|(n<<2)` (ADD) or `0xA200|(m<<6)|(n<<2)` (SUB) under mask
  `0xFECC`, with the existing `DUAL_DESTINATION_XAR_YAR` layout,
  `UNRESOLVED` repeatability, and
  `AUXILIARY_REGISTERS_STATUS_C_SXM_OVM` resources. Checked and trusted
  leaves share one operation/mode-parameterized core with per-row identity
  validation and completion validators; macro-driven dispatch binds all
  rows with no opcode-specific CPU branch and no public ABI change; all
  nine MOD0–MOD2 combinations per form are inventory `SUBSET` while MOD3
  neighborhoods stay undecoded and deferred. No addressing mode, CPU
  wiring, layout, timing, or benchmark-roster change is admitted.
- Evidence and architecture decisions: the W50B syntax-6 first-word
  evidence remains the source for D/Xar/Yar positions and the ADD/SUB
  discriminator, and the W51A mode evidence for MOD0–MOD2 update behavior
  with Xmod precedence; a clean-decode probe proved all sixteen
  neighborhoods unclaimed before activation. Registry ownership follows the
  W50B per-module split (catalog/decode/effects carry TI evidence;
  trusted-binding/CPU-retirement/inventory carry the project anchor), after
  the evidence gate rejected a single `src/isa/`-owned requirement.
  Dispatch and inventory were already green through shared generic proofs
  and are recorded `IMPLEMENTED` against the all-ID dispatch test and the
  nine-combo binding projection. Each new row carries thirty-two encodings
  (D by XAR by YAR), not sixteen as first estimated; the realized lookup
  grows by 512 to 3,543 populated words. `op_dual_arithmetic.c` grows to
  487 lines and stays under review against the 500-line quality bound.
- Performance is part of closure. The frozen MOD0 roster smoke re-runs
  unchanged as the shared-path characterization with preserved semantic
  hash `b40823092c0aa605` across 320,000 retirements per scenario
  (731.5–791.7 ns/instruction on this host, no threshold or TI-cycle
  claim). No new hosted scenarios were added.
- TDD and review: the four RED contracts (undeclared row identities at
  compile time) went green row by row — metadata, full decode fan-out,
  checked/trusted mode carriage with parity and wrong-identity rejection,
  and public-CPU retirement with per-row AR publication, same-AR Xmod
  precedence, and WAIT persistence. Two self-authored test bugs were caught
  and fixed during GREEN (an accumulator enum unavailable to the ops test,
  and an alias value spot that must read one address twice). Validation is
  GREEN: full compiled C suite 262/262 including `minimal`; focused and
  full CMake/CTest with ASan/UBSan green except the quality.python
  `-m32`/ILP32 probes, which fail with SIGSYS identically at clean HEAD
  and are pre-existing environmental failures (LSan itself fatals under
  sandbox ptrace, so sanitizer runs use `LSAN_OPTIONS=detect_leaks=0`);
  evidence, manifests, inventory, source boundaries, knobs, and
  observer-disabled structure pass. No GitHub or other hosted runner was
  invoked, and nothing was pushed.
- Closure accounting is 210 immutable evidence records; 344 requirements
  (306 IMPLEMENTED, 7 PLANNED, 31 UNRESOLVED); 292 tests (285 PASSING, 7
  PLANNED); and 7 sources. Catalog/ISA counts are 120 rows, `ID_COUNT=121`,
  3,543 lookup entries, 120 trusted bindings, 44/205 family coverage, and
  27/205 strict coverage. No knob was added or changed. The next documented
  work is the repeat-machinery phase (RPT immediate, single-repeat
  cardinality/gating/eligibility, and per-opcode repeatability), then
  AR0/BK indexed/circular addressing including MOD3 dual, then reset/MMR
  edges.

## 2026-09-09 — P2-W52A COMMON repeat-truth closure

- State: implementation commit
  `eaf8902ef8a455b58c35695deab63b6fb92fc6ad` closes the five `PLANNED`
  COMMON repeat requirements with executable TI-evidence-bound contracts
  over the existing W20D/W20E/W48A/W48B leaves. Decode pins the exhaustive
  `0xEC00`–`0xECFF` K neighborhood, the exact-`0xF070` full-range
  extension word, width-mismatch rejection, and explicit unknown-word
  pins. Operation chains the op leaf into `repeat_progress_start` to prove
  the exact operand reaches architectural RC with a bytewise
  `{REPEAT_START, operand}` effect and no status payload. Cardinality pins
  the `#2`-to-three, `#0`-to-one, and `#0xffff`-to-65,536 examples with
  widen-before-add. The IRQ gate pins maskable-plus-NMI deferral at
  remaining counts one and 65,536 with literal RS/reset admission at both
  endpoints and full admission at zero. Eligibility pins the admission
  tri-state, sweeps all 120 rows to a valid admission, spot-pins the
  documented `INELIGIBLE` rejections and the DLD/DST asymmetry, and audits
  the twenty W49B/W50B/W51C rows as staying `UNRESOLVED`.
- Evidence and architecture decisions: no production RPT catalog row, CPU
  repeat branch, public API, or timing change is admitted, so the W20F
  deferral stays in force and the three exact-ordering/PC-flow/predecode
  gaps remain `UNRESOLVED`. Full first-word value/mask metadata pins are
  deferred to the later RPT activation slice that registers production
  rows. The registry `PLANNED` test paths were honored as-is: the five new
  files were renamed to those placeholder paths and the CTest names kept
  the `*_common` suffix. Registry edits were surgical text edits because a
  `json.dump` round-trip is not byte-stable on `config/evidence.json`
  (+29KB reformat); `docs/evidence-index.md` state/runner cells were
  updated in parity, plus one `E-PROJECT-P2-W52A` closure record.
- TDD and review: the eligibility contract caught one self-authored test
  bug during GREEN (`metadata_for_id(UNKNOWN)` returns the index-0
  sentinel row, not NULL; only out-of-enum IDs return NULL). The audit
  found no misclassified row, so production code is unchanged.
  Validation is GREEN: full compiled C suite 266/266 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures (dual-word16, long32-pair,
  dld-dst probes, identical signature at clean HEAD); knobs, source
  boundaries, manifests, evidence, inventory, and observer-disabled
  structure pass. Fresh CMake/CTest with ASan/UBSan passes 272/272
  excluding `quality.python`, which fails only on those same three
  environmental probes (`LSAN_OPTIONS=detect_leaks=0`; LSan itself fatals
  under sandbox ptrace). No GitHub or other hosted runner was invoked,
  and nothing was pushed.
- Closure accounting is 211 immutable evidence records; 344 requirements
  (311 IMPLEMENTED, 2 PLANNED, 31 UNRESOLVED); 292 tests (290 PASSING, 2
  PLANNED); and 7 sources. Catalog/ISA counts remain 120 rows,
  `ID_COUNT=121`, 3,543 lookup entries, 120 trusted bindings, 44/205
  family coverage (21.463%), and 27/205 strict coverage. No knob was added
  or changed. The next documented work is AR0/BK indexed/circular
  addressing including MOD3 dual, then reset/MMR edges
  (`T-COMMON-RESET-SUBSET` and `T-COMMON-BK-MMR-ROUTING` stay `PLANNED`).

## 2026-09-09 — P2-W52B dual MOD3 AR0/BK planning checkpoint

- State: implementation commit
  `21ff12ce3203b4e8d9ba2037dcb52a0a3e7bc7a1` admits dual-indirect MOD3
  (`*ARx+0%`) through the address planner, the sequencer resolution seam,
  and the root commit validator with no opcode row, CPU branch, catalog,
  inventory, or public API change. MOD3 sides plan `addr = ARx` with a
  post-access AR0/BK update: BK=0 stages plain signed `ARx + AR0`
  addition, nonzero BK stages section-5.5.3.4 circular movement, and
  BK=0/AR0=0 plans the documented no-modification read with no update.
  Same-selector aliases keep Xmod precedence with one X-owned update.
  Resolution reads AR0/BK lazily with knownness (unknown rejects; BK is
  never read without a MOD3 side) and proves byte-identical resolution
  under CMPT=0/ARP=0 versus CMPT=1/ARP=7. The persistent dual snapshot
  widens 48 to 52 bytes with AR0/BK shadows for root revalidation while
  the root carriers hold at 336/308 (candidate union dominates).
- Evidence and architecture decisions: the MOD3 gap was closed with five
  new TI SPRU131G records verified against the official Rev G PDF (Table
  5-8 MOD3 row; §§5.5.4.2/5.5.4.3 indexed/circular; Tables 5-6/5-7 plus
  §5.5.5 direct-selector facts bounding the compatibility question; and
  the §5.5.3.4 step rules pinning a signed AR0 step bounded by BK with
  first access inside the buffer). AR0 is read as a signed 16-bit step:
  this makes the pinned algorithm's add-BK branch reachable and matches
  the documented negative-AR0 practice; unsigned-only readings would
  leave decrement use undefined. Indexed overflow/underflow, oversized
  steps, out-of-buffer starts, and results above `0xffff` reject as
  boundaries with no wrap inference. The shared MOD3 computation lives in
  `src/address/indirect_dual_mod3.[ch]` (used by planner and validator);
  dual effects keep kind AR with `STATE_POLICY_NONE`, so every generic
  intrinsic/dependency/apply seam passes unchanged. The full Table 5-8
  text also notes a Y-side MMR read caveat, recorded for the later
  reset/MMR phase.
- TDD and review: three RED contracts went green (planner matrix,
  resolution plus CMPT-invariance, root accept/reject matrix). GREEN
  caught and updated three obsolete MOD3-rejection pins in pre-existing
  tests (planner mode-3 loop, resolution INVALID_SPEC pins) and two size
  pins (snapshot 48 to 52 in two tests); the root carriers needed no
  re-freeze. One self-authored test bug was fixed during GREEN (a
  duplicated check line). A missing `Does not own:` line in the new
  module, caught by the source-boundary gate, was fixed before commit.
  Validation is GREEN: full compiled C suite 269/269 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source boundaries
  (503 files), manifests, evidence, inventory, and observer-disabled
  structure pass. Fresh CMake/CTest with ASan/UBSan passes 275/275
  excluding `quality.python`, which fails only on those same three
  environmental probes (`LSAN_OPTIONS=detect_leaks=0`). No GitHub or
  other hosted runner was invoked, and nothing was pushed.
- Closure accounting is 217 immutable evidence records; 347 requirements
  (314 IMPLEMENTED, 2 PLANNED, 31 UNRESOLVED); 295 tests (293 PASSING, 2
  PLANNED); and 7 sources. Catalog/ISA counts remain 120 rows,
  `ID_COUNT=121`, 3,543 lookup entries, 120 trusted bindings, 44/205
  family coverage (21.463%), and 27/205 strict coverage. No knob was added
  or changed. The next documented work is W52C MOD3 dual ADD/SUB
  activation (fourteen rows: all `(m,n)` in `{0..3}^2` minus the nine done
  in W50B/W51C), then reset/MMR edges.

## 2026-09-09 — P2-W52C dual ADD/SUB MOD3 activation

- Implementation commit `3800574` activates the fourteen MOD3-involving
  syntax-6 dual-memory ADD/SUB rows (`0xA000|(m<<6)|(n<<2)` and
  `0xA200|(m<<6)|(n<<2)` minus the nine MOD0–MOD2 combinations) with mask
  `0xFECC`, one program word, `DUAL_DESTINATION_XAR_YAR` layout,
  `UNRESOLVED` repeatability, and
  `AUXILIARY_REGISTERS_STATUS_C_SXM_OVM` resources. MOD3 sides retire
  through the generic W52B root with AR0/BK snapshots, atomic per-row
  address publication, single X-owned commits for same-AR aliases, and
  destination routing; no opcode-specific CPU branch and no public ABI
  change.
- Evidence and decisions: catalog grows to 134 rows, `ID_COUNT=135`,
  3,991 lookup entries, 134 trusted bindings; family coverage stays
  44/205 (21.463%) with unchanged 27/205 strict coverage since the new
  rows bind SUBSET to the existing `FORM-T2-01-06`/`FORM-T2-02-06`. All
  MOD3-rejection pins became admission pins (C decode/metadata/modes/
  memory-effect, six global `3543U` population pins to `3991U`, Python
  inventory counts to 134/135/3991 and SUBSET 93 to 107). The evidence
  registry gained the two W52C reverse back-links and the completed
  W52B-ROOT index row; closure accounting is 219 immutable evidence
  records, 353 requirements (320 IMPLEMENTED, 2 PLANNED, 31 UNRESOLVED),
  and 299 tests (297 PASSING, 2 PLANNED).
- Correction to the in-flight record: an earlier turn reported the C
  suite green from `make test`, but the `quality-test` prerequisite was
  failing first, so the C recipe never ran; the six stale `3543U` pins
  were caught by the sanitizer CTest instead. The true fast-suite result
  is 273/273 binaries plus `minimal`, verified by direct execution.
- Validation is GREEN: full compiled C suite 273/273 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source boundaries
  (507 files, size-review warnings only), manifests, evidence, inventory,
  and observer-disabled structure pass. Fresh CMake/CTest with ASan/UBSan
  passes 279/279 excluding `quality.python`
  (`LSAN_OPTIONS=detect_leaks=0`; LSan itself fatals under the sandbox
  ptrace restriction). No knob was added or changed. No GitHub or other
  hosted runner was invoked, and nothing was pushed.
- Follow-up work: the next documented work is reset/MMR edges
  (`T-COMMON-RESET-SUBSET` and `T-COMMON-BK-MMR-ROUTING` remain PLANNED).

## 2026-09-09 — Reset documented subset and BK MMR route edges

- Implementation commit `6d8601c` closes the last two PLANNED items:
  `REQ-COMMON-RESET-DOCUMENTED-SUBSET` (owner `src/sequencer/`) and
  `REQ-COMMON-BK-MMR-ROUTING` (owner `src/bus/`).
- Reset: the 25 documented §6.9 values already seeded by
  `ti_c54x_arch_state_reset_common` had no field-level architectural
  proof — 11 documented fields (ST1 BRAF/XF/HM/INTM/C16/FRCT, PMST
  IPTR/OVLY/AVIS/DROM/CLKOFF) had no checked readers at all. Added the
  11 readers in `src/state/status/registers.[ch]` following the existing
  `read_field` pattern (reads only; generic `write_bit` already covers
  writes). New RED contract `tests/unit/sequencer/test_reset.c` (CTest
  `ti_c54x.reset`) proves every documented value reads back known after
  seeding dirty state, SP/AR0-AR7/BK stay unknown, NULL rejects, and
  asserts nothing for XPC, MP/MC, SMUL/SST, or reserved ST1 bit 10.
- BK MMR: new `src/bus/mmr/bk_route.[ch]` owns the Table 3-3 address
  constant (`0x0019`) and a pure router over borrowed request/state:
  DATA reads service known BK, DATA writes replace BK, unknown-BK reads
  decline unknown, other spaces/addresses decline untouched, and NULL or
  malformed requests reject. State access stays in `src/state`. New RED
  contract `tests/unit/bus/test_bk_mmr.c` (CTest `ti_c54x.bk_mmr`).
  Mapping record only; full MMR/device routing remains future work.
- Validation is GREEN: fast C suite 275/275 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source boundaries
  (511 files), manifests, evidence (353 requirements, 299 tests),
  inventory, and observer-disabled structure pass. Fresh CMake/CTest
  with ASan/UBSan passes 281/281 excluding `quality.python`
  (`LSAN_OPTIONS=detect_leaks=0`). Registry closure is 219 immutable
  evidence records, 353 requirements (322 IMPLEMENTED, 0 PLANNED, 31
  UNRESOLVED), and 299 tests (299 PASSING, 0 PLANNED). No knob was added
  or changed. Nothing was pushed.
- Follow-up work: ISA family expansion — LD (17 unimplemented forms)
  and ST/STH/STL stores (20) are the biggest coverage blocks, followed
  by the ADD/SUB remainder and the MAC/MPY families.

## 2026-09-09 — P2-W53 LD Smem[,SHIFT],dst vertical slice

- Implementation commit `b059acd` activates Table 2-19 form
  `FORM-T2-19-05`, TI SPRU172C §4 LD syntax 4: one row `LD_SMEM_SHIFT`
  at first-word value `0x6F80` under mask `0xFFE0` (32 encodings), two
  program words with D in second-word bit 8 and signed five-bit SHIFT
  in bits 4-0, a dedicated `ACCUMULATOR_ARF_MOD_SHIFT5` layout,
  `UNRESOLVED` repeatability, and new `STATUS_SXM_OVM` resources.
- Semantics is `dst = SXM-widened Smem << SHIFT` with guard-vs-bit31
  overflow detection (proven by the TI `LD A,8,B` example), OVM
  saturation to `0x007FFFFFFF`/`0xFF80000000`, sticky destination
  overflow, and C/TC preserved. MOD0-MOD2 admit through the W42A seam;
  MOD3 rejects `INVALID_OPERAND` before any MEMORY effect. The
  second-word fixed pattern (`0xFEE0`/`0x0C40`) is enforced on both the
  checked leaf and the trusted leaf, which receives the full bundle via
  the execution packet — this closes a real hole, since production CPU
  execution runs the trusted fast path.
- Evidence: three new TI records (syntax-4 opcode, shift semantics,
  T2-28 repeatability boundary), plan anchor plus activation records,
  six requirements, and four RED contracts (metadata, decode, ops with
  an independent saturation oracle and four-way parity, CPU with a
  local two-word fixture). FORM-T2-19-05 is inventory SUBSET; the
  trusted-binding and inventory-catalog-binding tests extend their
  back-links. SHIFT=0 decodings are admitted (the TI assembler note
  records assembly equivalence, not a decode rejection).
- Validation is GREEN: fast C suite 279/279 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source boundaries
  (515 files), manifests, evidence (359 requirements with 328
  IMPLEMENTED, 303 tests all PASSING), inventory (45/205 family,
  27/205 strict), and observer-disabled structure pass. Fresh
  CMake/CTest with ASan/UBSan passes 285/285 excluding
  `quality.python` (`LSAN_OPTIONS=detect_leaks=0`). Registry closure is
  224 immutable evidence records. No knob was added or changed
  (STATUS_SXM_OVM is a resource token, not a knob). Nothing was pushed.
- Follow-up work: LD syntaxes 5-10 (`FORM-T2-19-06` through `-11`)
  remain the next LD block; the ordinal mapping is confirmed
  (05=Smem-SHIFT, 06=Xmem-SHFT dual, 07=#K, 08=#lk-SHFT, 09=#lk-16,
  10=src-ASM, 11=src-SHIFT).

## 2026-09-09 — P2-W54 LD src[,SHIFT],dst vertical slice

- Implementation commit `d3d53c5` activates Table 2-19 form
  `FORM-T2-19-11`, TI SPRU172C §4 LD syntax 10: one row `LD_SRC_SHIFT`
  at first-word value `0xF440` under mask `0xFCE0` (128 encodings),
  one program word with source in bit 9, destination in bit 8, and
  signed five-bit SHIFT in bits 4-0, reusing the
  `ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout, `UNRESOLVED`
  repeatability, and `ACCUMULATORS_STATUS_SXM_OVM` resources.
- Semantics is `dst = src40 << SHIFT` with no SXM widening of the
  40-bit source: SXM=0 right shifts are logical, SXM=1 arithmetic;
  guard-vs-bit31 overflow detection drives OVM saturation to
  `0x007FFFFFFF`/`0xFF80000000` with sticky destination overflow, and
  C/TC stay preserved. D==S encodings produce the documented OVsrc
  case through the destination-selected OV bit. No memory is touched:
  no AR dependency, no endpoint traffic, single-word PC advance.
- Evidence: three new TI records (syntax-10 opcode, shift semantics,
  T2-28 repeatability boundary), plan anchor plus activation records,
  six requirements, and four RED contracts (metadata, decode, ops with
  an independent oracle, CPU with a 4096-combination sweep plus
  OVM-off and unknown-source rejection proofs).
  `FORM-T2-19-11` is inventory COMPLETE; the trusted-binding and
  inventory-catalog-binding tests extend their back-links.
- Decisions: the RED plan text predicted `STATUS_SXM_OVM` and
  unchanged 27/205 strict coverage; implementation corrects both
  (`ACCUMULATORS_STATUS_SXM_OVM` is required or collection leaves
  both accumulators unknown and every execution rejects; the new
  COMPLETE form moves strict coverage to 28/205). Two test-side bugs
  were fixed, not production: the CPU word builder placed SHIFT at
  bits 10:6 instead of 4:0, and the untouched-accumulator oracle
  wrongly expected the shifted value when src==dst.
- Validation is GREEN: fast C suite 283/283 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source
  boundaries (521 files), manifests, evidence (365 requirements with
  334 IMPLEMENTED, 307 tests all PASSING), inventory (46/205 family
  22.439%, 28/205 strict), and observer-disabled structure pass.
  Fresh CMake/CTest with ASan/UBSan passes 289/289 excluding
  `quality.python` (`ASAN_OPTIONS=detect_leaks=0`; LSan itself cannot
  run under this container's ptrace). Registry closure is 229
  immutable evidence records. No knob was added or changed. Nothing
  was pushed.
- Follow-up work: LD syntaxes 5-9 (`FORM-T2-19-06` through `-10`)
  remain the next LD block, then ST/STH/STL stores; ADD/SUB remainder
  and MAC/MPY families follow.

## 2026-09-10 — P2-W55 LD #K,dst vertical slice

- Implementation commit `f767fdf` activates Table 2-19 form
  `FORM-T2-19-07`, TI SPRU172C §4 LD syntax 6: one row `LD_K` at
  first-word value `0xE800` under mask `0xFE00` (512 encodings), one
  program word with destination in bit 8 and unsigned 8-bit K in bits
  7-0, a new `ACCUMULATOR_IMMEDIATE_K8` layout, `UNRESOLVED`
  repeatability, and `NONE` input resources. Ground truth is dual-
  sourced: the SPRU172C mnemonic mirror and the SPRU179C algebraic
  volume agree on all ten LD opcode diagrams.
- Semantics is `dst = K` zero-extended: TI Example 5 (`LD #248, B`
  with SXM=1 yields `00 0000 00F8`) proves SXM is ignored, and with
  no SHIFT or ASM shift there is no OVM/OV effect, so C/TC/OVA/OVB
  stay preserved with no status inputs collected. No memory touched.
- Evidence: three new TI records (syntax-6 opcode, zero-extension
  semantics, T2-28 repeatability boundary), plan anchor plus
  activation records, six requirements, and four RED contracts
  (metadata, decode with the `0xEA00` DP-sibling pin, ops exhaustive
  over all 512 encodings with hostile-input independence and
  five-way parity, CPU over all 512 encodings with full status
  preservation). `FORM-T2-19-07` is inventory COMPLETE; the
  trusted-binding and inventory-catalog-binding tests extend their
  back-links. The row is reset-runnable (no inputs required).
- One neighbor update: `test_ld_status_fields_decode` dropped two
  near-miss entries (`0xE800` unregistered claim, `0xE900` DP-fixed
  claim) that are now valid `LD_K` encodings.
- Validation is GREEN: fast C suite 287/287 including `minimal`;
  contract quality unittest 265/268 with only the three pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source
  boundaries (527 files), manifests, evidence (371 requirements with
  340 IMPLEMENTED, 311 tests all PASSING), inventory (47/205 family
  22.927%, 29/205 strict), and observer-disabled structure pass.
  Fresh CMake/CTest with ASan/UBSan passes 293/293 excluding
  `quality.python` (`ASAN_OPTIONS=detect_leaks=0`; LSan itself cannot
  run under this container's ptrace). Registry closure is 234
  immutable evidence records. No knob was added or changed. Nothing
  was pushed.
- Follow-up work: LD syntaxes 5, 7, 8, 9 (`FORM-T2-19-06`, `-08`,
  `-09`, `-10`: Xmem dual, #lk immediate pair, src-ASM) remain, then
  ST/STH/STL stores; ADD/SUB remainder and MAC/MPY families follow.

## 2026-09-10 — P2-W56 LD src,ASM[,dst] vertical slice

- Implementation commit `9f7749c` activates Table 2-19 form
  `FORM-T2-19-10`, TI SPRU172C §4 LD syntax 9: one row `LD_SRC_ASM`
  at first-word value `0xF482` under mask `0xFCFF` (4 encodings),
  one program word with source in bit 9 and destination in bit 8,
  reusing the `ACCUMULATOR_SOURCE_DESTINATION` layout,
  `UNRESOLVED` repeatability, and a new
  `ACCUMULATORS_STATUS_SXM_OVM_ASM` resource combination. Ground
  truth is dual-sourced (SPRU172C mnemonic mirror plus the SPRU179C
  algebraic volume, which agree on all ten LD opcode diagrams).
- Semantics is `dst = src << ASM` with the 5-bit ASM field read
  signed (-16..15): SXM selects logical versus arithmetic right
  shift exactly as in W54, OVM saturates on guard-bit overflow with
  sticky destination overflow, and C/TC stay preserved. No memory
  touched. The new `STATUS_ASM` input bit (256) extends the input
  snapshot 48→56 bytes and the memory transaction 336→344 bytes,
  with an independent carrier proof; twelve 512-byte CPU test
  storages move to 1024 and frozen perf carrier sizes follow.
- Evidence: three new TI records (syntax-9 opcode, ASM-shift
  semantics, T2-28 repeatability boundary), plan anchor plus
  activation records, seven requirements (including the ASM carrier),
  and five RED contracts (metadata, decode with five sibling pins,
  ops exhaustive over S×D×ASM32 with an independent oracle and
  four-way parity, ASM carrier, CPU sweep with unknown-ASM
  rejection). `FORM-T2-19-10` is inventory COMPLETE; the
  trusted-binding and inventory-catalog-binding tests extend their
  back-links. The row is not reset-runnable (accumulators start
  unknown).
- Validation is GREEN: fast C suite 292/292 including `minimal`;
  contract quality unittest 264/268 with only the four pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source
  boundaries (534 files), manifests, evidence (378 requirements with
  347 IMPLEMENTED, 316 tests all PASSING), inventory (48/205 family
  23.415%, 30/205 strict), and observer-disabled structure pass.
  Fresh CMake/CTest with ASan/UBSan passes 298/298 excluding
  `quality.python` (`ASAN_OPTIONS=detect_leaks=0`; LSan itself cannot
  run under this container's ptrace). Registry closure is 239
  immutable evidence records. No knob was added or changed. Nothing
  was pushed.
- Follow-up work: LD syntaxes 5, 7, 8 (`FORM-T2-19-06`, `-08`,
  `-09`: Xmem dual, #lk immediate pair) remain, then ST/STH/STL
  stores; ADD/SUB remainder and MAC/MPY families follow.

## 2026-09-10 — P2-W57 LD #lk,16,dst vertical slice

- Implementation commit `8389161` activates Table 2-19 form
  `FORM-T2-19-09`, TI SPRU172C §4 LD syntax 8: one row `LD_LK_16` at
  first-word value `0xF062` under mask `0xFEFF` (two encodings),
  two program words with destination in bit 8 and the copied raw
  16-bit `lk` in word 1, a new `ACCUMULATOR_IMMEDIATE_LK16` layout,
  `UNRESOLVED` repeatability, and `STATUS_SXM` input resources.
  Ground truth is dual-sourced: the SPRU172C mnemonic mirror (printed
  pp. 4-66–4-68) and the SPRU179C algebraic volume (p. 4-67) agree on
  all ten LD opcode diagrams.
- Semantics is `dst = SXM-widen(lk) << 16`: SXM=0 zero-extends,
  SXM=1 sign-extends bit 15, followed by logical left shift 16 in
  canonical 40-bit space. Pinned vectors mirror the fixed-16 W38
  precedent (`0000` zero both modes; `7fff` `0x007fff0000`;
  `8000` `0x0080000000`/`0xff80000000`; `ffff`
  `0x00ffff0000`/`0xffffff0000`). C/TC/OV stay preserved with no OVM
  input; unknown SXM rejects transactionally. No memory touched.
- Evidence: three new TI records (syntax-8 opcode, shift-16
  semantics with the no-worked-lk-example caveat, T2-28
  repeatability boundary), plan anchor plus activation records, six
  requirements, and four RED contracts (metadata with 65,536-word
  disjointness sweep, decode with SFTL/OR/AND-immediate sibling
  pins, ops exhaustive over D×65536 lk×SXM with five-way parity
  and an independent oracle, CPU sweep with unknown-SXM
  rejection). `FORM-T2-19-09` is inventory COMPLETE; the
  trusted-binding and inventory-catalog-binding tests extend their
  back-links. The row is reset-runnable (SXM is reset-known).
- Review correction: independent review caught a bit-1 misread in
  the first implementation (`0xF060`; the diagram low byte
  `0110_0010` is `0x62`). The value, handler identity checks,
  near-miss pins (now SFTL/OR/AND-immediate siblings), and all
  evidence/plan assertions were corrected to `0xF062`/`0xF162`
  before commit; the corrected lookup nests cleanly between the
  XOR/AND immediate rows.
- Validation is GREEN: fast C suite (all binaries incl. the four
  new runners); contract quality unittest 264/268 with only the
  four pre-existing `-m32`/ILP32 SIGSYS environmental failures;
  knobs, source boundaries (540 files, SIZE_REVIEW warnings only),
  manifests, evidence (384 requirements with 353 IMPLEMENTED, 320
  tests all PASSING), inventory (49/205 family 23.902%, 31/205
  strict), and observer-disabled structure pass. Focused
  ASan/UBSan 4/4 GREEN (`ASAN_OPTIONS=detect_leaks=0`; LSan
  cannot run under this container's ptrace). Fresh CMake/CTest
  Debug passes 302/303 excluding `quality.python` (same four
  environmental subfailures); full sanitizer CMake/CTest passes
  302/302 excluding `quality.python`. Registry closure is 244
  immutable evidence records. No knob was added or changed.
  Nothing was pushed.
- Decisions: `operand_layout.c` retained cohesive at 731 lines
  (+21); the standing split-before-800 rule binds, so W58 must
  split before adding another layout. A `repeat_pending_request`
  CTest failure seen only under a non-CI `-O3` Release configure
  is a pre-existing struct-padding sensitivity unrelated to this
  slice (no state/sequencer file touched); the CI `Debug`
  configure is GREEN.
- Follow-up work: LD syntaxes 5, 7 (`FORM-T2-19-06` Xmem/SHFT
  dual, `FORM-T2-19-08` #lk/SHFT pair) remain, then ST/STH/STL
  stores; ADD/SUB remainder and MAC/MPY families follow.

## 2026-09-10 — P2-W58 LD #lk[,SHFT],dst vertical slice

- Prerequisite refactor commit `30388b8` splits `operand_layout.c`
  (731 lines) into the static piece/set tables file
  `operand_layout_sets.c` (340 lines) and the plans/resolver file
  (410 lines), with set objects externed via
  `operand_layout_internal.h`. Behavior-preserving: full suite
  green modulo the four pre-existing `-m32`/ILP32 environmental
  failures before and after.
- Implementation commit `d4cc3fe` activates Table 2-19 form
  `FORM-T2-19-08`, TI SPRU172C §4 LD syntax 7: one row
  `LD_LK_SHIFT` at first-word value `0xF020` under mask `0xFEF0`
  (destination bit 8, unsigned 4-bit SHFT bits 3:0, 32 encodings),
  two program words with the copied raw 16-bit `lk` in word 1, a
  new `ACCUMULATOR_IMMEDIATE_SHIFT4_LK16` layout, `UNRESOLVED`
  repeatability, and `STATUS_SXM_OVM` input resources. Ground
  truth is dual-sourced (SPRU172C pp. 4-66–4-68, SPRU179C p. 4-67).
- Semantics is `dst = SXM-widen(lk) << SHFT` with OVM saturation
  and sticky destination overflow on guard-bit overflow following
  the variable-shift W53/W54 precedent; C/TC stay preserved and
  unknown SXM/OVM rejects transactionally. Omitted SHFT is 0; the
  assembler redirect of SHFT=0 small lk to syntax 6 does not
  narrow decode.
- Evidence: three new TI records (syntax-7 opcode with assembler
  notes, shift semantics, T2-28 repeatability boundary), plan
  anchor plus activation records, six requirements, and four RED
  contracts (metadata with 65,536-word disjointness sweep, decode
  with AND/OR-ACC/SUB-ACC sibling pins, ops exhaustive over
  2×16×65536×2×2 with five-way parity and an independent
  saturation oracle, CPU sweep with unknown-SXM/OVM rejection).
  `FORM-T2-19-08` is inventory COMPLETE; the trusted-binding and
  inventory-catalog-binding tests extend their back-links. The W57
  decode test converts its `^0x0040` pin to an `LD_LK_SHIFT`
  sibling (`0xF022`, D=0 SHFT=2) as anticipated.
- Validation is GREEN: all 300 C test binaries pass; contract
  quality unittest 264/268 with only the four pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source
  boundaries (547 files, SIZE_REVIEW warnings only), manifests,
  evidence (390 requirements with 359 IMPLEMENTED, 324 tests all
  PASSING), inventory (50/205 family 24.390%, 32/205 strict), and
  observer-disabled structure pass. Focused ASan/UBSan 4/4 GREEN
  (`ASAN_OPTIONS=detect_leaks=0`). Fresh CMake/CTest Debug passes
  306/307 excluding `quality.python` (same four environmental
  subfailures); full sanitizer CMake/CTest passes 306/306
  excluding `quality.python`. Registry closure is 249 immutable
  evidence records. No knob was added or changed. Nothing was
  pushed.
- Process finding: the Makefile `test` recipe runs the Python
  quality gate before the C binaries, so a C regression hides
  behind the known environmental quality failure. Full C
  validation here ran all 300 binaries explicitly plus CTest;
  future slices must do the same until the recipe order is fixed.
- Follow-up work: LD syntax 5 (`FORM-T2-19-06` Xmem/SHFT dual,
  needs the dual-addressing seam) remains, then ST/STH/STL
  stores; ADD/SUB remainder and MAC/MPY families follow.

## 2026-09-10 — P2-W59 LD Xmem,SHFT,dst vertical slice

- Implementation commit `fcc9ead` activates Table 2-19 form
  `FORM-T2-19-06`, TI SPRU172C §4 LD syntax 5 (pp. 4-66–4-68,
  cross-checked against SPRU179C p. 4-67): one row
  `LD_XMEM_SHIFT` at first-word value `0x9400` under mask `0xFE00`
  (destination bit 8, Xmod bits 7:6, Xar bits 5:4, unsigned 4-bit
  SHFT bits 3:0, 512 encodings), one program word, a new
  `ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4` layout, `UNRESOLVED`
  repeatability, and new `AUXILIARY_REGISTERS_STATUS_SXM_OVM`
  input resources (no carry is read, so the existing
  `..._STATUS_C_SXM_OVM` combo would be dishonest).
- Semantics is `dst = SXM-widen(Xmem) << SHFT` in canonical
  40-bit space with OVM saturation and sticky destination
  overflow on guard-bit overflow following the variable-shift
  precedent; C/TC stay preserved. SHFT spans 0..15 at decode
  (the assembler's SHFT=0 redirect to syntax 1 constrains the
  assembler, never decode).
- W59A seam: new `DUAL_INDIRECT_WORD16_X_ONLY` address kind
  reusing the pair planner with Y self-aliased, packaged as a
  single WORD16 read through the single-read child (one D-bus
  DATA read, X commit only, no CMPT/ARP observation, AR0/BK
  shadows for MOD3). Wiring: spec validation in `memory_effect`
  and the single resolver, per-mode operand/update forgery
  rechecks plus a CMPT/ARP-free live-match arm in the candidate
  validator, and a CHILD_WORD16 flow-through with no endpoint
  change. The dual-pair instruction path stays untouched for the
  parallel forms.
- Evidence: four new TI records (syntax-5 opcode, shift
  semantics, SPRU131G §5.5.4/Figure 5-11/Tables 5-6–5-8 dual
  addressing with the D-bus note, T2-28 repeatability boundary),
  plan anchor plus activation records, six requirements, and four
  RED contracts (metadata with 65,536-word disjointness sweep,
  decode exhaustive over all 512 encodings with raw-Xar
  extraction, ops exhaustive over 512 forms with independent
  saturation oracle and checked/trusted/dispatched/trusted-packet
  parity, CPU with X-commit-only, MOD3 indexed/circular, fault,
  and boundary rejection). Anchor corrected 254→255 evidence
  records; the staged plan assertion was fixed to admit the one
  X-only address kind. `FORM-T2-19-06` is inventory COMPLETE.
- Review findings during construction: the RED decode test first
  asserted mapped AR2..AR5 extraction until the dual-arithmetic
  sibling showed decode exposes the raw Xar field with the +2
  mapping in the handler; the RED ops forgery flip hit variable
  bit 6 and was moved to fixed bit 9; the first CPU run exposed
  the `matches_live` CMPT0/ARP0 gate rejecting policy-NONE
  candidates, fixed with the dedicated live-match arm. A
  delegated adversarial review was spawned but its result proved
  unretrievable; the orchestrator performed the read-only seam
  review pass directly instead. No finding survived.
- Validation is GREEN: all 304 C test binaries pass; contract
  quality unittest 264/268 with only the four pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source
  boundaries, manifests, evidence (396 requirements with 365
  IMPLEMENTED and 31 UNRESOLVED, 328 tests all PASSING),
  inventory (51/205 family 24.878%, 33/205 strict), and
  observer-disabled structure pass. Focused ASan/UBSan 4/4 GREEN
  (`ASAN_OPTIONS=detect_leaks=0`, then `make clean`). Fresh
  CMake/CTest Debug passes 310/311 excluding `quality.python`
  (same four environmental subfailures); full sanitizer
  CMake/CTest passes 310/310 excluding `quality.python`.
  Registry closure is 255 immutable evidence records. No knob was
  added or changed. Nothing was pushed.
- Follow-up work: remaining LD syntax (LDM/ST MMR need the
  device seam; LTD/MACD need the delay seam), then ST/STH/STL
  stores (parallel ST needs the proven dual-pair path); ADD/SUB
  remainder (ADDM/SUBM need RMW ADD/SUB) and MAC/MPY families
  follow. Open: fix the `make test` recipe order so C
  regressions cannot hide behind the environmental quality
  failure.

## 2026-09-10 — P2-W60 LDR Smem,dst vertical slice

- Implementation commit `c913f37` activates Table 2-19 form
  `FORM-T2-19-19` as `SUBSET`, TI SPRU172C §4 LDR (printed p. 4-78,
  cross-checked against the SPRU179C algebraic volume p. 4-78,
  which agrees on opcode diagram, execution, and worked example):
  one row `LDR` at first-word value `0x1680` under mask `0xFEE0`
  (destination in bit 8, MOD0--MOD2 single-indirect Smem byte
  mirroring the `LD_SMEM` row, one program word, 64 encodings
  with MOD3 patterns admitted at decode and rejected
  transactionally at effect validation as write-only). The
  existing `ACCUMULATOR_ARF_MOD` layout carries destination, ARF,
  and MOD; repeatability is `UNRESOLVED` (Table 2-28 lists only
  `LD ARP`/`LD DP` among loads); input resources are
  `STATUS_SXM` (status bits say SXM only: no OVM input, no
  saturation, no OV effect).
- Semantics is `dst = (SXM-widen(Smem) << 16) + 0x8000` with bits
  14:0 cleared, i.e. the guard bits are the SXM sign extension,
  bits 31:16 are Smem verbatim, and bits 15:0 are always `0x8000`;
  C, TC, and OV stay preserved. The worked example pins
  `LDR *AR1,A` with Smem `0xFEDC` and SXM 0 to `00 FEDC 8000`.
  The checked/trusted leaves reuse the Smem common
  identity/mode validation and single-indirect staging with an
  LDR-specific rounding completion. Unknown SXM rejects
  transactionally. No new layout, address kind, effect kind, CPU
  branch, public ABI, timing, or device/MMR claim is admitted.
  MOD4--MOD15 Smem variants stay deferred to later slices
  following the W43B/W44B pattern.
- Evidence: three new TI records (opcode, semantics, T2-28
  repeatability boundary), plan anchor plus activation records,
  six requirements, and four RED contracts (metadata with
  65,536-word disjointness sweep, decode exhaustive over all 64
  encodings with fixed-bit pins onto `LDU_SMEM`/`LD_SMEM_TS`,
  ops exhaustive over 48 MOD0--MOD2 forms with independent
  rounding oracle plus MOD3 write-only rejection and
  checked/trusted/dispatched/trusted-packet parity, CPU with
  commit/rounding/fault/boundary coverage and no-overflow
  assertion). `FORM-T2-19-19` is inventory `SUBSET` under
  `STANDARD_INDIRECT_MOD0_MOD3`.
- Review findings during construction: the LDU decode near-miss
  pin on bit 10 flipped onto the new LDR row and was repinned
  as the W60 sibling (cross-slice near-miss rule); strict
  complete-form coverage correctly stays 33/205 because the form
  lands as `SUBSET` (family coverage advances to 52/205); the
  frozen SUBSET symbol set and `MOD0_MOD3` addressing expectation
  gained `LDR`.
- Correction to the W59 entry: the delegated adversarial review
  verdict arrived after the W59 commits landed and reports no
  correctness defects in the four focus areas (X-only resolver
  branch, candidate validator arms, effect shape check, LDR is
  out of its scope). The W59 ledger note claiming the result
  proved unretrievable is superseded by this verdict.
- Validation is GREEN: all 308 C test binaries pass; contract
  quality unittest 264/268 with only the four pre-existing
  `-m32`/ILP32 SIGSYS environmental failures; knobs, source
  boundaries, manifests, evidence (402 requirements with 371
  IMPLEMENTED and 31 UNRESOLVED, 332 tests all PASSING),
  inventory (52/205 family 25.366%, 33/205 strict), and
  observer-disabled structure pass. Focused ASan/UBSan 4/4 GREEN
  (`ASAN_OPTIONS=detect_leaks=0`, then `make clean`). Fresh
  CMake/CTest Debug passes 314/315 excluding `quality.python`
  (same four environmental subfailures); full sanitizer
  CMake/CTest passes 314/314 excluding `quality.python`.
  Registry closure is 260 immutable evidence records. No knob was
  added or changed. Nothing was pushed.
- Follow-up work: LDR MOD4--MOD15 variants, remaining LD Smem
  MOD9/MOD11 circular-AR0 plus MOD12--MOD15 long/absolute seams,
  LDM/ST MMR (device seam), LTD/MACD (delay seam), then
  ST/STH/STL stores; ADD/SUB remainder (ADDM/SUBM need RMW
  ADD/SUB) and MAC/MPY families follow. Open: fix the `make
  test` recipe order so C regressions cannot hide behind the
  environmental quality failure.

## 2026-09-10 — P2-W61 LDR Smem,dst indexed MOD5/MOD6 vertical slice

- Implementation commit `798c453` activates two Table 2-19 rows under
  `FORM-T2-19-19` as `SUBSET`, TI SPRU172C §4 LDR (printed p. 4-78,
  cross-checked against the SPRU179C algebraic volume p. 4-78) with
  AR0-indexed Smem per SPRU131G Table 5-4 (printed pp. 5-10–5-11):
  `LDR_MOD5` at first-word value `0x16A8` (MOD5 `*ARx-AR0` with
  `ARx = ARx - AR0`) and `LDR_MOD6` at `0x16B0` (MOD6 `*ARx+AR0`),
  both under mask `0xFEF8` with the `ACCUMULATOR_ARF_MOD4` layout
  (destination, ARF, 4-bit mode), one program word, `UNRESOLVED`
  repeatability, and `STATUS_SXM` resources, 16 encodings each.
  Semantics reuses the W60 rounding completion
  (`dst = (SXM-widen(Smem) << 16) + 0x8000`, C/TC/OV preserved, no
  OVM input) over the existing `INDEXED_AR0_WORD16` transaction
  seam with selected-AR post-update; borrow/carry boundary rejects,
  AR0 must be known, and unknown SXM rejects transactionally. One
  program word retires at `PC+1`. No new layout, address kind,
  effect kind, CPU branch, public ABI, timing, or device/MMR claim
  is admitted. The form stays `SUBSET` (MOD4/MOD7 reverse-carry and
  MOD8–MOD15 remain deferred).
- Evidence: TI opcode/addressing records reused from W60 plus the
  indexed MOD5/MOD6 composition records, six requirements
  (`REQ-COMMON-P2-W61-LDR-INDEXED-CATALOG/DECODE/EFFECTS`,
  `REQ-PROJECT-P2-W61-LDR-INDEXED-TRUSTED-BINDING/CPU-RETIREMENT/
  INVENTORY`), and four RED contracts extending the shared indexed
  tables (metadata rows 20→22, decode, load effects, CPU
  retirement) instead of adding new files, per the W43B
  shared-family convention. Registry closure is 263 immutable
  evidence records, 408 requirements (377 `IMPLEMENTED`, 0
  `PLANNED`, 31 `UNRESOLVED`), and 336 tests (336 `PASSING`).
  Coverage is 144 catalog rows, `ID_COUNT=145`, 5,309 lookup
  entries, 144 trusted bindings, family coverage 52/205 (25.366%),
  and 33/205 strict complete-form coverage (`FORM-T2-19-19` stays
  `SUBSET`; W61 adds rows to an already-covered family, so family
  coverage is unchanged from W60).
- Anchor correction: the W61 `(planned)` anchor as committed claims
  family coverage 53/205 (25.854%); the inventory checker and the
  covered-form count both prove 52/205 (25.366%). The anchor is
  corrected in the follow-up docs commit, never the tests or the
  checker. No sibling-slice finding was raised against W61.
- Validation is GREEN with one recorded baseline deviation: all
  308 C test binaries pass; contract quality unittest 267/268 with
  the single failure `test_supported_m32_assertions_compile`
  (W49A carrier-ABI probe), which compiles
  `tests/unit/cpu/input/test_collect_ovm_status.c` under `-m32`
  and trips its `sizeof(ti_c54x_op_input_snapshot) == 56U` ILP32
  assertion (message text still says 48 bytes). That file and
  `src/isa/ops/op_input.h` were last touched in the W56 era and
  are outside the W61 diff, and the failing test belongs to the
  known-good `-m32`/ILP32 family recorded in the runbook, so it is
  carried as baseline, not a W61 regression; the stale 48-byte
  messages and the optimistic ILP32 size expectation stay open as
  W49A-owner follow-up and were deliberately not fixed in this
  closure. Knobs, source boundaries (with the standing
  SIZE_REVIEW warning on the 620-line RMW transaction test),
  manifests, evidence, inventory, and observer-disabled structure
  pass. Focused ASan/UBSan 4/4 GREEN
  (`ASAN_OPTIONS=detect_leaks=0`, then `make clean` plus a normal
  rebuild). Fresh CMake/CTest Debug passes 314/315 with only
  `quality.python` failing (the same single unittest failure);
  full sanitizer CMake/CTest passes 314/314 excluding
  `quality.python`. `git diff --check` is clean. No knob was added
  or changed. Nothing was pushed.
- Follow-up work: LDR MOD4/MOD7 reverse-carry and MOD8–MOD15
  variants; the P3 5110-DSP execution focus section now sequences
  control flow, STM/STH, repeat, MAC/MPY, MMR/stack, bit ops, and
  port I/O ahead of remaining LD-family variants. Open: fix the
  `make test` recipe order so C regressions cannot hide behind the
  environmental quality failure.

## 2026-09-11 — P2-W62 B pmad unconditional-branch vertical slice

- Implementation commit `84b23a1` activates Table 2-12 form
  `FORM-T2-12-01` as `SUBSET`, TI SPRU172C §4 `B[D]` (printed
  p. 4-14, cross-checked against the SPRU179C algebraic volume
  `goto pmad`, class 29A, §4-14, which agrees on opcode diagram,
  `PC = pmad(15-0)` execution, and the worked example): one row
  `B_PMAD` at first-word value `0xF073` as an `EXACT` row (bits
  `1111_00Z0_0011_0011` with Z=0; the lookup generator rejects a
  full `0xFFFF` mask, so the single encoding is exact rather than
  masked), two program words. The new `BRANCH_PMAD16` layout
  carries the copied 16-bit pmad from the second bundle word;
  repeatability is `INELIGIBLE` on the manual's explicit "not
  repeatable" note plus the Table 2-28 absence boundary; input
  resources are `NONE`.
- Semantics is `PC = pmad` with all other state preserved; the
  worked example pins `B 2000h` with `PC=1F45` to `PC=2000`, and
  the CPU test retires exactly that vector from a nonzero origin.
  Z=1 (deferred delayed `BD`) decodes `UNKNOWN_WORD` under the
  cross-slice hazard rule. The slice adds the first PC-target
  seam: a new `TI_C54X_OP_EFFECT_PC_WRITE` kind carrying one
  copied `uint16_t` pmad, a `src/isa/ops/branch/op_b_pmad.[ch]`
  pure checked/trusted leaf, a
  `src/sequencer/program_pc_target.[ch]` planner in the bridge's
  transactional style, and a `run.c` route that commits the target
  atomically with the staged effect transaction instead of the
  sequential bridge plan. `effect_transaction` is untouched: its
  default arm safely rejects the new kind, keeping its
  no-program-counter-effects contract true. No MEMORY endpoint,
  new address kind, status/accumulator/T effect, public ABI,
  timing, device/MMR, or delay-slot claim is admitted.
- Evidence: three new TI records (opcode, semantics, explicit
  nonrepeatable note), plan anchor plus activation records, six
  requirements, and four RED contracts (metadata with 65,536-word
  disjointness sweep and single-encoding population, decode with
  Z=1 and four probed sibling pins plus twelve UNKNOWN flips,
  ops exhaustive over all 65,536 pmads with five-path parity and
  NONE-resource input independence, CPU with verbatim-target
  retirement, worked-example origin, and second-word fault
  transactionality). `FORM-T2-12-01` is inventory `SUBSET` under
  `ABSOLUTE_PMAD_NONDELAYED`; the subset schema forbids `ALL` for
  SUBSET bindings, so the absolute mode carries its own token.
- Review findings during construction: the generator's partial-mask
  rule forced the `EXACT` representation (recorded above, not a
  semantic change); a RED ops-test sequencing bug overwrote the
  rejection baseline with the OK-case effect and was fixed in the
  test, not the product; the frozen inventory binding test needed
  its SUBSET symbol set, count 111→112, and a `B_PMAD` addressing
  arm. The `make ... | head` SIGPIPE truncation that hid a build
  is recorded as a session-hygiene note: always log builds to a
  file, never filter through `head`.
- Validation is GREEN with one recorded baseline deviation: all
  312 C test binaries pass; contract quality unittest passes
  except the single pre-existing `test_supported_m32_assertions`
  `-m32`/ILP32 failure carried as baseline since W61 (W49A-owned,
  outside this diff); knobs, source boundaries (568 files, review
  warnings only), manifests, evidence (268 records, 414
  requirements with 383 `IMPLEMENTED`, 340 tests all
  `PASSING`), inventory (53/205 family 25.854%, 33/205 strict),
  and observer-disabled structure pass. Focused ASan/UBSan 4/4
  GREEN (`ASAN_OPTIONS=detect_leaks=0`, then `make clean` plus a
  normal rebuild). Fresh CMake/CTest Debug passes 318/319 with
  only `quality.python` failing (the same single unittest
  failure); full sanitizer CMake/CTest passes 318/318 excluding
  `quality.python`. Registry closure is 268 immutable evidence
  records, 414 requirements (383 `IMPLEMENTED`, 0 `PLANNED`, 31
  `UNRESOLVED`), and 340 tests (340 `PASSING`, 0 `PLANNED`).
  Coverage is 145 catalog rows, `ID_COUNT=146`, 5,310 lookup
  entries, 145 trusted bindings, family coverage 53/205 (25.854%),
  and 33/205 strict complete-form coverage (`FORM-T2-12-01` stays
  `SUBSET`). No knob was added or changed. Nothing was pushed.
- Follow-up work: wave-1 control flow continues with `BC`
  (conditional target selection over status conditions), then
  `CALL` (stack-link seam) and `RET` (return path); the deferred
  delayed `BD` variant waits on the delay-slot seam. Open: fix the
  `make test` recipe order so C regressions cannot hide behind the
  environmental quality failure.

## 2026-09-11 — P2-W63 BC pmad,cond conditional-branch vertical slice

- Implementation commit `d276feb` activates Table 2-12 form
  `FORM-T2-12-04` as `SUBSET`, TI SPRU172C §4 `BC[D]` (printed
  pp. 4-18–4-20, cross-checked against the SPRU179C algebraic
  volume `if (cond(s)) goto pmad`, class 31A, §4-18, which agrees
  on the opcode diagram, conditional `PC = pmad(15-0)` execution,
  and the worked examples): one row `BC_PMAD` at first-word value
  `0xF800` under mask `0xFF00` (bits `1111_10Z0` with Z=0 plus the
  8-bit condition code; Z=1 is the deferred delayed `BCD` variant
  and decodes `UNKNOWN_WORD`), two program words, 256 encodings.
  The new `BRANCH_COND_PMAD16` layout carries the copied condition
  code and second-word pmad; repeatability is `INELIGIBLE` on the
  manual's explicit "not repeatable" note; input resources are the
  new `ACCUMULATORS_STATUS_C_TC` combination (schema token set
  extended alongside the header macro).
- Semantics implements the 16 image-evidenced single conditions
  covering 1,752 of 1,754 static uses: TC/NTC/C/NC plus the twelve
  A/B compares as full-40-bit two's-complement signed
  project-interpretation pins (guard bits included; TI shows no
  guard-nonzero vector — the residual risk is recorded here, not
  hidden). Taken emits `PC_WRITE(pmad)` through the W62 route;
  not-taken emits effect `NONE` through the proven sequential
  bridge, so `run.c` is unchanged. Knownness is demanded only for
  consumed inputs (a TC condition ignores unknown accumulators).
  BIO/NBIO (external-pin seam), UNC, AOV/BOV/ANOV/BNOV (OVA/OVB
  effect seam), multi-condition combinations, and delayed `BCD`
  reject `INVALID_OPERAND` transactionally. Worked examples pin
  `BC 2000h,AGT` with `A=00 0000 0053,PC=1000` to `PC=2000` and
  with `A=FF FFFF FFFF` to `PC=1002`, both retired at CPU level.
- Evidence: three new TI records (opcode, semantics with condition
  table and combination rules, explicit nonrepeatable note), plan
  anchor plus activation records, six requirements, and four RED
  contracts (metadata with 65,536-word disjointness sweep and
  256-encoding population, decode exhaustive over all 256 codes
  with Z=1 and LD_K pins, ops exhaustive over 16 conditions with
  accumulator boundaries plus all-256-code classification against
  an independent oracle, CPU with taken/not-taken retirement,
  worked-example origins, unknown-input and second-word-fault
  transactionality). `FORM-T2-12-04` is inventory `SUBSET` under
  the `ABSOLUTE_PMAD_COND` token.
- Review findings during construction: the lookup generator's
  resource-token set needed the new combo alongside the header
  macro (a second registration site the W62 slice did not need);
  a bulk sed with a placeholder token damaged three
  symbol-count lines and was repaired line-by-line before
  re-running (recorded as a session-hygiene note: never bulk-sed
  without a repairing grep); the new BC range forced
  cross-slice-hazard updates to four earlier decode tests
  (`b_pmad`, `ld_k`, `ld_lk16`, `ld_lk_shift`), each pinning the
  overlapping word to `BC_PMAD` by identity; two RED-test
  expectation bugs (an UNKNOWN mask containing the LD_K overlap,
  a cond-bit flip expecting INVALID_DECODE instead of
  INVALID_OPERAND) were fixed in the tests, not the product; a
  `/tmp` disk-full from accumulated CMake build dirs blocked the
  sanitizer gate until stale dirs were removed (scratch build
  dirs are now cleaned after each slice). Split decision for the
  509-line ops test: kept whole — oracle, classification sweep,
  and rejection pins form one contract; no split before 800.
- Validation is GREEN with one recorded baseline deviation: all
  316 C test binaries pass; contract quality unittest passes
  except the single pre-existing `test_supported_m32_assertions`
  `-m32`/ILP32 failure carried as baseline since W61 (W49A-owned,
  outside this diff); knobs, source boundaries (574 files, review
  warnings only), manifests, evidence (273 records, 420
  requirements with 389 `IMPLEMENTED`, 344 tests all `PASSING`),
  inventory (54/205 family 26.341%, 33/205 strict), and
  observer-disabled structure pass. Focused ASan/UBSan 4/4 GREEN
  (`ASAN_OPTIONS=detect_leaks=0`, then `make clean` plus a normal
  rebuild). Fresh CMake/CTest Debug passes 322/323 with only
  `quality.python` failing (the same single unittest failure);
  full sanitizer CMake/CTest passes 322/322 excluding
  `quality.python`. Coverage is 146 catalog rows, `ID_COUNT=147`,
  5,566 lookup entries, 146 trusted bindings, family coverage
  54/205 (26.341%), and 33/205 strict complete-form coverage
  (`FORM-T2-12-04` stays `SUBSET`). No knob was added or changed.
  Nothing was pushed.
- Follow-up work: wave-1 control flow continues with `CALL`
  (stack-link seam) and `RET` (return path); the deferred delayed
  `BCD` variant, BIO/NBIO pin conditions, UNC, OV/NOV with
  OVA/OVB effects, and multi-condition combinations wait on
  their seams. Open: fix the `make test` recipe order so C
  regressions cannot hide behind the environmental quality
  failure.

## 2026-09-11 — P2-W64 stack-pointer input seam vertical slice

- Implementation commit `f4fa283` adds the stack-pointer input
  seam that W65 `CALL` will consume, mirroring the SST seam
  precedent (`d21b884`): one new input resource bit
  `STACK_POINTER` (512U) with snapshot fields
  `stack_pointer_raw16` plus `stack_pointer_known`, a `collect.c`
  arm reading the architectural SP through
  `ti_c54x_arch_state_read_sp` with explicit knownness, and
  `KNOWN_MASK` inclusion. Common reset leaves SP unknown per the
  zero-plus-unknown policy; no catalog row, layout, handler,
  effect, inventory form, or retirement change is admitted.
- Measured ABI outcome: the carrier stays 56 bytes on LP64
  (padding reflow absorbs the new fields) and grows 52 to 56 on
  ILP32. That growth resolves the `-m32` assertion failure
  carried as baseline since W61 (the ILP32 probe now meets the
  56-byte assert, and the stale 48-byte messages are corrected to
  56 as part of this ABI change), and moves the ILP32
  root-transaction pin 316 to 320 (the root embeds the snapshot;
  attribution verified, not assumed). The new 0x0200 bit forces
  seven invalid-mask probes to 0x0400. The full contract quality
  suite is green with zero failures for the first time.
- Evidence: plan-anchor plus activation records, one requirement
  (`REQ-PROJECT-P2-W64-SP-INPUT-SEAM`), and one RED contract
  (`T-PROJECT-P2-W64-SP-INPUT-SEAM`,
  `tests/unit/cpu/input/test_collect_stack_pointer.c`, CTest
  `ti_c54x.collect_stack_pointer`) covering reset-unknown
  canonicalization, typed-write knownness with SXM independence,
  and transactional rejection. Registry closure is 275 evidence
  records, 421 requirements (390 `IMPLEMENTED`, 0 `PLANNED`, 31
  `UNRESOLVED`), and 345 tests (345 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 317 C test
  binaries pass; contract quality unittest fully OK; knobs,
  source boundaries (575 files, review warnings only),
  manifests, evidence, inventory (unchanged 54/205 family
  26.341%, 33/205 strict), and observer-disabled structure pass.
  Focused ASan/UBSan GREEN for the new collect test
  (`ASAN_OPTIONS=detect_leaks=0`, then `make clean` plus a normal
  rebuild). Fresh CMake/CTest Debug passes 324/324; full
  sanitizer CMake/CTest passes 323/323 excluding
  `quality.python`. No knob was added or changed. Nothing was
  pushed.
- Follow-up work: W65 `CALL pmad` (Table 2-13 form
  `FORM-T2-13-02`) consumes this seam with a stack transaction
  (endpoint write of `PC+2` at `SP-1`, atomic `SP`/`PC` commit),
  then `RET`. Open: fix the `make test` recipe order so C
  regressions cannot hide behind the environmental quality
  failure.

## 2026-09-11 — P2-W65A stack-push MEMORY transaction seam

- Implementation commit `635c6ce` adds the generic stack-push
  MEMORY seam that W65B `CALL` will consume, mirroring the W42A
  seam precedent: one address kind `STACK_PUSH_WORD16` with an
  all-zero spec, a pure `src/address/stack_push.[ch]` planner
  mapping old SP to the TOS DATA WORD16 at `SP-1` with a
  `PRE_ACCESS` SP update, an `ADDRESS_UPDATE_SP` target through
  the intrinsic/dependency/validate/apply chain with no
  CMPT/ARP/CPL read, a DATA WRITE resolution arm, and root
  prepare/commit support that stages the live return address
  (bridge successor) into the frozen effect copy and commits SP
  plus the frozen pmad target atomically at READY. SP arithmetic
  wraps mod 65536 as a project-interpretation pin (the manual
  states no stack-overflow status). No catalog row, handler,
  layout, `run.c` change, inventory form, or TI claim is
  admitted; the root transaction size pins are untouched.
- Research correction: an early opcode misread (`0xF034`,
  which would have collided with `AND_IMMEDIATE_SHIFT`) was
  caught by cross-checking the `re/dsp` disassembly, which
  renders `f074` as `call`. `CALL` is `0xF074` (Z=0; `0xF274`
  `CALLD` deferred) and is unclaimed by every existing row, so
  W65B needs no generator precedence change.
- Evidence: plan-anchor plus activation records, four
  requirements (`REQ-PROJECT-P2-W65A-STACK-PUSH-PLANNER`,
  `-VALIDATE`, `-MEMORY`, `-ROOT`), and three RED contracts
  (`T-PROJECT-P2-W65A-STACK-PUSH-PLANNER`,
  `tests/unit/address/test_stack_push_planner.c`, CTest
  `ti_c54x.stack_push_planner`;
  `T-PROJECT-P2-W65A-STACK-PUSH-RESOLUTION`,
  `tests/unit/sequencer/memory/test_stack_push_resolution.c`,
  CTest `ti_c54x.stack_push_resolution`;
  `T-PROJECT-P2-W65A-STACK-PUSH-ROOT`,
  `tests/unit/sequencer/instruction/test_stack_push_root.c`,
  CTest `ti_c54x.stack_push_root`) covering the full SP domain
  with the `SP=0` wrap pin, mode independence under unknown
  CMPT/ARP/CPL, WRITE-only rejection, WAIT/READY retirement
  with atomic SP plus pmad-target commit, FAULT rollback,
  stale-SP rejection, and return-address staging. Registry
  closure is 277 evidence records, 425 requirements (394
  `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 348 tests
  (348 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 320 C test
  binaries pass; contract quality unittest fully OK; knobs,
  source boundaries, manifests, evidence, inventory (unchanged
  54/205 family 26.341%, 33/205 strict), and observer-disabled
  structure pass. Focused ASan/UBSan GREEN for the three new
  tests (`ASAN_OPTIONS=detect_leaks=0`, then `make clean`).
  Fresh CMake/CTest Debug passes 327/327; full sanitizer
  CMake/CTest passes 326/326 excluding `quality.python`. An
  `evidence.json` full-file reformat was caught before commit
  and repaired to a +86-line append. No knob was added or
  changed. Nothing was pushed.
- Follow-up work: W65B `CALL pmad` (`FORM-T2-13-02`, row
  `CALL_PMAD` at `0xF074` reusing `BRANCH_PMAD16` with
  `STACK_POINTER` input) binds the catalog row, handler,
  dispatch, CPU retirement, and inventory through this seam,
  then `RET`. Open: fix the `make test` recipe order so C
  regressions cannot hide behind the environmental quality
  failure.

## 2026-09-11 — P2-W65B CALL pmad vertical slice

- Implementation commit `dc14075` activates one Table 2-13 row
  under `FORM-T2-13-02`: `CALL_PMAD` at first-word value `0xF074`
  under mask `0xFFFF`, two program words, one encoding, reusing
  the `BRANCH_PMAD16` layout with `STACK_POINTER` input
  resources and `INELIGIBLE` repeatability on the manual's
  explicit note. The pure `src/isa/ops/branch/op_call_pmad.[ch]`
  handler validates identity and operands, demands known SP
  (`INVALID_OPERAND` otherwise), and stages a MEMORY
  `STACK_PUSH` WRITE with a zero return-address placeholder;
  the W65A root derives `PC+2` at prepare and commits SP, TOS,
  and `PC = pmad` atomically through the existing MEMORY route
  with no `run.c` change. Dispatch binds through the macro
  tables; the deferred delayed word `0xF274` decodes
  `UNKNOWN_WORD`. `FORM-T2-13-02` lands `SUBSET` with the
  `CALL_PMAD` symbol bound as `ABSOLUTE_PMAD_NONDELAYED`.
- The generator's input-resource token set lacked
  `STACK_POINTER` (W64 added the C bit but no row consumed
  it); the token is registered in
  `tools/isa/catalog/opcode_catalog_schema.py` and the lookup
  regenerates as a clean +1 line with the overlap validator
  passing.
- Evidence: three TI section records (opcode, semantics with
  the `CALL 3333h` worked example, nonrepeatable note) plus
  plan-anchor and activation records; six requirements
  (`REQ-COMMON-P2-W65B-CALL-PMAD-CATALOG/DECODE/EFFECTS` plus
  `REQ-PROJECT-P2-W65B-CALL-PMAD-TRUSTED-BINDING/CPU-RETIREMENT/INVENTORY`);
  four RED contracts
  (`T-COMMON-P2-W65B-CALL-PMAD-METADATA`,
  `tests/unit/isa/catalog/test_call_pmad_metadata.c`, CTest
  `ti_c54x.call_pmad_metadata`;
  `T-COMMON-P2-W65B-CALL-PMAD-DECODE`,
  `tests/unit/isa/decode/test_call_pmad_decode.c`, CTest
  `ti_c54x.call_pmad_decode`;
  `T-COMMON-P2-W65B-CALL-PMAD-SEMANTICS`,
  `tests/unit/isa/ops/branch/test_call_pmad.c`, CTest
  `ti_c54x.call_pmad`;
  `T-PROJECT-P2-W65B-CALL-PMAD-CPU`,
  `tests/unit/cpu/isa/branch/test_cpu_call_pmad.c`, CTest
  `ti_c54x.cpu_call_pmad`) covering exact disjointness over the
  full first-word space, the delayed-word pin, the fixed-bit
  near-miss audit (BC/SFTL/AND/XOR/OR16 siblings, matching B's
  neighborhood), full-pmad checked/trusted/dispatch parity,
  SP-knownness gating, the CPU worked example with TOS capture,
  WAIT resume, DATA fault rollback, unknown-SP rejection, and
  end-to-end mode independence with CMPT/ARP unknown. Registry
  closure is 282 evidence records (the W65B anchor projected
  279, undercounting the three TI section records), 431
  requirements (400 `IMPLEMENTED`, 0 `PLANNED`, 31
  `UNRESOLVED`), and 352 tests (352 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 324 C test
  binaries pass; contract quality unittest fully OK; knobs,
  source boundaries, manifests, evidence, inventory (55/205
  family 26.829%, 33/205 strict), and observer-disabled
  structure pass. Focused ASan/UBSan GREEN for the four new
  tests (`ASAN_OPTIONS=detect_leaks=0`, then `make clean`).
  Fresh CMake/CTest Debug passes 331/331; full sanitizer
  CMake/CTest passes 330/330 excluding `quality.python`. The
  per-row count pins (`ID_COUNT` 148, 5,567 lookup entries,
  147 bindings, 55/205 family, 114 SUBSET bindings) were
  updated across the contract suites. No knob was added or
  changed. Nothing was pushed.
- Follow-up work: `RET` (the return half of the call pair),
  then delayed `CALLD`/`RETD` in the delay seam. Open: fix the
  `make test` recipe order so C regressions cannot hide behind
  the environmental quality failure.

## 2026-09-11 — P2-W66A stack-pop MEMORY transaction seam

- Implementation commit `8bc7209` adds the generic stack-pop
  MEMORY seam that W66B `RET` will consume, mirroring the W65A
  seam precedent: one address kind `STACK_POP_WORD16` with an
  all-zero spec, a pure `src/address/stack_pop.[ch]` planner
  mapping old SP to the TOS DATA WORD16 at old SP with a
  `POST_ACCESS` SP update, the POST shape through the existing
  `ADDRESS_UPDATE_SP` target with the shared `STACK_PUSH`
  source kind (the snapshot is identical; only the phase
  differs), a DATA READ resolution arm with no mode read, and
  root prepare/commit support that proves 16-bit target
  plannability against the live PC carrier before any bus issue
  and commits SP plus the TOS target from the transaction's own
  READ observation atomically at READY. The READ value never
  routes through ordinary accumulator/status completion: null
  completion is exact for this kind. `SP=0xFFFF` pops to
  `SP=0x0000` as a project-interpretation pin. No catalog row,
  handler, layout, `run.c` change, inventory form, or TI claim
  is admitted; the root transaction size pins are untouched.
- Evidence: plan-anchor plus activation records, four
  requirements (`REQ-PROJECT-P2-W66A-STACK-POP-PLANNER`,
  `-VALIDATE`, `-MEMORY`, `-ROOT`), and three RED contracts
  (`T-PROJECT-P2-W66A-STACK-POP-PLANNER`,
  `tests/unit/address/test_stack_pop_planner.c`, CTest
  `ti_c54x.stack_pop_planner`;
  `T-PROJECT-P2-W66A-STACK-POP-RESOLUTION`,
  `tests/unit/sequencer/memory/test_stack_pop_resolution.c`,
  CTest `ti_c54x.stack_pop_resolution`;
  `T-PROJECT-P2-W66A-STACK-POP-ROOT`,
  `tests/unit/sequencer/instruction/test_stack_pop_root.c`,
  CTest `ti_c54x.stack_pop_root`) covering the full SP domain
  with the `SP=0xFFFF` wrap pin, mode independence under unknown
  CMPT/ARP/CPL, READ-only rejection, WAIT/READY retirement with
  atomic SP plus TOS-target commit, FAULT rollback, and stale-SP
  rejection. Registry closure is 284 evidence records, 435
  requirements (404 `IMPLEMENTED`, 0 `PLANNED`, 31
  `UNRESOLVED`), and 355 tests (355 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 327 C test
  binaries pass; contract quality unittest fully OK; knobs,
  source boundaries, manifests, evidence, inventory (unchanged
  55/205 family 26.829%, 33/205 strict), and observer-disabled
  structure pass. Focused ASan/UBSan GREEN for the three new
  tests (`ASAN_OPTIONS=detect_leaks=0`, then `make clean`).
  Fresh CMake/CTest Debug passes 334/334; full sanitizer
  CMake/CTest passes 333/333 excluding `quality.python`. No
  knob was added or changed. Nothing was pushed.
- Follow-up work: W66B `RET` (one-word row `RET` at `0xFC00`
  reusing `OPERAND_LAYOUT_NONE` with `STACK_POINTER` input)
  binds the catalog row, handler, dispatch, CPU retirement, and
  inventory through this seam, then delayed `CALLD`/`RETD` in
  the delay seam. Open: fix the `make test` recipe order so C
  regressions cannot hide behind the environmental quality
  failure.

## 2026-09-11 — P2-W66B RET subroutine-return slice

- Implementation commit `dd17dc9` binds one Table 2-15 row under
  `FORM-T2-15-04`: `RET` at first-word value `0xFC00` under mask
  `0xFFFF`, one program word, one encoding, no operands, reusing
  the `OPERAND_LAYOUT_NONE` layout with `STACK_POINTER` input
  resources and `INELIGIBLE` repeatability (manual note). The
  pure branch handler demands known SP and stages a `MEMORY`
  `STACK_POP` READ with zero value and null completion; the W66A
  root reads TOS at old SP and commits SP+1 and PC=TOS
  atomically through the existing `MEMORY` route with no `run.c`
  change. Dispatch binds through the macro tables; the deferred
  delayed word `0xFE00` decodes `UNKNOWN_WORD`.
  `FORM-T2-15-04` lands `SUBSET`. The BC near-miss audit now
  names RET as the bit-10 sibling. A BC decode sibling collision
  at `0xFC00` and row-count pins (147→148, 5567→5568, 55→56)
  were fixed with a lookup regen during the slice.
- Evidence: plan-anchor plus activation records, one requirement
  (`REQ-BRANCH-RET-NONDELAYED`), and four RED contracts
  (`tests/unit/isa/catalog/test_ret_metadata.c`,
  `tests/unit/isa/decode/test_ret_decode.c`,
  `tests/unit/isa/ops/branch/test_ret.c`,
  `tests/unit/cpu/isa/branch/test_cpu_ret.c`) covering metadata,
  decode (including `RETD` still unknown), handler staging, and
  CPU retirement with atomic SP+1/PC=TOS commit. Registry
  closure is 289 evidence records, 441 requirements (410
  `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 359 tests
  (359 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 331 C test
  binaries pass including the 4 new focused contracts; contract
  quality unittest fully OK; knobs, source boundaries,
  manifests, evidence, inventory (realized 148 catalog rows,
  `ID_COUNT=149`, 5,568 lookup entries, 148 bindings, family
  coverage 56/205 at 27.317%, 33/205 strict), and
  observer-disabled structure pass. Focused ASan/UBSan GREEN for
  the four new tests. Fresh CMake/CTest Debug passes 338/338;
  full sanitizer CMake/CTest passes 337/337 excluding
  `quality.python`. No knob was added or changed. Nothing was
  pushed.
- Follow-up work: delayed `RETD`/`CALLD` in the delay seam, then
  `RETE`/`RETF` which stay deferred with cross-slice hazard
  pins. Open: fix the `make test` recipe order so C regressions
  cannot hide behind the environmental quality failure.

## 2026-09-11 — Correction: W66B ledger requirement citation

- The W66B entry above cites `REQ-BRANCH-RET-NONDELAYED`. That
  ID was never registered; the W66B slice actually registered
  six requirements, all `IMPLEMENTED` in `config/evidence.json`:
  `REQ-COMMON-P2-W66B-RET-CATALOG`,
  `REQ-COMMON-P2-W66B-RET-DECODE`,
  `REQ-COMMON-P2-W66B-RET-EFFECTS`,
  `REQ-PROJECT-P2-W66B-RET-TRUSTED-BINDING`,
  `REQ-PROJECT-P2-W66B-RET-CPU-RETIREMENT`, and
  `REQ-PROJECT-P2-W66B-RET-INVENTORY`, proven by four `PASSING`
  tests (`T-COMMON-P2-W66B-RET-METADATA`,
  `T-COMMON-P2-W66B-RET-DECODE`,
  `T-COMMON-P2-W66B-RET-SEMANTICS`,
  `T-PROJECT-P2-W66B-RET-CPU`). The validation evidence, pins,
  and follow-up notes in the W66B entry stand unchanged.

## 2026-09-11 — P2-W67A delayed-branch MEMORY transaction seam

- Implementation commit `fdaa89a` adds the generic delay seam
  that W67B `RETD` and W67C `CALLD` will consume, mirroring the
  W65A/W66A precedent: two address kinds
  `STACK_PUSH_DELAYED_WORD16` (10) and
  `STACK_POP_DELAYED_WORD16` (11) sharing the immediate stack
  planning, all-zero specs, and access shapes through widened
  validity, resolution, and candidate-validation arms; one pure
  `src/sequencer/delayed_branch/delayed_branch.[ch]` countdown
  value (disarmed idle, arm with 16-bit deferred target and
  two-word countdown, saturating consume with take-effect
  reporting, replace-on-arm, strict rejection of disarmed,
  zero-width, and null calls); root prepare staging PC+4 for
  delayed pushes with `INVALID_CANDIDATE` past `0xFFFF` and the
  sequential successor otherwise; root commit writing SP plus
  the TOS value atomically at READY with PC advanced to the
  frozen sequential successor; and a `deferred_branch_armed`
  plus 16-bit deferred-PC report in the retirement result
  (TOS for pops, frozen pmad for pushes; ordinary retirements
  stay disarmed). One CPU-owned countdown lives in `struct
  ti_c54x_cpu` as pipeline state, cleared by init (memset) and
  reset. No catalog row, handler, layout, `run.c` change,
  inventory form, or TI claim is admitted; no knob was added or
  changed.
- TI semantics split (SPRU172C §4 `RET[D]` p. 4-139 and §4
  `CALL[D]` p. 4-27 with SPRU131G §6.3.1): stack effects publish
  at the delayed branch's own retirement; only the PC transfer
  waits two program words. The slice resolves a genuine TI
  conflict for the CALLD return address: SPRU131G Example 7-5
  labels the early return register `a3` (into the delay slots),
  while the §4-27 Execution block (`(PC)+4` → TOS) and its
  worked example (pushing `0029` for `CALLD` at `0025`) agree on
  after-the-slots. The Execution block plus example are
  normative here; the pipeline narrative is not adopted for the
  pushed value.
- Evidence: plan-anchor plus activation records, four
  requirements (`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-VALUE`,
  `-VALIDATE`, `-MEMORY`, `-ROOT`), and three RED contracts
  (`T-PROJECT-P2-W67A-DELAYED-BRANCH-VALUE`,
  `tests/unit/sequencer/delayed_branch/test_delayed_branch.c`,
  CTest `ti_c54x.delayed_branch`;
  `T-PROJECT-P2-W67A-DELAYED-BRANCH-POP-ROOT`,
  `tests/unit/sequencer/instruction/test_delayed_stack_pop_root.c`,
  CTest `ti_c54x.delayed_stack_pop_root`;
  `T-PROJECT-P2-W67A-DELAYED-BRANCH-PUSH-ROOT`,
  `tests/unit/sequencer/instruction/test_delayed_stack_push_root.c`,
  CTest `ti_c54x.delayed_stack_push_root`) covering countdown
  arm/consume/replace/saturation/rejection, delayed
  WAIT/READY/FAULT lifecycles with sequential-PC advance and
  deferred reporting, `SP=0xFFFF` wrap, forged-spec rejection,
  stale-SP rejection, PC+4 staging with top-of-memory
  rejection, and immediate-kind disarmed regression. The root
  proofs split pop/push one file per kind after a SIZE_REVIEW
  warning on the combined 513-line draft (each file now under
  500 lines). Registry closure is 291 evidence records, 445
  requirements (414 `IMPLEMENTED`, 0 `PLANNED`, 31
  `UNRESOLVED`), and 362 tests (362 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 334 C test
  binaries pass including the 3 new focused contracts; contract
  quality unittest fully OK; knobs, source boundaries (only
  pre-existing SIZE_REVIEW warnings remain),
  manifests, evidence, inventory (unchanged), and
  observer-disabled structure pass. Make ASan/UBSan GREEN for
  the three new tests. Fresh CMake/CTest Debug passes 341/341;
  full sanitizer CMake/CTest passes 340/340 excluding
  `quality.python` (sanitizer tree built on home disk after
  tmpfs quota failure; scratch removed). Nothing was pushed.
- Follow-up work: W67B `RETD` (one-word row `RETD` at `0xFE00`
  reusing `OPERAND_LAYOUT_NONE` with `STACK_POINTER` input)
  wires the run.c adopt/advance loop plus catalog, handler,
  dispatch, CPU retirement, and inventory through this seam;
  W67C `CALLD` (two-word row at `0xF274` with `BRANCH_PMAD16`)
  follows. Open: `memory_instruction_transaction.c` (728
  lines) needs an explicit split decision before 800 lines;
  `make test` recipe order still lets C regressions hide behind
  quality failures.

## 2026-09-11 — P2-W67B RETD delayed-return slice

- Implementation commit `ac428c8` binds one Table 2-15 row under
  `FORM-T2-15-04`: `RETD` at first-word value `0xFE00` under
  mask `0xFFFF`, one program word, one encoding, no operands,
  reusing `OPERAND_LAYOUT_NONE` with `STACK_POINTER` inputs and
  `INELIGIBLE` repeatability (manual note). The pure handler
  (`src/isa/ops/branch/op_retd.[ch]`) demands known SP and
  stages a `MEMORY` `STACK_POP_DELAYED` READ; dispatch binds
  through the macro tables with no per-row wiring. The W67A
  root commits `SP+1` with sequential PC and reports TOS
  deferred. `run.c` adopts the report into the CPU-owned
  countdown (replacing any pending branch) and, after every
  retirement that arms nothing new, disarms on an inner
  PC-discontinuity (non-delayed stack branch or `PC_WRITE`
  wins; TI forbids these in slots per SPRU131G §6.3.1 p. 6-6)
  or consumes the retired word count, publishing TOS to PC at
  zero with saturating consume. WAIT/FAULT/budget gaps leave
  the instance countdown untouched; reset clears it.
  `FORM-T2-15-04` stays `SUBSET` with a `STACK_POP_DELAYED`
  binding. The W66B `test_ret_decode.c` deferred pin now
  decodes `RETD` under the cross-slice hazard rule (bit-9
  near-miss updated; the bit-11 flip pins the genuine
  `ADD_ACC_SHIFT` sibling at `0xF600` in the new decode test).
- The W66B `REQ-COMMON-P2-W66B-RET-DECODE` registry claim is
  corrected in place (it pinned `0xFE00` to `UNKNOWN_WORD`,
  which this slice falsifies): it now states the Z=1 word
  decodes through the `RETD` row, backed by the new
  `E-TI-C54X-SPRU172C-S4-RETD-OPCODE-P4-139` alongside the old
  opcode record. The old PINNED opcode assertion ("Z=1 ...
  stays deferred to the delay seam") is retained under its
  still-true reading: `RETD` runs through the delay seam built
  in W67A. Nothing else historical was rewritten.
- Evidence: plan-anchor plus activation records, four TI
  records (opcode, semantics, nonrepeatable, §6.3.1 slots),
  six requirements (`REQ-COMMON-P2-W67B-RETD-CATALOG`,
  `-DECODE`, `-EFFECTS`,
  `REQ-PROJECT-P2-W67B-RETD-TRUSTED-BINDING`,
  `-CPU-RETIREMENT`, `-INVENTORY`), and four RED contracts
  (`tests/unit/isa/catalog/test_retd_metadata.c`,
  `tests/unit/isa/decode/test_retd_decode.c`,
  `tests/unit/isa/ops/branch/test_retd.c`,
  `tests/unit/cpu/isa/branch/test_cpu_retd.c`) covering
  metadata/disjointness, decode with the `ADD_ACC_SHIFT`
  sibling, handler staging with checked/trusted/dispatch
  parity, and CPU retirement (two 1-word slots, one 2-word
  slot with real `LD` execution, inner-branch takeover,
  oversize saturation, WAIT resume, budget-split persistence,
  reset clearing, slot-fetch fault hold, unknown-SP
  transactionality, full preservation). Registry closure is
  297 evidence records, 451 requirements (420 `IMPLEMENTED`, 0
  `PLANNED`, 31 `UNRESOLVED`), and 366 tests (366 `PASSING`, 0
  `PLANNED`).
- Validation is GREEN with no baseline deviation: all 338 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK; knobs, source boundaries
  (only pre-existing SIZE_REVIEW warnings),
  manifests, evidence, inventory (realized 149 catalog rows,
  `ID_COUNT=150`, 5,569 lookup entries, 149 bindings, 116
  SUBSET bindings, family coverage 56/205 at 27.317%, 33/205
  strict), and observer-disabled structure pass. Focused
  ASan/UBSan GREEN for the four new tests. Fresh CMake/CTest
  Debug passes 345/345; full sanitizer CMake/CTest passes
  344/344 excluding `quality.python` (sanitizer tree on home
  disk; scratch removed). Row-count pins advanced to 149 rows,
  149 bindings, 5,569 entries, and 116 SUBSET across the
  contract suites plus `ID_COUNT=150` across the C metadata
  suites. No knob was added or changed. Nothing was pushed.
- Follow-up work: W67C `CALLD` (two-word row at `0xF274` with
  `BRANCH_PMAD16`, PC+4 return staging already in the seam)
  reuses the run.c wiring unchanged. Open: split decision for
  `memory_instruction_transaction.c` before 800 lines;
  `make test` recipe order still lets C regressions hide
  behind quality failures.

## 2026-09-11 — P2-W67C CALLD delayed-call slice

- Implementation commit `b487f05` binds one Table 2-13 row under
  `FORM-T2-13-02`: `CALLD` at first-word value `0xF274` under
  mask `0xFFFF`, two program words, one encoding, copied 16-bit
  pmad, reusing `BRANCH_PMAD16` with `STACK_POINTER` inputs and
  `INELIGIBLE` repeatability (manual note; TI evidence reused
  from the W65B `CALL[D]` opcode, semantics, and nonrepeatable
  records, so no new TI record was needed). The pure handler
  (`src/isa/ops/branch/op_calld.[ch]`) demands known SP and
  stages a `MEMORY` `STACK_PUSH_DELAYED` WRITE; dispatch binds
  through the macro tables with no per-row wiring. The W67A
  root stages `PC+4` into TOS with immediate SP commit and
  sequential PC advance, reporting pmad deferred; the W67B run
  loop adopts the report into the CPU-owned countdown unchanged,
  so no `run.c` change was admitted. `FORM-T2-13-02` stays
  `SUBSET` with an `ABSOLUTE_PMAD_DELAYED` binding (a
  `COMPLETE` form remains one production catalog row per the
  inventory schema). The W65B `test_call_pmad_decode.c`
  deferred pin now decodes `CALLD` under the cross-slice hazard
  rule (bit-9 near-miss updated; the new decode test pins the
  genuine `SFTL_ACC`, `AND_IMMEDIATE_SHIFT`,
  `XOR_IMMEDIATE_SHIFT`, and `OR_IMMEDIATE_SHIFT16` siblings,
  each verified against the catalog rows).
- The W65B `REQ-COMMON-P2-W65B-CALL-PMAD-DECODE` registry claim
  is corrected in place (it pinned `0xF274` to `UNKNOWN_WORD`,
  which this slice falsifies): it now states the Z=1 word
  decodes through the `CALLD` row, backed by the existing
  `E-TI-C54X-SPRU172C-S4-CALL-OPCODE-P4-27` record. The old
  PINNED opcode assertion ("Z=1 ... stays deferred to the delay
  seam") is retained under its still-true reading: `CALLD` runs
  through the delay seam built in W67A. Nothing else historical
  was rewritten.
- Evidence: plan-anchor plus activation records, six
  requirements (`REQ-COMMON-P2-W67C-CALLD-CATALOG`, `-DECODE`,
  `-EFFECTS`, `REQ-PROJECT-P2-W67C-CALLD-TRUSTED-BINDING`,
  `-CPU-RETIREMENT`, `-INVENTORY`), and four RED contracts
  (`tests/unit/isa/catalog/test_calld_metadata.c`,
  `tests/unit/isa/decode/test_calld_decode.c`,
  `tests/unit/isa/ops/branch/test_calld.c`,
  `tests/unit/cpu/isa/branch/test_cpu_calld.c`) covering
  metadata/disjointness, decode with four genuine siblings,
  handler staging with checked/trusted/dispatch parity, and CPU
  retirement (worked-example PC+4 capture, two 1-word slots,
  one 2-word slot with real `LD` execution, inner-branch
  takeover, oversize saturation, WAIT resume, budget-split
  persistence, reset clearing, slot-fetch fault hold, DATA and
  second-word fault transactionality, unknown-SP rejection,
  full preservation). Registry closure is 299 evidence records,
  457 requirements (426 `IMPLEMENTED`, 0 `PLANNED`, 31
  `UNRESOLVED`), and 370 tests (370 `PASSING`, 0 `PLANNED`).
- Validation is GREEN with no baseline deviation: all 342 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK; knobs, source boundaries
  (only pre-existing SIZE_REVIEW warnings plus the new
  648-line `test_cpu_calld.c` warning, under the 800-line
  split bar), manifests, evidence, inventory (realized 150
  catalog rows, `ID_COUNT=151`, 5,570 lookup entries, 150
  bindings, 117 SUBSET bindings, family coverage 56/205 at
  27.317%, 33/205 strict), and observer-disabled structure
  pass. Focused ASan/UBSan GREEN for the four new tests plus
  the updated decode pin (instrumentation verified in the
  binaries; an early flag-only rebuild attempt that relinked
  nothing was discarded for a forced `-B` rebuild). Fresh
  CMake/CTest Debug passes 349/349; full sanitizer CMake/CTest
  passes 348/348 excluding `quality.python` (sanitizer tree on
  home disk; scratch removed). Row-count pins advanced to 150
  rows, 150 bindings, 5,570 entries, and 117 SUBSET across the
  contract suites plus `ID_COUNT=151` across the C metadata
  suites. No knob was added or changed. Nothing was pushed.
- Follow-up work: wave-67 delay family is complete (`RETD` +
  `CALLD`); `RETE`/`RETF` remain deferred with cross-slice
  hazard pins. Open: split decision for
  `memory_instruction_transaction.c` before 800 lines;
  `make test` recipe order still lets C regressions hide
  behind quality failures.

## 2026-09-11 — P2-W68 BD delayed-branch slice

- Implementation commit `d756fe0` binds one Table 2-12 row under
  `FORM-T2-12-01`: `BD` at first-word value `0xF273` under mask
  `0xFFFF`, two program words, one encoding, copied 16-bit pmad,
  reusing `BRANCH_PMAD16` with no input resources and
  `INELIGIBLE` repeatability (manual note; TI evidence reused
  from the W62 `B[D]` opcode, semantics, and nonrepeatable
  records, so no new TI record was needed). The slice adds the
  first non-MEMORY deferred-branch seam: a new
  `TI_C54X_OP_EFFECT_DELAYED_PC_WRITE` kind carrying one copied
  pmad (`src/isa/ops/op_effect.h`), a pure handler
  (`src/isa/ops/branch/op_bd.[ch]`) emitting it verbatim, macro
  dispatch binding with no per-row wiring, and a `run.c` route
  that validates the absolute target through the existing
  target planner, commits the sequential successor, retires,
  and arms the CPU-owned countdown with pmad, replacing any
  pending countdown under the last-wins rule. `FORM-T2-12-01`
  stays `SUBSET` with an `ABSOLUTE_PMAD_DELAYED` binding. The
  W62 `test_b_pmad_decode.c` deferred pin now decodes `BD`
  under the cross-slice hazard rule (bit-9 near-miss updated;
  the new decode test pins the genuine `SFTL_ACC`,
  `AND_IMMEDIATE_SHIFT`, `XOR_IMMEDIATE_SHIFT`, and
  `AND_IMMEDIATE_SHIFT16` siblings, each verified against the
  catalog rows).
- The perf contract
  (`test_trusted_hot_path_has_one_preparation_plan_and_generic_dispatch`)
  pins a single `bridge_plan_trusted` site in `run.c`; the
  first route draft added a second and failed it honestly. The
  fix hoists the sequential-successor preparation before kind
  dispatch so the ordinary and delayed-PC routes share it, with
  failure reported only by the routes that advance
  sequentially (top-of-memory MEMORY and immediate branches
  keep exact behavior). Folding apply into the shared status is
  exact: every apply failure maps through
  `bridge_status_to_stop` to the same `INVALID_STATE` the split
  checks produced. The pin was not bumped. `MEMORY`/`PC_WRITE`
  retirements now compute one discarded pure bridge plan; noted
  as accepted hot-path cost, no benchmark gate covers it.
- The W62 `REQ-COMMON-P2-W62-B-PMAD-DECODE` registry claim is
  corrected in place (it pinned `0xF273` to `UNKNOWN_WORD`,
  which this slice falsifies): it now states the Z=1 word
  decodes through the `BD` row, backed by the existing
  `E-TI-C54X-SPRU172C-S4-B-OPCODE-P4-14` record. The old PINNED
  opcode assertion ("Z=1 ... stays deferred to the delay
  seam") is retained under its still-true reading: `BD` runs
  through the new delayed-PC route. Nothing else historical was
  rewritten.
- Evidence: plan-anchor plus activation records, six
  requirements (`REQ-COMMON-P2-W68-BD-CATALOG`, `-DECODE`,
  `-EFFECTS`, `REQ-PROJECT-P2-W68-BD-TRUSTED-BINDING`,
  `-CPU-RETIREMENT`, `-INVENTORY`), and four RED contracts
  (`tests/unit/isa/catalog/test_bd_metadata.c`,
  `tests/unit/isa/decode/test_bd_decode.c`,
  `tests/unit/isa/ops/branch/test_bd.c`,
  `tests/unit/cpu/isa/branch/test_cpu_bd.c`) covering
  metadata/disjointness, decode with four genuine siblings,
  handler staging with checked/trusted/dispatch parity, and CPU
  retirement (verbatim deferral sweep, two 1-word slots, one
  2-word slot with real `LD` execution, inner-branch takeover,
  inner-delayed replacement, oversize saturation, budget-split
  persistence, reset clearing, slot-fetch fault hold,
  second-word fault transactionality, full preservation, no
  endpoint traffic by construction). Registry closure is 301
  evidence records, 463 requirements (432 `IMPLEMENTED`, 0
  `PLANNED`, 31 `UNRESOLVED`), and 374 tests (374 `PASSING`, 0
  `PLANNED`).
- Validation is GREEN with no baseline deviation: all 346 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK; knobs, source boundaries
  (only SIZE_REVIEW warnings; `run.c` at 667 lines stays under
  the 800-line split bar), manifests, evidence, inventory
  (realized 151 catalog rows, `ID_COUNT=152`, 5,571 lookup
  entries, 151 bindings, 118 SUBSET bindings, family coverage
  56/205 at 27.317%, 33/205 strict), and observer-disabled
  structure pass. Focused ASan/UBSan GREEN for the four new
  tests plus the updated B decode pin. Fresh CMake/CTest Debug
  passes 353/353; full sanitizer CMake/CTest passes 352/352
  excluding `quality.python` (sanitizer tree on home disk;
  scratch removed; `/tmp` too small for the sanitizer link).
  Row-count pins advanced to 151 rows, 151 bindings, 5,571
  entries, and 118 SUBSET across the contract suites plus
  `ID_COUNT=152` across the C metadata suites. No knob was
  added or changed. Nothing was pushed.
- Follow-up work: delayed conditional `BCD` (303 image
  occurrences) reuses the `DELAYED_PC_WRITE` route unchanged;
  `BANZD`, `CCD`, `RETE`/`RETF` remain deferred with
  cross-slice hazard pins. Open: split decision for
  `memory_instruction_transaction.c` before 800 lines (and now
  `run.c` at 667 approaching review range); `make test`
  recipe order still lets C regressions hide behind quality
  failures.

## 2026-09-11 — P2-W69 BCD delayed-conditional-branch slice

- Implementation commit `e0b3a3c` binds one Table 2-12 row
  under `FORM-T2-12-04`: `BCD` at first-word value `0xFA00`
  under mask `0xFF00`, two program words, 256 encodings, copied
  condition code plus 16-bit pmad, reusing `BRANCH_COND_PMAD16`
  with `ACCUMULATORS_STATUS_C_TC` inputs and `INELIGIBLE`
  repeatability (manual note; TI evidence reused from the W63
  `BC[D]` opcode, semantics, and nonrepeatable records, so no
  new TI record was needed). The sixteen image-evidenced single
  conditions evaluate through a new shared branch-condition
  helper (`src/isa/ops/branch/op_branch_cond_common.[ch]`);
  the `BC` leaf is refactored onto it with zero behavior change
  (its exhaustive ops plus CPU suites pass unmodified).
  Taken branches emit `DELAYED_PC_WRITE`, not-taken ones emit
  `NONE`; the W68 `run.c` delayed-PC route and the sequential
  bridge retire them, so no `run.c` change was admitted.
  `FORM-T2-12-04` stays `SUBSET` with an
  `ABSOLUTE_PMAD_COND_DELAYED` binding.
- The W63 `REQ-COMMON-P2-W63-BC-PMAD-DECODE` registry claim is
  corrected in place (it pinned the Z=1 words to
  `UNKNOWN_WORD`, which this slice falsifies): it now states
  `0xFA00`-`0xFAFF` decode through the `BCD` row, backed by the
  existing `E-TI-C54X-SPRU172C-S4-BC-OPCODE-P4-18` record. The
  old PINNED opcode assertion ("Z=1 ... stays deferred to the
  delay seam") is retained under its still-true reading: `BCD`
  runs through the W68 delayed-PC route. Nothing else
  historical was rewritten.
- The masked `BCD` row claimed one near-miss word each from
  three earlier audits, all updated under the cross-slice
  hazard rule and pinned as genuine `BCD` siblings after
  catalog cross-checks: `RETD` bit-10 (`0xFA00`), `CALLD`
  bit-11 (`0xFA74`), `BD` bit-11 (`0xFA73`). The new decode
  test pins its own genuine siblings (`BC_PMAD` bit-9,
  `LD_IMMEDIATE_DP` bit-12 at `0xEA00` — not `LD_K` as the
  `BC` audit has at `0xE800` — and `RETD` bit-10 at `0xFE00`).
- Evidence: plan-anchor plus activation records, six
  requirements (`REQ-COMMON-P2-W69-BCD-CATALOG`, `-DECODE`,
  `-EFFECTS`, `REQ-PROJECT-P2-W69-BCD-TRUSTED-BINDING`,
  `-CPU-RETIREMENT`, `-INVENTORY`), and four RED contracts
  (`tests/unit/isa/catalog/test_bcd_metadata.c`,
  `tests/unit/isa/decode/test_bcd_decode.c`,
  `tests/unit/isa/ops/branch/test_bcd.c`,
  `tests/unit/cpu/isa/branch/test_cpu_bcd.c`) covering
  metadata with 256-encoding disjointness, decode with three
  genuine siblings, handler staging with taken/not-taken
  parity over all sixteen conditions, and CPU retirement
  (taken deferral with countdown drain, not-taken fallthrough,
  worked-example shapes, 2-word slot execution, inner-branch
  takeover, slot-fault hold, unknown-input and second-word
  fault transactionality, full preservation, no endpoint
  traffic by construction). Registry closure is 303 evidence
  records, 469 requirements (438 `IMPLEMENTED`, 0 `PLANNED`,
  31 `UNRESOLVED`), and 378 tests (378 `PASSING`, 0
  `PLANNED`).
- Validation is GREEN with no baseline deviation: all 350 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK; knobs, source boundaries
  (no new SIZE_REVIEW warnings; the shared helper, `BCD`
  leaf, and refactored `BC` leaf total 465 lines across three
  small modules), manifests, evidence, inventory (realized 152
  catalog rows, `ID_COUNT=153`, 5,827 lookup entries, 152
  bindings, 119 SUBSET bindings, family coverage 56/205 at
  27.317%, 33/205 strict), and observer-disabled structure
  pass. Focused ASan/UBSan GREEN for the four new tests plus
  the refactored `BC` suites (instrumentation verified).
  Fresh CMake/CTest Debug passes 357/357; full sanitizer
  CMake/CTest passes 356/356 excluding `quality.python`
  (sanitizer tree on home disk; scratch removed). Row-count
  pins advanced to 152 rows, 152 bindings, 5,827 entries, and
  119 SUBSET across the contract suites plus `ID_COUNT=153`
  across the C metadata suites. No knob was added or changed.
  Nothing was pushed.
- Follow-up work: `BANZD` (66 image occurrences), `CCD`
  (29), `RETE`/`RETF` remain deferred with cross-slice hazard
  pins. Open: split decision for
  `memory_instruction_transaction.c` before 800 lines (and
  `run.c` at 667 approaching review range); `make test`
  recipe order still lets C regressions hide behind quality
  failures.

## 2026-09-11 — P2-W70 BANZ auxiliary-register-branch slice

- Implementation commit `1b0efb6` binds one Table 2-12 row
  under `FORM-T2-12-03`: `BANZ` at first-word value `0x6C00`
  under mask `0xFF00`, two program words, 256 encodings, copied
  Sind byte plus 16-bit pmad, using the new
  `BRANCH_SIND_PMAD16` layout (own set object; piece shape
  mirrors the condition layout so Sind/cond namespaces stay
  separate) with `AUXILIARY_REGISTERS` inputs and `INELIGIBLE`
  repeatability. Three new TI records carry the opcode diagram,
  the always-applied Sind side effect with worked examples, and
  the nonrepeatable note (SPRU172C §4 `BANZ[D]`, pp. 4-16–4-17).
- Decision and correction: the W70 plan anchor draft cited mask
  `0xFD00` alongside "256 encodings" and "Z=0 fixed". The
  catalog schema rejects overlapping rows, so a future `BANZD`
  `0x6E00`/`0xFF00` row could not coexist with an `0xFD00`
  `BANZ` row; the Z bit stays fixed per the `BC`/`BCD`
  precedent, and the anchor heading is left immutable with this
  correction recorded here. The bit-9 `BANZD` words stay pinned
  `UNKNOWN` for W71, which reuses the seam unchanged. A second
  correction: the `1b0efb6` message parenthetical misstates the
  requirement tally; the true closure is 475 requirements (444
  `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), 308 evidence
  records, 382 tests (382 `PASSING`, 0 `PLANNED`).
- Semantics: the Sind side effect always applies
  (post-decrement value-after on MOD1, observation on MOD0,
  even when not taken); taken/not-taken emit
  `BRANCH_AUXILIARY` carrying AR index, value-after, update,
  taken, clear delayed flag, and pmad over the image-evidenced
  I=1/MOD0 and I=1/MOD1 subset, with all other Sinds rejected
  as deferred and selected-AR-only knownness (`UNKNOWN_INPUT`
  per the branch-condition helper convention). The new `run.c`
  auxiliary-branch route gates CMPT=0, validates the absolute
  target, commits the flagged AR write, retires taken branches
  to the verbatim pmad and not-taken ones sequentially with no
  countdown, advances the pending countdown with taken as the
  redirect flag, and arms it for taken-delayed. Macro dispatch
  binds both leaves with no per-row wiring.
  `FORM-T2-12-03` lands `SUBSET` with an
  `ABSOLUTE_PMAD_AUXILIARY` binding.
- Four new executable contracts
  (`tests/unit/isa/catalog/test_banz_metadata.c`,
  `tests/unit/isa/decode/test_banz_decode.c`,
  `tests/unit/isa/ops/branch/test_banz.c`,
  `tests/unit/cpu/isa/branch/test_cpu_banz.c`) covering
  metadata with 256-encoding disjointness, decode with the
  deferred-`BANZD` pin and fixed-bit near-miss audit, handler
  staging with taken/not-taken parity over all eight ARs in
  both Sind modes plus deferred-Sind and transactional
  rejection, and CPU retirement (verbatim-target commit with
  writeback, not-taken fallthrough with wrap writeback,
  observation-only retirement, unknown-AR and CMPT=1
  transactionality, full preservation outside PC/selected AR,
  no endpoint traffic by construction).
- Validation is GREEN with no baseline deviation: all 354 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK (268); knobs, source
  boundaries, manifests, evidence, inventory (realized 153
  catalog rows, `ID_COUNT=154`, 6,083 lookup entries, 153
  bindings, 120 SUBSET bindings, family coverage 57/205 at
  27.805%, 33/205 strict), and observer-disabled structure
  pass. Focused ASan/UBSan GREEN for the four new tests
  (instrumentation verified). Fresh CMake/CTest Debug passes
  361/361; full sanitizer CMake/CTest passes 360/360
  excluding `quality.python` (scratch `/tmp` cleared of stale
  prior-slice build dirs to relieve disk pressure; current
  scratch retained). Row-count pins advanced to 153 rows, 153
  bindings, 6,083 entries, 120 SUBSET, and 57/205 family
  across the contract suites plus `ID_COUNT=154` across the C
  metadata suites. `generate_opcode_lookup.py --check` and
  `git diff --check` pass. No knob was added or changed.
  Nothing was pushed.
- Follow-up work: `BANZD` (delayed variant, reuses this seam;
  insert the `0x6E00`/`0xFF00` row and a delayed-emitting
  handler, then cover the taken-delayed route arm path which
  is currently exercised only by construction), `CCD` (29),
  `RETE`/`RETF` remain deferred with cross-slice hazard pins.
  Open: split decision for `run.c` before 800 lines (now 765
  after this route; was 667 at W69) and
  `memory_instruction_transaction.c`; `make test` recipe order
  still lets C regressions hide behind quality failures.

## 2026-09-11 — P2-W71 BANZD delayed auxiliary-register-branch slice

- Implementation commit `c33409a` binds one Table 2-12 row
  under `FORM-T2-12-03`: `BANZD` at first-word value `0x6E00`
  under mask `0xFF00`, two program words, 256 encodings, copied
  Sind byte plus 16-bit pmad, reusing the W70
  `BRANCH_SIND_PMAD16` layout with `AUXILIARY_REGISTERS`
  inputs and `INELIGIBLE` repeatability (TI records reused from
  W70, so no new TI record was needed). Shared Sind evaluation
  hoists into `src/isa/ops/branch/op_branch_sind_common.[ch]`
  (always-applied post-decrement, taken/not-taken
  `BRANCH_AUXILIARY` with selected-AR-only knownness,
  deferred-Sind rejection, caller-selected delay flag); the
  `BANZ` leaf is refactored onto it with zero behavior change
  (its exhaustive op plus CPU suites pass unmodified) and the
  new `BANZD` leaf emits with the delay flag set. The W70
  `run.c` auxiliary-branch route already retires taken-delayed
  and not-taken, so no `run.c` change was admitted; W71 covers
  the taken-delayed arm path W70 exercised only by
  construction. `FORM-T2-12-03` stays `SUBSET` with `BANZ` plus
  `BANZD` under an `ABSOLUTE_PMAD_AUXILIARY_DELAYED` binding.
- Cross-slice hazard notes: the W70 `BANZ` decode deferred pin
  now decodes `BANZD`, and the `BANZD` masked row claims one
  near-miss word (`0x6E80`) from the `LD_SMEM_SHIFT` audit,
  pinned as a genuine `BANZD` sibling. No other audit word
  moved.
- Four new executable contracts
  (`tests/unit/isa/catalog/test_banzd_metadata.c`,
  `tests/unit/isa/decode/test_banzd_decode.c`,
  `tests/unit/isa/ops/branch/test_banzd.c`,
  `tests/unit/cpu/isa/branch/test_cpu_banzd.c`) covering
  metadata with 256-encoding disjointness and variant
  separation, decode with the non-delayed sibling pin and
  fixed-bit near-miss audit, handler staging with the delay
  flag set and taken/not-taken parity over all eight ARs in
  both Sind modes plus deferred-Sind and transactional
  rejection, and CPU retirement (taken deferral with countdown
  drain and writeback, not-taken fallthrough, observation-only
  retirement, unknown-AR and CMPT=1 transactionality, full
  preservation, no endpoint traffic by construction).
- Validation is GREEN with no baseline deviation: all 358 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK (268); knobs, source
  boundaries, manifests, evidence, inventory (realized 154
  catalog rows, `ID_COUNT=155`, 6,339 lookup entries, 154
  bindings, 121 SUBSET bindings, family coverage unchanged at
  57/205, 33/205 strict), and observer-disabled structure
  pass. Focused ASan/UBSan GREEN for the four new tests plus
  the refactored `BANZ` op/CPU suites (instrumentation
  verified). Fresh CMake/CTest Debug passes 365/365; full
  sanitizer CMake/CTest passes 364/364 excluding
  `quality.python`. Row-count pins advanced to 154 rows, 154
  bindings, 6,339 entries, 121 SUBSET, family 57/205 across
  the contract suites plus `ID_COUNT=155` across the C metadata
  suites. `generate_opcode_lookup.py --check` and
  `git diff --check` pass. No knob was added or changed.
  Nothing was pushed.
- Follow-up work: `CCD` (29), `RETE`/`RETF` remain deferred
  with cross-slice hazard pins. Open: split decision for
  `run.c` before 800 lines (unchanged at 765; no `run.c` edit
  this slice) and `memory_instruction_transaction.c`;
  `make test` recipe order still lets C regressions hide
  behind quality failures.

## 2026-09-11 — P2-W72 CCD delayed-conditional-call slice

- Implementation commit `3d8e063` binds one Table 2-13 row
  under `FORM-T2-13-03`: `CCD` at first-word value `0xFB00`
  under mask `0xFF00`, two program words, 256 encodings, copied
  condition byte plus 16-bit pmad, reusing `BRANCH_COND_PMAD16`
  with the new `ACCUMULATORS_STATUS_C_TC` plus `STACK_POINTER`
  inputs and `INELIGIBLE` repeatability (three new TI records:
  opcode diagram, delayed PC+4 push with worked examples,
  nonrepeatable note; SPRU172C §4 `CC[D]`, pp. 4-29–4-31).
  The sixteen evidenced single conditions evaluate through the
  shared branch-condition helper; taken branches stage the
  `CALLD`-shaped `MEMORY` stack-push-delayed effect while
  not-taken branches emit `NONE`. Taken branches gate known SP
  per the `CALLD` convention; not-taken branches consume
  condition inputs only. The `CALLD` `MEMORY` route and the
  sequential bridge retire both outcomes, so no `run.c` change
  was admitted. `FORM-T2-13-03` lands `SUBSET` with a lone
  `CCD` symbol under an `ABSOLUTE_PMAD_COND_DELAYED` binding
  shared with `BCD`.
- Correction: the `3d8e063` message claims the `CCD` row takes
  a near-miss word each from the `BCD` and `LD_SMEM_SHIFT`
  audits. Only the `BCD` claim is real (`0xFB00` is `BCD`'s
  bit-8 flip, pinned as a genuine `CCD` sibling there). No
  `LD_SMEM_SHIFT` audit word moved; `CCD`'s own audit instead
  pins two genuine siblings (`BCD` at `0xFA00`,
  `LD_IMMEDIATE_DP` at `0xEB00`).
- Four new executable contracts
  (`tests/unit/isa/catalog/test_ccd_metadata.c`,
  `tests/unit/isa/decode/test_ccd_decode.c`,
  `tests/unit/isa/ops/branch/test_ccd.c`,
  `tests/unit/cpu/isa/branch/test_cpu_ccd.c`) covering
  metadata with 256-encoding disjointness and variant
  separation, decode with the deferred-`CC` pin and fixed-bit
  near-miss audit, handler staging with taken `MEMORY`
  push-delayed versus not-taken `NONE` over all sixteen
  conditions plus SP gating and transactional rejection, and
  CPU retirement (taken deferral with PC+4 TOS WRITE, SP
  commit, and countdown drain; not-taken fallthrough with no
  endpoint traffic; unknown-condition, taken-with-unknown-SP,
  and not-taken-with-unknown-SP cases; full preservation).
- Validation is GREEN with no baseline deviation: all 362 C
  test binaries pass including the 4 new focused contracts;
  contract quality unittest fully OK (268); knobs, source
  boundaries, manifests, evidence, inventory (realized 155
  catalog rows, `ID_COUNT=156`, 6,595 lookup entries, 155
  bindings, 122 SUBSET bindings, family coverage 58/205 at
  28.293%, 33/205 strict), and observer-disabled structure
  pass. Focused ASan/UBSan GREEN for the four new tests
  (instrumentation verified). Fresh CMake/CTest Debug passes
  369/369; full sanitizer CMake/CTest passes 368/368
  excluding `quality.python`. Row-count pins advanced to 155
  rows, 155 bindings, 6,595 entries, 122 SUBSET, and 58/205
  family across the contract suites plus `ID_COUNT=156`
  across the C metadata suites.
  `generate_opcode_lookup.py --check` and `git diff --check`
  pass. No knob was added or changed. Nothing was pushed.
- Follow-up work: non-delayed `CC` (74 image occurrences),
  `RETE`/`RETF` remain deferred with cross-slice hazard pins.
  Open: split decision for `run.c` before 800 lines
  (unchanged at 765; no `run.c` edit this slice) and
  `memory_instruction_transaction.c`; `make test` recipe order
  still lets C regressions hide behind quality failures.

## 2026-09-11 — P2-W73 CC non-delayed-conditional-call slice

- Implementation commit `7072c0a` binds one Table 2-13 row
  under `FORM-T2-13-03`: `CC` at first-word value `0xF900`
  under mask `0xFF00` (bit 9 clear selects the non-delayed
  variant; delayed `CCD` shipped in W72), two program words,
  256 encodings, copied condition byte plus 16-bit pmad,
  reusing `BRANCH_COND_PMAD16` with the W72
  `ACCUMULATORS_STATUS_C_TC` plus `STACK_POINTER` inputs and
  `INELIGIBLE` repeatability (no new TI records: opcode
  diagram, semantics, and nonrepeatable note are the W72
  records `E-TI-C54X-SPRU172C-S4-CC-OPCODE-P4-29`,
  `E-TI-C54X-SPRU172C-S4-CC-SEMANTICS-P4-29-4-31`, and
  `E-TI-C54X-SPRU172C-S4-CC-NONREPEATABLE-P4-31`; SPRU172C §4
  `CC[D]`, pp. 4-29–4-31). The sixteen evidenced single
  conditions evaluate through the shared branch-condition
  helper; taken branches stage the `CALL`-shaped `MEMORY`
  stack-push effect (the root transaction pushes PC+2, plans
  the absolute pmad target, and retires PC=pmad at once with
  no countdown) while not-taken branches emit `NONE`. Taken
  branches gate known SP per the `CALL` convention;
  not-taken branches consume condition inputs only. The
  `CALL` `MEMORY` route and the sequential bridge retire both
  outcomes, so no `run.c` change was admitted.
  `FORM-T2-13-03` stays `SUBSET` with `CCD` and `CC` symbols
  under a new `ABSOLUTE_PMAD_COND` binding shared with
  `BC_PMAD`.
- No correction: the `7072c0a` message's near-miss claims
  verify exactly — the `BC_PMAD` audit's bit-8 flip word
  `0xF900` and the `CCD` metadata/decode deferred pins
  (`0xF900`/`0xF930`) moved to genuine `CC` sibling pins,
  and `CC`'s own audit pins genuine `CCD` (`0xFB00`),
  `BC_PMAD` (`0xF800`), and `LD_K` (`0xE900`) siblings.
- Four new executable contracts
  (`tests/unit/isa/catalog/test_cc_metadata.c`,
  `tests/unit/isa/decode/test_cc_decode.c`,
  `tests/unit/isa/ops/branch/test_cc.c`,
  `tests/unit/cpu/isa/branch/test_cpu_cc.c`) covering
  metadata with 256-encoding disjointness and delayed-sibling
  separation, decode with the `CCD` sibling pin and
  fixed-bit near-miss audit, handler staging with taken
  `MEMORY` push versus not-taken `NONE` over all sixteen
  conditions plus SP gating and transactional rejection, and
  CPU retirement (taken immediate call with PC+2 TOS WRITE,
  SP commit, and direct PC=pmad with no countdown;
  not-taken fallthrough with no endpoint traffic;
  unknown-condition, taken-with-unknown-SP, and
  not-taken-with-unknown-SP cases; full preservation).
- Validation is GREEN with no baseline deviation: `make
  test` exits 0 with all 366 C test binaries passing
  including the 4 new focused contracts; contract quality
  unittest fully OK (268); knobs, source boundaries,
  manifests, evidence, inventory (realized 156 catalog rows,
  `ID_COUNT=157`, 6,851 lookup entries, 156 bindings, 123
  SUBSET bindings, family coverage unchanged at 58/205 at
  28.293%, 33/205 strict), and observer-disabled structure
  pass. Fresh CMake/CTest Debug passes 373/373; full
  sanitizer CMake/CTest passes 372/372 excluding
  `quality.python`, with the four new `CC` contracts
  confirmed passing under ASan/UBSan. Row-count pins
  advanced to 156 rows, 156 bindings, 6,851 entries, 123
  SUBSET, and unchanged 58/205 family across the contract
  suites plus `ID_COUNT=157` across the C metadata suites
  (including the spaceless repeat-eligibility sweep).
  `generate_opcode_lookup.py --check` and `git diff --check`
  pass. No knob was added or changed. Nothing was pushed.
- Decisions: family coverage counts distinct bound forms, so
  the second binding under `FORM-T2-13-03` correctly leaves
  58/205 unchanged; the new row takes `ABSOLUTE_PMAD_COND`
  (shared with `BC_PMAD`) rather than a new subset token;
  RED was observed as a build failure on the undeclared
  `TI_C54X_OPCODE_ID_CC` before the catalog row landed.
- Follow-up work: `RETE`/`RETF` remain deferred. Open:
  split decision for `run.c` before 800 lines (unchanged at
  765; no `run.c` edit this slice) and
  `memory_instruction_transaction.c` (now 728);
  `make test` recipe order still lets C regressions hide
  behind quality failures.

## 2026-09-11 — Wave-1 control-flow evidence audit and split decisions

- Evidence audit: the evidenced wave-1 control flow is
  complete through W73 (`B`, `BC`, `BCD`, `BANZ`, `BANZD`,
  `CALL`, `CALLD`, `CC`, `CCD`, `RET`, `RETD`, `BD`). Every
  remaining wave-1 control-flow form is evidence-blocked and
  stays `UNRESOLVED`/`UNIMPLEMENTED` with zero TI evidence
  records and no repo-evidenced encodings: `RETE`/`RETF`
  (additionally need nonexistent shadow state for `RETF`
  and a first status-mutating branch for `RETE`),
  `BACC`/`FBACC`/`CALA`/`FCALA`/`FCALL`/`FB`,
  `FRET`/`FRETE`, `XC`, `MAR`, `IDLE`, `RESET`. The `RC`
  mention count (34) is the repeat counter, not conditional
  return. Per the TDD rule no slice is opened for any of
  these until TI requirement, evidence, and executable test
  exist; nothing was fabricated.
- Split decision `src/cpu/execution/run.c` (765 lines):
  KEEP. The file is one orchestrating loop plus small
  helpers; fetch, input collection, dispatch, effect
  transaction, PC bridge/target, and the memory root are
  already separate modules. The loop grows only on new
  effect-kind routes (five arms today), and every remaining
  effect kind retires through an existing arm, so growth is
  capped. Extracting route blocks would only re-home loop
  locals into a context struct with no ownership boundary.
  Re-evaluate at 800 lines or on arrival of a sixth route.
- Split decision
  `src/sequencer/instruction/memory_instruction_transaction.c`
  (728 lines): KEEP. Dual/long32/RMW/pair-dispatch/candidate
  validation already live in six companion modules; the
  remainder is the irreducible prepare/commit/advance core
  whose arms share validation ordering and frozen-candidate
  state. Same re-evaluation triggers: 800 lines or a new
  address kind.
- No production, test, inventory, evidence, or plan change
  in this entry; `git diff --check` passes. Nothing was
  pushed.

## 2026-09-11 — P2-W74 RETE interrupt-return slice

- Unblocked by the TI manual shelf: `vendor/ti/` already
  held SPRU172C/SPRU179C/SPRU131G with text conversions, and
  a fresh ti.com download verified the 172C/179C mirror
  copies byte-identical to official (hashes match; README
  caveat updated in `5520c6d`). RETE evidence is new TI
  records `E-TI-C54X-SPRU172C-S4-RETE-OPCODE-P4-139`,
  `E-TI-C54X-SPRU172C-S4-RETE-SEMANTICS-P4-139`, and
  `E-TI-C54X-SPRU172C-S4-RETE-NONREPEATABLE-P4-139` from the
  vendored text (p. 4-139).
- Implementation commit `212f21c` binds one Table 2-15 row
  under `FORM-T2-15-05`: `RETE` at first-word value `0xF4EB`
  under mask `0xFFFF`, one program word, one encoding, no
  operands, reusing `NONE` with `STACK_POINTER` inputs and
  `INELIGIBLE` repeatability. The pure leaf stages a
  `MEMORY` `STACK_POP` READ with an INTM-clearing completion
  (`STATUS_BIT_WRITE` to ST1 bit 11); the root commits it
  through the generic pop-completion seam before the pop
  architecture commit. Shared-seam changes: pops honor a
  non-null completion exactly like loads
  (`memory_instruction_transaction.c` `commit_ready`) and
  pop validation accepts completions (`memory_effect.c`).
  No `run.c` change, no new effect kind, no new address
  kind. `FORM-T2-15-05` lands `SUBSET` with a lone `RETE`
  symbol under a `STACK_POP_NONDELAYED` binding shared with
  `RET`; delayed `RETED` stays deferred with cross-slice
  hazard pins.
- No correction: the `212f21c` message's audit claims verify
  exactly — no sibling audit word moved; `RETE`'s own audit
  pins genuine `CMPR_AR` (`0xF4AB`) and `SFTL_ACC`
  (`0xF0EB`) siblings.
- Four new executable contracts
  (`tests/unit/isa/catalog/test_rete_metadata.c`,
  `tests/unit/isa/decode/test_rete_decode.c`,
  `tests/unit/isa/ops/branch/test_rete.c`,
  `tests/unit/cpu/isa/branch/test_cpu_rete.c`) covering
  metadata with single-encoding disjointness and RETED
  separation, decode with the deferred-`RETED` pin and
  fixed-bit near-miss audit, handler staging with the
  INTM-clearing completion (TOS-independent, transactional
  NULL rejection) plus SP gating and parity, and CPU
  retirement (worked-example vector with TOS capture and
  INTM clear; WAIT resume with INTM untouched until commit;
  DATA fault, unknown-SP, and program-fault
  transactionality; preservation outside SP/PC/INTM).
- Validation is GREEN with no baseline deviation: `make
  test` exits 0 with all 370 C test binaries passing
  including the 4 new focused contracts; contract quality
  unittest fully OK (268); knobs, source boundaries,
  manifests, evidence, inventory (realized 157 catalog rows,
  `ID_COUNT=158`, 6,852 lookup entries, 157 bindings, 124
  SUBSET bindings, family coverage 59/205 at 28.780%,
  33/205 strict), and observer-disabled structure pass.
  Fresh CMake/CTest Debug passes 377/377; full sanitizer
  CMake/CTest passes 376/376 excluding `quality.python`,
  with the four new `RETE` contracts confirmed passing
  under ASan/UBSan. Row-count pins advanced to 157 rows,
  157 bindings, 6,852 entries, 124 SUBSET, and 59/205
  family across the contract suites plus `ID_COUNT=158`
  across the C metadata suites.
  `generate_opcode_lookup.py --check` and `git diff --check`
  pass. No knob was added or changed. Nothing was pushed.
- Decisions: pop completions generalize the load seam
  instead of a new effect kind or address kind; `RETE`
  inputs stay `STACK_POINTER` only (the INTM write consumes
  no input); RED was observed as a build failure on the
  undeclared `TI_C54X_OPCODE_ID_RETE` before the catalog row
  landed. A `-Wshadow` break on the first seam draft
  (locals shadowing the load path's) was fixed by
  pop-prefixed names before any test ran.
- Follow-up work: delayed `RETED`, `RETF` (needs RTN-state
  scoping first), and the remaining wave-1 tail stay
  deferred. Open: split re-evaluation triggers unchanged
  (800 lines or a new effect/address kind); `make test`
  recipe order still lets C regressions hide behind quality
  failures.

## 2026-09-11 — P2-W75 RETED delayed-interrupt-return slice

- Implementation commit `509ee6b` binds one Table 2-15 row
  under `FORM-T2-15-05`: `RETED` at first-word value `0xF6EB`
  under mask `0xFFFF`, one program word, one encoding, no
  operands, reusing `NONE` with `STACK_POINTER` inputs and
  `INELIGIBLE` repeatability (no new TI records: opcode with
  the Z delayed bit, semantics, and nonrepeatable note are
  the W74 records from SPRU172C p. 4-139). The pure leaf
  stages a `MEMORY` `STACK_POP_DELAYED` READ with the shared
  INTM-clearing completion now owned once in the new
  `op_rete_common` module (`op_rete.c` refactored onto it;
  all `RET`/`RETD`/`RETE` siblings hold). The root commits
  the completion through the generic pop-completion seam
  before the pop architecture commit. No `run.c` change, no
  new effect kind, no new address kind. `FORM-T2-15-05`
  stays `SUBSET` with `RETE` and `RETED` symbols under a
  `STACK_POP_DELAYED` binding shared with `RETD`.
- No correction: the `509ee6b` message's audit claims verify
  exactly — the deferred word moved from the `RETE`
  metadata/decode tests as a genuine `RETED` sibling, and
  `RETED`'s own audit pins genuine `CMPR_AR` (`0xF6AB`),
  `RETE` (`0xF4EB`), and `SFTL_ACC` (`0xF2EB`) siblings.
- Four new executable contracts
  (`tests/unit/isa/catalog/test_reted_metadata.c`,
  `tests/unit/isa/decode/test_reted_decode.c`,
  `tests/unit/isa/ops/branch/test_reted.c`,
  `tests/unit/cpu/isa/branch/test_cpu_reted.c`) covering
  metadata with single-encoding disjointness and RETE
  separation, decode with the non-delayed-`RETE` pin and
  fixed-bit near-miss audit, handler staging with the
  shared completion plus SP gating and parity, and CPU
  retirement (slots with retire-time INTM clear, WAIT with
  INTM untouched until commit, slot-fault and unknown-SP
  transactionality, inner-branch takeover keeping the
  retire-time clear, preservation outside SP/PC/INTM).
- Validation is GREEN with no baseline deviation: `make
  test` exits 0 with all 374 C test binaries passing
  including the 4 new focused contracts; contract quality
  unittest fully OK (268); knobs, source boundaries,
  manifests, evidence, inventory (realized 158 catalog rows,
  `ID_COUNT=159`, 6,853 lookup entries, 158 bindings, 125
  SUBSET bindings, family coverage unchanged at 59/205 at
  28.780%, 33/205 strict), and observer-disabled structure
  pass. Fresh CMake/CTest Debug passes 381/381; full
  sanitizer CMake/CTest passes 380/380 excluding
  `quality.python`, with the four new `RETED` contracts
  confirmed passing under ASan/UBSan. Row-count pins
  advanced to 158 rows, 158 bindings, 6,853 entries, 125
  SUBSET, and unchanged 59/205 family across the contract
  suites plus `ID_COUNT=159` across the C metadata suites.
  `generate_opcode_lookup.py --check` and `git diff --check`
  pass. No knob was added or changed. Nothing was pushed.
- Decisions: the INTM completion is owned once in
  `op_rete_common` rather than duplicated across the two
  leaves; INTM clears at retire alongside SP with only the
  TOS redirect deferred (the `RETD`-proven eager/deferred
  split); RED was observed as a build failure on the
  undeclared `TI_C54X_OPCODE_ID_RETED` before the catalog
  row landed.
- Follow-up work: `RETF` (needs RTN-state scoping first)
  and the remaining wave-1 tail stay deferred. Open: split
  re-evaluation triggers unchanged; `make test` recipe order
  still lets C regressions hide behind quality failures.

## 2026-09-16 — P2-W76 RETF fast-return slice

- Implementation commit `1f98043` binds one Table 2-15 row
  under `FORM-T2-15-06`: `RETF` at first-word value `0xF49B`
  under mask `0xFFFF`, one program word, one encoding, no
  operands, reusing `NONE` with the new
  `STACK_POINTER_RETURN_REGISTER` inputs and `INELIGIBLE`
  repeatability. The slice adds the CPU-internal 16-bit RTN
  fast-return register (SPRU131G §6, SPRU172C §4 RETF[D]):
  checked read/write/knownness accessors, common-reset
  zero-but-unknown seeding, copied input collection, and a
  `STACK_POP_RTN_WORD16` MEMORY kind whose planned READ never
  issues. Push commits stage the pushed return address into
  RTN atomically (CALL PC+2, delayed calls PC+4); `RETF`
  retires with no bus traffic, PC from frozen RTN, SP+1, and
  the shared INTM-clearing completion. No new effect kind
  beyond the RTN pop address kind, no new address kind
  beyond it, no timing claim. `FORM-T2-15-06` lands `SUBSET`
  with a lone `RETF` symbol under an `RTN_POP_NONDELAYED`
  binding.
- Landing corrections by the orchestrator on the in-flight
  tree: (1) `is_valid` split the RTN pop into its own arm
  accepting the prepare-frozen RTN value — the shared arm's
  zero-staged-value rule rejected every frozen candidate, so
  no `RETF` could retire; (2) transaction reset accepts the
  no-traffic COMPLETE-plus-IDLE-bus pairing — retirement
  succeeded but the run ended INVALID_STATE at reset;
  (3) the CALL-then-RETF chain fixture's pmad corrected
  `0x1001` to `0x1002` so the second fetch lands on the
  `RETF` word as the test's own expectations require;
  (4) ten stale quality pins advanced (rows 158→159,
  lookup 6,853→6,854, family 59→60, percent 28.780→29.268);
  (5) four call-sibling preservation proofs now pin the
  staged RTN value and known bit; (6) ID_COUNT 159→160 and
  the transaction size freeze 344/316→352/324 (LP64 executed
  here; ILP32 via headers-only `-m32` probe — that arm never
  executes on a 64-bit host and already measured 320 before
  this slice against its stale 316 pin).
- Seven new executable contracts (`op_retf.c/h`, the READY
  commit split `memory_instruction_commit.c/h`, catalog,
  decode, pure-handler, CPU retirement, input collection,
  plus extended state knownness) covering metadata with
  single-encoding disjointness, decode with near-miss audit,
  handler staging with SP/RTN gating, CPU retirement
  (worked-example vector, CALL-then-RETF chain, unknown-RTN
  and unknown-SP transactionality, preservation outside
  SP/PC/INTM), and RTN knownness lifecycle.
- Validation is GREEN with no baseline deviation: `make
  test` exits 0 with all 379 C test binaries passing;
  contract quality unittest fully OK (268); knobs, source
  boundaries, manifests, evidence, inventory (realized 159
  catalog rows, `ID_COUNT=160`, 6,854 lookup entries, 159
  bindings, 126 SUBSET bindings, family coverage 60/205 at
  29.268%, 33/205 strict), and observer-disabled structure
  pass. ASan/UBSan `make test` exits 0 with no findings.
  `generate_opcode_lookup.py --check` (inside the gate)
  and `git diff --check` pass. No knob was added or
  changed. The split leaves `memory_instruction_transaction.c`
  at 648 lines and the candidate validator at 727 lines;
  both stay KEEP with re-evaluation at 800 lines or on a
  new address kind.
- Decisions: RTN is sequencer-commit/test-only state — no
  ISA operand reads or writes it; calls staging RTN is
  evidenced TI behavior (SPRU131G §6), so the sibling
  preservation updates pin the new value rather than
  weaken; the frozen-RTN-in-value_word reuse is deliberate
  (no new carrier field, no size growth beyond the input
  resource); RED was the in-flight tree itself, completed
  here.
- Follow-up work: `RETFD` (delayed fast return) stays
  deferred, as does the remaining wave-1 tail (`XC`,
  far-branch, accumulator-addressed, `RCD` families) per
  the approved P3 slice order. Open: `make test` recipe
  order still lets C regressions hide behind quality
  failures.

## 2026-09-16 — P3 slice 1 denominator audit

- Implementation commit `fae90e0` closes the approved
  slice-1 scope audit with docs plus one executable
  contract; no production, catalog, inventory, or evidence
  change.
- Decision: Tables 2-12 through 2-16 hold 24 family forms
  with no delayed-variant row, so the 205-form denominator
  needs no expansion. All seven implemented delayed rows
  share their family form through a `_DELAYED`
  addressing-subset token, and every future delayed row
  composes the same way (`RPTBD`→`FORM-T2-16-04`,
  `FBD`→`FORM-T2-12-05`, `RETFD`→`FORM-T2-15-06`,
  `BACCD`→`FORM-T2-12-02`, `CALAD`→`FORM-T2-13-01`,
  `RCD`→`FORM-T2-15-03`). The new
  `test_inventory_delayed_family_binding.py` contract pins
  today's seven delayed symbols to their family forms and
  requires a non-delayed same-form sibling, so a
  delayed-only form ID fails the gate first. Branch
  readiness baseline is 8/24 `SUBSET` family forms.
- Correction to the 2026-09-11 wave-1 audit entry, which
  dismissed `RC` as a repeat-counter reference: `RC`
  (`FORM-T2-15-03`, in the Table 2-15 returns neighborhood
  between `FRETE` and `RET`, with the `RCD` delayed
  companion paralleling `BC`/`BCD` and `CC`/`CCD`) is the
  conditional-return instruction. Its 251 census words move
  from wave 3 to the wave-1 tail, and approved slice 5 now
  covers `RC`/`RCD` together.
- Validation: contract quality unittest fully OK (269,
  including the new lock); `git diff --check` passes. Full
  `make test` and sanitizer gates are unchanged from the
  W76 landing (both EXIT=0) since this slice touches no
  production or catalog surface. No knob was added or
  changed.
- Follow-up work: approved slice 2 (`XC`) is next.

## 2026-09-16 — P2-W77A catalog exact-word shadow seam

- Implementation commit `fc8c87d` admits the one overlap
  exception the pairwise catalog gate needs for
  conditional-return composition: an exact row declared
  before a covering masked row keeps its single word under
  the first-match lookup order, with the reversed order and
  all other collision classes still rejected. Three RED
  schema contracts pin accept/reversed-order-reject/
  partial-mask-reject. Production catalog output is
  byte-identical (`generate_opcode_lookup.py --check`
  green before and after). No knob, no C change.
- Decision: the unconditional word shared by an exact
  return row and a masked conditional-return row (for
  example `0xFC00` for `RET`/`RC`) behaves identically
  either way, so exact-wins is the faithful model; the
  declaration-order rule keeps generation deterministic
  without touching the flat 65,536-entry lookup.

## 2026-09-16 — P2-W77B RC conditional-return slice

- Implementation commit `f72e629` binds one Table 2-15 row
  under `FORM-T2-15-03`: `RC` at first-word value `0xFC00`
  under mask `0xFF00`, one program word, 256 encodings with
  the `0xFC00` word keeping its `RET` identity through the
  W77A seam, the new 1-word `BRANCH_COND8` layout, the
  `ACCUMULATORS_STATUS_C_TC_STACK_POINTER` input
  combination, and `INELIGIBLE` repeatability on the
  manual's explicit note. The pure leaf evaluates the
  sixteen image-evidenced single conditions through the
  shared seam (BIO/NBIO, UNC, overflow, multi-condition
  combinations reject as deferred, exactly like `BC`/`CC`),
  stages a `MEMORY` `STACK_POP` READ gated on known SP when
  taken and `NONE` when not taken, with no status effects.
  Taken retires through the unchanged `RET`-proven pop
  route; not-taken advances sequentially with no endpoint
  traffic. No `run.c` change, no new effect or address
  kind. `FORM-T2-15-03` lands `SUBSET` with the `RC` symbol
  under the shared `STACK_POP_NONDELAYED` binding (delayed
  `RCD` waits for W78).
- Composition costs the shadow exposed: (1) trusted slow
  prepare now verifies the bundle resolves to the
  binding's lookup identity instead of only matching its
  pattern, so the `RET` word can never prepare as `RC`
  (the fast path is untouched — its precondition already
  guarantees lookup provenance); (2) the shared
  trusted-packet test builds canonical bundles from the
  lowest lookup-resolving word; (3) five sibling
  near-miss lists now pin their `RC`-range flips
  (`LD_SRC_SHIFT`, `LD_SRC_ASM`, `RET` low byte,
  `RETE`/`RETF` bit 11) instead of UNKNOWN.
- Seven new executable contracts (catalog with 255+shadow
  population proof, decode with exhaustive 256-word loop
  and transactional rejection, handler with oracle parity
  across four routes, CPU retirement with taken-pop/
  not-taken-sequential/unknown-rejection cases) plus the
  W77A seam tests; evidence adds 3 TI plus 2 project
  records, 6 requirements, and 4 test records with index
  rows and shared-test reverse links.
- Validation is GREEN with no baseline deviation: `make
  test` exits 0 with all 383 C test binaries passing;
  contract quality unittest fully OK (272); knobs, source
  boundaries, manifests, evidence (329 records, 511
  requirements with 480 `IMPLEMENTED`, 406 tests all
  `PASSING`), inventory (realized 160 catalog rows,
  `ID_COUNT=161`, 7,109 lookup entries, 160 bindings, 127
  SUBSET bindings, family coverage 61/205 at 29.756%,
  33/205 strict), and observer-disabled structure pass.
  ASan/UBSan `make test` exits 0 with no findings.
  `generate_opcode_lookup.py --check` and `git diff
  --check` pass. No knob was added or changed. All touched
  handwritten files stay far below the split triggers.
- Decisions: `RC` reuses the `RET` pop seam and the
  `BC`/`CC` condition seam unchanged; deferred-condition
  rejection (rather than partial multi-condition support)
  matches the evidenced-code precedent; the plan's slice 5
  now reads `RC`/`RCD`; the approved P3 track is
  re-sequenced seam-first (S1 MMR/device routing leads
  after W78) with the arch-completeness criterion
  excluding only far/XPC per the baseline.
- Follow-up work: W78 `RCD` (delayed conditional return,
  evidence in hand: Z=1 selects `0xFE00`/`0xFF00`, two slot
  words execute) is next, then the S1 MMR/device seam.
  Open: `make test` recipe order still lets C regressions
  hide behind quality failures.

## 2026-09-16 — P2-W78 RCD delayed-conditional-return slice

- Commit `47de61a` lands the W78 slice (branch family complete
  through `RCD`): one catalog row at `0xFE00`/`0xFF00` under
  `FORM-T2-15-03`, `BRANCH_COND8`, `INELIGIBLE`, sixteen
  image-evidenced conditions, taken `MEMORY`
  `STACK_POP_DELAYED` READ through the unchanged W67B RETD
  route, not-taken `NONE`. The `0xFE00` word keeps `RETD`
  identity under the W77A shadow seam; 255 encodings decode
  `RCD`.
- Four new executable contracts (metadata, decode, handler
  with trusted/dispatch parity, CPU retirement with
  taken-pop/not-taken-sequential cases) plus registry
  records (334 evidence, 517 requirements with 486
  `IMPLEMENTED`, 410 tests all `PASSING`) with index rows
  and shared-test reverse links.
- Validation is GREEN: `make test` exits 0; ASan/UBSan
  `make test` exits 0 with no findings; `python -m unittest
  discover -s tests` OK; `generate_opcode_lookup.py --check`
  and `git diff --check` pass. Closure numbers match the
  plan projections exactly: 161 catalog rows,
  `ID_COUNT=162`, 7,364 lookup entries, 161 trusted
  bindings, 128 SUBSET bindings, family coverage 61/205 at
  29.756%, 33/205 strict. No knob was added or changed.
- Decisions: taken CPU retirement advances sequentially
  (`start+1`) with a two-word delayed-branch countdown per
  the RETD seam — the test's `+2` skip was fixed, not the
  route; `RETD`/`RETED` near-miss flips landing in the RCD
  range are pinned as `RCD` siblings (low-byte flips;
  `0xFF00` stays UNKNOWN); the `test_rcd.c` trusted-handler
  check referenced `ti_c54x_trusted_op_rc` by copy-paste and
  now pins `ti_c54x_trusted_op_rcd`; frozen wave pins bumped
  160→161 rows, 7109→7364 lookup, 127→128 SUBSET,
  `EXPECTED_DELAYED` gains `RCD` under `FORM-T2-15-03`.
- Follow-up work: S1 MMR/device routing seam is next (STM
  unlock, ~7000 words), then RPT activation. Open: `make
  test` recipe order still lets C regressions hide behind
  quality failures.

## 2026-09-16 — P3-S1 MMR/device routing seam (slice 6)

- Commit `6ccd4e7` lands the S1 sealed seam with no catalog rows
  (W43A pattern): Table 3-3 map plus owner dispatch versus a
  private 5-cell file (`IMR`/`TRN`/`BRC`/`RSA`/`REA` nested in
  `arch_state`, zero-plus-unknown under the existing reset),
  new whole-word `ST0`/`ST1`/`PMST` and accumulator-word
  accessors with owner-held knownness, a side-effect-free
  access planner gating READ knownness and guard-range writes,
  and root-transaction local service with zero endpoint calls,
  routed RMW support, and the kind-keyed deferred tail
  replicated. Reserved `02`–`05` and `XPC` decline to the
  endpoint unchanged.
- Four RED lanes green (`ti_c54x.mmr_route`,
  `ti_c54x.mmr_resolver`, `ti_c54x.mmr_transaction`,
  `ti_c54x.mmr_owner_access`); registry adds 3 evidence, 4
  requirements, 4 test records (337/521/414). Project
  requirements cite project evidence only; the TI map/STM
  records stand alone per the claim-scope rule.
- Validation is GREEN: `make test` exits 0 (398/398 CTests
  plus quality suites); ASan/UBSan `make test` exits 0;
  `generate_opcode_lookup.py --check`, evidence registry, and
  `git diff --check` pass. No knob was added or changed.
  Inventory coverage unchanged (seam adds no rows).
- Decisions: whole-word status knownness requires the full
  mask; accumulator-word knownness follows the accumulator
  bit with zero-based RMW writes; `reset_common` seeds `ST0`
  fully known (test premise corrected, not the owner);
  unknown MMR reads reject `INVALID_CANDIDATE`
  transactionally; existing CPU tests whose stacks addressed
  the file now pin routed behavior (new CALL PMAD
  push-to-`IMR` case) or use plain-RAM origins (`RET`,
  `RETE`, `CALLD`, CALL PMAD origin case).
- Split review: `memory_instruction_transaction.c` is 792
  lines — the next slice touching it must extract the MMR
  branch/RMW helper into its own module before 800;
  `registers.c` is 496 lines — cohesive, no split, revisit
  at 800. All other touched files stay far below triggers.
- Follow-up work: slice 7 `STM` composes on this seam (2
  words, `0111_011I_AAAAAAAA` plus `lk`, 9-MSB-clear address
  formation is family-slice work), then S2 RPT activation.

## 2026-09-16 — P3-S7 STM immediate-MMR-store slice (slice 7)

- Commit `8ceff88` lands the first family slice on the S1
  seam: one catalog row `STM` at `0x7700` under mask `0xFF00`
  (256 encodings, two program words) with the new
  `DIRECT7_EXTENSION16` layout (`I` flag, `addr7`, `lk16`),
  a DP-independent page-0 planner behind the new `DIRECT`
  memory address kind, and checked plus trusted
  direct-store leaves that defer the `I=1` indirect form as
  `INVALID_OPERAND`. `FORM-T2-20-14` lands `SUBSET` with
  the `STM` symbol.
- Five RED lanes green (`ti_c54x.stm_metadata`,
  `ti_c54x.stm_decode`, `ti_c54x.stm_ops`, `ti_c54x.cpu_stm`,
  `ti_c54x.direct_page0`); registry adds 2 evidence, 6
  requirements, 4 test records (339/527/418). The CPU lane
  proves MMR retirement with zero endpoint calls, DP0-RAM
  retirement with one endpoint WRITE, page-0
  CMPT-independence, and the deferred indirect-form stop.
- Validation is GREEN: full CMake CTest 403/403; `make
  test` exits 0 (101 plus 272 python plus evidence/knob
  suites); ASan/UBSan `make test` exits 0;
  `generate_opcode_lookup.py --check`, evidence registry,
  and `git diff --check` pass. No knob was added or
  changed. Coverage 62/205 (30.244%), strict 33/205;
  `ID_COUNT=163`, 7,620 lookup entries.
- Decisions: the `DIRECT` kind needed a page-0 liveness arm
  in `matches_live` (`NONE` policy plus dma echo plus
  sequential PC, no CMPT/ARP read per the page-0
  no-state contract) — without it every direct `STM`
  retired `INVALID_STATE` despite a valid shape, plan, and
  route; the arm rejects forged dma and stale PC
  transactionally. The two `ST #lk,Smem` fixed-bit-8
  near-miss flips (`0x7780`/`0x7790`) genuinely resolve
  into the `STM` row (indirect form) and now pin `STM`
  identity per the RETD/RETED sibling precedent; the
  61/205 inventory pins bumped to 62/205 with strict
  unchanged at 33/205.
- Split review: `memory_instruction_candidate_validate.c`
  is 767 lines and `memory_instruction_transaction.c`
  stays 792 (untouched this slice) — neither exceeds 800,
  so no split; the next slice touching either file must
  re-evaluate, with the MMR-branch/RMW extraction noted
  under S1 still open. All other touched files stay far
  below triggers.
- Follow-up work: S2 RPT activation is next (the repeat
  seam `STM` leaves `UNRESOLVED`), then the deferred
  `STM` indirect Smem form and `STH`/`STLM` family slices.

## 2026-09-16 — P3-S2 single-instruction repeat slice (slice 8)

- Commit `6cf9924` activates the S2 repeat seam: two catalog
  rows `RPT_K` at `0xEC00` under mask `0xFF00` (256
  encodings, one program word, `FORM-T2-16-02`) and
  `RPT_LK` exact at `0xF070` (16-bit extension, two
  program words, `FORM-T2-16-03`), both `INELIGIBLE` with
  no inputs, reusing the `RPT_IMMEDIATE_K`/`RPT_IMMEDIATE_LK`
  layouts and the `op_rpt_immediate` core behind checked and
  trusted leaves. `RPT Smem` (`FORM-T2-16-01`) stays
  `UNIMPLEMENTED`; `RPTB`/`RPTBD`/`RPTZ` are untouched.
- The CPU owns `ti_c54x_repeat_progress` plus the W48B
  pending latch and a body-PC word. The `REPEAT_START`
  route starts progress, rejects `RPT` in delay slots and
  nested `RPT` while `ACTIVE`, and retires sequentially
  onto the body. The predecode gate admits only `ELIGIBLE`
  bodies; a new `repeat_single` driver under
  `src/sequencer/repeat/` owns start, predecode, and
  body-retired decisions while `run.c` owns no repeat
  policy. No row is `ELIGIBLE` yet, so end-to-end replay
  still waits on the first positively-evidenced `ELIGIBLE`
  classification.
- Eight RED lanes green (`ti_c54x.rpt_k_metadata`,
  `ti_c54x.rpt_lk_metadata`, `ti_c54x.rpt_k_decode`,
  `ti_c54x.rpt_lk_decode`, `ti_c54x.rpt_k_ops`,
  `ti_c54x.rpt_lk_ops`, `ti_c54x.repeat_single_flow`,
  `ti_c54x.cpu_rpt`); registry adds 2 evidence, 10
  requirements, 8 test records (341/537/426).
- Validation is GREEN: full CMake CTest 411/411; `make
  test` exits 0 (101/101); ASan/UBSan `make test` exits 0
  (101/101); python `unittest discover`, evidence registry,
  ISA inventory, source-boundary, opcode-lookup golden, and
  `git diff --check` pass. No knob was added or changed.
  Coverage 64/205 (31.220%), strict 33/205 unchanged; 164
  inventory bindings.
- Decisions: the RPT-neighbor single-bit flips re-pin
  `RPTZ`/`RPTB` identity as rejected non-collisions (the
  `STM` near-miss precedent applied to the repeat word
  family). The shared `op_rpt_immediate` core is reused
  behind per-row checked/trusted leaves with the layout
  split (`RPT_IMMEDIATE_K` vs `RPT_IMMEDIATE_LK`) rather
  than a forked core. The commit message's draft
  759-line note was amended pre-push to the true counts
  (`run.c` 766 → 795, helper 61 + 54 header).
- Split review: `run.c` is 795 lines after extracting
  `repeat_activate.c` (61 + 54 header) — under 800, so no
  split; the next slice touching `run.c` must re-evaluate.
  `test_repeat_single_flow.c` is 398 lines (test, exempt
  from the handwritten-source split rule). All other new
  files stay under 110 lines.
- Follow-up work: S3 MAC/MPY datapath is next, then the
  first `ELIGIBLE` classification with its retirement-tail
  `HOLD` call sites; deferred `RPT Smem`, `RPTB`/`RPTZ`,
  vectors/take-path/NMI/timing, and the `STM` indirect
  Smem form.

## 2026-09-16 — P3-S3a multiply-accumulate seam slice (slice 9)

- Commit `598cc61` lands the S3 datapath seam with no catalog
  rows: a pure core under `src/isa/ops/multiply/` evaluating
  signed 16x16 T-times-multiplicand products with `FRCT`
  left shift, `R` rounding of the accumulate/store result,
  `MAC` add and `MAS` subtract over a copied raw40
  accumulator, `MPY` store independent of accumulator
  knownness, 32-bit overflow detection with `OVM`
  saturation (`00 7FFF FFFF` / `FF 8000 0000`) and `OVdst`
  disposition, and transactional unknown/invalid stops with
  no staged output. A `STATUS_FRCT` input resource (bit
  2048) carries `ST1.FRCT` with canonical unknownness plus
  the `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM` composite;
  the snapshot stays at 64 bytes.
- Two RED lanes green (`ti_c54x.multiply_core`,
  `ti_c54x.cpu_input_collect_frct_status`); registry adds 7
  evidence, 2 requirements, 2 test records (348/539/428).
  The core proves the three TI `MAC` vectors (pp. 4-81–4-83)
  and three `MPY` vectors (pp. 4-101–4-103) digit-for-digit,
  including the `FRCT` shift and `R` rounding pairs.
- Validation is GREEN: full CMake CTest 413/413; `make
  test` exits 0 (101/101); ASan/UBSan `make test` exits 0
  (101/101, no findings); python `unittest discover`,
  evidence registry, ISA inventory, source-boundary,
  opcode-lookup golden, knob registry, and `git diff
  --check` pass. No knob was added or changed. Coverage
  stays 64/205 (31.220%), strict 33/205; 164 bindings, 165
  rows, 7,877 lookup entries. The README `Status` snapshot
  is refreshed to P3-S2+S3a (it was frozen at P2-W51C).
- Decisions: overflow is 32-bit signed overflow of the true
  sum (uniform with the `ADD` precedent and the `OVM`
  saturation values), not 40-bit; rounding applies before
  the single overflow determination; `SMUL` product
  pre-saturation is out of scope with the common target on
  the `SMUL`-0 baseline (PMST bit, revision-gated
  devices). Ten neighbor collect tests moved their
  invalid-mask probe from `0x0800` to `0x1000` (two
  spellings: `U`/`UINT16_C`/`UINT32_C`); decode-test
  `0x0800` opcode-bit flips are a different domain and
  untouched. The index mirror needed one fixup round
  (missing rows, then runner-cell quoting) before the
  registry gate passed.
- Split review: `op_multiply.c` is 102 lines plus a 67-line
  header; both test files are tests (exempt). No file
  approaches triggers.
- Follow-up work: S3b `MAC` Smem row (`FORM-T2-04-01`)
  with metadata, decode, ops, CPU retirement, `ELIGIBLE`
  classification, and `RPT MAC` end-to-end replay; then
  S3c `MPY` Smem and `MPYA`/`MAS`/`MACR` in census order.

## 2026-09-16 — P3-S3b MAC Smem row slice (slice 10)

- Commit `eb50b85` lands the first `MAC` family row on the S3a
  seam: `MAC_SMEM` at `0x2800` under mask `0xFC00` (1024
  encodings, one program word) with the new shared
  `ROUND_SELECT_INDIRECT_ADDR7` layout (round, select,
  indirect, addr7), `ELIGIBLE` repeatability by the Table
  2-28 omission pin (pp. 2-20–2-21), and the
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM` inputs. Checked
  and trusted leaves stage direct page-0 and
  standard-indirect MOD0–MOD3 reads (arf `addr[2:0]`, 4-bit
  mode `addr[6:3]`), defer exotic modes as `INVALID_OPERAND`
  per the `STM` precedent, and complete the read value
  through the multiply core into an accumulator write with
  `OVdst` disposition. `FORM-T2-04-01` binds at `SUBSET`
  over `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`.
- Five RED lanes green (`ti_c54x.mac_smem_metadata`,
  `ti_c54x.mac_smem_decode`, `ti_c54x.mac_smem_ops`,
  `ti_c54x.cpu_mac_smem`, `ti_c54x.cpu_rpt_mac_replay`);
  registry adds 2 evidence, 6 requirements, 5 test records
  (350/545/433). `RPT #2` plus three `MAC` bodies replays
  end-to-end (`A` = `0x1800`, `RC` = 0, phase `COMPLETE`)
  through the new retirement-tail `HOLD` wiring: a body-PC
  latch set at `REPEAT_START` retire, `ADVANCE`/`HOLD`/`INVALID`
  recording at all three retirement sites (MEMORY fresh,
  MEMORY resume, ordinary), latch cleared on `ADVANCE`,
  `COMPLETE`, init, and reset. This closes the S2 `HOLD`-tail
  item; delayed-branch/repeat composition stays deferred.
- Validation is GREEN: full CMake CTest 418/418; `make
  test` exits 0 (101/101); ASan/UBSan `make test` exits 0
  (101/101); python `unittest discover`, evidence registry,
  ISA inventory, source-boundary, opcode-lookup golden, knob
  registry, and `git diff --check` pass. No knob was added
  or changed. Coverage 65/205 (31.707%), strict 33/205; 165
  bindings, 166 rows, 8,901 lookup entries.
- Decisions: overflow stays 32-bit (uniform with `ADD` and
  the `OVM` values); `MPY`-style store is not special-cased
  in the shared leaf. Neighbor pins move to ID 166, 165
  bindings/rows, 8,901 entries, 65/205 with 31.707%,
  including the frozen SUBSET set plus addressing token.
  The evidence gate needed two fixup rounds (scope leak of
  TI evidence into a project requirement, then a locator
  parse that ate `2-28` out of `Table 2-28` as a page —
  reworded to the digit-free chapter name).
- Split review: the `HOLD` sites pushed `run.c` to 816
  lines, so its static result/stop/branch plumbing moves to
  the new `run_support` module (`run.c` 816 → 657,
  `run_support` 164). `op_mac_smem.c` is 227 lines;
  remaining new files stay small; test files are exempt.
- Follow-up work: S3c `MPY` Smem row (`FORM-T2-03-01`)
  reusing the shared layout with `STORE` completion and a
  `D`-destination select, then `MPYA`/`MAS`/`MACR` in census
  order; deferred exotic indirect modes, `MACA`/`MACD`/
  `MACP`, dual `#lk` words, and far/XPC/timing.

## 2026-09-16 — Correction to the S3b entry (line count)

- The S3b entry states `op_mac_smem.c` is 227 lines; the
  committed file is 269 lines. The split conclusion is
  unaffected (well under triggers), and no other S3b figure
  changes.

## 2026-09-16 — P3-S3c MPY Smem row slice (slice 11)

- Commit `828e45f` lands the `MPY` Smem row (`FORM-T2-03-01`)
  on the S3a seam: `MPY_SMEM` at `0x2000` under mask `0xFC00`
  (1024 encodings, one program word) reusing the shared
  `ROUND_SELECT_INDIRECT_ADDR7` layout, `ELIGIBLE`
  repeatability by the Table 2-28 omission pin, and the
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM` inputs. Checked
  and trusted `op_mpy_smem` leaves stage direct page-0 and
  standard-indirect MOD0–MOD3 reads, defer exotic modes as
  `INVALID_OPERAND`, and complete the read value through the
  multiply core `STORE` path into an accumulator write with
  `OVdst` disposition and no accumulation.
- Four RED lanes green (`ti_c54x.mpy_smem_metadata`,
  `ti_c54x.mpy_smem_decode`, `ti_c54x.mpy_smem_ops`,
  `ti_c54x.cpu_mpy_smem`); registry adds 1 evidence, 5
  requirements, 4 test records (351/550/437). The ops lane
  retires the independent TI oracles: MPY Example 4
  (`T` = `0x1234`, Smem = `0x5678`, `MPYR` stores
  `00 0626 0000`) and Example 1 (`T` = 6, Smem = 7,
  `FRCT` = 1 stores 84). No dedicated `RPT MPY` replay
  lane: `ELIGIBLE` replay is proven generically by the S3b
  `RPT MAC` test plus the catalog-wide admission sweep.
- Validation is GREEN: full CMake CTest 422/422; `make
  test` exits 0 with no failures; ASan/UBSan `make test`
  exits 0 with no failures; evidence registry, ISA
  inventory, source-boundary, opcode-lookup `--check`, and
  `git diff --check` pass. No knob was added or changed.
  Coverage 66/205 (32.195%), strict 33/205; 166 bindings,
  167 rows, 9,925 lookup entries.
- Decisions: the `STORE` completion passes
  `accumulator_known = false` with a zeroed raw40 (the core
  ignores both on the `STORE` path), and the leaf drops the
  destination-knownness gate — pinned by a dedicated
  unknown-accumulator-tolerated test that distinguishes
  `MPY` from `MAC`. The `LD_T` fixed-bit `0x1000` flip
  (`0x3090` → `0x2090`) now lands in the `MPY` row, so both
  `LD_T` near-miss loops carve it out as the named P3-S3c
  sibling after the file-local `MOD6`/`MOD8`/`MOD10`
  precedent. Neighbor pins move to ID 167, 166
  bindings/rows, 9,925 entries, 66/205 with 32.195%,
  including the frozen `SUBSET` set (plus `MPY_SMEM`) and
  the addressing token. The `README` snapshot was stale
  since S3a, so it now records S3b (`ELIGIBLE` plus `RPT
  MAC` replay) and S3c through the live numbers.
- Split review: `op_mpy_smem.c` is 254 lines and
  `op_mpy_smem.h` is 46 lines; no split required. Test
  files are exempt.
- Follow-up work: `MPYA`/`MAS`/`MACR` in census order;
  deferred exotic indirect modes, `MACA`/`MACD`/`MACP`,
  dual `#lk` words, and far/XPC/timing.

## 2026-09-16 — P3-S3d MPYA Smem row slice (slice 12)

- Commit `d5b49af` lands `MPYA` Smem syntax 1
  (`FORM-T2-03-06`): `MPYA_SMEM` at `0x3100` under mask
  `0xFF00` (256 encodings, one program word) with the new
  `INDIRECT_ADDR7` layout, `ELIGIBLE` repeatability by the
  Table 2-28 omission pin, and the new
  `ACCUMULATORS_STATUS_FRCT_OVM` inputs (T is written, not
  read). Checked and trusted `op_mpya_smem` leaves stage
  direct page-0 and standard-indirect MOD0–MOD3 reads with
  the read-phase T load, defer exotic modes as
  `INVALID_OPERAND`, and complete the read value through
  the multiply core `STORE` path as Smem times A-high into
  fixed B with `OVdst` disposition.
- Six RED lanes green (`ti_c54x.indirect_addr7_layout`,
  `ti_c54x.mpya_smem_metadata`, `ti_c54x.mpya_smem_decode`,
  `ti_c54x.mpya_smem_ops`, `ti_c54x.cpu_mpya_smem`,
  `ti_c54x.memory_instruction_commit` T-load lanes);
  registry adds 2 evidence, 7 requirements, 6 test records
  (352/557/443). The ops and CPU lanes retire the TI
  oracle: MPYA Example 1 (`MPYA *AR2`, `A` =
  `FF 8765 1111`, `T` = `1234`, mem = `5678`, `FRCT` = 0
  yields `B` = `FF D743 6558`, `T` = `5678`). A-high is
  bits 31:16 as signed 16-bit, verified by hand-computed
  decimal (`-30875 * 22136 = -683449000`) against both
  printed examples before the leaf existed. No dedicated
  `RPT MPYA` replay lane: `ELIGIBLE` replay stays proven
  generically.
- The T-write/B-store dual commit is the slice's seam:
  the completion returns one effect, so the memory effect
  grows one `load_t_with_read` flag (LP64 40→48, outer
  effect 48→56, transaction root 352→360; ILP32 36→40,
  40→44, 324→328, with the ILP32 root confirmed by the
  `-m32` perf probe) and the read commit loads T after the
  read-value completion commits, transactionally on a
  candidate. The flag validates READ-only without RMW;
  unflagged reads preserve T by test.
- Validation is GREEN: full CMake CTest 427/427; `make
  test` 101 ok with no failures; ASan/UBSan `make test`
  with no failures; evidence registry, ISA inventory,
  source-boundary, opcode-lookup `--check`, and `git
  diff --check` pass. No knob was added or changed.
  Coverage 67/205 (32.683%), strict 33/205; 167 bindings,
  168 rows, 10,181 lookup entries.
- Decisions: mask first drafted as `0xFF80` would have
  pinned I=0 and excluded indirect `*AR2`; writing the RED
  metadata test forced the bit math to `0xFF00`/256
  before any production edit. The `LD_T` fixed-bit
  `0x0100` flip (`0x3090` → `0x3190`) lands in the MPYA
  row, so both `LD_T` near-miss loops carve it out as the
  named P3-S3d sibling (`0x0080` → `0x3010` stays
  unknown; an early carve-out of the wrong mask was
  caught by the tests and reverted). Neighbor pins move
  to ID 168, 167 bindings/rows, 10,181 entries, 67/205
  with 32.683%, including the frozen `SUBSET` set (plus
  `MPYA_SMEM`) and the addressing token.
- Split review: `op_mpya_smem.c` is 248 lines and
  `op_mpya_smem.h` is 47 lines; no split required. The
  commit-path T load is 17 lines inside the existing
  commit module; no split required.
- Follow-up work: `MAS`/`MACR` in census order (plus
  `MPYA` syntax 2, the exact `0xF48C`/`0xF49C` rows with
  no memory seam); deferred exotic indirect modes,
  `MACA`/`MACD`/`MACP`, dual `#lk` words, and
  far/XPC/timing.

## 2026-09-16 — P3-S3e MAS Smem row slice (slice 13)

- Commit `a5d0670` lands `MAS` Smem syntax 1
  (`FORM-T2-04-14`): `MAS_SMEM` at `0x2C00` under mask
  `0xFC00` (1024 encodings, one program word) reusing the
  shared `ROUND_SELECT_INDIRECT_ADDR7` layout, `ELIGIBLE`
  repeatability by the Table 2-28 omission pin, and the
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM` inputs.
  Checked and trusted `op_mas_smem` leaves stage direct
  page-0 and standard-indirect MOD0–MOD3 reads, defer
  exotic modes as `INVALID_OPERAND`, and complete the read
  value through the multiply core `SUBTRACT` path as
  destination minus T times Smem with `OVdst` disposition
  and R-suffix rounding applied to the
  multiply-subtract result.
- Four RED lanes green (`ti_c54x.mas_smem_metadata`,
  `ti_c54x.mas_smem_decode`, `ti_c54x.mas_smem_ops`,
  `ti_c54x.cpu_mas_smem`); registry adds 2 evidence, 5
  requirements, 4 test records (353/562/447). The ops and
  CPU lanes retire the TI oracles: MAS Example 1 (`MAS
  *AR5+`, `A` = `1000`, `T` = `0400`, mem = `1234`,
  `FRCT` = 0 yields `A` = `FF FFB7 4000`, `AR5` = `0101`)
  and MAS Example 3 (`MASR`, same pre-state yields `A` =
  `FF FFB7 0000`). Example 3 distinguishes
  result-after-subtract rounding (which the shared core
  already implements) from product rounding, and is
  verified by hand decimal before the leaf existed. No
  dedicated `RPT MAS` replay lane: `ELIGIBLE` replay stays
  proven generically.
- Validation is GREEN: full CMake CTest 431/431; `make
  test` 101 ok with no failures; ASan/UBSan `make test`
  with no failures; evidence registry, ISA inventory,
  source-boundary, opcode-lookup `--check`, and `git
  diff --check` pass. No knob was added or changed.
  Coverage 68/205 (33.171%), strict 33/205; 168 bindings,
  169 rows, 11,205 lookup entries.
- Decisions: the `MASR` R variant shares the base row
  encoding (MAC precedent), so only `FORM-T2-04-14` binds
  and `-15` stays `UNIMPLEMENTED`. The `BANZ`/`BANZD`
  fixed-bit `0x4000` flips (`0x6C00` → `0x2C00`,
  `0x6E00` → `0x2E00`) land in the MAS row, so both
  near-miss loops carve them out as named P3-S3e
  siblings. A dropped nibble in hand-computed 40-bit
  expectations (`0xFFFB7…` vs `0xFFFFB7…`) was caught by
  re-deriving the two's complement width before GREEN.
  Neighbor pins move to ID 169, 168 bindings/rows,
  11,205 entries, 68/205 with 33.171%.
- Split review: `op_mas_smem.c` is 269 lines and
  `op_mas_smem.h` is 46 lines; no split required.
- Follow-up work: `MACR` in census order (plus `MPYA`
  syntax 2 and dual `Xmem`/`Ymem` syntaxes); deferred
  exotic indirect modes, `MACA`/`MACD`/`MACP`, dual `#lk`
  words, and far/XPC/timing.

## 2026-09-16 — P3-S3f MACR Smem lock-in slice (slice 14)

- Commit `45b764f` closes the census `MACR` Smem form
  without a production change: `MACR` Smem shares the
  `MAC_SMEM` row encoding with R=1 (verified against the
  TI `MAS[R]`-style opcode table and the `MAC[R]` section
  covering Examples 5/6 on pp. 4-82–4-84). Two Example 5
  vectors lock the previously untested R=1
  accumulate-into-A combination through the maintained
  suites: `test_completion_macr_example5` in
  `test_mac_smem_ops.c` (leaf completion yields
  `0x490000`) and `test_macr_example5_retires` in
  `test_cpu_mac_smem.c` (retirement with AR5
  post-increment). Both passed on the first run against
  the S3b leaf, so the slice is lock-in, not RED/GREEN.
- Evidence gains 1 activation record, 2 requirements, and
  2 test records (354/564/449) over the maintained MAC
  suites, which share their CTest binaries with the S3b
  records. The first draft put both vectors under one
  requirement owned by `src/isa/ops/multiply/` citing the
  project activation; the gate rejected the owner mirror
  (CPU test) and the scope leak (contract-only evidence
  under an architectural claim), so the requirement
  split by owner with TI-only evidence, matching the S3b
  shape. No catalog, inventory, or pin changes; the dual
  `Xmem`/`Ymem` `MACR` syntaxes (`FORM-T2-04-05/06`)
  stay `UNIMPLEMENTED`.
- Validation is GREEN: full CMake CTest 431/431; `make
  test` 101 ok with no failures; ASan/UBSan `make test`
  with no failures; evidence registry gate passes. No
  knob was added or changed.
- Follow-up work: dual `Xmem`/`Ymem` syntaxes
  (`MACR`/`MASR`/`MAC`/`MPYA` syntax 2); deferred exotic
  indirect modes, `MACA`/`MACD`/`MACP`, dual `#lk` words,
  and far/XPC/timing.

## 2026-09-16 — P3-S3g MAC Xmem,Ymem dual slice (slice 15)

- Commit `f24e8ed` lands `FORM-T2-04-04` (TI SPRU172C
  `MAC[R]` syntax 2, opcode `1011 0RSD XXXX YYYY`,
  pp. 4-82–4-84) as nine `MAC_DUAL_Mx_My` rows at
  `0xB000|xmode<<6|ymode<<2` under mask `0xF8CC` with
  `UNRESOLVED` repeatability. The new
  `DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR` layout packs
  the contiguous R/S/D bits into one 3-bit RSD field
  plus XAR/YAR, after the first draft used five
  one-bit fields and smashed the stack: the frozen
  trusted-operand capacity is 4 and the generic
  trusted extractor refuses wider layouts, while the
  trusted packet size is pinned against growth. The
  checked/trusted leaves stage dual-indirect
  MOD0–MOD2 reads with `load_t_with_read` and complete
  `(X)x(Y)+(src)` into `dst` through the multiply
  core; the dual READY commit now honors the T-load
  flag with the X-side word (mirroring the single-word
  commit). TI Examples 4/6 retire at leaf and CPU:
  derived vectors keep TI's values/FRCT/T-load but
  ride Y on `*AR4+` because TI's `*AR6+` is outside
  the frozen AR2–AR5 dual root; an M2_M2 lane proves
  both post-increments and MACR rounding.
- RED was observed three times: the leaf stub failed
  `ti_c54x.mac_dual_ops` at staging; the committed
  T-load was stashed to prove `ti_c54x.cpu_mac_dual`
  fails exactly at the T assertion without it; and a
  hand-computed `0x0C4C10C4` expectation in the
  author's own src-B vector was corrected to
  `0x0C4C00C4` after the harness printed
  actual=`0x0C4C00C4` (the leaf was right). The
  five-field RSD draft also exposed the capacity
  limit through ASan-visible stack smashing before
  the three-field redesign.
- Evidence gains 2 records, 2 requirements (owner
  split `src/isa/ops/multiply/` vs `src/cpu/`, TI-only
  evidence), and 2 test records (356/564/449→358/566/451).
  The first evidence draft
  cited section `MAC[R]`; the gate's locator parser
  wants the bare section, so the record uses `MAC`
  with the `4-83`–`4-84` page range. Neighbor pins
  move to 178 IDs/rows/bindings, 12,357 lookup
  entries, 69/205 (33.659%); eleven near-miss flips
  (`LD_T`/`LD_LK16`/`LD_LK_SHIFT`/`LD_SRC_ASM`/
  `LD_XMEM_SHIFT`/`B_PMAD`/`BD`/`CALL_PMAD`/`CALLD`/
  `RETF`/`RPT_LK`) now land in the new rows and are
  carved out as named S3g siblings. `make test` caught
  two pin classes the CMake run hid behind stale
  binaries (`ID_COUNT` and population counts), which
  is why both harnesses stay in the gate battery.
- Validation is GREEN: full CMake CTest 433/433 on a
  fresh full build; `make test` 101 ok; ASan/UBSan
  `make test` clean; Python `unittest discover` OK;
  evidence, inventory, source-boundary, and lookup
  `--check` gates pass; `git diff --check` clean. No
  knob was added or changed. New files peak at 402
  lines (`op_mac_dual.c`); no split required.
- Follow-up work: `MPY`/`MAS` dual rows reusing the
  S3g leaf/commit pattern, `MPYA` syntax 2, AR6/AR7
  dual-Y widening against the CPU addressing
  reference, MOD3 neighborhoods, dual replay
  classification; deferred exotic indirect modes,
  `MACA`/`MACD`/`MACP`, dual `#lk` words, and
  far/XPC/timing.

## 2026-09-16 — P3-S3h MPY Xmem,Ymem dual slice (slice 16)

- Commit `58d84a7` lands `FORM-T2-03-03` (TI SPRU172C
  `MPY` syntax 2, opcode `1010 010D XXXX YYYY`) as
  nine `MPY_DUAL_Mx_My` rows at
  `0xA400|xmode<<6|ymode<<2` under mask `0xFECC`,
  reusing the `DUAL_DESTINATION_XAR_YAR` layout with
  `UNRESOLVED` repeatability — no new layout, no
  trusted-capacity pressure. Checked/trusted leaves
  stage dual-indirect MOD0–MOD2 reads with
  `load_t_with_read` (TI Execution 2 loads
  `(Xmem)` into T) and complete `(X)x(Y)` through the
  multiply core with STORE overwrite; the core
  accepts unknown accumulator input for STORE, so the
  new `AUXILIARY_REGISTERS_STATUS_FRCT_OVM` mask
  drops both accumulators and T. TI MPY Example 2
  (`MPY *AR2-, *AR4+0%, B`, p. 4-102) retires at leaf
  and CPU: X `0010`, Y `0002`, B `0020`, T `0010`,
  AR2 `01FE`, AR4 `0301`. Syntax 2 carries no R bit,
  so `MPYR` stays a syntax-1 row sharing and
  `FORM-T2-03-02` stays `UNIMPLEMENTED` by the S3c/S3f
  convention. The existing TI MPY record already
  spans pp. 4-101–4-103 (verified per-PDF-page), so
  no new TI record was needed — only the activation,
  two owner-split requirements, and two test records
  (358/566/451→359/568/453).
- RED was observed on the stub (`mpy_dual_ops` fails
  at staging). The CPU failure mid-slice was the
  author's own M0_M0 case, not the leaf: the word
  `0xA402` selects Y=AR4 while the test seeded AR3,
  and a stage-by-stage `/tmp` probe then proved the
  production path retires Example 2 exactly
  (B=`0x20`, T=`0x10`, AR updates). A second
  author-side trip: the fresh-CPU FRCT bit is
  unknown, so the CPU test clears FRCT and marks it
  known for the FRCT=0 oracle.
- Neighbor pins move to 187 IDs, 186 rows/bindings,
  12,645 lookup entries, 70/205 (34.146%), SUBSET
  bindings 153 with a `MPY_DUAL_SUBSET` contract set.
  The 0xA4xx near-miss sweep (decode plus metadata
  tests) is clean — no carve-outs needed, unlike the
  eleven S3g siblings. `make test` versus CTest
  accounting: only 101 verbose `: ok` lines print;
  the multiply suites pass silently (rc=0), so the
  count is stable while CTest tracks the true total.
- Validation is GREEN: full CMake CTest 435/435 on a
  fresh full build; `make test` with no failures;
  ASan/UBSan `make test` clean; Python `unittest
  discover` OK (272); evidence, inventory,
  source-boundary, knob, and lookup `--check` gates
  pass; `git diff --check` clean. No knob was added
  or changed. New files peak at 377 lines
  (`op_mpy_dual.c`); no split required.
- Follow-up work: `MAS` dual rows reusing the S3g/S3h
  leaf/commit pattern, `MPYA` syntax 2, AR6/AR7
  dual-Y widening, MOD3 neighborhoods, dual replay
  classification; deferred exotic indirect modes,
  `MACA`/`MACD`/`MACP`, dual `#lk` words, and
  far/XPC/timing.

## 2026-09-17 — P3-S3i MAS Xmem,Ymem dual slice (slice 17)

- Commit `bb7c419` lands `FORM-T2-03-03`-sibling
  `FORM-T2-04-16` (TI SPRU172C `MAS` syntax 2, opcode
  `1011 1RSD XXXX YYYY`) as nine `MAS_DUAL_Mx_My` rows
  at `0xB800|xmode<<6|ymode<<2` under mask `0xF8CC`,
  reusing the `DUAL_ROUND_SOURCE_DESTINATION_XAR_YAR`
  layout with `UNRESOLVED` repeatability and
  `ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM`
  resources — no new layout, no trusted-capacity
  pressure. Checked/trusted leaves stage dual-indirect
  MOD0–MOD2 reads with `load_t_with_read` (TI Execution
  loads `(Xmem)` into T) and complete `(src)` minus
  `(X)x(Y)` through the multiply core with FRCT/OVM
  and R rounding; R=1 words execute through the same
  rows, so no separate `MASR` dual form exists. CPU
  retirement proves the TI Example 2/4-derived silicon
  vectors (X `5678`, Y `1234`, src A `1000`, FRCT 1
  yield B `FF F3B4 0F40` unrounded and `FF F3B4 0000`
  rounded, T `5678`); TI prints B `FF F9DA 0FA0/0000`,
  which equals the FRCT=0 computation exactly, so a
  dedicated FRCT=0 lane pins the printed value while
  FRCT=1 executes the SPRU131-shifted product (new
  TI record `E-TI-C54X-SPRU131G-ST1-FRCT-P4-5` plus
  activation, two requirements, and two test records
  359/568/453→361/570/455).
- RED was observed on the stub (`mas_dual_ops` fails
  at staging). Read-only review before commit found no
  blocking finding: row values follow
  `0xB800|xmode<<6|ymode<<2` exactly and the full-suite
  disjointness sweep rules out neighbor collisions.
  `git diff --check` clean. New files peak at 429
  lines (`test_mas_dual_ops.c`); no split required.
- Neighbor pins move to 196 IDs, 195 rows/bindings
  (162 SUBSET plus 33 COMPLETE), 13,797 lookup
  entries, 71/205 (34.634%) with the `FORM-T2-04-16`
  contract set; strict stays 33/205 (16.098%). The
  0xB8xx near-miss sweep (decode plus metadata tests)
  is clean — no carve-outs needed.
- Validation is GREEN: full CMake CTest 437/437 on the
  working build; fresh Debug configure plus full CTest
  437/437 in `/tmp/ti-c54x-s3i-debug`; full sanitizer
  CMake/CTest 436/436 (quality.python excluded, green
  separately); `make test` exit 0 with no failures;
  focused ASan/UBSan `mas_dual` tests rc=0 with
  `detect_leaks=0`; Python `unittest discover` OK
  (272); evidence, inventory, source-boundary, knob,
  manifest, observer-artifact, and lookup gates pass.
  No knob was added or changed. README snapshot now
  reads P3-S3i complete.
- Follow-up work: `MPYA` syntax 2, `MACA`/`MACD`/
  `MACP` family, AR6/AR7 dual-Y widening, MOD3
  neighborhoods, dual replay classification; deferred
  exotic indirect modes, dual `#lk` words, and
  far/XPC/timing.

## 2026-09-17 — P3-S3j MPYA-dst register-only slice (slice 18)

- Commit `f53e120` lands `FORM-T2-03-07` (TI SPRU172C
  `MPYA` syntax 2, opcode `1111 010D 1000 1100`,
  dual-sourced against the SPRU179C algebraic
  `dst = T * hi(A)`) as one `MPYA_DST` catalog row at
  `0xF48C` under mask `0xFEFF` carrying both D
  encodings through the reused `MAX_MIN_ACC`
  single-destination-bit layout with `ELIGIBLE`
  repeatability (Table 2-28 lists no MPYA entry, the
  recorded S3d omission basis) — no new layout,
  resource, effect, address, state, CPU, public-API,
  or manifest seam. Checked/trusted leaves complete
  `(T) x (A(32-16))` into dst through the multiply
  core with STORE select and no rounding (the S3d
  `hi(A)` feed with T and multiplicand roles
  swapped); no memory effect, no T update, C/TC
  preserved, `OVdst` follows the core. CPU
  retirement proves TI MPYA Example 2 (`A FF 8765
  1111`, `T 4567`, FRCT 0 yield `B FF DF4D B2A3`
  with A and T unchanged; the vector was recomputed
  independently before pinning). New TI record
  `E-TI-C54X-SPRU172C-S4-MPYA-DST-P4-104-4-105`
  plus activation, five owner-split requirements, and
  four test records (361/570/455→363/575/459); the
  PROJECT INVENTORY requirement rides the shared
  `T-PROJECT-ISA-INVENTORY-CATALOG-BINDING` test per
  the S3e convention, and the binding uses the new
  `REGISTER_DST` subset token (SUBSET bindings may
  not use `ALL`).
- RED was observed on the stub (absent
  `TI_C54X_OPCODE_ID_MPYA_DST` identity). One
  author-side trip: the fresh-CPU FRCT bit is
  unknown, so the CPU test clears FRCT and marks it
  known for the FRCT=0 oracle (the S3h lesson). A
  second: `fixture.h`'s `seed_address` helper is
  unused by the register-only form and trips
  `-Werror`, silenced with the `(void)` precedent.
  `git diff --check` clean. New files peak at 238
  lines (`test_mpya_dst_ops.c`); no split required.
- Neighbor pins move to 197 IDs, 196 rows/bindings
  (163 SUBSET plus 33 COMPLETE), 13,799 lookup
  entries, 72/205 (35.122%), SUBSET bindings 163
  with the `FORM-T2-03-07` contract set; strict stays
  33/205 (16.098%). The 0xF48C near-miss sweep
  (decode plus metadata tests) is clean — two exact
  rows would collide on the unmasked D bit, so the
  single masked row is the honest shape.
- Validation is GREEN: full CMake CTest 441/441 on
  the working build; fresh Debug configure plus full
  CTest 441/441 in `/tmp/ti-c54x-s3j-debug`; full
  sanitizer CMake/CTest 440/440 (quality.python
  excluded, green separately); `make test` exit 0
  with observer artifact OK; focused ASan/UBSan
  `mpya_dst` tests rc=0 with `detect_leaks=0`;
  Python `unittest discover` OK (272); evidence
  (363/575/459), inventory (72/205), source-boundary,
  knob, manifest, and lookup gates pass. No knob was
  added or changed. README snapshot now reads
  P3-S3j complete. A mid-slice `/tmp` disk-quota
  failure from accumulated scratch builds was
  resolved by removing this session's superseded
  `/tmp/ti-c54x-*` configure dirs; no repo state was
  touched.
- Follow-up work: `MACR` Smem lock-in, `MPYU`,
  `SQUR` family, `MACA`/`MACD`/`MACP` family,
  AR6/AR7 dual-Y widening, MOD3 neighborhoods, dual
  replay classification; deferred exotic indirect
  modes, dual `#lk` words, and far/XPC/timing.

## 2026-09-17 — P3-S3k SQUR-Smem square slice (slice 19)

- Commit `d65369d` lands `FORM-T2-03-09` (TI SPRU172C
  `SQUR` syntax 1, opcode `0010 011D IAAA AAAA`,
  dual-sourced against the SPRU179C algebraic
  `B = square(@30)`) as one `SQUR_SMEM` catalog row
  at `0x2600` under mask `0xFE00` carrying 512
  D/I/addr7 encodings through the new
  `DESTINATION_INDIRECT_ADDR7` layout (D destination
  plus indirect/addr7 with no round or select bits;
  reusing the shared round/select layout would
  mislabel fixed bit 9 as rounding) with `ELIGIBLE`
  repeatability (Table 2-28 lists no SQUR entry).
  Checked/trusted leaves stage direct page-0 and
  standard-indirect MOD0–MOD3 reads with read-phase
  T load and exotic-mode deferral; read-value STORE
  completion squares Smem into the D-selected
  accumulator through the multiply core with
  FRCT/OVM and no rounding. CPU retirement proves
  TI SQUR Example 1 (mem `000F`, FRCT 0 yield B `00
  0000 00E1` and T `000F`). New TI record
  `E-TI-C54X-SPRU172C-S4-SQUR-P4-160-4-161` plus
  activation, six owner-split requirements
  (CATALOG, DECODE, EFFECTS, CPU-RETIREMENT, LAYOUT,
  INVENTORY), and five test records
  (363/575/459→365/581/464); the PROJECT INVENTORY
  requirement rides the shared binding test and the
  binding uses the `DIRECT_PAGE0_STANDARD_INDIRECT_
  MOD0_MOD3` subset token shared with the other
  Smem rows.
- RED was observed on the stub (absent
  `TI_C54X_OPCODE_ID_SQUR_SMEM` identity plus
  absent layout). Two author-side trips, both caught
  by the slice's own tests before commit: the first
  mask `0xFE80` wrongly fixed the varying I bit and
  yielded 256 lookup entries instead of 512 (the
  generator expansion is the honest oracle here —
  corrected to `0xFE00` in the row, leaf, tests,
  and anchor); the decode invalid lane pinned
  `0x2800`, which is the live `MAC_SMEM` row, and
  was replaced with verified-unknown `0x2400`.
  `git diff --check` clean. New files peak at 238
  lines (`test_squr_smem_ops.c`); no split required.
  `operand_layout.c` grows by six lines with no
  split decision (well under 500/800).
- Neighbor pins move to 198 IDs, 197 rows/bindings
  (164 SUBSET plus 33 COMPLETE), 14,311 lookup
  entries, 73/205 (35.610%) with the `FORM-T2-03-09`
  contract set; strict stays 33/205 (16.098%). The
  0x26xx near-miss sweep is clean. A manifest-order
  trip (Make/CMake source lists must agree in set
  AND order) was caught by the manifest gate and
  fixed before commit.
- Validation is GREEN: full CMake CTest 446/446 on
  the working build; fresh Debug configure plus full
  CTest 446/446 in `/tmp/ti-c54x-s3k-debug`; full
  sanitizer CMake/CTest 445/445 (quality.python
  excluded, green separately); `make test` exit 0
  with observer artifact OK; focused ASan/UBSan
  `squr_smem` tests rc=0 with `detect_leaks=0`;
  Python `unittest discover` OK (272); evidence
  (365/581/464), inventory (73/205),
  source-boundary, knob, manifest, and lookup gates
  pass. No knob was added or changed. README
  snapshot now reads P3-S3k complete. Scratch-build
  hygiene from S3j holds: superseded
  `/tmp/ti-c54x-*` dirs are removed as they are
  replaced.
- Follow-up work: SQUR syntax 2 (`SQUR A,dst`,
  register-only, updates the `test_mpya_dst_decode`
  UNKNOWN lane), `SQURA`/`SQURS`, `MPYU`
  (unsigned core support, shares the S3k layout
  shape), `MACA`/`MACD`/`MACP` family, AR6/AR7
  dual-Y widening, MOD3 neighborhoods, dual replay
  classification; deferred exotic indirect modes,
  dual `#lk` words, and far/XPC/timing.

## 2026-09-17 — P3-S3l SQUR-Adst register-only slice (slice 20)

- Commit `99b579d` lands `FORM-T2-03-10` (TI SPRU172C
  `SQUR` syntax 2, opcode `1111 010D 1000 1101`,
  dual-sourced against the SPRU179C algebraic
  `B = square(hi(A))`) as one `SQUR_ADST` catalog
  row at `0xF48D` under mask `0xFEFF` carrying both
  D encodings through the reused `MAX_MIN_ACC`
  single-destination-bit layout with `ELIGIBLE`
  repeatability (Table 2-28 lists no SQUR entry).
  Checked/trusted leaves complete `(A(32-16))`
  squared into dst through the multiply core with
  STORE select and no rounding (no memory effect, no
  T update; T is never read, so the leaf stays green
  with T unknown); C/TC preserved, `OVdst` follows
  the core. CPU retirement proves TI SQUR Example 2
  (`A 00 000F 0000`, FRCT 1 yield `B 00 0000 01C2`
  with A and T unchanged; the 15-squared-shifted
  vector was recomputed independently before
  pinning). New TI record
  `E-TI-C54X-SPRU172C-S4-SQUR-ADST-P4-162` plus
  activation, five owner-split requirements, and
  four test records (365/581/464→367/586/468); the
  PROJECT INVENTORY requirement rides the shared
  binding test with the `REGISTER_DST` subset
  token. No new layout, resource, effect, address,
  state, CPU, public-API, or manifest seam is
  added.
- RED was observed on the stub (absent
  `TI_C54X_OPCODE_ID_SQUR_ADST` identity). The
  recorded W57→W58 cross-slice update landed here:
  the `test_mpya_dst_decode.c` UNKNOWN lane that
  pinned `0xF48D`/`0xF58D` moved to verified-unknown
  `0xF49C`/`0xF59C` with an explanatory comment.
  `git diff --check` clean. New files peak at 233
  lines (`test_squr_adst_ops.c`); no split required.
- Neighbor pins move to 199 IDs, 198 rows/bindings
  (165 SUBSET plus 33 COMPLETE), 14,313 lookup
  entries, 74/205 (36.098%) with the `FORM-T2-03-10`
  contract set; strict stays 33/205 (16.098%). The
  0xF48D near-miss sweep is clean — the single
  masked row follows the S3j precedent.
- Validation is GREEN: full CMake CTest 450/450 on
  the working build; fresh Debug configure plus full
  CTest 450/450 in `/tmp/ti-c54x-s3l-debug`; full
  sanitizer CMake/CTest 449/449 (quality.python
  excluded, green separately); `make test` exit 0
  with observer artifact OK; focused ASan/UBSan
  `squr_adst` plus updated `mpya_dst_decode` tests
  rc=0 with `detect_leaks=0`; Python `unittest
  discover` OK (272); evidence (367/586/468),
  inventory (74/205), source-boundary, knob,
  manifest, and lookup gates pass. No knob was
  added or changed. README snapshot now reads
  P3-S3l complete. Scratch-build hygiene holds:
  superseded `/tmp/ti-c54x-*` dirs are removed as
  they are replaced.
- Follow-up work: `SQURA`/`SQURS`, `MPYU`
  (unsigned core support, shares the S3k layout
  shape), `MACA`/`MACD`/`MACP` family, AR6/AR7
  dual-Y widening, MOD3 neighborhoods, dual replay
  classification; deferred exotic indirect modes,
  dual `#lk` words, and far/XPC/timing.

## 2026-09-17 — P3-S3m SQURA/SQURS accumulate-squares slice (slice 21)

- Commit `ae8945f` lands `FORM-T2-04-21`/`FORM-T2-04-22`
  (TI SPRU172C `SQURA` p. 4-163, `SQURS` p. 4-164)
  as two catalog rows sharing one leaf with
  ADD/SUBTRACT select: `SQURA_SMEM` at `0x3800`
  and `SQURS_SMEM` at `0x3A00`, each under mask
  `0xFE00` with 512 S/I/addr7 encodings through
  the reused S3k `DESTINATION_INDIRECT_ADDR7`
  layout, `ELIGIBLE` repeatability (Table 2-28
  lists no SQURA/SQURS entry), and
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`
  resources. Checked/trusted leaves stage the
  Smem read with read-phase T load, then complete
  `(src)` plus/minus `(Smem)x(Smem)` into the
  S-selected accumulator through the multiply
  core with no rounding; C/TC preserved, `OVdst`
  follows the core. CPU retirement proves both
  Example 1 vectors (recomputed independently
  before pinning). New TI records
  `E-TI-C54X-SPRU172C-S4-SQURA-P4-163` and
  `E-TI-C54X-SPRU172C-S4-SQURS-P4-164` plus
  activation, six owner-split requirements, and
  four test records (367/586/468→370/592/472);
  the PROJECT INVENTORY requirements ride the
  shared binding test with the `DESTINATION_`
  `INDIRECT_ADDR7` subset token. No new layout,
  resource, effect, address, state, CPU,
  public-API, or manifest seam is added.
- RED was observed on the stub (absent
  `TI_C54X_OPCODE_ID_SQURA_SMEM`/`SQURS_SMEM`
  identities). Diagnosis note: the mid-slice
  `squr_acc_ops` failure at `first.id == id` was
  a test-word typo (`0x3BAD`, SQURS family, in
  the exotic-mode case expecting `SQURA_SMEM`),
  not a decoder defect — a direct probe of the
  same library decoded `0x391E`→171,
  `0x3A09`→172, `0x3B82`→172 correctly; the
  word is now `0x39AD`. The recorded
  LD-T→SQURA cross-slice update landed here:
  the `test_ld_t_smem_metadata.c`/`test_ld_t_smem_decode.c`
  UNKNOWN lanes for the `0x0800` flip (`0x3890`)
  moved to the new `SQURA_SMEM` row with an
  explanatory comment.
  `git diff --check` clean. New files peak at 373
  lines (`op_squr_acc.c`); no split required.
- Neighbor pins move to 201 IDs, 200 rows/bindings
  (167 SUBSET plus 33 COMPLETE), 15,337 lookup
  entries, 76/205 (37.073%) with the
  `FORM-T2-04-21`/`FORM-T2-04-22` contract set;
  strict stays 33/205 (16.098%). The
  0x3800/0x3A00 near-miss sweeps are clean — each
  single masked row carries its 512 encodings.
- Validation is GREEN: full CMake CTest 454/454 on
  the working build; fresh Debug configure plus full
  CTest 454/454 in `/tmp/ti-c54x-s3m-debug`; full
  sanitizer CMake/CTest 454/454 in
  `/tmp/ti-c54x-s3m-asan` (ASan/UBSan); `make test`
  clean; focused new-slice binaries green; quality
  unittest 272 OK; evidence gate passes
  (370/592/472).
- Follow-up work: `MPYU` (unsigned core support,
  shares the S3k layout shape),
  `MACA`/`MACD`/`MACP` family, AR6/AR7 dual-Y
  widening, MOD3 neighborhoods, dual replay
  classification; deferred exotic indirect modes,
  dual `#lk` words, and far/XPC/timing.

## 2026-09-17 — P3-S3n MPYU unsigned-multiply slice (slice 22)

- Commit `94a0159` lands `FORM-T2-03-08` (TI
  SPRU172C `MPYU` p. 4-106, opcode `0010 010D
  IAAA AAAA`, dual-sourced against the TI
  SPRU307A functional-overview summary) as one
  `MPYU_SMEM` catalog row at `0x2400` under mask
  `0xFE00` carrying 512 D/I/addr7 encodings
  through the reused S3k
  `DESTINATION_INDIRECT_ADDR7` layout with
  `ELIGIBLE` repeatability (Table 2-28 lists no
  MPYU entry) and
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`
  resources. The shared multiply core gains an
  unsigned-operands select (zero-extended 16x16
  product through the single-sourced FRCT shift
  and OVM saturation path, keeping the positive
  direction for non-negative products); all
  eleven signed leaves pin the select false, and
  the core unit test gains unsigned lanes.
  Checked/trusted leaves stage direct page-0 and
  standard-indirect MOD0-MOD3 reads with
  exotic-mode deferral and no T load, then
  complete `uns(T)` x `uns(Smem)` into the
  D-selected accumulator with no rounding;
  C/TC preserved, `OVdst` follows the core. CPU
  retirement proves TI MPYU Example 1
  (`MPYU *AR0-, A`, `A 00 3F80 0000` with `T`
  `4000` unchanged; `0x4000` x `0xFE00`
  recomputed independently before pinning). New
  TI record `E-TI-C54X-SPRU172C-S4-MPYU-P4-106`
  plus activation, five owner-split
  requirements, and four test records
  (370/592/472→372/597/476); the PROJECT
  INVENTORY requirement rides the shared binding
  test with the `DIRECT_PAGE0_STANDARD_`
  `INDIRECT_MOD0_MOD3` subset token. No new
  layout, resource, effect, address, state, CPU,
  public-API, or manifest seam is added.
- RED was observed on the stub (absent
  `TI_C54X_OPCODE_ID_MPYU_SMEM` identity).
  Diagnosis notes: the mid-slice CPU-test
  failure (`requests[0].space` INVALID with the
  instruction retired) was a test-addressing
  mistake, not a leaf defect — dma7 `0x09`
  falls in the `0x00`-`0x1F` MMR window, so the
  read was served internally with no bus
  transaction; the word is now `0x2434`
  (dma7 `0x34`). The recorded SQUR→MPYU
  cross-slice update landed here: the
  `test_squr_smem_decode.c` UNKNOWN lanes for
  `0x2400`/`0x2480`/`0x25FF` moved to the new
  `MPYU_SMEM` row with an explanatory comment.
  The evidence gate needed two fixes: the
  binding test record's reverse link gained the
  new INVENTORY requirement, and the CPU
  requirement claim dropped the banned singular
  `vector` term. `git diff --check` clean. New
  files peak at 323 lines
  (`test_mpyu_smem_ops.c`); no split required.
- Neighbor pins move to 202 IDs, 201
  rows/bindings (168 SUBSET plus 33 COMPLETE),
  15,849 lookup entries, 77/205 (37.561%) with
  the `FORM-T2-03-08` contract set; strict
  stays 33/205 (16.098%). The `0x2400`
  neighborhood is fully bookended (MPY below,
  SQUR above) with no remaining UNKNOWN lane.
- Validation is GREEN: full CMake CTest 458/458
  on the working build; fresh Debug configure
  plus full CTest 458/458 in
  `/tmp/ti-c54x-s3n-debug`; full sanitizer
  CMake/CTest 458/458 in `/tmp/ti-c54x-s3n-asan`
  (ASan/UBSan); `make test` clean; focused
  new-slice binaries green; quality unittest
  272 OK; evidence gate passes (372/597/476);
  inventory gate reports 77/205 (37.561%).
- Follow-up work: `MACA`/`MACD`/`MACP` family,
  AR6/AR7 dual-Y widening, MOD3 neighborhoods,
  dual replay classification; deferred exotic
  indirect modes, dual `#lk` words, and
  far/XPC/timing.

## 2026-09-17 — P3-S3o MACA Smem accumulate-A slice (slice 23)

- Commit `2adba58` lands `FORM-T2-04-07` (TI
  SPRU172C `MACA[R]` p. 4-85, opcode `0011 01R1
  IAAA AAAA`) as one `MACA_SMEM` catalog row at
  `0x3500` under mask `0xFD00` carrying 512
  R/I/addr7 encodings through the new
  `ROUND_INDIRECT_ADDR7` layout with `ELIGIBLE`
  repeatability (Class 3A/3B, no Table 2-28
  entry) and
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`
  resources. The R bit carries the MACAR
  rounding variant on the shared row by the
  recorded S3c/S3f R-sharing convention, so
  `FORM-T2-04-09` stays `UNIMPLEMENTED`.
  Checked/trusted leaves stage direct page-0
  and standard-indirect MOD0-MOD3 reads with
  exotic-mode deferral and read-phase T load,
  then complete `(B)` plus `(Smem)` times
  `A(32-16)` into fixed B through the multiply
  core with ADD select and R rounding; C/TC
  preserved, `OVdst` follows the core for B.
  A-high is the multiplicand, so A and B must
  be known while T needs no prior knownness.
  CPU retirement proves TI MACA Example 1
  (`B 00 0626 0060` with `T 5678`, `AR5`
  post-increment; `0x1234` x `0x5678`
  recomputed independently before pinning) and
  the MACAR Example 3 rounding lane
  (`B 00 0626 0000`). New TI record
  `E-TI-C54X-SPRU172C-S4-MACA-P4-85` plus
  activation, six owner-split requirements
  (including the layout requirement), and five
  test records (372/597/476→374/603/481); the
  PROJECT INVENTORY requirement rides the
  shared binding test with the
  `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`
  subset token. One new layout; no new
  resource, effect, address, state, CPU,
  public-API, or manifest seam beyond the leaf.
- RED was observed on the stub in two stages:
  the layout test failed on the absent
  `ROUND_INDIRECT_ADDR7` token, then the slice
  tests failed on the absent `MACA_SMEM`
  identity. A mid-slice value bug put the row
  at `0x3400` (bit 8 clear) against the TI
  `0011 01R1` shape (bit 8 fixed 1); the row
  is now `0x3500` and every test word moved to
  the `0x35`/`0x37` high bytes. The recorded
  cross-slice updates landed here: the
  `test_squr_acc_decode.c` UNKNOWN lane for
  `0x37FF` and the `test_stm_decode.c` UNKNOWN
  lane for the `0x4000` flip (`0x3700`) moved
  to the new `MACA_SMEM` row with explanatory
  comments. `git diff --check` clean. New
  files peak at 294 lines
  (`test_maca_smem_ops.c`); no split required.
- Neighbor pins move to 203 IDs, 202
  rows/bindings (169 SUBSET plus 33 COMPLETE),
  16,361 lookup entries, 78/205 (38.049%)
  with the `FORM-T2-04-07` contract set;
  strict stays 33/205 (16.098%).
- Validation is GREEN: full CMake CTest 463/463
  on the working build; fresh Debug configure
  plus full CTest 463/463 in
  `/tmp/ti-c54x-s3o-debug`; full sanitizer
  CMake/CTest 463/463 in `/tmp/ti-c54x-s3o-asan`
  (ASan/UBSan); `make test` clean; focused
  new-slice binaries green; quality unittest
  272 OK; evidence gate passes (374/603/481).
- Follow-up work: `MACA` T,src syntax-2 row,
  `MACD`/`MACP` family (program-memory
  operands plus the delay seam), AR6/AR7
  dual-Y widening, MOD3 neighborhoods, dual
  replay classification; deferred exotic
  indirect modes, dual `#lk` words, and
  far/XPC/timing.

## 2026-09-17 — P3-S3p MACA T,src register-only slice (slice 24)

- Commit `dc47103` lands `FORM-T2-04-08` (TI
  SPRU172C `MACA[R]` p. 4-85, opcode `1111 01SD
  1000 100R`) as one `MACA_T` catalog row at
  `0xF488` under mask `0xFCFE` carrying 8 S/D/R
  encodings through the new
  `SOURCE_DESTINATION_ROUND` layout with
  `ELIGIBLE` repeatability (Class 3A/3B, no
  Table 2-28 entry) and
  `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`
  resources. The R bit carries the MACAR T,src
  rounding variant on the shared row by the
  recorded S3c/S3f R-sharing convention, so
  `FORM-T2-04-10` stays `UNIMPLEMENTED`.
  Checked/trusted register-only leaves
  complete `(src)` plus `(T)` times `A(32-16)`
  into the D-selected accumulator through the
  multiply core with ADD select and R rounding;
  T, A, and the unselected accumulator are
  preserved, C/TC ride through unchanged, and
  `OVdst` follows the core for dst (which is
  src when dst is not specified). T, A, and
  src must be known. CPU retirement proves TI
  MACA Example 2 (`B 00 009D 4BA0` with `T`
  unchanged; `0x0444` x `0x1234` = `0x4DA5D0`
  recomputed independently before pinning) and
  the MACAR Example 4 rounding lane
  (`B 00 009D 0000`) with endpoint silence. No
  new TI record was needed (the existing TI
  MACA section record spans both syntaxes, per
  the recorded S3f convention) — only the
  activation, six owner-split requirements
  (including the layout requirement), and five
  test records (374/603/481→375/609/486); the
  PROJECT INVENTORY requirement rides the
  shared binding test with the `REGISTER_DST`
  subset token alongside the register-only
  siblings. One new layout; no new resource,
  effect, address, state, CPU, public-API, or
  manifest seam beyond the leaf.
- RED was observed on the stub in two stages:
  the layout test failed on the absent
  `SOURCE_DESTINATION_ROUND` token, then the
  slice tests failed on the absent `MACA_T`
  identity. Two mid-slice test-authoring slips
  were corrected against the hardware facts:
  `0xF487` belongs to catalog row 11 (the
  invalid lane is now `0xF480`), and the
  oracle product `0x0444` x `0x1234` is
  `0x4DA5D0` (the src-A lane is now
  `0x12CF4BA0`). `git diff --check` clean. New
  files peak at 230 lines
  (`test_maca_t_ops.c`); no split required.
- Neighbor pins move to 204 IDs, 203
  rows/bindings (170 SUBSET plus 33 COMPLETE),
  16,369 lookup entries, 79/205 (38.537%)
  with the `FORM-T2-04-08` contract set;
  strict stays 33/205 (16.098%). The `0xF488`
  neighborhood borrows no other row.
- Validation is GREEN: full CMake CTest 468/468
  on the working build; fresh Debug configure
  plus full CTest 468/468 in
  `/tmp/ti-c54x-s3p-debug`; full sanitizer
  CMake/CTest 468/468 in `/tmp/ti-c54x-s3p-asan`
  (ASan/UBSan); `make test` clean; focused
  new-slice binaries green; quality unittest
  272 OK; evidence gate passes (375/609/486).
- Follow-up work: `MACD`/`MACP` family
  (program-memory operands plus the delay
  seam), AR6/AR7 dual-Y widening, MOD3
  neighborhoods, dual replay classification;
  deferred exotic indirect modes, dual `#lk`
  words, and far/XPC/timing.

## 2026-09-17 — P3-S3q MACP Smem,Pmad program-coefficient slice (slice 25)

- Commit `45290af` lands `FORM-T2-04-12` (TI
  SPRU172C `MACP` p. 4-89, word 0
  `0111_100S_IAAA_AAAA` plus verbatim `pmad`
  second word) as one `MACP_SMEM` catalog row
  at `0x7800` under mask `0xFE00` carrying 512
  S/I/addr7 encodings through the new
  `DESTINATION_INDIRECT_ADDR7_PMAD16` layout
  with `UNRESOLVED` repeatability (two-word STM
  precedent) and
  `ACCUMULATORS_STATUS_FRCT_OVM` resources
  (MPYA_SMEM precedent: T written, not read; no
  R rounding bit). Checked and trusted leaves
  stage direct page-0 and standard-indirect
  MOD0-MOD2 Smem DATA reads paired with an
  absolute PROGRAM coefficient read with
  exotic-mode deferral and read-phase Smem T
  load; pair completion runs `(dst)` plus
  `(Smem)`x`(pmad)` into the S-selected
  accumulator with ADD select and no rounding.
  The new DATA+PROGRAM pair seam (address kind
  14, resolution delegating the Smem leg to the
  generic single-leg resolver, X-DATA/Y-PROGRAM
  reducer, root with commit and candidate
  validation reusing the snapshot carrier with
  a zeroed PROGRAM commit) issues the DATA read
  then the PROGRAM read with X-side T load.
  Resolution lives in the pair snapshot owner
  so the shared root transaction stays at 799
  lines under the 800 gate (one line of
  margin; the next seam touching that file
  takes the explicit split decision). No new
  knob, resource, effect shape beyond the pair
  carrier, or public API.
- RED was observed on real contracts throughout:
  the pair root test failed on the absent kind
  in the generic shape check (fixed by the
  pair arm), the CPU test failed on the same
  root cause, and the consolidation pass
  exposed a skipped generic-resolve arm plus a
  bypassed PREPARED phase (fixed by the
  kind-guard and in-helper phase assignment;
  the /tmp probe proved DATA-then-PROGRAM
  order with AR update before the tests
  re-greened). Two cross-slice near-miss lanes
  moved: BC/CC bit-15 flips now pin the
  `MACP_SMEM` sibling. `git diff --check`
  clean. New files peak at 763 lines (pair
  root test); no split required.
- Neighbor pins move to 205 IDs, 204
  rows/bindings (171 SUBSET plus 33 COMPLETE),
  16,881 lookup entries, 80/205 (39.024%)
  with the `FORM-T2-04-12` contract set;
  strict stays 33/205 (16.098%). The `0x7800`
  neighborhood borrows no other row (ST/STM
  live at `0x76xx`-`0x77xx`).
- Validation is GREEN: full CMake CTest
  475/475 on the working build; fresh Debug
  configure plus full CTest 475/475;
  full sanitizer CMake/CTest 475/475
  (ASan/UBSan); `make test` clean; focused
  new-slice binaries green; quality unittest
  272 OK; evidence gate passes (377/617/493);
  inventory provisional 80/205 with strict
  33/205.
- Follow-up work: `MACD` Smem,Pmad (delay
  sidecar: Smem+1 write after pair completion;
  reuses the S3q seam and layout),
  `MACR`/`MACSU`/`MPYR`/`SQDST` leaves, S4 XC,
  S1 STM/MMR, S6 PORTR/PORTW, AR6/AR7 dual-Y,
  MOD3, dual replay, RPTB/RPTZ/RPTBD,
  far/XPC/timing.

## 2026-09-17 — P3-S3r MACD Smem,Pmad delay-write slice (slice 26)

- Commit `04e8cbe` lands `FORM-T2-04-11` (TI
  SPRU172C `MACD` p. 4-87, word 0
  `0111_101S_IAAA_AAAA` plus verbatim `pmad`
  second word) as one `MACD_SMEM` catalog row
  at `0x7A00` under mask `0xFE00` carrying 512
  S/I/addr7 encodings through the shared S3q
  `DESTINATION_INDIRECT_ADDR7_PMAD16` layout
  (no new layout) with `UNRESOLVED`
  repeatability (two-word STM precedent) and
  `ACCUMULATORS_STATUS_FRCT_OVM` resources
  (MPYA_SMEM precedent: T written, not read; no
  R rounding bit). Checked and trusted leaves
  stage direct page-0 and standard-indirect
  MOD0-MOD2 Smem DATA reads paired with an
  absolute PROGRAM coefficient read with
  exotic-mode deferral and read-phase Smem T
  load; pair completion runs `(dst)` plus
  `(Smem)`x`(pmad)` into the S-selected
  accumulator with ADD select and no rounding.
  The delay sidecar rides the new
  `write_smem_delay` `MEMORY` carrier flag
  (second boolean in the existing padding; the
  48-byte effect ABI is unchanged and only the
  DATA+PROGRAM pair kind may set it, enforced
  by carrier validation): after both reads
  complete, the pair root issues a third-ID
  DATA WRITE of the Smem word to `Smem+1` with
  16-bit wraparound through the reused child
  transaction while the pair wrapper holds
  staged values in `COMPLETE`, before the Smem
  post-access commit and PC publish. WAIT
  resumes only the delay WRITE under the third
  ID, a delay FAULT or cancel publishes
  nothing, and cancel/reset route by
  pair-versus-delay child state with no new
  root phase and no CPU resume-path change.
  The delay stage lives in the pair-root owner
  (`memory_instruction_data_program.c` at 744
  lines) so the shared root transaction stays
  at 798 lines under the 800 gate. No new
  knob, resource, layout, public API, or
  observer path; dispatch bindings (checked
  and trusted) ride the catalog `.def` X-macro.
- RED was observed on real contracts throughout:
  the CPU test failed on a nonzero
  `read_value_word` in the fake WRITE
  observation (bus contract requires zero;
  fixed in the fake, production code untouched),
  the delay test failed on a mid-flight
  accumulator read of reset-unknown state
  (replaced with PC/AR non-publication pins),
  and the wraparound case failed prepare on
  MOD2 post-increment from `0xFFFF` (address
  planner rejects the wrap; the case now uses
  MOD0 `*AR5`, which is the reachable
  wraparound shape). A flag-off isolation run
  proved the leaf and commit retire cleanly
  without the sidecar. Three cross-slice
  near-miss lanes moved: the MACP invalid lane
  `0x7A00` to `0x7C00` and the BCD/CCD bit-15
  flips now pin the `MACD_SMEM` sibling.
  `git diff --check` clean. New files peak at
  324 lines (ops test); no split required.
- Neighbor pins move to 206 IDs, 205
  rows/bindings (172 SUBSET plus 33 COMPLETE),
  17,393 lookup entries, 81/205 (39.512%)
  with the `FORM-T2-04-11` contract set;
  strict stays 33/205 (16.098%). The `0x7A00`
  neighborhood borrows no other row. Evidence
  stands at 379 records, 623 requirements, 498
  tests (498 `PASSING`, 0 `PLANNED`), matching
  the S3r anchor projections exactly.
- Validation is GREEN: full CMake CTest
  480/480 on the working Debug build; full
  sanitizer CMake/CTest 480/480 (ASan/UBSan);
  `make test` clean (all binaries plus knobs,
  boundaries, manifests, evidence, inventory,
  observer gates); contract unittest discovery
  OK; evidence gate passes (379/623/498);
  inventory provisional 81/205 with strict
  33/205.
- Follow-up work: `MACSU`/`MACR`/`MPYR`/`SQDST`
  leaves, S4 XC, S1 STM/MMR, S6 PORTR/PORTW,
  AR6/AR7 dual-Y, MOD3, dual replay,
  RPTB/RPTZ/RPTBD, far/XPC/timing.
## 2026-09-17 — P3-S3s MACSU Xmem,Ymem dual slice (slice 27)

- Commit `cf3b5c5` lands `FORM-T2-04-13` (TI
  SPRU172C `MACSU` p. 4-91, word 0
  `1010_011S_XXXX_YYYY`) as nine
  `MACSU_DUAL_Mx_My` catalog rows at `0xA800`
  plus X/Y mode bits under mask `0xFECC`
  carrying 32 S/Xar/Yar encodings each (288
  total) through the shared
  `DUAL_DESTINATION_XAR_YAR` layout (no new
  layout, no trusted-capacity pressure) with
  `UNRESOLVED` repeatability and
  `ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM`
  resources. Checked and trusted leaves stage
  dual-indirect MOD0-MOD2 reads with
  read-phase X-side T load, and pair
  completion runs `(src)` plus
  `unsigned-(Xmem)` times `signed-(Ymem)`
  into the S-selected accumulator (S selects
  the shared src/dst) through the multiply
  core with ADD select, FRCT/OVM handling,
  and no rounding. The mixed-sign product
  rides the new `unsigned_t_only`
  multiply-core select (T unsigned,
  multiplicand signed; both-unsigned keeps
  precedence for `MPYU`; all sixteen signed
  leaves set it false explicitly; FRCT shift,
  accumulate, saturation, and overflow
  disposition unchanged). CPU retirement
  proves the recomputed oracle vector
  (`A 00 1000 0000` with `T 4000`, `AR4 0101`,
  `AR5 0201`), the FRCT lane
  (`A 00 2000 0000`), S selection, and the
  mixed-sign lane (`A FD E26C 000`... pinned
  as `0xFFDE26C000`, distinct from both
  uniform-signedness readings). The LD_K
  bit-14 near-miss lane moves to the new row
  as the recorded cross-slice update (the
  first pin attempt targeted the wrong flip
  `0x8000`; the failing mask was `0x4000`
  landing on `0xA800`). `git diff --check`
  clean. New files peak at 424 lines (ops
  test); no split required. No new knob,
  layout, public API, observer path, or root
  phase.
- RED was observed on the real contracts: the
  core test failed to compile before the
  `unsigned_t_only` field existed; the LD_K
  near-miss failure exposed the `0xA800`
  neighborhood claim. The static row-overlap
  sweep over the `.def` reports zero overlaps
  for the nine new rows.
- Neighbor pins move to 215 IDs, 214
  rows/bindings (181 SUBSET plus 33
  COMPLETE), 17,681 lookup entries, 82/205
  (40.000%) with the `FORM-T2-04-13`
  contract set; strict stays 33/205
  (16.098%). Evidence stands at 381 records,
  627 requirements, 500 tests (500 `PASSING`,
  0 `PLANNED`), matching the S3s anchor
  projections exactly.
- Validation is GREEN: full CMake CTest
  482/482 on the working Debug build; full
  sanitizer CMake/CTest 482/482 (ASan/UBSan);
  `make test` clean (all binaries plus knobs,
  boundaries, manifests, evidence, inventory,
  observer gates); contract unittest discovery
  OK; evidence gate passes (381/627/500);
  inventory provisional 82/205 with strict
  33/205.
- Follow-up work: `MACR`/`MPYR`/`SQDST`
  leaves, S4 XC, S1 STM/MMR, S6 PORTR/PORTW,
  AR6/AR7 dual-Y, MOD3, dual replay,
  RPTB/RPTZ/RPTBD, far/XPC/timing.

### Correction — S3s ledger mixed-sign lane formatting
- The S3s entry above garbles the mixed-sign accumulator rendering as
  ``A FD E26C 000``; the pinned value is `0xFFDE26C000` (40-bit
  two's-complement of `-567885824`), as the same entry states and the
  `ti_c54x.macsu_dual_ops` / `ti_c54x.cpu_macsu_dual` tests pin.
## 2026-09-17 — P3-S3t MACR Xmem,Ymem dual lock-in slice (slice 28)

- Commit `cb46eb1` closes the census MACR dual
  syntaxes (`FORM-T2-04-05`/`FORM-T2-04-06`,
  TI SPRU172C `MAC[R]` syntax 2, opcode
  `1011_0RSD_XXXX_YYYY`, pp. 4-82–4-84)
  through the shared `MAC_DUAL` rows with
  R=1: no catalog, inventory, production,
  layout, or pin change was needed. The R=1
  rounding path locks end to end on the
  maintained suites — the `0xB532` leaf
  completion (`(0x5678` shifted by FRCT`) x
  `0x1234` + 4 rounded to `0x0C4C0000` into
  B), the `0xB5BA` CPU retirement with MOD2
  post-increments, and the new trusted R=1
  equivalence lane through the trusted MAC
  dual entry point. The two R-variant forms
  stay `UNIMPLEMENTED` with empty symbol
  lists by the recorded S3f R-sharing
  convention (shared rows cannot
  double-bind). README remaining-work wording
  drops `MACR`. `git diff --check` clean.
- The slice is lock-in, not RED/GREEN: the
  checked-path vectors predate it (S3g) and
  pass unchanged; the only new executable
  content is the trusted rounding lane.
- Evidence stands at 382 records, 629
  requirements, 502 tests (502 `PASSING`,
  0 `PLANNED`), matching the S3t anchor
  projections exactly.
- Validation is GREEN: full CMake CTest
  482/482 on the working Debug build;
  sanitizer MAC dual suites clean; `make test`
  clean; evidence gate passes (382/629/502).
- Follow-up work: `MPYR` (closed at Smem;
  dual has no R bit)/`SQDST` leaves, S4 XC,
  S1 STM/MMR, S6 PORTR/PORTW, AR6/AR7 dual-Y,
  MOD3, dual replay, RPTB/RPTZ/RPTBD,
  far/XPC/timing.

## 2026-09-17 — P3-S3u MASR Smem and dual lock-in slice (slice 29)

- Commit `95e4ab3` closes the census MASR
  Smem and dual syntaxes
  (`FORM-T2-04-15`/`FORM-T2-04-17`, TI
  SPRU172C `MAS[R]` Examples 3/4) through
  the shared `MAS_SMEM`/`MAS_DUAL` rows
  with R=1: no catalog, inventory,
  production, layout, or pin change was
  needed. The R=1 rounding path locks end
  to end on the maintained suites — the
  `0x2E95` leaf completion (round(A -
  T*Smem) to `0xFFFFB70000` into A) and
  the `0x2E34` CPU retirement already pin
  the checked Smem path; the `0xBD32` leaf
  completion (rounded `0xFFF3B40000` into
  B) and the `0xBDBA` CPU retirement
  already pin the checked dual path. Two
  new executable proofs: trusted R=1
  completion through
  `ti_c54x_trusted_op_mas_smem` on
  `0x2E95`, and trusted R=1 rounding
  equivalence through
  `ti_c54x_trusted_op_mas_dual_m0_m0` on
  `0xBD32`. The two R-variant forms stay
  `UNIMPLEMENTED` with empty symbol lists
  by the recorded R-sharing convention
  (shared rows cannot double-bind). README
  remaining-work wording drops `MASR`.
  `git diff --check` clean.
- The slice is lock-in, not RED/GREEN: the
  checked-path vectors predate it and pass
  unchanged; the only new executable
  content is the two trusted rounding
  lanes.
- Evidence stands at 383 records, 631
  requirements, 504 tests (504 `PASSING`,
  0 `PLANNED`), matching the S3u anchor
  projections exactly.
- Validation is GREEN: full CMake CTest
  482/482 on the working Debug build;
  sanitizer MAS suites clean; `make test`
  clean; evidence gate passes
  (383/631/504); contract quality suites
  pass.
- Sanitizer method note: a flagged
  `make TI_C54X_ENABLE_SANITIZERS=1 test`
  over a warm `build-fast/` tree is a
  no-op — Make does not rebuild stale
  objects on a flag change, and the
  resulting binaries carry no sanitizer
  symbols. S3u ran genuinely instrumented
  suites by `rm -rf build-fast`, rebuilding
  the two MAS targets with the flag set
  (73 asan/ubsan symbols present), and
  executing both binaries directly
  (both exit 0). Earlier "sanitizer suites
  clean" claims should be re-read with
  this caveat; re-verify with a clean
  flagged rebuild when a slice's claim
  matters.
- Follow-up work: `MACAR`/`MASA` R-variant
  lock-ins, `MPYR` (closed at Smem; dual
  has no R bit)/`SQDST` leaves, S4 XC, S1
  STM/MMR, S6 PORTR/PORTW, AR6/AR7 dual-Y,
  MOD3, dual replay, RPTB/RPTZ/RPTBD,
  far/XPC/timing.

## 2026-09-17 — P3-S3v MACAR Smem lock-in slice (slice 30)

- Commit `53b0c0c` closes the census MACAR
  Smem syntaxes (`FORM-T2-04-09`/
  `FORM-T2-04-10`, TI SPRU172C `MACA[R]`
  Example 3) through the shared
  `MACA_SMEM` row with R=1: no catalog,
  inventory, production, layout, or pin
  change was needed. The `0x3734` leaf
  completion (Example 1 product rounded to
  `0x6260000` into B) already pins the
  checked path. Two new executable proofs:
  CPU retirement of the `0x3734` direct
  word (same pre-state as the Example 1
  direct lane, B rounded to `0x6260000`
  with the read-phase T load), and trusted
  R=1 completion through
  `ti_c54x_trusted_op_maca_smem` on
  `0x3734`. The two R-variant forms stay
  `UNIMPLEMENTED` with empty symbol lists
  by the recorded R-sharing convention
  (shared rows cannot double-bind). README
  remaining-work wording drops `MACAR`.
  `git diff --check` clean.
- Unlike the pure lock-ins, S3v carries
  real new retirement content: no CPU R=1
  lane existed before this slice.
- Evidence stands at 384 records, 633
  requirements, 506 tests (506 `PASSING`,
  0 `PLANNED`), matching the S3v anchor
  projections exactly.
- Validation is GREEN: full CMake CTest
  482/482 on the working Debug build;
  sanitizer MACA pair clean (clean
  flagged rebuild, direct runs exit 0);
  `make test` clean on a fresh unflagged
  `build-fast/` rebuild (exit 0); evidence
  gate passes (384/633/506); contract
  quality suites pass.
- Build-hygiene note: the mid-slice
  flagged partial rebuild left an
  ASan-instrumented `libti_c54x.a` in
  `build-fast/`, which broke the next
  unflagged incremental link
  (`__asan_init` undefined in
  `test_cpu_api`). Resolved by
  `rm -rf build-fast` plus a full
  unflagged `make test`. Rule: never mix
  flag configurations in one `build-fast/`
  tree — a flag change always starts with
  `rm -rf build-fast`.
- Follow-up work: `MASA` R-variant
  lock-in, `MPYR` (closed at Smem; dual
  has no R bit)/`SQDST` leaves, S4 XC, S1
  STM/MMR, S6 PORTR/PORTW, AR6/AR7 dual-Y,
  MOD3, dual replay, RPTB/RPTZ/RPTBD,
  far/XPC/timing.

## 2026-09-17 — P3-S3w MASA Smem and T slice (slice 31)

- Commit `6f56f4e` closes the census MASA
  syntaxes (`FORM-T2-04-18`/
  `FORM-T2-04-19`, TI SPRU179C `MASA`
  pp. 4-99–4-100) with two new catalog
  rows: `MASA_SMEM` at `0x3300` under mask
  `0xFF00` through the shared
  `INDIRECT_ADDR7` layout, and `MASA_T` at
  `0xF48A` under mask `0xFCFE` through the
  shared `SOURCE_DESTINATION_ROUND`
  layout (S bit 9, D bit 8, R bit 0; R=1
  carries the future MASAR T variant;
  low bytes `0x8A`/`0x8B` stay disjoint
  from `MACA_T`). Both handlers reuse the
  multiply core with the SUBTRACT select
  and the A-high multiplicand, FRCT/OVM
  handling, and C/TC preservation. Both
  rows are `UNRESOLVED` repeatability
  with no Table 2-28 MASA fact on record
  (P2-W48C precedent). The TI vectors pin
  end to end: Example 1 Smem (`0x5678`
  read, B `00 0002 0000` to `FF F9DB
  FFA0` with T load) at the leaf, through
  the trusted entry, and on the CPU
  (direct plus MOD2 indirect); Example 2
  T (T `0x0444`, FRCT=1, B to `FF FF66
  B460`) at the leaf, through the trusted
  entry, and on the CPU with endpoint
  silence; Example 3 rounding (R=1, B to
  `FF FF67 0000`) at the leaf and on the
  CPU. The generated lookup gains 264
  entries with zero neighbor moves. The
  `MASAR` forms stay `UNIMPLEMENTED` with
  empty symbol lists. README
  remaining-work wording drops `MASA`.
  `git diff --check` clean.
- This is a full RED slice, not a
  lock-in: catalog rows, leaves, CPU
  lanes, and bindings are all new. The
  census pins move to 217 IDs, 216
  rows/bindings (183 SUBSET plus 33
  COMPLETE), 17,945 lookup entries,
  84/205 (40.976%); strict stays 33/205.
  Absorbed near-miss lanes move to the
  new rows as recorded cross-slice
  updates (`LD_ASM` bit-8 to `MASA_SMEM`,
  `LD_SRC_ASM` bit-3 and `RETF` bit-4 to
  `MASA_T`; `MACA_T` invalid pin moves to
  `0xF48F`).
- Evidence stands at 386 records, 637
  requirements, 510 tests (510 `PASSING`,
  0 `PLANNED`), matching the S3w anchor
  projections exactly. The TI source is
  registered under the pre-existing
  `SRC-TI-SPRU179C` registry with its own
  printed pages (SPRU179C pagination
  differs from SPRU172C by ~2 pages, so
  no SPRU172C page claim is made).
- Validation is GREEN: full CMake CTest
  486/486 on the working Debug build;
  sanitizer MASA quartet clean (clean
  flagged rebuild, 73 asan/ubsan symbols,
  direct runs exit 0, `build-fast/`
  removed after); `make test` clean on a
  fresh unflagged rebuild (exit 0);
  evidence, inventory, and lookup gates
  pass; contract quality suites pass.
- Follow-up work: `MASAR` T lock-in (R=1
  through `MASA_T`) plus the `MASAR`
  Smem question (syntax 1 carries no R
  bit), `MPYR`/`SQDST` leaves (SQDST
  needs a two-accumulator-write effect
  channel that does not exist yet), S4
  XC, S1 STM/MMR, S6 PORTR/PORTW, AR6/AR7
  dual-Y, MOD3, dual replay,
  RPTB/RPTZ/RPTBD, far/XPC/timing.

## 2026-09-17 — P3-S3x MASAR T lock-in slice (slice 32)

- Commit `da335f9` locks the census MASAR
  T syntax (TI SPRU179C `MASA` syntax 2
  with `rnd()`) through the shared
  `MASA_T` row with R=1: no catalog,
  inventory, production, layout, or pin
  change was needed. The `0xF78B` leaf
  completion (Example 2 product rounded
  to `0xFFFF670000` into B) and the
  `0xF78B` CPU retirement with endpoint
  silence already pin the checked path.
  One new executable proof: trusted R=1
  completion through
  `ti_c54x_trusted_op_masa_t` on
  `0xF78B`. The two MASAR forms stay
  `UNIMPLEMENTED` with empty symbol lists
  by the recorded R-sharing convention
  (shared rows cannot double-bind).
  README remaining-work wording drops
  MASAR T. `git diff --check` clean.
- Evidence stands at 387 records, 638
  requirements, 511 tests (511 `PASSING`,
  0 `PLANNED`), matching the S3x anchor
  projections exactly.
- Validation is GREEN: full CMake CTest
  486/486; sanitizer MASA_T lane clean
  (clean flagged rebuild, direct run
  exit 0, `build-fast/` removed after);
  evidence gate passes (387/638/511).
- Follow-up work: the `MASAR` Smem
  question (syntax 1 carries no R bit),
  `MPYR`/`SQDST` leaves (SQDST needs a
  two-accumulator-write effect channel
  that does not exist yet), S4 XC, S1
  STM/MMR, S6 PORTR/PORTW, AR6/AR7
  dual-Y, MOD3, dual replay,
  RPTB/RPTZ/RPTBD, far/XPC/timing.

## 2026-09-17 — P3-S3y MPYR Smem lock-in slice (slice 33)

- Commit `77c7278` locks the census MPYR
  Smem syntax (`FORM-T2-03-02`, TI
  SPRU179C `MPY` syntax 1 with `rnd()`,
  evidenced under the existing
  SPRU172C `MPY` pp. 4-101–4-103 source
  record) through the shared `MPY_SMEM`
  row with R=1: no catalog, inventory,
  production, layout, or pin change was
  needed. The `0x2200` leaf completion
  (T `0x1234` times Smem `0x5678`
  rounded to `0x6260000` into A per TI
  MPY Example 4) and the `0x2320` CPU
  retirement already pin the checked
  path. One new executable proof:
  trusted R=1 completion through
  `ti_c54x_trusted_op_mpy_smem` on
  `0x2200`. The MPYR form stays
  `UNIMPLEMENTED` with an empty symbol
  list by the recorded R-sharing
  convention (shared rows cannot
  double-bind). README remaining-work
  wording drops MPYR. `git diff --check`
  clean.
- Evidence stands at 388 records, 639
  requirements, 512 tests (512 `PASSING`,
  0 `PLANNED`), matching the S3y anchor
  projections exactly.
- Validation is GREEN: full CMake CTest
  486/486; sanitizer MPY lane clean
  (clean flagged rebuild, direct run
  exit 0, `build-fast/` removed after);
  evidence gate passes (388/639/512).
- Follow-up work: the `MASAR` Smem
  question (syntax 1 carries no R bit),
  `SQDST` (needs a two-accumulator-write
  effect channel that does not exist
  yet), S4 XC, S1 STM/MMR, S6
  PORTR/PORTW, AR6/AR7 dual-Y, MOD3, dual
  replay, RPTB/RPTZ/RPTBD,
  far/XPC/timing.

## 2026-09-17 — Correction: MASAR form numbering in S3w/S3x assertions

- The S3w activation assertion
  (`E-PROJECT-P3-S3W-MASA-ACTIVATION`)
  and the S3x activation assertion
  (`E-PROJECT-P3-S3X-MASAR-ACTIVATION`)
  name `FORM-T2-04-20`/`FORM-T2-04-21`
  as the `(MASAR)` forms. Only
  `FORM-T2-04-20` is `MASAR`;
  `FORM-T2-04-21` is `SQURA` (already
  `SUBSET`). The census has a single
  `MASAR` form because TI `MASA`
  syntax 1 carries no R bit, so the only
  `MASAR` encoding is syntax-2 R=1,
  locked through the shared `MASA_T` row
  in S3x. The `UNIMPLEMENTED`/empty-list
  status both assertions record remains
  true of `FORM-T2-04-20`; the
  parenthetical just over-names 04-21.
  Evidence records are immutable, so
  this correction stands in the ledger
  instead of an edit. Future slice
  follow-up lines should name only
  `FORM-T2-04-20` for `MASAR`.

## 2026-09-17 — P3-S3z ADD Smem slice (slice 34)

- Commit `53dff1c` lands `FORM-T2-01-01`/`02`/`03`
  (TI SPRU179C `ADD` pp. 4-4–4-5) as
  three new rows: `ADD_SMEM` at `0x0000`
  under mask `0xFE00`, `ADD_SMEM_TS` at
  `0x0400` under mask `0xFE00`, and
  `ADD_SMEM_SH16` at `0x3C00` under mask
  `0xFC00` through the shared arithmetic
  core with SXM extension, OVM
  saturation with sticky OVdst, and
  per-syntax C rules. Syntaxes 1–2 share
  the new `SELECT_INDIRECT_ADDR7`
  layout; syntax 3 takes the new
  `SOURCE_DESTINATION_INDIRECT_ADDR7`
  layout. All three rows are `UNRESOLVED`
  repeatability with no Table 2-28
  ADD-Smem fact on record. Lookup gains
  2048 entries (17945 to 19993) with
  zero neighbor moves; census moves to
  220 IDs, 219 bindings, 87/205 families
  (42.439%), 33 strict-complete, 186
  SUBSET. Derived vectors pin end to end
  at the leaf, through the trusted
  entries, and on the CPU. README
  remaining-work wording lands ADD Smem.
  `git diff --check` clean.
- Evidence stands at 390 records, 644
  requirements, 516 tests (516 `PASSING`,
  0 `PLANNED`). The S3z anchor projected
  390/641/514 with two requirements and
  two test records: the evidence count
  matches (the TI `ADD` source record
  pre-existed under `SRC-TI-SPRU179C`,
  so no new TI source record was
  needed), and the extra three
  requirements plus two tests are the two
  layout rows and the inventory binding
  row the S3D/S3K convention requires
  for new layouts and new bindings. No
  knob change.
- Decisions: mid-`.def` placement keeps
  the `ADD` family together and shifts
  the catalog-order contract positions
  18–23 by three with new row
  assertions; the scope gate forbids
  `COMMON` requirements from citing the
  `PROJECT_CONTRACT_ONLY` activation, so
  the two `COMMON` requirements cite the
  TI formula record only. Absorptions:
  `0x0000`/`0x3C00` leave older
  `UNKNOWN` pins, `0x4480`^`0x4000`
  resolves to `ADD_SMEM_TS`, and the
  `0x0000` CPU unknown-probes move to
  `0x3000`.
- Validation is GREEN: full CMake CTest
  490/490 on the working Debug build;
  sanitizer ADD quartet clean (clean
  flagged rebuild, direct runs exit 0,
  `build-fast/` removed after); `make
  test` clean (exit 0); evidence,
  inventory, and lookup gates pass;
  contract quality suites pass.
- Follow-up work: `SQDST` two-write
  effect seam, S4 XC, S1 STM/MMR, S6
  PORTR/PORTW, AR6/AR7 dual-Y, MOD3, dual
  replay, RPTB/RPTZ/RPTBD,
  far/XPC/timing.

## 2026-09-17 — P3-S4 XC conditional-execution slice (slice 35)

- Commit `fed89f8` lands `FORM-T2-18-07`
  (TI SPRU172C `XC` pp. 4-198–4-200) as
  one new row: `XC` at `0xFD00` under
  mask `0xFD00` (N at bit 9, cond at
  7:0) through the new `N_CONDITION8`
  layout. Taken emits `NONE`; not-taken
  stages the new `CONDITIONAL_SKIP`
  effect and the CPU retires n NOP words
  with PC advance, no fetch/effect/bus
  traffic, delay-slot countdown
  consumption, and T preservation.
  Sixteen image-evidenced single
  conditions plus `UNC` evaluate through
  the shared branch-condition core;
  `BIO`/`NBIO`, `AOV`/`ANOV`/`BOV`/`BNOV`,
  and multi-condition combos reject as
  deferred `INVALID_OPERAND`.
  `INELIGIBLE` repeatability on the
  explicit TI `not repeatable` statement.
  Lookup gains 512 entries (19993 to
  20505) with zero neighbor moves;
  census moves to
  221 IDs, 220 bindings, 88/205 families
  (42.927%), 33 strict-complete, 187
  SUBSET. The TI Example vector
  (`XC 1,ALEQ` with A −1) pins end to
  end at the leaf, through the trusted
  entries, and on the CPU. README
  wave-1 tail wording lands XC.
  `git diff --check` clean.
- Evidence stands at 392 records, 648
  requirements, 519 tests (519
  `PASSING`, 0 `PLANNED`). The S4 anchor
  projected 392/648/519 with one TI
  source record, one activation record,
  four requirements, and three test
  records: all counts match exactly (the
  TI `XC` source record is new under
  `SRC-TI-SPRU172C`; no separate
  plan-anchor record — PROJECT
  requirements cite the activation
  record, COMMON requirements cite the
  TI record only per the scope gate).
  The new `N_CONDITION` addressing token
  follows the per-slice token precedent
  (`ALL` is schema-reserved for
  `COMPLETE`). No knob change.
- Decisions: the not-taken skip commits
  the widened (1 + n) plan at the single
  shared bridge-plan site instead of a
  second plan call, so the
  one-preparation-plan perf contract
  holds unchanged with no test edit;
  mid-`.def` placement (after `RCD`)
  shifts no catalog-order contract
  positions (positional pins cover only
  rows 0–26), so the XC catalog-contract
  assertion uses symbol lookup.
  Absorptions: `0xFD00`/`0xFF00` leave
  older `UNKNOWN` pins in the
  CC/CCD/RC/RCD/RET/RETD near-miss
  lanes, now re-pinned to `XC`.
- Validation is GREEN: full CMake CTest
  493/493 on the working Debug build;
  sanitizer XC trio clean (clean flagged
  ASan/UBSan rebuild, direct runs exit
  0, `build-s4-xc-asan/` removed after);
  `make test` clean (exit 0); evidence,
  inventory, and lookup gates pass;
  contract quality suites pass.
- Follow-up work: `SQDST` two-write
  effect seam, S1 STM/MMR, S6 PORTR/PORTW,
  AR6/AR7 dual-Y, MOD3, dual replay,
  RPTB/RPTZ/RPTBD, far/XPC/timing.

## 2026-09-17 — P3-S5 accumulator-addressed push seam (slice 36)

- Commit `321a428` lands the S5 seam
  with no catalog rows, following the
  W43A pattern. Two new WORD16 MEMORY
  address kinds,
  `STACK_PUSH_ACCUMULATOR_WORD16` (15)
  and
  `STACK_PUSH_ACCUMULATOR_DELAYED_WORD16`
  (16), carry accumulator-targeted
  pushes for the future `CALA[D]` rows
  (TI SPRU172C §4 `CALA[D]`, printed
  pp. 4-24–4-25: one-word call to
  src(15–0), TOS `PC+1` nondelayed and
  `PC+3` delayed over `SP-1`, no status
  effects, explicit not-repeatable
  note). TOS planning is identical to
  the immediate pushes; the A/B select
  rides `arf` (0/1) while every other
  first-arm field stays zero. The PC
  target is the low 16 bits of the
  selected accumulator read from the
  frozen input snapshot gated on
  accumulator knownness at prepare
  (unknown rejects before any bus
  issue); commit re-derives the same
  target deterministically from the
  frozen input, so no frozen field
  grows and root sizes stay frozen.
  Return addresses reuse the bridge
  successor. No catalog row, opcode
  identity, handler, CPU opcode branch,
  knob, timing, public-ABI, or new
  public CPU snapshot claim.
- The slice tripped the 800-line split
  gate (`memory_instruction_transaction.c`
  reached 891): the whole stack-push
  prepare arm plus deferred push
  reporting moved to the new
  `memory_instruction_stack_push`
  owner (root back to 779, owner at
  168). Behavior is preserved: the
  three seam lanes plus the existing
  `CALL[D]` root tests stay green.
- Evidence stands at 394 records, 651
  requirements, 522 tests (522
  `PASSING`, 0 `PLANNED`), matching the
  S5 projection exactly: one TI source
  record, one plan-anchor record, three
  requirements, three test records. No
  knob change.
- Correction to the S4 entry above:
  `REQ-PROJECT-P3-S4-XC-INVENTORY`
  claims the binding lands "under the
  `ALL` addressing token"; the bound
  token is `N_CONDITION` (`ALL` is
  schema-reserved for `COMPLETE`). The
  machine record (inventory binding) was
  always correct; only the requirement
  prose is off. This correction stands
  in the ledger instead of an edit.
- Validation is GREEN: full CMake CTest
  496/496 on the working Debug build
  (493 plus the 3 seam lanes); `make
  test` clean (exit 0); evidence,
  inventory, and lookup gates pass;
  contract quality suites pass. The
  effect-validity lane was observed RED
  before GREEN (missing kinds at
  compile); resolver and transaction
  lanes are new-file coverage of the
  new kinds.
- Follow-up work: `BACC`/`BACCD` family
  on the branch seam (no S5 use),
  `CALA`/`CALAD` family on this seam,
  S1 STM/MMR, S6 PORTR/PORTW, AR6/AR7
  dual-Y, MOD3, dual replay,
  RPTB/RPTZ/RPTBD, far/XPC/timing.

## 2026-09-17 — P3-S8 BACC accumulator-addressed branch family (slice 37)

- Commit `b2c06bb` lands the `BACC`
  family on the S5 seam: two new
  catalog rows, `BACC` (`0xF4E2`) and
  `BACCD` (`0xF6E2`), under the shared
  `0xFEFF` mask through the shared
  `ACCUMULATOR_SOURCE` layout (S at bit
  8: 0 selects A, 1 selects B). Checked
  and trusted leaves branch PC to the
  16 low bits of the selected
  accumulator unconditionally; CPU
  retirement takes `BACC` followers
  normally and `BACCD` followers
  through the two-word delay window.
  Both rows are repeatability
  `INELIGIBLE` on the explicit TI not-
  repeatable note (TI SPRU172C
  §`BACC[D]`, printed p. 4-15).
  `FORM-T2-12-02` lands `SUBSET` with
  `BACC` under `ACCUMULATOR_NONDELAYED`
  and `BACCD` under
  `ACCUMULATOR_DELAYED`.
- Numbering: the family is P3-S8, not
  P3-S6, because S6 is reserved for the
  port I/O seam in the approved seam-
  first table (families take fresh
  numbers per the P3-S7 STM precedent).
- Decisions: the scope gate rejected a
  `COMMON` CPU requirement citing the
  `PROJECT_CONTRACT_ONLY` activation,
  so `REQ-COMMON-P3-S8-BACC-CPU-
  RETIREMENT` cites only the two TI
  semantics records (XC precedent). The
  shared layout serves the future
  `CALA`/`CALAD` rows with no new
  requirement. Five RED lanes: leaf,
  trusted, and CPU per row plus the
  shared layout suite.
- Contract pins move a layer at a time:
  catalog rows 220 to 222, bindings 220
  to 222, `OPCODE_ID_COUNT` 221 to 223,
  lookup entries 20505 to 20509 (two
  rows by two S values, zero neighbor
  moves), bound forms 88 to 89
  (43.415%), `SUBSET` bindings 187 to
  189, `BACCD` joins `EXPECTED_DELAYED`
  as the docstring anticipated. No knob
  change.
- Evidence stands at 400 records, 655
  requirements, 527 tests (527
  `PASSING`, 0 `PLANNED`), matching the
  S8 projection exactly: five TI
  source records, one activation
  record, four requirements, five test
  records.
- Validation is GREEN: `make test`
  clean (exit 0, full fast suite plus
  contract quality 272/272); evidence,
  inventory, and lookup gates pass.
  The local CMake `build/` tree is
  stale (enumerates 496 pre-BACC
  tests); CI reconfigures it from
  scratch, so no action is needed.
- Follow-up work: `CALA`/`CALAD`
  family on the S5 seam (TI record
  `E-TI-C54X-SPRU172C-S4-CALA-P4-24-4-
  25` already pinned), S1 STM/MMR, S6
  PORTR/PORTW, far/XPC/timing. Push of
  the local stack to `origin/main`
  still awaits user approval, so remote
  CI has not yet seen the S4/S5/S8
  fixes.

## 2026-09-17 — P3-S9 CALA accumulator-addressed call family (slice 38)

- Commit `1d9a9ea` lands the `CALA`
  family on the S5 seam plus fix
  `5dd96b0` for two `OPCODE_ID_COUNT`
  pins missed by the slice commit. Two
  new catalog rows, `CALA` (`0xF4E3`)
  and `CALAD` (`0xF6E3`), under the
  shared `0xFEFF` mask through the
  shared `ACCUMULATOR_SOURCE` layout
  (TI SPRU172C §`CALA[D]`, printed pp.
  4-24–4-25, opcode
  `1111_01ZS_1110_0011`). Checked and
  trusted leaves stage a MEMORY
  `STACK_PUSH_ACCUMULATOR(_DELAYED)`
  TOS WRITE with the select in `arf`
  and a zero return placeholder; the S5
  root derives the PC target from the
  frozen snapshot and stages PC+1
  (`CALA`) or PC+3 (`CALAD`). Both rows
  `INELIGIBLE` repeatability.
  `FORM-T2-13-01` lands `SUBSET` with
  `CALA` under `ACCUMULATOR_NONDELAYED`
  and `CALAD` under
  `ACCUMULATOR_DELAYED`. No `run.c`
  change: retirement reuses the generic
  MEMORY route.
- Decisions: new input-resource token
  `ACCUMULATORS_STACK_POINTER` carries
  exactly the two consumed inputs (no
  status claim); collector and
  `KNOWN_MASK` already cover both bits.
  Leaf gates SP knownness as
  `INVALID_OPERAND` (CALL precedent)
  and selected-accumulator knownness as
  `UNKNOWN_INPUT` (BACC precedent);
  prepare keeps its own gate as
  defense in depth. No new layout
  requirement: the S8 layout record
  already names the future rows.
- Size review: `test_cpu_calad.c` is
  555 lines (advisory `SIZE_REVIEW`
  over 500, hard split at 800). Split
  decision: keep whole, mirroring the
  653-line `test_cpu_calld.c` shape;
  the delay-window and transactionality
  coverage reads as one fixture
  narrative. No knob change.
- Contract pins: rows 222 to 224,
  `OPCODE_ID_COUNT` 223 to 225, lookup
  20509 to 20513 (zero neighbor
  moves), bound forms 89 to 90
  (43.902%), `SUBSET` 189 to 191,
  `CALAD` joins `EXPECTED_DELAYED`.
  `RETED`/`RETE` near-miss tables drop
  bit-3 flips onto `CALAD`/`CALA` and
  pin them as siblings (RCD/RC
  precedent). One process slip: a
  brace-typo from pin `sed` on the
  one-line reverse-carry test broke
  collection; fixed by hand and
  verified minimal by `git diff`.
  Prefer the editing tool for pin
  edits going forward.
- Evidence stands at 406 records, 658
  requirements, 531 tests (531
  `PASSING`, 0 `PLANNED`), matching the
  S9 projection exactly: five TI
  source records, one activation
  record, three requirements, four
  test records.
- Validation is GREEN: `make test`
  clean (exit 0, 105 fast binaries
  plus contract quality 272/272);
  evidence, inventory, and lookup
  gates pass. Four RED lanes observed
  RED (missing `op_cala.h`) before
  GREEN.
- Follow-up work: S1 STM/MMR, S6
  PORTR/PORTW, far/XPC/timing. Push of
  the local stack to `origin/main`
  still awaits user approval, so remote
  CI has not yet seen the S4/S5/S8/S9
  fixes.

## 2026-09-17 — P3-S10 STH syntax-1 Smem store (slice 39)

- Commit `41d24f1` lands STH syntax 1
  (`STH src, Smem`, TI SPRU172C §STH,
  pp. 4-168–4-171, opcode
  `1000_001S_IAAAAAAA`) plus fix for
  two `OPCODE_ID_COUNT` pins. Seven
  rows mirror the `STL_SMEM` family
  with bit 9 set: `STH_SMEM`
  (`0x8280`/`0xFEE0`) and MOD4–MOD10
  (`0x82A0`–`0x82D0`/`0xFEF8`) through
  the shared `ACCUMULATOR_ARF_MOD`/
  `MOD4` layouts. Checked and trusted
  leaves stage indirect, indexed,
  circular, and reverse-carry MEMORY
  WRITE effects carrying bits 31–16 of
  the selected accumulator. All rows
  `UNRESOLVED` repeatability (no TI
  note; `STL_SMEM` precedent).
  `FORM-T2-20-05` lands `SUBSET` with
  all seven symbols under the reused
  `STL_SMEM` mode token. No `run.c`
  change. Direct-DMA7 and shifted
  syntaxes 2–4 stay for later slices.
- Decisions: value derivation is the
  only new logic — `(raw40 >> 16)`
  where STL takes the low word — so
  the op files derive from the STL
  sources with the shared commons
  untouched. My own CPU-test vector
  for B was miscomputed (`0xCDEF`
  for `0x00ABCDEF01>>16`); the leaf
  was right, the test was wrong, fixed
  to `0xABCD`. Leaf-first debugging
  order stands: run the leaf lane
  before assuming the CPU lane.
- Contract pins: rows 224 to 231,
  `OPCODE_ID_COUNT` 225 to 232, lookup
  20513 to 20673 (+160 entries, zero
  neighbor moves), bound forms 90 to
  91 (44.390%), `SUBSET` 191 to 198.
  The catalog order test needed a
  positional shift: `STH_SMEM` pins
  rows[6] after `STL_SMEM`, later
  indices shift by one (S3z
  precedent). `RETED`/`RETE` near-miss
  tables were unaffected this time
  (no 0x82xx words targeted). No knob
  change.
- Process slip, repeated: the same two
  `OPCODE_ID_COUNT` pins
  (`test_xc.c`, repeat eligibility)
  missed the slice commit again and
  landed as a fix commit. The slice
  checklist now names those two files
  explicitly for every row-adding
  slice.
- Evidence stands at 409 records, 661
  requirements, 533 tests (533
  `PASSING`, 0 `PLANNED`), matching the
  S10 projection exactly: two TI
  source records, one activation
  record, three requirements, two test
  records.
- Validation is GREEN: `make test`
  clean (exit 0); evidence, inventory,
  and lookup gates pass. Both RED
  lanes observed RED (missing
  `op_sth_smem.h`) before GREEN.
- Follow-up work: STH syntaxes 2–4,
  STLM, S1-family MMR/stack moves, S6
  PORTR/PORTW, far/XPC/timing. Push of
  the local stack to `origin/main`
  still awaits user approval, so remote
  CI has not yet seen the recent
  fixes.

## 2026-09-17 — P3-S11 STH by-ASM Smem store (slice 40)

- Commit `3d03555` lands STH syntax 2
  (`STH src, ASM, Smem`, TI SPRU172C
  §STH, pp. 4-168–4-171, opcode
  `1000_011S_IAAAAAAA`). Seven rows
  mirror the `STH_SMEM` family with bit
  9 set: `STH_ASM_SMEM`
  (`0x8680`/`0xFEE0`) and MOD4–MOD10
  (`0x86A0`–`0x86D0`/`0xFEF8`) through
  the shared `ACCUMULATOR_ARF_MOD`/
  `MOD4` layouts. Checked and trusted
  leaves shift the selected accumulator
  by ASM (0–15 left, 16–31 arithmetic
  right with sign fill) and stage
  indirect, indexed, circular, and
  reverse-carry MEMORY WRITE effects
  carrying bits 31–16 of the shifted
  value. All rows `UNRESOLVED`
  repeatability (no TI note; `STH_SMEM`
  precedent). `FORM-T2-20-06` lands
  `SUBSET` with all seven symbols under
  the reused mode token. No `run.c`
  change. Syntaxes 3–4 and `STLM` stay
  for later slices.
- Decisions: SXM is proven out of the
  value path — both documented SXM
  rules coincide with arithmetic fill
  over the whole ASM domain — so the
  new `ACCUMULATORS_STATUS_ASM` token
  claims ASM alone, and SXM rides
  hostile and unconsumed in every
  vector. My own CPU-test vector for
  ASM=15/B was miscomputed (`0x2A19`);
  the leaf and an independent Python
  oracle agreed on `0xA190`
  (`(0xFF87654321 << 15)` mod 2^40 is
  `0xB2A1908000`), the test was wrong,
  fixed with a derivation comment. TI
  worked examples (`STH B,-8` to FF84,
  `STH A,-4` under SXM=1 to F842)
  pinned at both lanes. Leaf-first
  order stands again: the leaf lane was
  green while the CPU lane carried the
  bad vector.
- Debug artifact: throwaway probes that
  read `effect.payload` inside the same
  `printf` argument list as the op call
  observed `0x0000` from unspecified
  argument evaluation order; the
  library was correct throughout.
  Probes now sequence the call before
  any field read. Scratch probes stay
  under `/tmp`, never in the repo.
- Contract pins: rows 231 to 238,
  `OPCODE_ID_COUNT` 232 to 239, lookup
  20673 to 20833 (+160 entries, zero
  neighbor moves), bound forms 91 to
  92 (44.878%), `SUBSET` 198 to 205.
  The catalog order test needed a
  positional shift: `STH_ASM_SMEM` pins
  rows[7] after `STH_SMEM`, later
  indices shift by one more (S10
  precedent). No knob change.
- Process slip: one more
  `OPCODE_ID_COUNT` pin missed the
  first sweep — the reverse-carry
  metadata file spells it `==232`
  without spaces. The row-adding
  checklist now sweeps flexible
  spacing, not just `== 232`.
- Evidence stands at 412 records, 664
  requirements, 535 tests (535
  `PASSING`, 0 `PLANNED`), matching the
  S11 projection: two TI source
  records, one activation record, three
  requirements, two test records.
- Validation is GREEN: `make test`
  clean (exit 0) — 272 quality tests
  OK, evidence and inventory gates
  pass, 105 C binaries ok. Both RED
  lanes observed RED before GREEN.
- Follow-up work: STH syntaxes 3–4,
  STLM, S1-family MMR/stack moves, S6
  PORTR/PORTW, far/XPC/timing. Push of
  the local stack to `origin/main`
  still awaits user approval, so remote
  CI has not yet seen the recent
  slices.

## 2026-09-17 — P3-S12 STH by-SHFT Xmem store (slice 41)

- Commit `0e99faf` lands STH syntax 3
  (`STH src, SHFT, Xmem`, TI SPRU172C
  §STH, pp. 4-168–4-171, opcode
  `1001_101S_XXXXSHFT`). One row,
  `STH_SHFT_XMEM` (`0x9A00`/`0xFE00`),
  placed after `LD_XMEM_SHIFT` so the
  early positional catalog pins do not
  move. It reuses the shared
  `ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4`
  layout (Xmod in bits 7–6, raw Xar in
  bits 5–4 mapping to AR2–AR5,
  unsigned SHFT in bits 3–0). Checked
  and trusted leaves shift the
  selected accumulator arithmetically
  right by `(16-SHFT)` and stage
  dual-indirect X-only MEMORY WRITE
  effects carrying bits 31–16. Every
  encoded shift is a right shift of at
  most 16, so the stored field is pure
  source bits and SXM rides hostile
  and unconsumed under the new
  `ACCUMULATORS_AUXILIARY_REGISTERS`
  token. The row lands `UNRESOLVED`
  repeatability (no TI note;
  `LD_XMEM_SHIFT` precedent). No
  `run.c` change. Syntax 4 and `STLM`
  stay for later slices.
- Seam work: STH syntax-3 is the first
  Xmem store. The X-only resolver
  (`address_resolution.c`) and the
  dual planner (`indirect_dual.c`)
  were read-only; address arithmetic
  is direction-independent, so both
  gained minimal WRITE arms while the
  full-dual resolver stays read-only.
  One planner contract test
  (`test_indirect_dual_mod0.c`) pinned
  the old read-only gate and was
  evolved to the new contract with
  justification, not weakened. The TI
  manual itself was fetched from
  ti.com and its STH/STLM bit tables
  extracted from the PDF to settle
  syntaxes 3–4 and STLM evidence.
- Decisions: the manual's
  `(SHFT-16)` execution formula plus
  Example 2 (`STH B,-8`, encoded SHFT
  8, B FF_8421_1234 storing FF84)
  pin the unsigned-SHFT reading
  against the glossary (`0-15`);
  Example 2's `*AR7-` assembles as
  syntax 4, not syntax 3, so the
  X-only AR2–AR5 contract stands.
  `FORM-T2-20-07` lands `COMPLETE`
  (one row covers the whole syntax;
  `LD_XMEM_SHIFT` precedent), not
  `SUBSET`.
- Contract pins: rows 238 to 239,
  `OPCODE_ID_COUNT` 239 to 240, lookup
  20833 to 21345 (+512 entries, zero
  neighbor moves), bound forms 92 to
  93 (45.366%), strict complete bound
  33 to 34 (16.585%), `SUBSET` stays
  205. No catalog renumber this time.
  No knob change.
- Process slip: a `sed` broke a quote
  in `sources.cmake` (caught by the
  manifest gate, repaired). The
  row-adding checklist now verifies
  the manifest gate explicitly after
  mechanical edits.
- Evidence stands at 415 records, 667
  requirements, 537 tests (537
  `PASSING`, 0 `PLANNED`), matching the
  S12 projection: two TI source
  records, one activation record,
  three requirements, two test
  records.
- Validation is GREEN: `make test`
  clean (exit 0) — 272 quality tests
  OK, evidence and inventory gates
  pass, both S12 lanes plus the
  evolved planner contract green.
  Both RED lanes observed RED before
  GREEN (missing header, then CPU
  retirement through the read-only
  seam).
- Follow-up work: STH syntax 4
  (2-word SHIFT/Smem), STLM, S1-family
  MMR/stack moves, S6 PORTR/PORTW,
  far/XPC/timing.

## 2026-09-17 — P3-S13 STH by-SHIFT Smem store (slice 42)

- Commit `151486b` lands STH syntax 4
  (`STH src,SHIFT,Smem`, two words, TI SPRU172C
  §STH, pp. 4-168–4-171). Seven rows:
  `STH_SHIFT_DIRECT` (`0x6F00`/`0xFF80`,
  7-bit dma) plus `STH_SHIFT_SMEM_MOD4`
  through `STH_SHIFT_SMEM_MOD10`
  (`0x6FA0`–`0x6FD0`/`0xFFF8`, ARF in
  bits 2-0, MOD in bits 6-3). The 0xFFF8
  mask (not 0xFEF8) is forced: the 0x6F
  high byte sets bit 8, which the mask
  must fix. MOD0–MOD3 stay deferred on
  the 0x6F80 LD Smem,SHIFT collision.
- Checked and trusted leaves in
  `src/isa/ops/store/op_sth_shift_smem.[hc]`
  (14 entry points) over two new layouts
  (`ACCUMULATOR_DIRECT7_SHIFT5`,
  `ACCUMULATOR_ARF_MOD4_SHIFT5`) with
  checked sets, trusted plans, and schema
  tokens. Second-word mask is 0xFEE0:
  bits 15-9 and 7-5 fixed at 0x0C60,
  S (bit 8) and SHIFT (bits 4-0) outside
  (an 0xFFE0/0xFDE0 confusion cost two
  debug rounds; verified against an
  independent mask computation).
- Semantics: arithmetic shift by the
  signed 5-bit count (-16 to +15),
  store bits 31-16; SXM hostile and
  unconsumed; no ARF0 rejection (the
  two-word form carries no ARP alias).
  Direct words gate DP unknownness at
  the leaf through the new
  `ACCUMULATORS_STATUS_DP` resource
  (bit 4096, thirteenth resource) with
  snapshot and collect arms.
- `DIRECT_DP_WORD16` reuses the shared
  `long_offset_word` (DP page) and
  `direct_dma7` (offset): a first attempt
  with dedicated fields broke the pinned
  24-byte address ABI (static assert),
  so the kind reinterprets per-kind with
  no arm growth. Resolution routes
  through the shared DP planner;
  candidate validation gains a shape arm
  plus a `matches_live` arm re-reading
  CPL==0 and DP==dp9 (the generic
  CMPT0/ARP0 gate does not apply).
- The validator file hit the 800-line
  split gate (849) and was split: DIRECT
  shape/live arms plus the shared PC
  check moved to
  `memory_candidate_direct.[hc]`
  (751/175 lines).
- ABI deltas: address spec stays 24;
  input snapshot 60 to 64 bytes on
  ILP32 only (LP64 unchanged at 64);
  frozen m32 root pin 328 to 332.
- RED-contract repairs (delegate-authored
  tests): flat spec shape (no
  `.dp`/`.offset` sub-struct), layout-id
  type, CPU budget 1 (budgets count
  instructions), LD/BANZD near-miss
  siblings now pin the S13 rows, and the
  0x1000U unknown-resource-bit pins move
  to 0x2000U (the DP bit claimed it).
- Pins carried: rows 239 to 246
  (COUNT 247), lookup 21345 to 21521,
  bound forms 93 to 94 of 205 (strict
  stays 34), SUBSET bindings 205 to 212.
  FORM-T2-20-08 lands SUBSET (seven
  symbols). No knob change.
- Evidence stands at 418 records, 670
  requirements, 540 tests (540
  `PASSING`, 0 `PLANNED`), matching the
  S13 projection: two TI records, one
  activation, three requirements, three
  test records.
- Validation is GREEN: `make test`
  clean (exit 0) — full C suite plus 272
  quality tests OK, evidence and
  inventory gates pass. All three RED
  lanes observed RED before GREEN
  (missing header, word-2 mask, CPU
  retirement through the DP seam).
- Process: first slice under the new
  logging process (slice doc born
  `(planned)` in the docs commit,
  flipped here). Late correction: the
  S12 slice doc and plan link still read
  `(planned)` after its landing; flipped
  to implemented in this commit.
- Follow-up work: STH MOD0–MOD3
  second-word disambiguation, STLM/MMR,
  S1-family MMR/stack moves, S6
  PORTR/PORTW, far/XPC/timing, then
  worktree fan-out for the remaining
  leaves.

## 2026-09-17 — S12/S13 activation-anchor correction

- Correction naming the P3-S13 ledger
  entry (`b071b72`): its status flip
  changed the S12/S13 slice-doc headings
  and plan links to `(implemented;
  local verification green)` but left
  the `E-PROJECT-P3-S12-STH-SHFT-ACTIVATION`
  and `E-PROJECT-P3-S13-STH-SHIFT-ACTIVATION`
  locator anchors at `(planned)`, which
  reddened `check_evidence.py` (2 LOCATOR
  errors) at HEAD. Anchors are synced to
  the landed headings (W22 precedent).
- Process fix: a ledger status flip must
  sync the slice's ACTIVATION anchors in
  `config/evidence.json` and
  `docs/evidence-index.md` in the same
  commit, or the gate reds on the flip.
  Applied to S14/S15 below.

## 2026-09-17 — P3-S14 SUB Smem pure leaf (slice 43)

- Commit `ef121f6` lands the first
  isolated-worktree fan-out pilot batch
  with P3-S15. Triage ranked SUB Smem
  (FORM-T2-02-01) leaf-ready on existing
  seams; the pilot worker delivered four
  bounded new files with no shared-file
  edits and no commits, reviewed and
  merged by the maintainer.
- Checked/trusted syntaxes 1-3
  (`src -= Smem`, `src -= Smem << TS`,
  `dst = src - (Smem << 16)`) in
  `src/isa/ops/arithmetic/op_sub_smem.[hc]`
  (459/67 lines) keyed on the shared
  SELECT_INDIRECT_ADDR7 and
  SOURCE_DESTINATION_INDIRECT_ADDR7
  layouts: SXM extension, T-register
  0x00-0x1F left and 0x30-0x3F
  arithmetic-right bands, fixed logical
  shift left 16, checked 40-bit
  subtraction, C from the bit-32 borrow
  (syntaxes 1-2) and set-on-no-borrow
  (syntax 3), OVM saturation to
  0x007FFFFFFF/0xFF80000000 with sticky
  destination overflow, TC preserved,
  MOD0-MOD3 reads through the shared
  MEMORY effect, exotic modes defer as
  INVALID_OPERAND with the caller effect
  untouched.
- Proved by
  `tests/unit/isa/ops/arithmetic/test_sub_smem.c`
  (16 cases, CTest `ti_c54x.sub_smem_ops`),
  wired in `sources.mk`/`sources.cmake`
  and `tests.mk`/`CMakeLists.txt`.
  Catalog rows plus dispatch binding stay
  deferred to the inventory-owning slice;
  FORM-T2-02-01 remains UNIMPLEMENTED.
- Registry: `E-PROJECT-P3-S14-SUB-SMEM-ACTIVATION`,
  `REQ-COMMON-P3-S14-SUB-SMEM-SEMANTICS`
  on the existing TI SUB section record
  `E-TI-C54X-SPRU172C-S4-SUB-P4-187-190-ACCUMULATOR-SHIFT5`,
  `T-COMMON-P3-S14-SUB-SMEM-SEMANTICS`.
  Slice doc born `(planned)` in the impl
  commit, flipped here with its anchor.
- Validation: focused binaries pass,
  ASan/UBSan clean, CMake configure plus
  `ctest -R` 2/2 pass, full `make test`
  green. No knob change.

## 2026-09-17 — P3-S15 AND Smem pure leaf (slice 44)

- Commit `ef121f6` lands the second
  fan-out pilot leaf. AND Smem
  (FORM-T2-07-01, TI SPRU172C §AND,
  p. 4-11: `dst &= Smem`) as checked and
  trusted entry points in
  `src/isa/ops/logical/op_and_smem.[hc]`
  (203/48 lines): DESTINATION_INDIRECT_ADDR7
  shape, single WORD16 READ over direct
  page-0 or standard-indirect MOD0-MOD3
  through the shared MEMORY effect,
  zero-extended unshifted word combined
  full-width into the destination
  accumulator, C and TC preserved, no
  SXM/OVM consumed, exotic modes defer
  as INVALID_OPERAND.
- Proved by
  `tests/unit/isa/ops/logical/test_and_smem.c`
  (13 checks, CTest `ti_c54x.and_smem_ops`),
  wired in `sources.mk`/`sources.cmake`
  and `tests.mk`/`CMakeLists.txt`.
  Catalog row plus dispatch binding stay
  deferred; FORM-T2-07-01 remains
  UNIMPLEMENTED.
- Registry: `E-PROJECT-P3-S15-AND-SMEM-ACTIVATION`,
  `REQ-COMMON-P3-S15-AND-SMEM-SEMANTICS`
  on `E-TI-C54X-SPRU172C-S4-AND-P4-11`,
  `T-COMMON-P3-S15-AND-SMEM-SEMANTICS`.
  Slice doc born `(planned)` in the impl
  commit, flipped here with its anchor.
- Validation shared with the S14 entry
  (same gate run). Evidence stands at
  420 records, 672 requirements, 542
  tests. No knob change.
- Fan-out hardening in the same impl
  commit: agent worktrees (`.muse`) are
  now excluded from manifest reachability
  (`IGNORED_TREE_NAMES`) and the knob
  project-file walk, so the gate stays
  green while leaf workers are in flight.
- Follow-up work: catalog/dispatch
  binding slices for SUB Smem and AND
  Smem (needs TI opcode-diagram rows),
  then the next pilot wave over the
  triaged leaf-ready list.

## 2026-09-17 — Status-flip backlog catch-up

- The plan index and 27 slice docs still
  read `(planned)` for slices whose
  registry ACTIVATIONs assert implemented
  and green with IMPLEMENTED requirements
  and PASSING tests: S1, S2, S3 (plus
  S3B-S3I ACTIVATIONs anchored to the S3
  seam doc), S3J-S3Z, S4, S5, S7, S8,
  S9, S10, S11. A bulk audit verified
  every ACTIVATION is backed (all linked
  requirements IMPLEMENTED, all linked
  tests PASSING with files present, all
  anchors resolving); the S3F/G/H/I/T/U/V/W/X/Y
  lock-in slices retire through shared
  rows with no new modules, which the
  assertions already state.
- This commit flips all 27 headings plus
  their anchor sentences and plan links
  to `(implemented; local verification
  green)` and syncs the 38 registry
  anchors and 38 index cells quoting
  them (same-commit sync per the S12/S13
  correction). S12/S15 body sentences
  get the same quote sync. No code,
  test, requirement, or inventory change;
  `check_evidence.py` PASS (420/672/542).
- True remaining plan: S6 port seam (no
  slice doc yet), far/XPC (deferred by
  the arch criterion), the 111-form
  UNIMPLEMENTED backlog minus landed
  rows, and SUB/AND Smem catalog
  binding (blocked on TI opcode-diagram
  rows).

## 2026-09-18 — P3-S16 OR Smem and P3-S17 XOR Smem pure leaves (slices 45-46)

- Commit `c906f1c` lands fan-out wave 2.
  OR Smem (FORM-T2-08-01, TI SPRU172C
  §OR p. 4-123) and XOR Smem
  (FORM-T2-09-01, §XOR p. 4-201) as
  catalog-free pure leaves mirroring the
  S15 AND pattern: checked-values plus
  trusted three-operand entry points in
  `src/isa/ops/logical/op_or_smem.[hc]`
  and `op_xor_smem.[hc]` (203 lines
  each), single WORD16 READ over direct
  page-0 or standard-indirect MOD0-MOD3
  through the shared MEMORY effect,
  zero-extended unshifted word combined
  full-width, C/TC preserved, no SXM/OVM
  consumed, exotic modes defer as
  INVALID_OPERAND.
- Proved by `test_or_smem.c` and
  `test_xor_smem.c` (13 checks each,
  CTest `ti_c54x.or_smem_ops` /
  `ti_c54x.xor_smem_ops`), wired in
  `sources.mk`/`sources.cmake` and
  `tests.mk`/`CMakeLists.txt`. Catalog
  rows plus dispatch binding deferred;
  both forms remain UNIMPLEMENTED.
- Registry: `E-PROJECT-P3-S16-OR-SMEM-ACTIVATION`
  and `E-PROJECT-P3-S17-XOR-SMEM-ACTIVATION`,
  `REQ-COMMON-P3-S16-OR-SMEM-SEMANTICS`
  on `E-TI-C54X-SPRU172C-S4-OR-P4-123`,
  `REQ-COMMON-P3-S17-XOR-SMEM-SEMANTICS`
  on `E-TI-C54X-SPRU172C-S4-XOR-P4-201`,
  plus the two PASSING test records.
  Slice docs born `(planned)`, flipped
  here with anchors synced same-commit.
- Validation: focused binaries pass
  standard and ASan/UBSan builds, CMake
  configure plus `ctest -R` 2/2, full
  `make test` exit 0. Evidence stands at
  422 records, 674 requirements, 544
  tests. No knob change.
- Wave-3 note: mnemonic-level TI
  matching is insufficient triage (MPY
  Smem and MAC Smem rows already landed
  with dispatch, so T2-03-04/05 and
  T2-04-02/03 are non-Smem syntaxes).
  Wave 3 opens with a syntax-level
  triage child before any leaf dispatch.

## 2026-09-18 — P3-S18 planning slice and wave-3 triage stop

- The syntax-triage workflow ran ~30
  turns without delivering, so it was
  stopped (`work_stop` accepted); its
  read-only brief, if ever recovered,
  can only inform wave 5 now. Lesson:
  timebox delegated triage and keep an
  independent elimination map (tables
  2-20/2-01/2-02/2-23/2-24/2-25 decoded
  from catalog symbols in-session).
- Maintainer analysis unblocked STH
  MOD0-MOD3 without TI input: their
  first words sit inside the
  `LD_SMEM_SHIFT` slot (`0x6F80/FFE0`),
  but the second words are disjoint
  under `0xFEE0` (LD `0x0C40` vs STH
  `0x0C60`, differing in bit 5), proven
  from the landed LD trusted handler and
  the S13 record. Decode is first-word
  only, so this becomes seam P3-S18
  (`docs/plan/p3-s18-sth-mod0-3-second-word-seam.md`,
  planned): word-2-qualified generator
  rows plus four STH rows reusing the
  S13 handlers, LD decode bit-identical.
- This commit carries the planning slice
  plus its plan link only; no code,
  registry, or inventory change. The TI
  request for Smem opcode diagrams,
  PORT, and variant syntax lists stands.

## 2026-09-18 — P3-S18 RED lane 1: word-2-qualified catalog rows

- The catalog schema
  (`tools/isa/catalog/opcode_catalog_schema.py`)
  gains the `MASKED2` row form with
  `second_word_value`/`second_word_mask`
  (defaulting `None` so every existing
  positional construction keeps
  working), strict word-2 validation
  (two-word minimum, 16-bit range,
  value-within-mask, qualifier requires
  the form), and the overlap exemption
  for dual-qualified rows whose word-2
  patterns disagree on a commonly masked
  bit. Qualified-vs-unqualified first-
  word sharing stays an overlap error.
- Proved by seven new contract tests
  (`Word2QualifiedRowTests` in
  `tests/contract/quality/test_opcode_catalog.py`),
  written RED first (4 failing) then
  green with the 7 pre-existing tests
  untouched. No `.def` change yet, so
  the generated lookup is byte-identical
  and the full gate stays green.
- Next lanes: generator/decode support
  for qualified slots, LD row
  qualification with regression cover,
  the four STH MOD0-MOD3 rows, then
  registry and inventory binding. No
  knob change.

## 2026-09-18 — P3-S18 lane 2a lands as a3f931d

- Lane 1 (schema `MASKED2` plus overlap
  exemption) is consumed by lane 2a:
  `render_qualified_slots` renders
  declaration-order entries plus an
  `UNKNOWN` sentinel, wired through an
  opt-in `--qualified-output` flag so
  existing CLI tests and temp-dir runs
  never touch a second artifact; the
  tracked empty fragment
  `generated/isa/catalog/opcode_qualified_slots.inc`
  is freshness-checked by a new
  production test.
- C side: `ti_c54x_qualified_slot` plus
  `ti_c54x_first_word_has_qualified_slot`
  and `ti_c54x_resolve_qualified_slot`
  in the private catalog seam, with
  `MASKED2` X-macro definitions at all
  five `.def` consumers (enum,
  metadata, binding, prototypes, dispatch
  table). Qualifiers live ONLY in the
  slot table: growing `ti_c54x_opcode_metadata`
  tripped the pinned 24-byte guard, and
  the guard was right, so the struct is
  untouched and the single-source design
  stands. Proved by the empty-table C
  test plus python suites (29 tests OK)
  and the full gate green.
- Split commit discipline resumes here:
  impl `a3f931d`, this ledger-only
  follow-up. Next is lane 2b (LD row
  qualification with parity probe plus
  trusted re-resolution).

## 2026-09-18 — P3-S18 lane 2b lands as a74cb5a

- Lane 2b qualifies the `0x6F80/FFE0` slot: `LD_SMEM_SHIFT` becomes
  `MASKED2` (`0x0C40` under `FEE0`) and four `STH_SHIFT_SMEM_MOD0-MOD3`
  `MASKED2` rows (`0x6F80/0x6F88/0x6F90/0x6F98` under `FFF8`, MOD in
  first-word bits 6:3, `0x0C60` under `FEE0`) are declared after LD so
  first-word lookup stays bit-identical LD. The word-2 patterns differ
  only in bit 5, which is the whole discriminator (parity probe).
- Decode seam: `ti_c54x_decode_complete_instruction` re-resolves
  slotted words through a validated `complete_resolved` helper and
  declines unmatched second words with new status
  `SECOND_WORD_UNMATCHED`; `ti_c54x_trusted_execution_packet_prepare`
  re-resolves the same way instead of the first-word table. The fast
  path is untouched by design (private precondition: no re-resolution).
- Leaves: eight `sth_shift_smem_mod0-3` checked/trusted wrappers reuse
  the S13 indirect core; the values core gains a MOD0-MOD3 arm staging
  `SINGLE_INDIRECT_WORD16` WRITEs through the shared MEMORY effect.
  Latent break fixed: the trusted-binding `MASKED2` macro emitted nine
  initializers for the pinned 7-field metadata.
- RED-first: extended `test_qualified_slots` (match/mismatch/parity),
  new `test_qualified_slot_decode`, `test_sth_shift_smem_mod0_3_metadata`
  (pins `ID_COUNT` 251), and `test_sth_shift_smem_mod0_3_ops` (checked,
  trusted, packet, and checked-dispatch equivalence plus transactional
  rejection). Two exhaustive suites needed slot-aware canonical bundles
  (`test_trusted_execution_packet`, `test_trusted_semantic_dispatch`).
  LD vectors unchanged; the LD decode contract note now points at the
  qualified-slot test.
- Registry: `FORM-T2-20-08` advances to `COMMON_CANDIDATE` at `SUBSET`
  with four bindings under the grown
  `DIRECT_DP_STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`
  token; new `E-PROJECT-P3-S18-STH-MOD0-3-ACTIVATION` plus
  `REQ-PROJECT-P3-S18-STH-MOD0-3-INVENTORY` (evidence 423, requirements
  675). Count pins move to 250 rows / 250 bindings / 216 SUBSET. The S18
  plan heading stays `(planned)` as the immutable anchor; the S13
  deferral retires via the superseding S18 record, not an edit.
- Split commit discipline: impl `a74cb5a`, this ledger-only follow-up.
  `make test` green (C suite plus quality gate: evidence PASS,
  inventory, manifests, observer artifact). Next: S18 exit review, then
  the S6 port seam before agent fan-out.

## 2026-09-18 — P3-S6 port I/O seam planning slice

- S18 exit review: all four RED lanes hold (generator slot coverage,
  decode re-resolution with decline, LD regression, MOD0-MOD3 execution
  through the shared MEMORY effect) and FORM-T2-20-08 carries eleven
  bound symbols at COMMON_CANDIDATE/SUBSET with the S13 deferral
  retired by the superseding S18 record. Remaining arch seam: S6 port
  I/O (PORTR 186 + PORTW 322 static words); S1-S5 all read implemented.
- S6 opens as a sealed seam with no catalog rows (S1/W43A pattern):
  TI Table 2-26 plus §4-129/130 give PORTR/PORTW word-1 bases
  0xE8/0xE9 with I-bit Smem cookie and word-2 PA16; the MVDK diagram
  corroborates the shape. Direct cookie (I=0, dma7) is example-proved;
  the indirect cookie (I=1, ARF+MOD) is derived with landed precedent
  and stays vector-gated to the family lane. Kind 18 reuses the
  pair-arm shape; resolver maps to IO space; the generic commit root
  serves the read/write completion chain. No timing claim, no knob.
- RED lanes named: `ti_c54x.port_resolver`,
  `ti_c54x.port_transaction`. Follow-ups: direct family lane,
  indirect Smem lane, FORM-T2-26-09/10 binding with TI evidence
  records. Planning commit carries plan plus index plus this entry.

## 2026-09-18 — P3-S6 lane 1 lands as 6321be6

- Seam core: address kind 18 (`PORT_WORD16`) with a `port` arm shaped
  like the pair arm (indirect select, smem_mode/smem_arf/smem_dma7,
  16-bit port_address, reserved words; union stays 20 bytes, spec
  stays 24) plus the pure `ti_c54x_sequencer_plan_port_access`
  IO-leg planner. DATA-leg access selects the inverse IO leg;
  PORTW WRITE templates carry zero for the lane-2 reducer to replace.
  Single-request resolution cannot serve the mixed-direction pair, so
  the planner (MMR-plan precedent) owns validation plus IO
  construction; the DATA leg resolves in the lane-2 reducer through
  existing single-leg planners.
- RED-first `test_port_resolver`: direct/indirect legs, template
  rule, nine malformed arms plus wrong-kind/argument rejects. No
  registry deltas until the seam exits (S18-lane precedent); the plan
  already names the RESOLVER/TRANSACTION REQs. `make test` green.
- Split commit discipline: impl `6321be6`, this ledger-only
  follow-up. Next is lane 2 (PORT child reducer plus transaction
  RED), then PORTR/PORTW family lanes.

## 2026-09-18 — P3-S6 lane 2 lands as 7ecca10

- PORT pair reducer: `CHILD_PORT_WORD16_PAIR` (5) wired through
  prepare (IO-leg plan plus DATA leg via the single-leg resolver),
  pair dispatch (advance/cancel/reset), shape, liveness, and commit.
  PORTR retires IO READ then DATA WRITE; PORTW retires DATA READ
  then IO WRITE with the observed DATA word replacing the zero
  template. Leg faults report FAULT with no second issue and no
  architectural write; in-flight cancel reaches the endpoint.
- Root causes (all reproduced with throwaway `/tmp` probes before
  fixing): the candidate union aliases `address_resolution` with
  `port_address_snapshot`, so the snapshot now owns the DATA-leg
  `data_resolution` and shape/liveness/commit read it there; the
  WORD16 bus adapter was DATA-only and now admits IO
  (PROGRAM/invalid still rejected); the RED fault script carried a
  stale read value on FAULT and now honors the endpoint contract.
- SIZE split: single-leg resolution shape moves verbatim to
  `memory_candidate_single_leg` (459 lines); PORT pair checks move
  to `memory_instruction_port_candidate_validate` (93 lines); the
  root validator drops 844 to ~350 lines. Direct-live gains a
  resolution-explicit variant with the single-leg wrapper
  preserved. Root grows 360 to 384 LP64 (348 native-32 by
  construction, 352 measured -m32); all C/Python size pins move
  with measured values.
- RED `test_port_memory_transaction` (7 cases): PORTR/PORTW over
  indirect (AR2 MOD2) and direct-DP legs, malformed-arm prepare
  reject, leg-fault atomicity (no retire, PC held), in-flight
  cancel. `make test` green (exit 0, zero failures), observer-on
  objects compile clean.
- Decisions: `memory_instruction_port.c` holds at 512 lines with an
  explicit keep (split when approaching 650 or when PORTW-device
  work lands); no knob added; no timing/IRQ/RPT claim. Follow-ups:
  direct/indirect opcode rows plus handlers, FORM-T2-26-09/10
  registry binding with TI evidence records.
- Split commit discipline: impl `7ecca10`, this ledger-only
  follow-up.

## 2026-09-18 — P3-S6 family rows land as 6de7192 (PORTR) and f3a4731 (PORTW)

- PORTR PA,Smem and PORTW Smem,PA rows over the new two-word
  INDIRECT_ADDR7_PA16 layout (I flag, 7-bit cookie, 16-bit PA):
  PORTR_DIRECT 0x7400/0xFF80 plus PORTR_SMEM 0x7480/0xFF80 and
  PORTW_DIRECT 0x7500/0xFF80 plus PORTW_SMEM 0x7580/0xFF80, with
  plain plus trusted op_portr/op_portw handlers staging PORT arms
  (DATA WRITE for PORTR, DATA READ for PORTW, zero template, DP
  gated at the leaf, MOD9 rejected, crossed-row I mismatch
  declined). RED test_portr_ops/test_portw_ops prove decode
  identity, resources, trusted equivalence, and rejects.
- Evidence correction with primary-source proof: the seam plan
  transcribed PORTR/PORTW at 0xE8/0xE9, but TI SPRU172C 4-129/130
  pin 0x74/0x75 (bit 15 is 0) and the 0xE8 range is dual-sourced
  LD_K. Rows, handlers, and vectors use the TI-true bases; six TI
  opcode/semantics records plus per-mnemonic activation records,
  ARCHITECTURAL plus inventory requirements, and index rows land
  with the lanes. FORM-T2-26-09/10 bind at SUBSET (direct rows
  under DIRECT_DP, Smem rows under the full-indirect token).
- Census pins move twice (250 to 252 to 254 rows, ID count to
  255, lookup 21521 to 21777 to 22033, FORM coverage 94 to 95 to
  96/205); six decode near-miss tests pin the new 0x74xx/0x7500
  siblings under the cross-slice hazard rule. `make test` green
  at both lanes (exit 0, zero failures).
- Decisions: one shared PORT layout and one handler file per
  mnemonic (direct/indirect branch on the I flag; per-row plain
  plus trusted entries satisfy the `ti_c54x_op_##handler`
  consumer contract); no knob added; no timing/IRQ/RPT claim;
  long-offset and absolute Smem extension words stay undecoded.
- Split commit discipline: impls `6de7192` and `f3a4731`, this
  ledger-only follow-up.

## 2026-09-18 — P3-S19 SUB Smem rows (slice 47)

- Commit `cd6cfb9` closes the P3-S14 pilot deferral: SUB Smem
  syntaxes 1-3 land as SUB_SMEM (0x0800/0xFE00), SUB_SMEM_TS
  (0x0C00/0xFE00), and SUB_SMEM_SH16 (0x4000/0xFC00) over the shared
  ADD-Smem layouts, dispatching to the landed plain plus trusted
  op_sub_smem handlers by the catalog X-macro. TI basis is
  dual-sourced in-session from vendor/ti/spru172c.txt (§SUB opcode
  diagrams, pp. 4-186-4-187: 0000_100S, 0000_110S, 0100_00SD, class
  3A/3B), recorded as E-TI-C54X-SPRU172C-S4-SUB-SYNTAX123-OPCODE.
  FORM-T2-02-01/02/03 bind at SUBSET (one form per syntax, mirroring
  ADD 01-01/02/03) under DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3.
- Census pins move with the three rows (257 rows, ID count 258,
  lookup 22033 to 24081 with zero neighbor moves, SUBSET bindings
  220 to 223, FORM coverage 96 to 99/205 at 48.293%, strict-complete
  bound steady at 34). Three near-miss absorptions pin the new
  words: LD-SHIFT16 0x4080/0x4090 and ST-T 0x0C90 as
  SUB_SMEM_SH16/SUB_SMEM_TS siblings. Full `make test` green
  (exit 0): C/Python suites, evidence registry PASS at 431 evidence,
  680 requirements, 546 tests, knobs/boundaries/manifests PASS.
- Decisions: rows-only slice (no handler/layout/decode/CPU change);
  PROJECT rows requirement cites activation plus the two PROJECT
  inventory proofs only (COMMON TI opcode record and COMMON handler
  proof stay out by the scope-leak rule, caught and fixed
  pre-commit); no knob added; syntax 4/Xmem/dual/#lk/accumulator
  syntaxes plus SUBB/SUBC/SUBS stay unimplemented.
- Wave-3 triage result recorded: all arch seams S1-S6 verified
  landed in code; RPT/ST-T/STL candidates eliminated (already
  bound); every remaining empty form needs new TI facts (Smem
  opcode diagrams for AND/OR/XOR rows next) or new seams
  (RPTB/RPTZ, far addressing excluded by the arch criterion).
  Maintainer sources TI facts from vendor/ti/ per slice; delegated
  triage stays timeboxed per the wave-3 stop lesson.
- Follow-up: slices 48-50 (AND/OR/XOR Smem rows on the same
  pattern, one family per commit), then worker fan-out for new
  handler-leaves with manual-sourced TI briefs.
- Split commit discipline: impl `cd6cfb9`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S20 AND Smem row (slice 48)

- Commit `8d77708` lands AND Smem syntax 1 as AND_SMEM
  (0x1800/0xFE00) over SELECT_INDIRECT_ADDR7 with a new plain
  `ti_c54x_op_and_smem` entry (layout-gated 1/1/7 extraction into
  the landed values core) plus the landed trusted entry. TI basis
  dual-sourced from vendor/ti/spru172c.txt (§AND opcode diagram,
  p. 4-11: 0001_100S, class 3A/3B). FORM-T2-07-01 binds at SUBSET
  under DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3.
- Census pins move with the row (258 rows, ID count 259, lookup
  24081 to 24593 with zero neighbor moves, SUBSET bindings 223 to
  224, FORM coverage 99 to 100/205 at 48.780%, strict-complete
  bound steady at 34). No near-miss absorption: a flip sweep over
  decode/catalog suites for 0x1800-0x19FF is empty. Full `make
  test` green (exit 0); evidence registry PASS at 433 evidence,
  681 requirements, 546 tests; focused ASan/UBSan run of the
  extended AND suite clean; knobs/boundaries/manifests PASS.
- Decisions: rows-plus-thin-adapter slice (no layout/decode/CPU
  change); PROJECT rows requirement cites activation plus the two
  PROJECT inventory proofs only (scope-leak rule); S15 handler
  proof stays COMMON-uncited; no knob added; syntaxes 2-4 and
  ANDM stay as bound.
- Process notes: a bulk pin script aborted mid-loop with a
  misleading assert, so all pins were re-audited by direct grep
  to exact targets (rows 258, bindings 258, ID 259, lookup
  24593, covered 100, 48.780, SUBSET 224); prefer audited
  per-file edits over scripted bulk replacement. Cross-slice
  absorption: the S19 activation locator anchor flips to
  implemented here (S14 convention: anchor flips in the
  follow-up, assertion stays PINNED).
- Follow-up: slices 49-50 (OR/XOR Smem rows, same pattern, one
  family per commit).
- Split commit discipline: impl `8d77708`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S21 OR Smem row (slice 49)

- Commit `d1c5e66` lands OR Smem syntax 1 as OR_SMEM (0x1A00/0xFE00)
  over SELECT_INDIRECT_ADDR7 with a new plain `ti_c54x_op_or_smem`
  entry (layout-gated 1/1/7 extraction into the landed values
  core) plus the landed trusted entry. TI basis dual-sourced from
  vendor/ti/spru172c.txt (§OR opcode diagram, p. 4-123:
  0001_101S, class 3A/3B). FORM-T2-08-01 binds at SUBSET under
  DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3.
- Census pins move with the row (259 rows, ID count 260, lookup
  24593 to 25105 with zero neighbor moves, SUBSET bindings 224 to
  225, FORM coverage 100 to 101/205 at 49.268%, strict-complete
  bound steady at 34). One near-miss absorption pins the new
  words: LDU-Smem 0x1A80/0x1A90 as OR_SMEM siblings (decode plus
  metadata twins). Full `make test` green (exit 0); evidence
  registry PASS at 435 evidence, 682 requirements, 546 tests;
  focused ASan/UBSan run of the extended OR suite clean;
  knobs/boundaries/manifests PASS.
- Decisions: rows-plus-thin-adapter slice (no layout/decode/CPU
  change); PROJECT rows requirement cites activation plus the two
  PROJECT inventory proofs only (scope-leak rule); S16 handler
  proof stays COMMON-uncited; no knob added; syntaxes 2-4 and
  ORM stay as bound.
- Process notes: pin updates used assert-counted per-pattern
  replacements after the S20 bulk-script lesson; the
  distinct-symbol count lines needed separate handling from the
  total lines. The flip sweep missed the LDU probes (base spelled
  via a local alias, not a static const); sweep now covers
  #define and local-alias bases. Cross-slice absorptions: S19/S20
  checklist lines flip to implemented here.
- Follow-up: slice 50 (XOR Smem row, same pattern).
- Split commit discipline: impl `d1c5e66`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S22 XOR Smem row (slice 50)

- Commit `bd53076` lands XOR Smem syntax 1 as XOR_SMEM
  (0x1C00/0xFE00) over SELECT_INDIRECT_ADDR7 with a new plain
  `ti_c54x_op_xor_smem` entry (layout-gated 1/1/7 extraction into
  the landed values core) plus the landed trusted entry. TI basis
  dual-sourced from vendor/ti/spru172c.txt (§XOR opcode diagram,
  p. 4-201: 0001_110S, class 3A/3B). FORM-T2-09-01 binds at SUBSET
  under DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3. This closes the
  S15-S17 handler deferrals (SUB/AND/OR/XOR Smem all bound).
- Census pins move with the row (260 rows, ID count 261, lookup
  25105 to 25617 with zero neighbor moves, SUBSET bindings 225 to
  226, FORM coverage 101 to 102/205 at 49.756%, strict-complete
  bound steady at 34). No near-miss absorption: the flip sweep
  for 0x1C00-0x1DFF is empty. Full `make test` green (exit 0);
  evidence registry PASS at 437 evidence, 683 requirements, 546
  tests; focused ASan/UBSan run of the extended XOR suite clean;
  knobs/boundaries/manifests PASS.
- Decisions: rows-plus-thin-adapter slice (no layout/decode/CPU
  change); PROJECT rows requirement cites activation plus the two
  PROJECT inventory proofs only (scope-leak rule); S17 handler
  proof stays COMMON-uncited; no knob added; syntaxes 2-4 and
  XORM stay as bound.
- Process notes: a `pat[:-3]` string-surgery shortcut corrupted
  nine pin lines (`len\(...\), 25260)` literals); repaired line
  by line with exact-match edits and re-audited. Rule restated:
  no programmatic string surgery on pin literals — per-file
  assert-counted edits plus grep audit only.
- Follow-up: SUB syntax-4/Xmem/dual/#lk tails, SUBB/SUBC/SUBS,
  and ADDC/ADDM/ADDS need TI section facts; worker fan-out for
  handler-leaves resumes with manual-sourced TI briefs.
- Split commit discipline: impl `bd53076`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S23/S24 SUBS/SUBB Smem leaves (slices 51-52, wave-4 fan-out)

- Commit `c4afcc6` merges the first managed handler fan-out: two
  isolated-worktree workers delivered op_subs_smem (FORM-T2-02-13)
  and op_subb_smem (FORM-T2-02-11) with plain plus trusted entries
  over SELECT_INDIRECT_ADDR7, 12 and 11 cases on TI worked-example
  vectors (SUBS p. 4-194 unsigned rule, SUBB p. 4-191 borrow
  rule). Maintainer review found both faithful (borrow folded
  into the 40-bit subtrahend, C from the full-lane comparison,
  SUB-style OVM saturation); fixed a `mode4` contract typo in
  both files, rebuilt and re-verified in-tree, both binaries
  green plus focused ASan/UBSan runs clean.
- Fan-out mechanics that worked: bounded ownership (3 new files
  each plus append-only wiring), no commits by workers, TI-true
  briefs with exact opcodes/vectors sourced by the maintainer
  from vendor/ti/, one family per worker. One collision: both
  workers appended wiring at the same anchor lines, so the
  second merge needed manual wiring (atomic git-apply rejected
  the whole patch). Lesson: stagger wiring anchors per worker
  (e.g. alphabetical placement) or serialize wiring in the
  maintainer merge.
- No catalog rows, dispatch, decode, or CPU change; both forms
  stay UNIMPLEMENTED. Evidence: 2 TI section records, 2 plan
  anchors, 2 ARCHITECTURAL semantics requirements, 2 COMMON test
  records with index rows. Full `make test` green (exit 0);
  registry PASS at 441 evidence, 685 requirements, 548 tests.
- Follow-up: SUBB/SUBS rows plus bindings (opcodes pinned:
  SUBB 0000_111D 0x0E00/FE00, SUBS 0000_101S 0x0A00/FE00);
  wave-5 (#lk immediates, Xmem, src/ASM) with manual briefs.
- Split commit discipline: impl `c4afcc6`, this ledger-only
  follow-up. Slice docs born `(planned)` in the impl commit,
  flipped here with their anchors.

## 2026-09-18 — P3-S25 SUBB Smem row (slice 53)

- Commit `dfa5622` lands SUBB Smem as SUBB_SMEM (0x0E00/0xFE00)
  over SELECT_INDIRECT_ADDR7, wiring the landed P3-S24 plain plus
  trusted `op_subb_smem` handlers through the catalog X-macro
  (`ti_c54x_op_##handler` binding; dispatch include only, no
  handler/layout/decode/CPU change). TI basis from
  vendor/ti/spru172c.txt (§SUBB opcode diagram, p. 4-191:
  0000_111D, class 3A/3B). New `ACCUMULATORS_STATUS_C_OVM` input
  combination (accumulators plus C plus OVM, no SXM) registered in
  the catalog header and schema allowlist. FORM-T2-02-11 binds at
  SUBSET under DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3.
- Census pins move with the row (261 rows, ID count 262, lookup
  25617 to 26129 as 512 insertions with zero neighbor moves,
  SUBSET bindings 226 to 227, FORM coverage 102 to 103/205 at
  50.244%, strict-complete bound steady at 34). Full `make test`
  green (exit 0); evidence registry PASS at 686 requirements, 548
  tests; focused contract modules (catalog, lookup generator,
  add/sub plus catalog binding) 33 tests OK; knobs, boundaries,
  manifests, and observer-artifact gates PASS inside `make test`.
- Decisions: row-plus-registered-token slice mirroring the
  AND/OR/XOR row pattern; PROJECT rows requirement cites
  activation plus the two PROJECT inventory proofs only
  (scope-leak rule); no knob added; long-offset and absolute Smem
  extension words stay undecoded.
- Process notes: a `pat[:-4]` string-surgery shortcut corrupted
  pin lines (`len\(...\),2261` literals) on first edit; repaired
  with explicit per-file replace plus exact-match edits and a
  diff-shape audit (56 ID-count plus 7 lookup lines, no other
  churn). Rule restated: no programmatic string surgery on pin
  literals — per-file assert-counted edits plus grep audit only.
- Follow-up: slice 54 SUBS Smem row plus FORM-T2-02-13, then
  wave-5 (#lk immediates, Xmem, src/ASM) with manual briefs.
- Split commit discipline: impl `dfa5622`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S26 SUBS Smem row (slice 54)

- Commit `d9ef620` lands SUBS Smem as SUBS_SMEM (0x0A00/0xFE00)
  over SELECT_INDIRECT_ADDR7, wiring the landed P3-S23 plain plus
  trusted `op_subs_smem` handlers through the catalog X-macro
  (`ti_c54x_op_##handler` binding; dispatch include only, no
  handler/layout/decode/CPU change). TI basis from
  vendor/ti/spru172c.txt (§SUBS opcode diagram, p. 4-194:
  0000_101S, class 3A/3B). New `ACCUMULATORS_STATUS_OVM` input
  combination (accumulators plus OVM) registered in the catalog
  header and schema allowlist. FORM-T2-02-13 binds at SUBSET
  under DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3.
- Census pins move with the row (262 rows, ID count 263, lookup
  26129 to 26641 as 512 insertions with zero neighbor moves,
  SUBSET bindings 227 to 228, FORM coverage 103 to 104/205 at
  50.732%, strict-complete bound steady at 34). Full `make test`
  green (exit 0, 105 C suites ok); evidence registry PASS at 687
  requirements, 548 tests; focused contract modules (catalog,
  lookup generator, add/sub plus catalog binding) 33 tests OK;
  knobs, boundaries, manifests, and observer-artifact gates PASS
  inside `make test`.
- Decisions: row-plus-registered-token slice; the C bit stays out
  because SUBS writes C but never reads it (SUB Smem precedent —
  reusing the C_OVM token would have falsely claimed a C input);
  SXM stays out because Smem is zero-extended regardless of SXM.
  PROJECT rows requirement cites activation plus the two PROJECT
  inventory proofs only (scope-leak rule); no knob added;
  long-offset and absolute Smem extension words stay undecoded.
- Process notes: numeric pin bumps via assert-counted `sed` plus
  diff-shape audit only (no string surgery on literals); one
  missed SUBSET-count pin (227) caught by the focused contract
  run before the full suite. Rule restated: grep-audit every
  census literal family (rows, IDs, lookup, SUBSET, FORMs) after
  each bump.
- Follow-up: wave-5 (#lk immediates, Xmem, src/ASM) with manual
  briefs.
- Split commit discipline: impl `d9ef620`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-WAVE5 ADD/SUB leaf fan-out (second managed fan-out)

- Commit `c4b5b21` merges six catalog-free ADD/SUB handler leaves
  from three isolated-worktree workers: op_add_lk/op_sub_lk
  (syntaxes 7-8), op_add_xmem/op_sub_xmem (syntax 5 over the
  0x9000/0x9200 fixed prefixes), op_add_src_asm/op_sub_src_asm
  (syntax 10 over 0xF480/0xF481), each plain plus trusted with
  SXM-widened shifts, OVM saturation, C written, TC preserved.
  TI basis dual-pinned per family (§ADD pp. 4-3–4-4, §SUB
  pp. 4-186–4-187); syn7 worked examples as vectors. Review
  found all three faithful; worker B even documented the
  0x9200/0x9300 S-encoding relationship correctly. 74 cases
  across six suites (12/12/12/12/13/13; the impl message says 72
  — corrected here), ASan/UBSan clean, full `make test` green
  from clean (exit 0). Evidence: 3 TI section records plus this
  activation, 6 ARCHITECTURAL requirements, 6 COMMON test records
  (447 evidence, 693 requirements, 554 tests).
- Fan-out mechanics: triage agent first (8 LEAF-NEEDED forms in
  3 disjoint families, nothing BLOCKED), then three workers with
  maintainer-sourced briefs carrying exact opcode words and TI
  vectors. Two merges collided (B/C context drift after A on the
  shared manifests); maintainer applied end-of-list plus
  relative-anchor placement instead. Lesson restated: shared
  wiring files serialize at merge even with staggered anchors —
  next time hand each worker a disjoint wiring file or merge
  strictly A-B-C with rebase between workers.
- Gate failures fixed in-tree: ARCHITECTURAL requirements must
  cite COMMON_TI evidence only (activation record dropped from
  the six evidence lists, index rows realigned). Lesson: mirror
  wave-4 link shapes exactly (TI-only evidence on ARCHITECTURAL
  requirements).
- No catalog rows, dispatch, decode, or CPU change; all eight
  forms stay UNIMPLEMENTED. Follow-up: rows slices for the eight
  #lk/Xmem/src-ASM forms, then the triage-adjacent tails (ADD/SUB
  Smem[,SHIFT] ord4, ADDM #lk, MPY/MAC #lk/dual).
- Split commit discipline: impl `c4b5b21`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S27 ADD/SUB #lk rows (slice 55)

- Commit `60e8888` lands ADD_LK_SHIFT (0xF000/0xFCF0), ADD_LK_16
  (0xF060/0xFCFF), SUB_LK_SHIFT (0xF010/0xFCF0), and SUB_LK_16
  (0xF061/0xFCFF) over the shared 2-word accumulator layouts.
  The schema forbids shared handler tokens, so eight thin
  per-syntax checked entries (plain layout-gated plus trusted
  operand-count-gated) delegate to the landed wave-5 cores,
  mirroring the Smem _TS/_SH16 precedent; four focused wrapper
  tests join the suites. Census: 266 rows, ID count 267, lookup
  26641 to 26777 as 136 insertions with zero neighbor moves,
  SUBSET bindings 228 to 232, FORMs 104 to 108/205 at 52.683%.
  Full `make test` green (exit 0); evidence PASS at 448 evidence,
  694 requirements, 554 tests.
- Decisions: forms bind at SUBSET under IMMEDIATE_LK (the RPT_LK
  precedent; the schema forbids SUBSET bindings at ALL). Seven
  decode flip sweeps absorb nearby words by design
  (ld_lk16/ld_lk_shift/bc_pmad/bcd/cc/ccd/rpt_lk); each gains an
  explicit valid-row sibling pin per the file-local convention
  instead of weakening the sweep. New house rule from this
  slice: after adding rows, sweep ALL unit binaries directly
  (seconds) before the slow full suite — it found four of the
  seven absorptions in one pass.
- Near-miss worth recording: the SUB syn7 TI vector comment
  reads ambiguously until you decode 0xF118 as S=0/D=1 (src A,
  dst B); verified by hand against the manual before trusting
  the green suite. Rule restated: a passing suite never retires
  TI-vector doubt — recompute the vector by hand.
- Follow-up: S28/S29 Xmem/src-ASM rows, then wave-6 MPY/MAC
  leaves merge (workers report green).
- Split commit discipline: impl `60e8888`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-WAVE6 MPY/MAC #lk leaf fan-out (third managed fan-out)

- Commit `5607c65` merges op_mpy_lk (MPY syntax 4, 0xF066|D<<8)
  and op_mac_lk (MAC syntax 3, 0xF067|S<<9|D<<8) from two
  isolated-worktree workers, each plain plus trusted through
  the P3-S3 multiply datapath. Review found both faithful
  (unrounded, T read-only, FRCT/OVM, OVsrc-when-dst==src for
  MAC, C/TC preserved). 26 cases across two suites (12+14; the
  impl message says 24 — corrected here), ASan/UBSan clean,
  full `make test` green from clean (exit 0). Evidence: 2 TI
  section records plus this activation, 2 ARCHITECTURAL
  requirements, 2 COMMON test records (451 evidence, 696
  requirements, 556 tests).
- Fan-out mechanics keep improving: staggered anchors (tails
  for MPY, heads for MAC) applied with zero merge fixups — the
  wave-5 lesson holds. One maintainer normalization: the MAC
  test target arrived as `mac_lk` and was renamed to `mac_lk_ops`
  at merge for the house convention. Next briefs will pin the
  `_ops` target name explicitly.
- No catalog rows, dispatch, decode, or CPU change; both forms
  stay UNIMPLEMENTED. Follow-up: MPY/MAC #lk rows slices, then
  the seam-blocked tails (syn4 Smem-SHIFT, ADDM, syn3/4 Smem-lk
  need layout/RMW seams).
- Split commit discipline: impl `5607c65`, this ledger-only
  follow-up. Slice doc born `(planned)` in the impl commit,
  flipped here with its anchor.

## 2026-09-18 — P3-S28 ADD/SUB Xmem rows (slice 56)

- Commit `134633e` lands ADD_XMEM (0x9000/0xFE00) and SUB_XMEM
  (0x9200/0xFE00) over the shared accumulator/dual-XMOD/XAR/
  shift-4 layout, wiring the landed wave-5 plain plus trusted
  op_add_xmem/op_sub_xmem cores through the catalog X-macro
  (two dispatch includes, no new handlers). Census: 268 rows,
  ID count 269, lookup 26777 to 27801 as 1024 insertions with
  zero neighbor moves, COMPLETE bindings 34 to 36, FORMs 108
  to 110/205 at 53.659%, strict-complete bound 34 to 36.
  Full `make test` green (exit 0); evidence PASS at 452
  evidence, 697 requirements, 556 tests; sanitizer spot
  rebuild of dispatch plus catalog clean on the xmem/decode/
  metadata binaries.
- Decisions: forms bind at COMPLETE under ALL (the
  LD_XMEM_SHIFT precedent; the landed leaves cover all four
  XMOD modes) and flip UNRESOLVED to COMMON_CANDIDATE with
  the rows. Input resources follow the LD_XMEM_SHIFT sibling
  (AUXILIARY_REGISTERS_STATUS_SXM_OVM). Two decode flip
  sweeps absorb nearby words by design (ldu_smem bit-15 ->
  SUB_XMEM at the lookup and decode levels, ld_xmem_shift
  bit-10 -> ADD_XMEM); each gains an explicit valid-row
  sibling pin per the file-local convention instead of
  weakening the sweep.
- Near-miss worth recording: the add/sub binding test asserts
  COMMON_CANDIDATE scope on every tabled form, so binding to
  an UNRESOLVED form fails closed — the S27 scope-flip-with-
  rows precedent applies, not a test weakening. A second
  near-miss: several family suites pin the strict-complete
  percent (16.585) and the rendered provisional line
  (108/205, 34/205), not just the family percent — sweep all
  three pin shapes after any COMPLETE-binding slice.
- Follow-up: S29 src-ASM rows, then MPY/MAC #lk rows
  (FORM-T2-03-05/FORM-T2-04-02), then the seam-blocked tails.
- Split commit discipline: impl `134633e`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor, matching the
  landed state; plan link stays `(planned)` per the S27/WAVE6
  convention.

## 2026-09-18 — P3-S29 ADD/SUB src-ASM rows (slice 57)

- Commit `6228ccf` lands ADD_SRC_ASM (0xF480/0xFCFF) and
  SUB_SRC_ASM (0xF481/0xFCFF) over the shared accumulator
  source/destination layout, wiring the landed wave-5 plain
  plus trusted op_add_src_asm/op_sub_src_asm cores through
  the catalog X-macro (two dispatch includes, no new
  handlers). Census: 270 rows, ID count 271, lookup 27801 to
  27809 as 8 insertions with zero neighbor moves, COMPLETE
  bindings 36 to 38, FORMs 110 to 112/205 at 54.634%,
  strict-complete bound 36 to 38. Full `make test` green
  (exit 0); evidence PASS at 453 evidence, 698 requirements,
  556 tests; sanitizer spot rebuild of dispatch clean on the
  src-asm ops/decode binaries.
- Decisions: forms bind at COMPLETE under ALL (the LD_SRC_ASM
  precedent; the leaves cover all four S/D combinations) and
  flip UNRESOLVED to COMMON_CANDIDATE with the rows. Four
  decode flip sweeps absorb nearby words by design
  (ld_status_fields, ld_src_asm, maca_t, st_immediate at
  lookup level); each gains an explicit valid-row sibling pin
  per the file-local convention instead of weakening the
  sweep.
- Near-miss worth recording: the 0xF48x cluster is dense —
  LD_SRC_ASM (0xF482), MAX/MIN/EXP (0xF486/87/8E), and the
  new rows sit within a few flips of each other, so every
  neighbor sweep in that band must be re-run, not just the
  obvious LD_SRC_ASM one. The st_immediate bit-15 case shows
  mask-clearing flips (0xF680 -> 0xF480 under 0xFCFF) land
  farther than they read — compute the masked value by hand
  before trusting a sweep list.
- Follow-up: MPY/MAC #lk rows (FORM-T2-03-05/FORM-T2-04-02),
  then the seam-blocked tails.
- Split commit discipline: impl `6228ccf`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S30 MPY/MAC #lk rows (slice 58)

- Commit `3bb17fc` lands MPY_LK (0xF066/0xFEFF) and MAC_LK
  (0xF067/0xFCFF) over the shared 2-word accumulator layouts,
  wiring the landed wave-6 plain plus trusted op_mpy_lk/
  op_mac_lk cores through the catalog X-macro (two dispatch
  includes, no new handlers). Census: 272 rows, ID count 273,
  lookup 27809 to 27815 as 6 insertions with zero neighbor
  moves, COMPLETE bindings 38 to 40, FORMs 112 to 114/205 at
  55.610%, strict-complete bound 38 to 40. Full `make test`
  green (exit 0); evidence PASS at 454 evidence, 699
  requirements, 556 tests; sanitizer spot rebuild of dispatch
  clean on the mpy/mac-lk ops/decode binaries.
- Decisions: forms bind at COMPLETE under ALL. Verified
  against the vendored TI opcode tables (not the wave-6 test
  comment): MPY syntax 4 (`1111 00 0 D 0 110 0110`) and MAC
  syntax 3 (`1111 00 S D 0 110 0111`) carry no R bit — both
  are unrounded-only encodings — so the unrounded leaves
  cover them fully. The requirement links only the
  PROJECT_CONTRACT binding suite; the scope gate rejects
  linking the COMMON_TI wave-6 semantics suites to a PROJECT
  requirement even though those suites execute the rows'
  semantics. One decode flip sweep absorbs a nearby word by
  design (ld_lk16 bit-2 -> MPY_LK); explicit sibling pin.
- Near-miss worth recording: the wave-6 mpy test comment
  mislabels 0xF067 as the "MPYR #lk rounded base" — per TI
  it is the MAC syn3 base, and no MPYR #lk encoding exists
  (syntax 4 is MPY-only). The rejection itself is correct;
  only the comment's rationale is wrong. Left as-is (test
  intent is clear); revisit if that file is touched.
- Follow-up: the seam-blocked tails (syn4 Smem-SHIFT, ADDM,
  syn3/4 Smem-lk need layout/RMW seams) — the arch-seam
  phase; leaves after.
- Split commit discipline: impl `3bb17fc`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S31 ADD/SUB Smem SHIFT rows with layout seam (slice 59)

- Commit `7912b1d` lands Seam 1 end to end: the shared two-word
  layout INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5, catalog-free
  op_add_smem_shift/op_sub_smem_shift leaves with plain plus
  trusted entries, MASKED2 rows ADD_SMEM_SHIFT and SUB_SMEM_SHIFT
  (0x6F00/0xFF00 with 0x0C00/0x0C20 word-1 patterns), seven STH
  shift rows converted MASKED to MASKED2 (0x0C60/0xFEE0), schema
  layout-token registration, bindings SUBSET, and forms
  FORM-T2-01-04/FORM-T2-02-04 at 116/205 (56.585%). Full `make
  test` green (exit 0); evidence PASS at 456 evidence, 702
  requirements, 558 tests; sanitizer spot rebuild clean. The
  commit also refreshes the README status section past the S3s
  era at the user's request (P3-S30 snapshot, S3t-S31 coverage,
  remaining-work list).
- Decisions: Smem cookie rides ONE 8-bit operand (two pieces)
  because trusted packets cap at four operands — the first
  layout to need the merge. STH conversions carry the TI-pinned
  0x0C60/0xFEE0 pattern with no handler change (completion
  already rejects bad second words wherever a slot exists).
  Syn4 rows sit after every 0x6Fxx row so first-match keeps
  LD/STH first-word identities (the file's own convention).
- Near-misses worth recording: (1) the word-0 mask must be
  0xFF00, not 0xFF80 — the I bit is free, and 0xFF80 silently
  excludes every indirect word (caught by the MOD0 unit lane);
  (2) the schema has its own layout-token registry, so a new
  layout fails generation until the token lands there; (3) the
  qualified-slots fragment needs the opt-in --qualified-output
  flag — a bare regenerate leaves it stale; (4) the evidence
  index TI locator needs the §-section backtick-page form, not
  prose; (5) shared-first-word rows only surface in the
  trusted-packet sweep, so probe every binding, not just the
  new rows.
- Follow-up: Seam 2 (ADDM RMW-ADD: carrier op + reducer + C/OVA
  status design decision), Seam 3 (MPY syn3 / MAC syn4 layouts
  with D/S+D + Smem + lk), then leaf fan-out.
- Split commit discipline: impl `7912b1d`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S32 ADDM RMW-ADD seam (slice 60)

- Commit `c1579be` lands Seam 2 end to end: the RMW_OPERATION_ADD
  carrier value (lk addend in value_word, OVM/SXM in the frozen
  input snapshot, carrier size unchanged), the value-owned 16-bit
  reducer rmw_transform_add (wrapped sum, bit-16 carry,
  SXM-selected overflow, OVM saturation 0x7FFF/0x8000 signed and
  0xFFFF unsigned), sequencer flag staging in the root scalar
  padding (root size unchanged on LP64/ILP32), sticky C/OVA
  commit shared by the bus and MMR RMW-WRITE paths,
  op_addm_smem_mod0_3 leaf with plain plus trusted entries and
  OVM/SXM knownness gating, MASKED row ADDM_SMEM_MOD0_3
  (0x6B80/0xFFE0, TI SPRU172C section ADDM), regenerated
  first-word lookup (+32 encodings, zero neighbor moves), and
  FORM-T2-01-12 bound at SUBSET with the row (covered 117/205,
  57.073%). Full `make test` green (exit 0); evidence PASS at
  457 evidence, 705 requirements, 562 tests; sanitizer rebuild
  clean; generator --check and diff --check clean. The commit
  also refreshes the README snapshot to P3-S32 with current
  counts (275 rows, 27,895 entries).
- Decisions: OVM/SXM travel outside the carrier in the
  already-copied frozen input snapshot (no ABI change, no
  transfer_reserved repurposing); the advance dispatch helper
  lives in memory_instruction_rmw.c (its owning module), which
  also keeps memory_instruction_transaction.c under the 800-line
  split limit (786 after the move). Sticky-set C/OVA mirrors
  the accumulator ADD precedent (WRITE_1 on the frozen flags,
  otherwise preserved), staged on a status candidate so a
  rejected write leaves live status untouched. SXM=0 overflow
  is the bit-16 carry and OVM=1 saturates it to 0xFFFF, derived
  from the unconditional "Affected by OVM" line plus the
  accumulator saturation precedent. There is no SUBM in the TI
  source (zero hits in SPRU172C), so the seam covers ADDM only;
  earlier ledger "ADDM/SUBM" phrasing is corrected here, not
  rewritten there.
- Near-misses worth recording: (1) the transaction test
  endpoint must zero read_value_word on WRITE observations or
  the adapter reports MALFORMED_OBSERVATION (caught by the
  first commit-vector run); (2) the evidence OWNER policy
  requires sequencer-path tests under a sequencer-owned
  requirement, so REQ-COMMON-RMW-ADD-TRANSACTION split off from
  the leaf EFFECTS requirement; (3) the frozen SUBSET symbol
  set in test_inventory_catalog_binding.py must gain each new
  symbol explicitly — counts alone do not cover it; (4) the
  no-space `==275` spelling in the reverse-carry metadata file
  needs its own sed pattern.
- Follow-up: Seam 3 (MPY syn3 / MAC syn4 layouts with D/S+D +
  Smem + lk), then managed leaf fan-out through agent worktrees.
- Split commit discipline: impl `c1579be`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S33 MPY syn3 / MAC syn4 Smem-lk rows with layout seam (slice 61)

- Commit `b8f4fc7` lands Seam 3 end to end: the shared two-word
  layouts DESTINATION_INDIRECT_ADDR7_EXTENSION16 (D/I/addr7 +
  lk) and SOURCE_DESTINATION_INDIRECT_ADDR7_EXTENSION16
  (S/D/Smem-cookie + lk, the cookie keeping trusted packets at
  four operands), op_mpy_smem_lk (STORE completion) and
  op_mac_smem_lk (ADD completion with S/D split knownness)
  leaves with plain plus trusted entries and read-phase T load
  through the multiply core, MASKED rows MPY_SMEM_LK
  (0x6200/0xFE00, TI SPRU172C section MPY syntax 3) and
  MAC_SMEM_LK (0x6400/0xFC00, section MAC syntax 4), both
  ELIGIBLE with STATUS_FRCT_OVM and
  ACCUMULATORS_STATUS_FRCT_OVM inputs, regenerated first-word
  lookup (+1,536 encodings, zero neighbor moves), and
  FORM-T2-03-04/FORM-T2-04-03 bound at SUBSET with the rows
  (covered 119/205, 58.049%). Full `make test` green (exit 0);
  evidence PASS at 459 evidence, 711 requirements, 568 tests;
  sanitizer rebuild clean; generator --check and diff --check
  clean. The commit also refreshes the README snapshot to
  P3-S33 with current counts (277 rows, 29,431 entries).
- Decisions: Table 6-18 omits every MPY/MAC form, so both rows
  are ELIGIBLE (the MAC_LK/MPY_LK UNRESOLVED rows are a
  separate conservative choice for T-operand forms, not a
  precedent for Smem-reading forms). TI pins no R bit in
  either opcode space, so the unrounded leaves cover both rows
  fully. The new sets split into
  operand_layout_sets_smem_lk.c (owned file) to keep
  operand_layout_sets.c under the 800-line limit (791 after
  the move). New STATUS_FRCT_OVM combo added because no honest
  existing macro covers FRCT/OVM without accumulators (T is
  written, never read; Smem arrives on the bus).
- Near-misses worth recording: (1) a 40-bit test literal needs
  10 hex digits — 0xFFFFFFFFFFFE is 48 bits, the -2 product is
  0xFFFFFFFFFE (caught by the first MPY syn3 run); (2) four
  established decode/metadata tests pin fixed-bit flips as
  UNKNOWN that the new rows legitimately claim (LD_SMEM_SHIFT16
  0x2000, BANZ 0x0800, BANZD 0x0800, ST_IMMEDIATE 0x1000, STM
  0x1000, plus ST_IMMEDIATE metadata 0x1000) — each converted
  to a pinned cross-row sibling assertion, never deleted;
  sweeping every test binary catches them all in one pass;
  (3) sibling bindings need DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3,
  not the narrower indirect-only token.
- Follow-up: all arch seams are now complete (Seam 1 syn4
  SHIFT, Seam 2 RMW-ADD, Seam 3 Smem-lk); next is managed leaf
  fan-out through agent worktrees.
- Split commit discipline: impl `b8f4fc7`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S34 RND/SAT accumulator rows with WRITE_0 seam (slice 62)

- Landed: worker-built W7A pilot leaves op_rnd_acc (dst40 = src40
  plus 8000h, 40-bit wrap, OVM-gated saturation, sticky OVdst,
  preserved C/TC) and op_sat_acc (src40 clamp to signed 32 bits
  regardless of OVM, OVsrc recomputed) with checked plus trusted
  entries; shared-contract WRITE_0 recompute-to-clear
  destination-overflow action with sequencer commit branch;
  RND_ACC (0xF49F/0xFCFF, INELIGIBLE, ACCUMULATORS_STATUS_OVM)
  and SAT_ACC (0xF483/0xFEFF, UNRESOLVED, ACCUMULATORS) rows;
  regenerated first-word lookup (+6 encodings, zero neighbor
  moves); FORM-T2-06-13/FORM-T2-06-14 bound at SUBSET under
  REGISTER_DST with the rows (covered 121/205, 59.024%). Full
  `make test` green (exit 0); contract suite 284 tests OK;
  evidence PASS at 461 evidence, 718 requirements, 574 tests;
  sanitizer rebuild plus targeted runs clean. README snapshot
  refreshed to P3-S34 (279 rows, 29,437 entries).
- Decisions: SAT stays UNRESOLVED (omission from Tables 2-28/6-18
  never implies eligibility) matching its MAX/MIN/EXP/CMPL
  accumulator-unary neighbors; RND is INELIGIBLE by explicit
  section note plus Table 2-28 listing. SAT Example 3 (OVB x→0)
  forced the WRITE_0 vocabulary extension (appended as value 2
  to avoid renumbering) instead of PRESERVE. Pilot row-less
  UNKNOWN id checks graduated to real row ids at merge; pilot
  uninitialized stack `staged` structs fixed to `= {0}` after
  the trusted/checked parity sweep caught padding divergence.
- Near-misses worth recording: (1) SAT mask 0xFEFF covers
  {0xF483,0xF583}, not {0xF482,0xF483} — no overlap with
  LD_SRC_ASM, verified in the generator output; (2) two
  established decode tests pinned fixed-bit flips as UNKNOWN
  that the new rows claim (LD_SRC_ASM 0xF483, RETF 0xF49F) —
  each converted to a pinned cross-row sibling assertion;
  (3) MACAR T,src / MASAR T,src already ride the MACA_T/MASA_T
  R-bit rows (row comments say so) — future COVERED bindings,
  not new rows; MACR/MASR Xmem,Ymem are dual-memory and need
  the deferred dual seam.
- Follow-up: pre-existing branch leaves share the
  uninitialized-`staged` pattern (passing by luck of identical
  stack layout); candidate hygiene sweep in a later slice.
  Remaining census 84 UNIMPLEMENTED; next is the COVERED
  rounded-variant bindings plus continued Wave-7 fan-out.
- Split commit discipline: impl `b9a5e75`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S35 rounded-variant shared-row bindings (slice 63)

- Landed: six R-suffixed forms bound to established rows with no
  catalog, decode, or leaf change (MPYR Smem→MPY_SMEM, MACR
  Smem→MAC_SMEM, MACAR Smem→MACA_SMEM, MACAR T→MACA_T, MASR
  Smem→MAS_SMEM, MASAR T→MASA_T), each SUBSET under its row's
  addressing token, flipping UNRESOLVED/UNIMPLEMENTED to
  COMMON_CANDIDATE/SUBSET (covered 127/205, 61.951%). Minimal
  schema evolution: symbol-uniqueness retired for exact-pair
  uniqueness, with positive plus negative schema tests. Full
  `make test` green (exit 0); contract suite 286 tests OK;
  sanitizer rebuild clean; requirements/tests unchanged at 718
  and 574. README snapshot refreshed to P3-S35.
- Decisions: every R bit pinned against its TI [R]-section
  diagram (MPY 4-101, MAC 4-82, MACA 4-85, MAS 4-94, MASA
  4-97); MASA_T/MACA_T carry R at bit 0, the Smem rows at bit
  9. No new evidence/requirement/test records: row CATALOG,
  DECODE, and EFFECTS records plus existing rounding suites
  already cover the behavior. Dual-memory MACR/MASR Xmem,Ymem
  stay deferred to the dual seam. Branch-leaf uninitialized
  `staged` hygiene noted in S34 stands.
- Near-misses worth recording: schema v1's one-binding-per-
  symbol rule initially rejected the shared rows (6 checker
  errors); the report already counted distinct bound forms, so
  the relaxation cannot inflate the denominator (alias test
  still guards). Reverse-carry coverage assertion used the
  compact `{"covered":121` spelling the sweep missed — fixed
  by direct edit.
- Follow-up: remaining census 78 UNIMPLEMENTED; next is
  continued Wave-7 leaf fan-out per the fan-out plan.
- Split commit discipline: impl `ac9ed63`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S36 Smem-lk test/store layout seam (slice 64)

- Landed: shared two-word `INDIRECT_ADDR7_EXTENSION16` layout
  (I/A from word 0, copied lk from word 1) as the last layout
  id with plan and resolve wiring plus a field-extraction
  proof suite. No rows yet. Full `make test` green (exit 0).
- Decisions: three plain operands (no Smem cookie — only the
  MAC form needs the packet-saving cookie); T neither read nor
  written per the BITF/CMPM/ST sections. Leaves land via
  managed fan-out on this base.
- Follow-up: BITF/CMPM/BITT/ST-lk rows plus leaves (S36
  completion); BIT Xmem dual form stays deferred.

## 2026-09-18 — P3-S36 BITF/CMPM/BITT/ST rows completion (slice 65)

- Landed: four TI-pinned rows on the S36 seam — BITF
  `0x6100/0xFF00`, CMPM `0x6000/0xFF00`, BITT `0x3400/0xFF00`,
  direct-only ST `0x7600/0xFF80` under `ST_IMMEDIATE_SMEM`
  (`0x7680/0xFFE0`) — with leaves, dispatch, metadata/decode/effects
  suites, inventory flips (`FORM-T2-11-02/03/04`, `FORM-T2-20-03`
  to `COMMON_CANDIDATE`/`SUBSET`), 12 evidence records, index rows,
  README/plan/slice updates, and frozen-count bumps (283 rows, 289
  bindings, 30,333 lookup, 249 SUBSET, 131/205 at 63.902%, 730
  requirements, 586 tests).
- Validation: `make build` clean, `make test` exit 0 (105 C binaries
  ok, 286 Python tests), `check_evidence.py` PASS (730/586),
  `check_isa_inventory.py` exit 0 (131/205, strict 40/205),
  lookup regen pure (+896), `TI_C54X_ENABLE_SANITIZERS=1 make build`
  exit 0.
- Decisions: new rows legitimately claim five old negative probes
  (`0x3490` BITT, `0x7610`/`0x7600` ST direct) — converted to pinned
  cross-row siblings per file convention instead of narrowing masks.
  Slice draft slot hexes corrected to the TI-pinned values before
  landing (BITF `0x61xx`, CMPM `0x60xx`, BITT `0x34xx`, ST `0x76xx`
  per SPRU172C diagrams); ST leaf header contract updated from
  row-less/UNKNOWN to the wired row id.
- Knobs: none added, changed, or retired.
- Near-misses: a placeholder `s/29437U/29433X/` sed touched six
  lookup-count pins — repaired to `30333U` and verified zero
  `29437` residue outside stale build binaries; contract
  `test_inventory_catalog_binding.py` needed the four symbols in
  both the frozen SUBSET set and the addressing-subset chain.
- Follow-up: remaining census 74 unbound forms; next is continued
  Wave-7 leaf fan-out per the fan-out plan
  (`docs/plan/p3-wave7-leaf-fanout-plan.md`, still untracked).
- Split commit discipline: impl `1f1bd2e`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)` per convention.

## 2026-09-18 — P3-S37 W7 pilot ABS/NEG/NORM accumulator rows (slice 66)

- Landed: first managed fan-out — three TI-pinned rows (ABS
  `0xF485/0xFCFF`, NEG `0xF484/0xFCFF`, NORM `0xF48F/0xFCFF`)
  on the existing accumulator source/destination layout, all
  `UNRESOLVED` (Tables 2-28/6-18 omit them; SAT precedent).
  Leaves built row-less by isolated workers (W7-ABS/W7-NEG/W7-NORM)
  against maintainer TI briefs from SPRU172C/SPRU131G, merged with
  row-id flips, dispatch includes, manifests, 6 row metadata/decode
  suites, inventory flips (`FORM-T2-06-02/10/11` to
  `COMMON_CANDIDATE`/`SUBSET` + `REGISTER_DST` bindings), 21
  evidence records (3 TI + 9 req + 9 test), index rows, frozen-count
  bumps (286 rows, 292 bindings, 30,345 lookup, 252 SUBSET,
  134/205 at 65.366%, 739 requirements, 595 tests), README/plan/slice.
- Validation: `make build` clean, `make test` exit 0 (105 C binaries
  ok, 286 Python tests), `check_evidence.py` PASS (739/595),
  `check_isa_inventory.py` exit 0 (134/205, strict 40/205),
  lookup regen pure (+12), `TI_C54X_ENABLE_SANITIZERS=1 make build`
  exit 0.
- Decisions: NEG needed the general signed-32 saturation rule, not
  just the minimum edge — the TI prose edge (`0xFF_8000_0000` to
  `0x00_8000_0000`) overflows too; applied at merge with a dedicated
  test. New words collided with five old probes (`0xF486` MAX,
  `0xF487` MIN, `0xF48E` EXP, `0xF490` ROR, MACA-T probe to `0xF47F`)
  — all converted to pinned siblings. Caught a maintainer hex slip
  (S/D transposition) and a worker 48-bit constant typo via probe
  before landing.
- Near-misses: order-destroying `sort_keys=True` JSON dumps bloated
  the config diff 18k/15k lines — repaired by checkout plus
  order-preserving re-application (final: +306/+42, deletions are
  the three form flips only).
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 71 unbound forms; next is W7 batch 2
  (ADDC/ADDS/SUBC leaf-only briefs ready; SFTA needs a 5-bit-shift
  layout micro-seam first). Wave-7 plan doc still untracked.
- Split commit discipline: impl `a9bdced`, this ledger-only follow-up.
  Slice doc born `(implemented; local verification green)` in the
  impl commit with its anchor; plan link stays `(planned)`.

## 2026-09-18 — P3-S38 W7 batch-2 ADDC/ADDS/SUBC Smem rows (slice 67)

- Landed: second managed fan-out — three TI-pinned rows (ADDC
  `0x0600/0xFE00`, ADDS `0x0200/0xFE00`, SUBC `0x1E00/0xFE00`)
  on the existing select/indirect/addr7 layout, all `UNRESOLVED`
  repeatability (SUBB precedent). Leaves built row-less by
  isolated workers (W7-ADDC/W7-ADDS/W7-SUBC) against maintainer
  TI briefs from SPRU172C, merged with row-id flips, dispatch
  includes, manifests, 9 leaf/metadata/decode suites, inventory
  flips (`FORM-T2-01-11/01-13/02-12` to `COMMON_CANDIDATE`/
  `SUBSET` + `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`
  bindings), 18 evidence records (9 req + 9 test; TI assertions
  already pinned), index rows, frozen-count bumps (289 rows, 295
  bindings, 31,881 lookup, 255 SUBSET, 137/205 at 66.829%, 748
  requirements, 604 tests), README/plan/slice.
- Validation: `make build` clean, `make test` exit 0 (578 C
  binaries ok, 286 Python tests), `check_evidence.py` PASS
  (748/604), `check_isa_inventory.py` exit 0 (137/205, strict
  40/205), lookup regen pure (+1,536), sanitizer rebuild of all
  touched files exit 0.
- Decisions: SUBC needs its own `ACCUMULATORS_STATUS_SXM`
  composite (reads SXM, ignores OVM); ADDS saturation stays
  OVM-gated per the ADD_SMEM sibling. Worker ADDC test vector
  fixed at merge (SXM-suppression case forgot C=1; the comment
  stated `0 + 0x8000 + 0`). New words collided with three old
  probes (`0x0290` LDU decode/metadata, `0x0680`/`0x1E80` LDR
  decode) — all converted to pinned ADDC/ADDS/SUBC siblings.
  Evidence ids follow the S37 hyphen convention
  (`REQ-COMMON-ADDC-SMEM-*`, not `_SMEM-`); first append used
  the wrong separator and the registry gate caught it.
- Near-misses: `sort_keys=True` JSON dumps reshuffled
  `inventory.json` again — repaired by checkout plus
  order-preserving re-application (final diff +33/-9, no
  bloat). Two `OPCODE_ID_COUNT == 287` pins hid outside the
  catalog dir (sequencer repeat, branch XC) plus one spaceless
  `COUNT==287` variant — now swept repo-wide.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 68 unbound forms; next is W7
  batch 3 (SFTA needs the 5-bit-shift layout micro-seam first).
  Wave-7 plan doc still untracked.
- Split commit discipline: impl `c377c07`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)`.

## 2026-09-18 — P3-S39 W7 batch-3 SFTA accumulator row (slice 68)

- Landed: third managed fan-out — one TI-pinned row (SFTA
  `0xF460/0xFCE0`) on the existing accumulator S/D/shift5
  layout, `UNRESOLVED` repeatability (SFTL precedent). Leaf
  built row-less by an isolated worker (W7-SFTA) against a
  maintainer TI brief sourced from the mnemonic reference
  (both worked examples hand-verified: `FF 8765 0055h,-5`
  gives `FF FC3B 2802h`/C=1; `80 AA00 1234h,+5` gives
  `15 4002 4680h`/C=0), merged with a row-id flip, dispatch
  include, manifests, 3 leaf/metadata/decode suites, inventory
  flip (`FORM-T2-10-04` to `COMMON_CANDIDATE`/`SUBSET` +
  `REGISTER_DST` binding), 7 evidence records (1 TI + 3 req +
  3 test), index rows, frozen-count bumps (290 rows, 296
  bindings, 32,009 lookup, 256 SUBSET, 138/205 at 67.317%,
  751 requirements, 607 tests), README/plan/slice.
- Validation: `make build` clean, `make test` exit 0 (581 C
  binaries ok, 286 Python tests), `check_evidence.py` PASS
  (751/607), `check_isa_inventory.py` exit 0 (138/205, strict
  40/205), lookup regen pure (+128), sanitizer rebuild of all
  touched files exit 0.
- Decisions: SFTA overflow uses the no-sign-loss rule (src
  bits 39..(32-N) must equal the landing sign); right/zero
  shifts never overflow. The new words collided with ten old
  probes (branch `0x0400` flips, RETE/RETED `0x0080` flips,
  LD_LK16/LD_SRC_SHIFT/RPT_LK flips, MACA_T `0xF47F`
  literal) — all converted to pinned SFTA siblings. Six C
  population pins and two out-of-dir `COUNT` pins swept.
- Near-misses: none beyond the probe collisions above; the
  worker leaf needed zero merge fixes.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 67 unbound forms; W7 fan-out
  continues with the next TI-pinned briefs. Wave-7 plan doc
  still untracked.
- Split commit discipline: impl `db883f9`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)`.

## 2026-09-18 — P3-S40 W7 batch-4 LTD Smem row (slice 69)

- Landed: fourth managed fan-out — one TI-pinned row (LTD
  `0x4C00/0xFF00`) on the existing indirect/addr7 layout,
  `UNRESOLVED` repeatability (LD_T precedent). Leaf built
  row-less by an isolated worker (W7-LTD) against a maintainer
  TI brief (Z-delay: T = Smem plus the Smem word to DATA
  Smem+1, worked example hand-verified), merged with a row-id
  flip, dispatch include, manifests, 3 leaf/metadata/decode
  suites, inventory flip (`FORM-T2-19-21` to
  `COMMON_CANDIDATE`/`SUBSET` +
  `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3` binding), 7
  evidence records (1 TI + 3 req + 3 test), index rows,
  frozen-count bumps (291 rows, 297 bindings, 32,265 lookup,
  257 SUBSET, 139/205 at 67.805%, 754 requirements, 610
  tests), README/plan/slice.
- Validation: `make build` clean, `make test` exit 0 (584 C
  binaries ok, 286 Python tests), `check_evidence.py` PASS
  (754/610), `check_isa_inventory.py` exit 0 (139/205, strict
  40/205), lookup regen pure (+256), sanitizer rebuild of the
  leaf exit 0.
- Decisions: LTD composes two existing effect shapes (MACD
  delay write-back staging + LD_T T-write completion), so no
  new seam was needed. The new words collided with three old
  probes (BANZ `0x2000` flip, LD_SMEM_SHIFT16 `0x0800` flip in
  both metadata and decode) — all converted to pinned LTD
  siblings. Six C population pins were stale binaries, fixed
  by rebuild.
- Near-misses: CMPS assessed for this batch and deferred —
  its TRN write is a genuine mini-seam (TRN exists in the MMR
  file with no architectural writer yet).
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 66 unbound forms; W7 fan-out
  continues. Wave-7 plan doc still untracked.
- Split commit discipline: impl `66b8ebe`, this ledger-only
  follow-up. Slice doc born `(implemented; local verification
  green)` in the impl commit with its anchor; plan link stays
  `(planned)`.

## 2026-09-18 — P3-S41 single-word Smem delay write-back seam (slice 70)

- Landed: arch seam unblocking LTD execution (S40 staged the leaf
  with `write_smem_delay` set, which the WORD16 root rejected) and
  the future DELAY leaf. Carrier generalizes the flag to
  single-word DIRECT/SINGLE_INDIRECT DATA READs (READ-only, no RMW,
  optional completion; 48-byte ABI unchanged); new
  `memory_instruction_single_delay` owner issues/resumes the DATA
  Smem+1 WRITE through the reused child with the READ word stashed
  in an RMW-disjoint union slot; shared READY tail moves to the
  commit owner (transaction.c 786 -> 780 lines, stays under the 800
  split limit); delay on MMR-routed reads rejects transactionally.
  RED seam proof `test_single_word_delay_write` (LTD shape with the
  TI 6CACh worked example, null-completion DELAY shape, WAIT
  resume, delay FAULT silence, WRITE/RMW and MMR rejects), 3
  evidence records (1 activation + 1 req + 1 test), index rows,
  frozen-count bumps (755 requirements, 611 tests; rows/bindings/
  lookup unchanged at 291/297/32,265). Slice doc born
  `(implemented; local verification green)` in the impl commit.
- Validation: `make build` clean, `make test` exit 0,
  `check_evidence.py` PASS (755/611),
  `check_isa_inventory.py` exit 0 (139/205, strict 40/205),
  sanitizer rebuild of the four touched sources + focused test
  exit 0 with no findings, normal build restored (0 asan
  symbols) with the focused test re-passing.
- Decisions: delay WRITE is DATA-space endpoint traffic by the
  MACD pair precedent (no MMR branch); null completion skips both
  the completion call and the follow-on stage; MMR+1 semantics
  stay fail-closed (INVALID_CANDIDATE, preserving the S40 freeze
  behavior). PROJECT requirement cites PROJECT evidence only (TI
  LTD pin lives in the activation assertion prose) per the scope
  policy — first submission leaked COMMON_TI and was fixed.
- Near-misses: initial tail refactor pushed transaction.c to 904
  lines (quality-gate split violation); fixed by moving the tail
  to the commit owner and the orchestration to the delay owner.
  Test endpoint needed a cancel callback (adapter rejects
  partial endpoints); WAIT test first used MMR-range direct
  addresses and was retargeted to 0x20+.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 66 unbound forms; DELAY leaf/row
  next on this seam; W7 fan-out continues. Wave-7 plan doc still
  untracked.
- Split commit discipline: impl `51fe34e`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S42 W7 batch-5 DELAY Smem row (slice 71)

- Landed: one TI-pinned single-word row on INDIRECT_ADDR7
  (DELAY_SMEM 0x4D00/0xFF00, UNRESOLVED, no resources): DELAY
  stages a Smem read with the S41 single-word delay write-back
  and a null completion. Leaf built row-less by an isolated
  worker against a maintainer TI brief (SPRU172C DELAY p. 4-41
  fetched from the public TI manual volume; Z-delay, no status
  bits, 6CAC worked example) and merged with no fixes and no id
  flip. Maintainer wired the row, lookup regen (+256 pure),
  dispatch include (both tables auto-bind from the row stem),
  load sources, ops/decode/metadata suites plus registrations,
  LTD 0x4D00 neighbor flips, inventory form flip plus binding,
  7 evidence records (1 TI + 3 req + 3 test), index rows, README,
  and frozen-count bumps (292 rows, 298 bindings, 32,521
  lookup, 140/205 at 68.293%, 258 SUBSET bindings, 758
  requirements, 614 tests). Slice doc born `(implemented;
  local verification green)` in the impl commit.
- Validation: `make build` clean, `make test` exit 0,
  `check_evidence.py` PASS (758/614),
  `check_isa_inventory.py` exit 0 (140/205, strict 40/205),
  sanitizer rebuild of the new/touched sources with the DELAY
  and LTD suites passing under ASan/UBSan, normal build
  restored (0 asan symbols) with the suites re-passing.
- Decisions: DELAY reuses the LTD addressing subset
  (DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3); no CPU-level
  retirement test (LTD precedent; execution is covered by the
  S41 seam proof). PROJECT/COMMON scope policy held: the new
  ARCHITECTURAL reqs cite COMMON_TI evidence only.
- Near-misses: first full run exposed 16 stale Python frozen
  pins plus 7 C-side pins (rows, bindings, lookup, coverage,
  SUBSET set/map) fixed as maintainer bumps; worker worktree
  sources were GC'd after the child terminated, recovered
  byte-identical from the worker's /tmp/delay_hold copies
  (line counts, contract heads, and 59-check suite all match;
  all suites re-pass in-tree). Lesson: copy worker
  deliverables into the parent tree before the child result
  resolves.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 65 unbound forms; W7 fan-out
  continues with the next TI-pinned briefs. Wave-7 plan doc
  still untracked.
- Split commit discipline: impl `45f1adb`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S43 W7 batch-6 ST T direct row (slice 72)

- Landed: direct slice of TI syntax-1 ST T,Smem on a fixed-zero
  I bit (FORM-T2-22-01 ST T direct, 0x8C00/0xFF80, SPRU172C ST
  pp. 4-167–4-168) on the shared INDIRECT_ADDR7 layout with
  UNRESOLVED repeatability per the LD_T/LTD/ST_T precedent.
  The leaf stages one DIRECT_WORD16 page-0 DATA WRITE of the
  copied T register with a null completion and no delay flag;
  a set I bit rejects as INVALID_OPERAND since the indirect
  half composes through the established ST_T MOD rows from
  0x8C80. Leaf built row-less by an isolated worker against a
  maintainer TI brief and merged with no fixes. Maintainer
  wired the row, lookup regen (+128 pure), dispatch include,
  store sources, ops/decode/metadata suites plus
  registrations, ST_T neighbor flips, inventory form flip plus
  binding, evidence records, index rows, README, and
  frozen-count bumps (293 rows, 299 bindings, 32,649 lookup,
  141/205 at 68.780%, strict 40/205, 761 requirements, 617
  tests). Slice doc born `(implemented; local verification
  green)` in the impl commit.
- Validation: `make build` clean, `make test` exit 0 (full
  gate), focused `test_st_t_smem_direct_{metadata,decode,ops}`
  pass, `check_evidence.py` PASS (761/617),
  `check_isa_inventory.py` exit 0 (141/205, strict 40/205).
  Evidence gate takes ~5m20s wall (99% CPU, single-threaded
  manifest/requirement cross-checks); slow but finite, no fix
  needed.
- Decisions: no CPU-level retirement test (ST_T MOD precedent;
  execution is covered by the shared staging core proof).
  PROJECT/COMMON scope policy held: new ARCHITECTURAL reqs
  cite COMMON_TI evidence only. ST TRN (FORM-T2-22-02) stays
  deferred: the copied input snapshot carries no TRN field, so
  it needs a TRN input seam (P2-W39 T_REGISTER precedent)
  before its leaf.
- Near-misses: none; frozen COUNT/lookup/SUBSET pins flipped
  in the same maintainer pass with no sibling fallout beyond
  the pinned ST_T probes.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 64 unbound forms; W7 fan-out
  continues with the next TI-pinned briefs, starting with the
  TRN input seam. Wave-7 plan doc still untracked.
- Split commit discipline: impl `8953957`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S44 TRN input seam (slice 73)

- Landed: maintainer arch seam unblocking deferred ST TRN
  (`FORM-T2-22-02`). New `TI_C54X_INPUT_RESOURCES_TRN_REGISTER`
  bit (8192, fourteenth resource, `KNOWN_MASK` included),
  snapshot fields `trn_register_raw16`/`trn_register_known`, a
  `collect.c` arm reading the architectural TRN through
  `ti_c54x_mmr_file_read_trn` (OK copies exact WORD16 known,
  UNKNOWN leaves zero/unknown, anything else rejects), and a
  borrowed `mmr_file` source pointer on `ti_c54x_input_sources`
  wired once in `run.c` from `cpu->arch_state.mmr_file`.
  Unrequested TRN is not read. Snapshot carrier 64 to 72
  bytes LP64 (68 ILP32, both verified by probe); embedded
  instruction transaction root 384 to 392 (348 to 356, LP64
  executed and -m32 probe executed, native-32 by
  construction); eleven `0x2000` invalid-resource probes moved
  to `0x4000`. RED proof
  `tests/unit/cpu/input/test_collect_trn_register.c` mirrors
  the T collection contract (exact copies, unknown
  canonicalization, selectivity, transactional rejection) and
  was observed failing before the seam. No catalog row,
  layout, handler, effect, inventory form, or CPU retirement
  change; catalog, lookup, bindings, and family coverage
  unchanged (293 rows, 299 bindings, 32,649 lookup, 141/205
  at 68.780%), 762 requirements, 618 tests. Slice doc born
  `(implemented; local verification green)` in the impl
  commit.
- Validation: `make build` clean, `make test` exit 0 (full
  gate), focused input suite 12/12, `check_evidence.py` PASS
  (762/618), `check_isa_inventory.py` exit 0 (141/205,
  strict 40/205), ASan/UBSan clean on the TRN/T/root/carrier
  suites with the normal build restored (0 asan symbols).
- Decisions: dedicated `mmr_file` source pointer (mirrors the
  T precedent) instead of reaching through `arch_state`;
  single production wiring site. PROJECT/COMMON scope policy
  held: the new PROJECT_CONTRACT req cites the project
  contract source only.
- Near-misses: first full run exposed four more pinned
  carriers beyond the input asserts (dual-MOD3 root, W51B
  contract, DLD carrier, two perf ABI probes); all moved to
  probe-verified sizes in the same pass. Lesson: a snapshot
  field addition ripples to every embedding carrier — grep
  `_Static_assert.*sizeof` and `sizeof(...) ==` pins before
  the full gate, not after.
- Knobs: none added, changed, or retired.
- Follow-up: ST TRN direct leaf (S45) can now go to managed
  fan-out against this seam; remaining census 64 unbound
  forms minus the seam itself. Wave-7 plan doc still
  untracked.
- Split commit discipline: impl `4bd4ef2`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S45 W7 batch-7 ST TRN direct row (slice 74)

- Landed: direct slice of TI syntax-2 ST TRN,Smem on a
  fixed-zero I bit (FORM-T2-22-02, 0x8D00/0xFF80 from opcode
  diagram 1000_1101_IAAAAAAA, SPRU172C ST pp. 4-166–4-168,
  fetched from the public TI manual volume). Execution is
  (TRN) into Smem with no status effects; worked example ST
  TRN,5 stores TRN=0x1234. I=1 rejects as INVALID_OPERAND
  (no ST_TRN MOD rows exist yet), UNRESOLVED repeatability,
  TRN_REGISTER inputs over the S44 seam, one DIRECT_WORD16
  page-0 DATA WRITE with null completion. Leaf built row-less
  by an isolated worker against a maintainer TI brief and
  merged with two comment truth fixes. Maintainer wired the
  row, schema token, lookup regen (+128 pure, no collisions),
  dispatch include, store sources, ops/decode/metadata suites
  plus registrations, inventory form flip plus binding, 3 req
  plus 3 test records reusing the ST TI evidence, index rows,
  README, and frozen-count bumps (294 rows, 300 bindings,
  32,777 lookup, 142/205 at 69.268%, strict 40/205, 765
  requirements, 621 tests). Slice doc born `(implemented;
  local verification green)` in the impl commit.
- Validation: `make build` clean, `make test` exit 0 (full
  gate), three new TRN suites pass in-tree,
  `check_evidence.py` PASS (765/621),
  `check_isa_inventory.py` exit 0 (142/205, strict 40/205).
- Decisions: worker protocol amended for row-less leaves —
  the worker may apply scratch wiring in its own tree only
  (catalog row, regen, dispatch include, sources line) to
  compile and run its tests, and delivers new files only;
  the maintainer redoes all wiring authoritatively and runs
  the full gates. No worker commits, no shared-file delivery.
- Near-misses: (1) worker comments claimed established
  ST_TRN MOD rows; fixed at merge since none exist. (2) full
  gate exposed the standard pin sweep (COUNT 295, lookup
  32777, SUBSET set/map/percent, row+1 identity) plus a
  DIRECT_PAGE0 addressing case, all flipped in the maintainer
  pass. (3) S44 correction: the S44 status flip broke its own
  project evidence anchor (`(planned)` heading renamed);
  the anchor now points at the landed heading. Lesson: a
  project-evidence anchor must name the post-landing heading
  when the slice flips on landing, or the evidence gate fails
  after the flip.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 63 unbound forms; W7 fan-out
  continues with the next TI-pinned briefs. Wave-7 plan doc
  still untracked; stale S43/S45 worker worktrees under
  `.muse/worktrees/` await GC.
- Split commit discipline: impl `f8951cc`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S46 W7 batch-8 STL direct rows (slice 75)

- Landed: direct slices of TI STL syntax 1 (FORM-T2-20-10,
  0x8000/0xFE80) and syntax 2 (FORM-T2-20-11, 0x8400/0xFE80)
  on a fixed-zero I bit (SPRU172C STL pp. 4-172–4-173). S@8
  selects accumulator A/B through the existing
  SELECT_INDIRECT_ADDR7 layout (no layout seam); 7-bit page-0
  address; UNRESOLVED repeatability; page-0 DATA WRITE with
  null completion. Syntax 1 stages accumulator low unshifted
  (worked example STL A,11 stores 0x4321); syntax 2 shifts by
  the signed ASM count with SXM-gated fill per the ADD/STH
  precedent and stages bits 15:0 (worked example STL B,−8
  stores 0x2112). I=1 rejects as INVALID_OPERAND (no STL MOD
  rows exist). Two isolated workers built the leaves row-less
  in parallel against one maintainer brief (W8A syntax 1, W8B
  syntax 2, disjoint files, no commits) and merged with
  MOD-rows comment truth fixes plus a composite token rename
  to the LD_SRC_ASM ordering (SXM_ASM). Maintainer wired both
  rows, schema tokens, lookup regen (+512 pure, no
  collisions), dispatch includes, store sources, six suites
  plus registrations, inventory flips plus bindings, 6 req
  plus 6 test records reusing the STL TI evidence, index
  rows, README, and frozen-count bumps (296 rows, 302
  bindings, 33,289 lookup, 144/205 at 70.244%, strict 40/205,
  771 requirements, 627 tests). Slice doc born `(implemented;
  local verification green)` in the impl commit.
- Validation: `make build` clean, `make test` exit 0 (full
  gate), six new STL suites pass in-tree,
  `check_evidence.py` PASS (771/627),
  `check_isa_inventory.py` exit 0 (144/205, strict 40/205).
- Decisions: SXM kept as a declared syntax-2 input per TI's
  "Affected by SXM" plus the ADD_SRC_ASM precedent (new
  ACCUMULATORS_STATUS_SXM_ASM composite); no new layout was
  needed. Parallel two-worker fan-out with disjoint ownership
  needed no conflict resolution.
- Near-misses: (1) projection arithmetic wrote +256 lookup
  for two rows; actual +512 (33,289) caught by the generator
  test and corrected in the doc. (2) full gate exposed the
  standard double pin sweep (COUNT 297, lookup 33289, SUBSET
  set/map/percent, row+1 identity) plus an LD_XMEM_SHIFT
  decode sibling flip (0x8400 now STL ASM, pinned). Lesson:
  compute multi-row projections as rows × encodings, and grep
  fixed-bit near-miss probes for the new first words before
  the full gate.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 61 unbound forms; W7 fan-out
  continues with the next TI-pinned briefs (STLM/LDM,
  conditional stores, filter ops, dual-memory seam). Wave-7
  plan doc still untracked; stale worker worktrees under
  `.muse/worktrees/` await GC.
- Split commit discipline: impl `4ebd543`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S47 W7 batch-9 STLM/LDM direct rows (slice 76)

- Landed: direct slices of TI STLM src,MMR (FORM-T2-20-13,
  0x8800/0xFE80, SPRU172C STLM p. 4-175) and LDM MMR,dst
  (FORM-T2-19-18, 0x4800/0xFE80, SPRU172C LDM p. 4-73) on a
  fixed-zero I bit. S/D@8 select over the existing
  SELECT_INDIRECT_ADDR7 layout (no layout seam); 7-bit
  DP-independent page-0 MMR address; UNRESOLVED
  repeatability. STLM stages a DATA WRITE of accumulator low
  with null completion (worked example STLM A,BRC; register
  routing downstream at the bus, as for STM). LDM stages a
  DATA READ whose static completion zero-extends into
  dst(15:0) and clears dst(39:16) (worked examples LDM AR4,A
  and LDM 060h,B; LDU completion precedent; first
  DIRECT-addressing load; SXM untouched). I=1 rejects as
  INVALID_OPERAND (indirect MMR deferred). Two isolated
  workers built the leaves row-less in parallel (W9A STLM,
  W9B LDM, disjoint files, no commits) and merged with no
  fixes — both applied the MOD-rows truth lesson
  unprompted. Maintainer wired both rows, lookup regen (+512
  pure, no collisions), dispatch includes, store/load
  sources, six suites plus registrations, inventory flips
  plus bindings, two new TI records, 6 req plus 6 test
  records, index rows, README, and frozen-count bumps (298
  rows, 304 bindings, 33,801 lookup, 146/205 at 71.220%,
  strict 40/205, 777 requirements, 633 tests). Slice doc born
  `(implemented; local verification green)` in the impl
  commit.
- Validation: `make build` clean, `make test` exit 0 (full
  gate), six new suites pass in-tree,
  `check_evidence.py` PASS (777/633),
  `check_isa_inventory.py` exit 0 (146/205, strict 40/205).
- Decisions: SXM kept out of the LDM row (TI "not affected
  by SXM", LDU precedent). New TI records use manual
  locators (chapter/section/pages), matching the registry
  convention.
- Near-misses: (1) workers guessed COUNT 297/298; actual
  299 (298 rows + 1) — probe before asserting, especially
  with two rows landing at once. (2) S46 correction: its
  README shipped the projected 33,033 lookup instead of the
  actual 33,289 (comma-form miss in the fix script); fixed
  here to 33,801.
- Open question (follow-up, not blocking): I=0 direct slices
  stage DP-independent absolute page-0 (DIRECT_PAGE0,
  project-wide convention across ADD/SUB/ST/LTD rows) while
  TI direct addressing is DP-relative; only the STH family
  models DP via DIRECT_DP. DP-relative direct for the I=0
  slices remains future work.
- Knobs: none added, changed, or retired.
- Follow-up: remaining census 59 unbound forms; W7 fan-out
  continues (conditional stores, filter ops, dual-memory
  seam, control/interrupt/stack). Wave-7 plan doc still
  untracked; stale worker worktrees under `.muse/worktrees/`
  await GC.
- Split commit discipline: impl `71e6006`, this ledger-only
  follow-up. Plan link stays `(planned)` until the slice lands
  (matches S40 convention).

## 2026-09-18 — P3-S48 conditional-store seam (slice 77)

- Scope: maintainer arch seam for the class-15 SACCD/SRCCD/STRCD
  leaves (FORM-T2-21-02/03/04): BRC_REGISTER input (bit 16384,
  MMR cell, probes to 0x8000), store-owned 4-bit cond evaluator
  (`op_store_cond_common`), OVERWRITE/WRITEBACK RMW operations,
  SELECT_DUAL_XMOD_XAR_COND4 layout. No catalog row, handler,
  inventory form, or CPU retirement change.
- Validation: `make build` clean, `make test` exit 0 (full gate:
  source boundaries PASS over 1104 files, observer artifact OK),
  new suites `store_cond_evaluate_ops` and
  `rmw_overwrite_writeback_transform` pass in-tree,
  `check_evidence.py` PASS (779 requirements, 635 tests),
  `check_isa_inventory.py` exit 0 (catalog 298 rows, 304
  bindings, 33,801 lookup, 146/205 families at 71.220%,
  strict 40/205 — all unchanged from the S47 freeze).
- Decisions: (1) TI ground truth fetched from SPRU172C
  (pp. 4-152/4-165/4-186): cond A-codes are `0xxx` and B-codes
  `1xxx` — the planned slice's `01xx`/`10xx` was wrong and was
  corrected in the slice, evaluator, and tests before landing.
  (2) OVERWRITE-on-true / WRITEBACK-on-false is not an
  approximation: the TI execution pages prescribe exactly
  "reads Xmem and writes the value back ... always read and
  updated". (3) Evaluator is store-owned and dependency-free
  rather than delegating to `branch_cond_evaluate` because no
  store-to-branch include exists. (4) Split decision:
  `operand_layout_sets.c` hit 818 lines on the COND4 addition,
  so the branch/XC tables moved to
  `operand_layout_sets_branch.c` (757/87). (5) S-bit
  convention S=0→A/S=1→B confirmed against the STL direct
  sibling, not assumed from TI.
- Near-misses: (1) new test initially called a file-static
  RMW validator — acceptance is proven through the transform
  instead. (2) Contract assertion first copied S44's size
  deltas; measured S48 deltas are snapshot stays 72 (LP64),
  root 392→392 (LP64) and 356→360 (ILP32) — fixed before
  committing. (3) Layout test comment mislabeled the 0x9D5B
  vector as SACCD shape; it is SRCCD-family — fixed.
- Knobs: none added, changed, or retired.
- Follow-up: S49 leaves (SACCD must shift `src << (ASM-16)`
  before staging per TI "affected by ASM and SXM"; SRCCD/STRCD
  TI `*AR5-,AGT` vectors pinned in the slice); 59 unbound forms
  remain. Stale worker worktrees under `.muse/worktrees/`
  await GC.
- Split commit discipline: impl `99c535f`, this ledger-only
  follow-up.

## 2026-09-18 — P3-S49 conditional-store Xmem leaves (slice 78)

- Scope: maintainer merge of three row-less leaves from isolated
  fan-out workers (SACCD/SRCCD/STRCD, FORM-T2-21-02/03/04):
  dual-X Xmem READ with OVERWRITE/WRITEBACK selected by the S48
  cond evaluator, one AR update via the canonical-zero RMW read
  leg, checked/trusted parity, transactional rejection.
  Maintainer wired catalog rows/IDs (0x9E00/0xFE00, 0x9D00,
  0x9C00), three resource combos, dispatch includes, lookup
  regen (+1024 pure insertions), inventory flips to COMPLETE,
  evidence, README, and frozen-count bumps.
- Validation: `make test` exit 0 (613/613 unit binaries incl.
  the three new leaf suites, 286/286 python contract tests,
  source boundaries PASS over 1113 files, evidence PASS,
  observer artifact OK). Closure: 301 rows, 307 bindings,
  34,825 lookup entries, 149/205 forms at 72.683% with strict
  43/205 at 20.976%, 782 requirements, 638 tests.
- Decisions: (1) SACCD worker staged bits 31-16 with unsigned
  ASM and no TI vector; TI p. 4-153 (0xFC00) proves low-16
  with signed ASM — helper, full vector table (recomputed
  against an independent oracle that also reproduced TI), and
  stale comments corrected at merge. (2) Brief said reject
  XMOD 3; both store workers staged 0-3 matching landed
  STH_SHFT_XMEM while the address core owns MOD3 (W52B root)
  — accepted, brief corrected. (3) STRCD S-mismatch returned
  INVALID_DECODE on the checked path; moved into the values
  core as INVALID_OPERAND both paths (SRCCD parity); resource
  token renamed to canonical order in code and test.
  (4) COMPLETE (not SUBSET) per ADD_XMEM single-row precedent;
  COMPLETE bindings require addressing_subset ALL.
- Near-misses: two decode near-miss sweeps flipped into the
  claimed range (LD_XMEM_SHIFT 0x9C00, ST_T_SMEM 0x9C90) —
  promoted to pinned STRCD siblings; six metadata population
  pins 33801 to 34825; 79 ID_COUNT pins 299 to 302; percent
  pins to 72.683/20.976.
- Knobs: none added, changed, or retired.
- Follow-up: 56 unbound forms remain (table 2-21 leaves CMPS);
  stale `indirect_dual.h` MOD3-deferred comment predates W52B;
  worker worktrees under `.muse/worktrees/` await GC.
- Split commit discipline: impl `c23ccc5`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S50 CMPS max-select-store slice (slice 79)

- Scope: direct-only CMPS src,Smem leaf (FORM-T2-21-01, last of
  table 2-21): signed 16-bit max of the S-selected accumulator
  halves stages a page-0 Smem WRITE with a completion staging
  the compound TRN_SHIFT_WRITE follow-on ((trn << 1) | taken,
  TC = taken) on the S50 TRN_SHIFT_WRITE effect + ordinary
  transaction arm. Maintainer wired the catalog row
  (0x8E00/0xFE80, 256 encodings), ACCUMULATORS_TRN_REGISTER
  resource (+ schema token), dispatch include, lookup regen,
  inventory flip to SUBSET/DIRECT_PAGE0, evidence, README,
  and frozen-count bumps. The deferred I=1 half stays
  decodable and rejects INVALID_OPERAND on both paths.
- Validation: `make test` exit 0 (617/617 unit binaries incl.
  the four new CMPS suites, 286/286 python contract tests,
  source boundaries PASS, evidence PASS, observer artifact
  OK). Independent end-to-end probe retired CMPS through
  `ti_c54x_cpu_run` before the committed CPU suite landed.
  Closure: 302 rows, 308 bindings, 35,081 lookup entries,
  150/205 forms at 73.171% with strict 43/205 at 20.976%,
  786 requirements, 642 tests.
- Decisions: (1) RED tie vector 0x00FF expected taken,
  contradicting the 0x1234 tie vector; strict-wins design
  rules ties untaken, so the expectation (not the leaf) was
  fixed to false. (2) Landing closed the TRN-cell completion
  gap: the pipeline admitted no WRITE-with-completion shape,
  so CMPS retired INVALID_STATE at the CPU. Admitted WRITE +
  NONE + completion in the validator (staged value feeds the
  completion) and invoked it in the single-leg WRITE branch
  with the ordinary follow-on commit before the architecture
  commit; RMW paired-write legs still never carry
  completions. Follow-on commit is infallible for
  well-formed effects (binary tc_bit enforced twice), so the
  post-write fault arm is defensive only. (3) Probe dma 0x10
  hit the MMR AR0 cell and routed internally; committed CPU
  test uses dma 0x20/0x21 on the DATA bus and leaves DP
  unseeded to prove DP-independence.
- Near-misses: six lookup-population pins 34825 to 35081;
  one unspaced ID_COUNT pin; 79 spaced ID_COUNT pins 302 to
  303; SUBSET 264 to 265 with CMPS added to the DIRECT_PAGE0
  tuple; percent pins to 73.171. CPU test links a new
  src/cpu/-owned CPU-RETIREMENT requirement (MPYA
  precedent); the OWNER gate rejects cpu/ tests under
  isa/ops-owned requirements.
- Tool fix: `check_evidence` hung (100% CPU, no output) on
  the S50 tree. Root cause in the checker, not the
  registry: `repo_files` post-filtered a whole-tree rglob,
  walking gigabytes of accumulated per-slice `build-*`
  trees. Fixed by pruning ignored trees during traversal
  (identical results) and covering the `build-*` family;
  walks now take 0.08 s. No manifests live in build trees
  (verified empty).
- Knobs: none added, changed, or retired.
- Follow-up: 55 unbound forms remain (table 2-21 fully
  bound: CMPS SUBSET direct, SACCD/SRCCD/STRCD COMPLETE);
  CMPS indirect (I=1) MOD slice still deferred; stale
  `indirect_dual.h` MOD3-deferred comment predates W52B;
  worker worktrees under `.muse/worktrees/` await GC.
- Split commit discipline: impl `127b685`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S51 FRAME stack-pointer-offset slice (slice 80)

- Scope: FRAME K (FORM-T2-17-01, first of table 2-17): signed
  8-bit K in bits 7:0 stages one SP_WRITE carrying
  (SP + K) mod 65536, committed through checked arch state
  (marks SP known) on a run.c ordinary-path arm ahead of the
  effect-transaction stage, which admits no SP target. New
  FRAME_K8 layout, row 0xEE00/0xFF00 (256 encodings),
  STACK_POINTER input, repeatability UNRESOLVED (Table 2-28
  lists no FRAME form). Unknown SP rejects transactionally
  (INVALID_OPERAND at the leaf, INVALID_STATE at the CPU).
- Validation: 5 new tests (catalog 256-population + neighbors,
  decode + K extraction, 4-path checked/trusted/dispatch parity
  with 8 K/SP vectors incl. both wraps, apply value/knownness/
  atomicity, CPU retire/wrap/unknown-SP). Full `make test`
  exit 0 under default ASan/UBSan; 286 contract tests OK;
  evidence gate 791/647; cmake-build + CTest 5/5; harness
  synthetic NOP/NOP/FRAME run retires 2 then stops
  INVALID_STATE on reset-unknown SP with 3/3 coverage.
- Decisions: (1) SP commits through a dedicated run.c arm
  (repeat-route placement pattern), not the shared
  effect-transaction, whose signatures the six memory-commit
  call sites share. (2) Review fix: bridge resolves before any
  SP mutation (ordinary path stages before its bridge check;
  the arm now matches). (3) Uninitialized effect padding broke
  four-path memcmp parity; the leaf memsets the staged effect.
  (4) Catalog handler token is `frame_k` (dispatch derives
  `ti_c54x_op_frame_k`); first write used `frame` and failed
  to link. (5) Commit message for `d7fb653` carries a garbled
  "UNIMPLEMENTED-adjacent UNRESOLVED" phrase; the row is
  plain UNRESOLVED per the W52A rule — message left as-is,
  recorded here.
- Near-misses: BANZD/RPT_K/RCD/RETD decode tests each lost one
  unknown-mask lane into 0xEE00 and now pin FRAME siblings;
  spaced ID_COUNT pins 303 to 304 (plus nospace variants);
  population pins 35081 to 35337 (three spellings); bindings
  308 to 309 with FRAME in the frozen SUBSET set
  (IMMEDIATE_K branch shared with RPT_K); SUBSET 265 to 266;
  percents to 73.659 with 151/205; catalog_rows 302 to 303.
- Knobs: none added, changed, or retired (harness takes argv
  config; no environment reads).
- Follow-up: 54 unbound forms remain (table 2-17 needs the
  S52 pair-leg slice: PSHD/POPD/PSHM/POPM direct, doc planned);
  RESET slice unblocked with TI-pinned opcode 0xF7E0 and
  INELIGIBLE evidence; harness DATA/IO report fields live but
  unreachable from cold reset until register seeding exists.
- Split commit discipline: impl `d7fb653`, hosted harness
  `3f987e6`, this ledger-only follow-up.

## 2026-09-19 — P3-S52 PSHD/POPD/PSHM/POPM stack pair-leg slice (slice 81)

- Scope: table 2-17 closure (FORM-T2-17-02..05, POPD/POPM/PSHD/PSHM,
  to SUBSET): four direct rows (PSHD 0x4B00, POPD 0x8B00, PSHM
  0x4A00, POPM 0x8A00, each 0xFF00, INDIRECT_ADDR7, STACK_POINTER
  input, UNRESOLVED), checked/trusted pair leaves with zero
  template (Smem full page-0 range; MMR below 0x20; I-set rejects
  in the leaf; unknown SP rejects), and the pair root reusing the
  S6 PORT carrier shape with both legs in the pair snapshot, MMR
  legs through the S1 seam, routed SP commit, and CPU retirement.
  Indirect Smem (16B/17B) defers to a follow-on over the same
  carrier; no status-bit effects; no timing.
- Validation: 5 new tests (catalog 4x256-population + neighbors,
  decode + I/cookie extraction, 4-path parity over 14
  cookie vectors with I-set/MMR-range/unknown-SP rejection, 8
  root lifecycle proofs incl. MMR routing with TOS-only endpoint
  traffic and stale-SP resume rejection, 5 CPU proofs incl. the
  TI BRC/AR5 examples). Full `make test` exit 0; 286 contract
  tests OK; evidence gate 796/652; cmake-build + full CTest
  634/634; lookup `--check` clean; harness selftest PASS (2
  vectors).
- Decisions: (1) Both legs live in the pair snapshot, never the
  generic slot: the candidate union aliases them, found by
  reading the PORT hook comment before writing hooks; the SP
  commit rides a routed address_commit. (2) MMR DATA legs route
  inside the pair advance per leg (plan-then-serve, synchronous,
  no bus ID); routed first legs fall through to second-leg issue
  in the same call, so PENDING always implies an in-flight
  endpoint leg and cancel/reset are unchanged. (3) All four rows
  stay 0xFF00 per the slice plan with I=1 claimed-but-rejected
  in the leaf (STLM precedent); PSHM/POPM have no indirect TI
  class so their I-set half stays rejected permanently.
  (4) Pair DATA liveness binds the resolution dma echo against
  the frozen pair arm, not the generic direct-dma7 union offset.
  (5) WAIT observations must carry a zero read value: the
  stale-SP test leaves its read value zero; failure observed as
  MALFORMED before the fix. Unsequenced `printf` of a result
  beside its producing call reads stale state; probes now
  sequence the read.
- Near-misses: spaced and nospace ID_COUNT pins 304 to 308;
  population pins 35337 to 36361 (three spellings); row pins 303
  to 307; bindings 309 to 313 with the four pair symbols in the
  frozen DIRECT_PAGE0 tuple; SUBSET 266 to 270; percents to
  75.610 with 155/205; transaction pins 392/360 to 448/416
  (LP64/ILP32, both probed); LTD/ST_T decode and metadata lanes
  lost 0x4BFF/0x8BFF unbound lanes into PSHD/POPD and now pin
  the siblings; evidence LEAKAGE gate rejects the word RAM in
  common claims (rephrased to DATA endpoint).
- Knobs: none added, changed, or retired.
- Follow-up: 50 unbound forms remain (table 2-17 fully bound:
  FRAME SUBSET, four pair legs SUBSET direct); PSHD/POPD
  indirect (I=1, 16B/17B classes) deferred over the same pair
  carrier; worker worktrees under `.muse/worktrees/` await GC.
- Split commit discipline: impl `b79746b`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S53 PSHD/POPD indirect Smem pair-leg slice (slice 82)

- Scope: single-indirect Smem DATA legs (TI classes 16B/17B)
  through the S52 pair carrier with no new rows: `PSHD` admits
  MOD0--MOD2 (MOD3 is write-only), `POPD` admits MOD0--MOD3,
  `PSHM`/`POPM` keep rejecting I=1 (no indirect MMR class).
  Cookie splits `mode4 = cookie >> 3`, `arf = cookie & 7` (LTD
  precedent); indirect legs zero `smem_dma7` (DATA_PROGRAM pair
  precedent); row inputs stay `STACK_POINTER` (AR unknownness
  rejects transactionally downstream). No status-bit effects,
  no timing, repeatability UNRESOLVED.
- Validation: extended the three S52 area tests RED-first (leaf
  parity over 4 indirect vectors with direction-gated mode
  rejection, MOD1/MOD3 root retirement with AR publication,
  MOD1/MOD2 CPU retirement). Full `make test` exit 0; 286
  contract tests OK; evidence gate 799/652; lookup `--check`
  clean; slice CTest 5/5; harness selftest PASS.
- Decisions: (1) Direction-gated modes mirror the landed
  load/store asymmetry (loads MOD0-2, stores MOD0-3) and the
  resolver-level MOD3-read rejection: defense runs leaf,
  effect-validation, synthesis, and resolver deep. (2) DATA-leg
  liveness mirrors the generic CMPT0/ARP0 indirect tail with
  the AR selector echo bound to the frozen pair arm; the mode
  is structural (shape-pinned). (3) The DATA-leg AR update
  publishes beside SP in the generic commit (dual-pair
  precedent); direct/MMR DATA legs carry no update. (4) New
  requirements reuse the existing area test records (sharing
  precedent); both link directions updated for the LINK gate.
- Near-misses: the pair `is_valid` case comment still said
  direct-only; the S52 row comments still said I-set rejected;
  the kind comments still claimed no AR dependence.
- Knobs: none added, changed, or retired.
- Follow-up: table 2-17 pair legs now direct + indirect Smem
  (MMR direct only); indirect forms stay SUBSET.
- Split commit discipline: impl `d540229`, this ledger-only
  follow-up.

## 2026-09-19 — Correction: P3-S18 and P3-S6 exit-review flips land late

- Correction (appended, no rewrite): S18 lanes 1/2a/2b (`2e1c972`,
  `a3f931d`, `a74cb5a`) plus the exit review recorded in the S6
  planning entry, and S6 lanes 1/2 plus family rows (`6321be6`,
  `7ecca10`, `6de7192`, `f3a4731`), all verified green at the time,
  but both plan-doc `###` headings and both
  `implementation-plan.md` slice links still read `(planned)`.
  This entry advances all four parentheticals to `(implemented;
  local verification green)`; title text is unchanged.
- The lane-2b note that the S18 heading "stays `(planned)` as the
  immutable anchor" is superseded: the anchor is the title text,
  and the status parenthetical follows the same exit-review rule
  as S13/S52/S53. The two plan bodies' stale `` `(planned)` ``
  quotes are dropped by the same token; no semantics change.
- Verification for this flip: full `make test` exit 0 this
  session (only match for fail/error patterns is the pre-existing
  SIZE_REVIEW warning line). No code, registry, or generated
  artifact changes in this commit.
- Split commit discipline: ledger-and-status-only follow-up; no
  impl hash. Next remains the planned P3-S27 ADD/SUB #lk rows.

## 2026-09-19 — Correction: P3-S27 checklist link lands late

- Correction (appended, no rewrite): commit `60e8888` plus
  ledger `0cb344b` landed S27 green (266 rows, FORMs 108/205)
  and the plan-doc heading already reads `(implemented; local
  verification green)`, but the `implementation-plan.md` slice
  link still read `(planned)`. This entry advances that link;
  title text is unchanged.
- No code, registry, or generated artifact changes: the tree
  is code-identical to the green `make test` run this session.
- Split commit discipline: ledger-and-status-only follow-up.
  Next remains the planned P3-S28 Xmem rows.

## 2026-09-19 — Correction: fourteen P3 checklist links land late

- Correction (appended, no rewrite): P3-WAVE6 plus P3-S28
  through P3-S40 all landed green with their plan-doc headings
  already reading `(implemented; local verification green)`,
  but fourteen `implementation-plan.md` slice links still read
  `(planned)`. This entry advances all fourteen; title text is
  unchanged. No code, registry, or generated artifact changes:
  the tree is code-identical to the green `make test` run this
  session.
- Split commit discipline: ledger-and-status-only follow-up.
  The P3 checklist is now fully consistent with git history;
  no further landed-but-unflipped slice remains.

## 2026-09-19 — P3-S54 STL SHFT Xmem store slice lands as 6c16ff7

- Commit `6c16ff7` closes FORM-T2-20-12 (the only unbound STL
  form) as an exact S12 mirror: `STL_SHFT_XMEM` (`0x9800/0xFE00`,
  TI SPRU172C STL syntax 3 opcode `1001_100S_XXXXSHFT`,
  pp. 4-171-4-172) over the shared
  `ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4` layout, with plain plus
  trusted `op_stl_shft_xmem` leaves staging X-only WRITE effects
  carrying bits 15-0 of the left-shifted accumulator. The stored
  low 16 bits of a left shift are pure source bits, so SXM rides
  hostile and unconsumed (same proof as S12). Dispatch and
  trusted binding pick the row up through the catalog X-macro;
  no other handler, layout, decode, or CPU change.
- RED-first: new `test_stl_shft_xmem` (exhaustive 1024
  encodings, 8 hand-computed vectors including the TI STL A,7
  example storing `0x1A00`, SXM-hostile seeds, trusted
  equivalence, transactional rejects; observed RED on the
  missing header), new `test_stl_shft_xmem_metadata` (512-word
  population with `0x97FF` UNKNOWN and `0x9A00` STH sibling
  pins), new `test_cpu_stl_shft_xmem` (retirement, WAIT resume,
  unknown-accumulator rejection). Direct unit sweep caught six
  population-pin failures plus one spaceless ID pin before the
  slow suite (S27 house rule working).
- Census pins move: ID 308 to 309 (81 sites plus one spaceless),
  lookup 36361 to 36873 (+512, zero neighbor moves), quality
  census to 308 rows / 314 bindings / 156/205 forms (76.098%) /
  44 strict-complete (21.463%). FORM-T2-20-12 binds COMPLETE
  under ALL.
- Registry: `E-TI-...-STL-SHFT-OPCODE-P4-171`,
  `E-TI-...-STL-SHFT-SEMANTICS-P4-171-4-172`, and
  `E-PROJECT-P3-S54-STL-SHFT-ACTIVATION` plus
  `REQ-COMMON-P3-S54-STL-SHFT-SEMANTICS` and
  `-CPU-RETIREMENT` (evidence 491, requirements 801). No
  per-slice PROJECT inventory requirement (S32 precedent); the
  binding rides the shared inventory-catalog-binding sweep.
  Index gains the 3 evidence plus 2 REQ plus 2 TEST rows beside
  the S12 block.
- Decisions: (1) S54 chosen over the larger unbound clusters
  (T2-05 doubles need Lmem/C16 ALU; T2-23/24/25 are parallel
  forms; MVxx touches MMR-pending-P0-W4; far/RPTB/INTR are
  arch-excluded) as the only single-row S12-shaped gap.
  (2) Evidence edits done by surgical text insertion after a
  whole-file rewrite attempt produced formatting churn on
  unrelated records (reverted). (3) The dropped PROJECT
  inventory REQ keeps LINK bidirectional-clean; a planned
  PROJECT-contract row would have forced shared-test growth
  the S32 line deliberately avoids. (4) S18/S6 plan docs gain
  superseded-anchor notes: the exit-review flips removed the
  `(planned)` strings the PINNED activation locators
  substring-match, and immutable records cannot be edited.
  (5) README remaining-work 55 to 54 by exactly the closed
  item (the 55 was never the raw unbound count).
- Validation: full `make test` exit 0 (629/629 unit binaries
  direct plus 286 quality tests, evidence PASS); generator
  `--check` clean; `make cmake-build` plus ctest slice 8/8
  (ops, metadata, CPU, STH siblings); harness selftest PASS.
- Knobs: none added, changed, or retired.
- Follow-up: next genuine thread is T2-05 doubles (Lmem/C16
  seam, needs a plan slice) or READA/WRITA (PAR/RC, needs a
  plan slice); T2-04 MACR/MASR leftovers stay unbound by the
  S3f R-sharing convention.
- Split commit discipline: impl `6c16ff7`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S55 lane 1 lands as 55acf82 (Lmem LONG32 seam)

- Commit `55acf82` opens the T2-05 double-precision thread
  (chosen over READA/WRITA: DADD-family words weigh heavily in
  the 5110 census, LONG32/C16 ground exists, PAR does not):
  address kind 21 plus the pair-shaped `long32_single` arm plus
  the pure LONG32 leg planner, with no catalog rows and no
  registry deltas (S6-lane-1 precedent). The DADD pilot row
  plus C16 ALU core follow as lane 2.
- RED-first `long32_single_resolver` (observed RED on the
  missing planner header): direct even/odd canonical pairing,
  MOD0-MOD2 two-word deltas, MOD3 WRITE pre-update with READ
  decline, nine malformed arms plus wrong-kind/argument
  rejects. 631/631 unit binaries direct, quality-test green,
  cmake ctest 1/1.
- Decisions: reuse (not new) union footprint; compose (not
  fork) the landed DP/indirect resolvers; single planner entry
  with caller-supplied AR (direct legs ignore it); READ and
  WRITE pass through (MOD3-write serves future DST-side use).
- Knobs: none added, changed, or retired.
- Follow-up: lane 2 (pair-transaction commit root, DADD row +
  leaves, FORM-T2-05-01 binding), then the five sibling
  doubles.
- Split commit discipline: impl `55acf82`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S55 lane 2 lands as 31bc99d (DADD row + kind-21 retirement)

- Commit `31bc99d` binds the DADD pilot row (`0x5000/0xFC00`,
  shared source/destination/indirect/addr7 layout) with the pure
  `op_dadd` leaf (one kind-21 LONG32 READ; C16=0 40-bit add with
  OVM/C/OV, C16=1 dual independent 16-bit adds) and the full
  kind-21 root seam through CPU retirement. Registry:
  FORM-T2-05-01 SUBSET
  (`DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD2`), regenerated
  lookup (+1024, census 37897), coverage 157/205 (76.585%).
- Root causes fixed during bring-up (all reproduced RED-first
  against `test_cpu_dadd`): (1) `memory_effect.c` had no kind-21
  `is_valid` case (new `memory_effect_long32_single` module,
  keeping the parent under the 500-line gate); (2) page-0 direct
  liveness misread the union-aliased `direct_dma7` slot (new
  `memory_candidate_long32_single` shape+liveness module, routed
  in `matches_live`); the aliased `complete`/`complete_long32`
  union slot also bit once. (3) prepare never routed kind 21 to
  the LONG32 child, so the WORD16 commit tail mis-called the
  completion (segfault, caught under ASan).
- Decisions: single-word directs are page-0 (S7/STM precedent,
  DP slot reserved zero; MOD3 reads decline at resolve time);
  dual-mode guard fill sign-extends result bit 31 (only the TI
  positive worked example pins this — still the weakest
  convention in the slice); C16 takes resource bit 15, fully
  allocating the 16-bit mask, so stale invalid-mask probes in
  eight collect tests became positive reset-state C16 checks.
- Validation: full `make test` exit 0 (636/636 unit binaries
  direct plus 286 quality tests, evidence PASS); generator
  `--check` clean; `make cmake-build` plus full ctest 643/643.
  ABI pins moved: root 456/420, input 80/76, rows 309, IDs 310.
- Knobs: none added, changed, or retired.
- Follow-up: T2-05 sibling doubles (DADST/DRSUB/DSADT…), then
  READA/WRITA; first standalone dast_boot probe (same
  `dsp_full.bin` DSP_COSIM uses, staged overlay in /tmp only)
  retires the reset vector FF80→0F00 per the profile, then
  halts on unbound `LD #lk, ARn` (`0x7214` at `0xF07`,
  black-box oracle shows `0x7212` writing AR2) past a `0x0028`
  peripheral DATA read. No firmware bytes enter the repo.
- Split commit discipline: impl `31bc99d`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S56 lands as 59d1398 (MVDM absolute + ABSOLUTE source)

- Commit `59d1398` binds the MVDM absolute row (`0x7200/0xFFE0`,
  MMR5_ABSOLUTE16 layout: 5-bit MMR destination plus second-word
  16-bit absolute DATA address) with the pure `op_mvdm` leaf (one
  ABSOLUTE_DATA_WORD16 READ with the read-phase MMR store sidecar)
  and the full root seam through CPU retirement. Registry:
  FORM-T2-26-03 SUBSET (`ABSOLUTE_DATA`), regenerated lookup
  (+32, census 37929), coverage 158/205 (77.073%).
- Root causes fixed during bring-up (all reproduced RED-first
  against `test_cpu_mvdm`): (1) the test word targeted a
  single-word form that does not exist — MVDM absolute is two
  words (`0x7214` + extension, per the 5110 boot halt); the
  shared CPU fixture learned a second program word for extension
  fetch. (2) Borrowing the DP planner stamped DIRECT_DP, whose
  dependency gate demands live CPL/DP — wrong for absolute
  addressing. New ABSOLUTE source kind (10) with its own planner
  (`src/address/absolute.c`), intrinsic/dependency arms, and
  candidate echo; the absolute resolution branch then extracted
  to its own module. (3) `valid_source_kind` (not the policy
  predicate) gates new kinds — one-line miss, caught by direct
  validate probing. (4) the MMR sidecar's spare byte broke the
  ILP32 40-byte effect ABI (LP64 padding hid it); dropped the
  spare, both ABIs hold.
- Decisions: MMR store rides the local `mmr_route` (no endpoint
  traffic; one DATA READ per retirement), publishing after the
  read resolves and before the architecture commit; unroutable
  MMR ids (2,3,4,5,0x1E,0x1F) reject at the shape gate, so the
  commit-sidecar decline arm is forgery-only. Absolute observes
  no state: DP/CPL/CMPT/ARP/AR unknownness never rejects.
- Validation: full `make test` exit 0 (640/640 unit binaries
  direct plus 286 quality tests, evidence PASS); `make cmake-build`
  plus full ctest 647/647. ABI pins moved: rows 310, IDs 311,
  lookup 37929.
- Knobs: none added, changed, or retired.
- Follow-up: MVDM indirect flavor (own slice), then MVMD/MVMM;
  the 5110 boot probe now retires past `MVDM 0x0000,AR4` at
  `0xF07` toward the unbound `LD #lk` halt. No firmware bytes
  enter the repo.
- Split commit discipline: impl `59d1398`, this ledger-only
  follow-up.

## 2026-09-19 — P3-S57 lands (5110 boot-probe first light, docs-only)

- No code, test, registry, or generated changes: single docs
  commit carrying the new slice doc, the profile refresh, the
  README refresh, the plan link, and this entry. Tree verified
  green by the S56 gates (`make test` exit 0, ctest 647/647);
  docs changes re-run nothing.
- Probe datum (image `dsp_full.bin` staged at `/tmp` only):
  reset `ENTRY=0xFF80` retires `STM #0xFFA8,PMST`, `B 0xFF85`,
  `B 0x0F00` (attempted=4, retired=3, `prog_reads=7`), then
  `INVALID_STATE` on the zero overlay word at `0x0F00` (decodes
  `ADD_SMEM`, DP unknown). PROM `ENTRY=0x3000` stops
  `INVALID_STATE` on `CALL_PMAD 0x23CC` (SP unknown). Both stops
  are correct cold-reset behavior and match the profile's
  predicted coverage limit (`data_txn=0` in both reports).
- Decisions: the S56-predicted `MVDM`-at-`0xF07` halt is not yet
  observable — the `0xF00` overlay is MCU-uploaded (zeros in the
  raw image) and the sibling `re/dsp/block*.bin` files are DATA
  uploads, so the upload map stays a follow-up; no image bytes
  staged, patched, or committed anywhere in this slice.
- Knobs: none added, changed, or retired.
- Follow-up: MCU overlay upload map, harness DP/AR/SP seeding
  (or core reset coverage), then MVMD/MVMM rows toward the next
  halt.

## 2026-09-19 — P3-S58 lands as d9349db (MVMD MMR-to-Smem slice)

- Implementation commit `d9349db`: MVMD `0x73` row mirrors MVDM on
  the shared MMR5_ABSOLUTE16 layout with the MMR sidecar in the
  absolute DATA spec arm (`STORE_WITH_READ`/`LOAD_WITH_WRITE`);
  resume-path MMR snapshot patching prepared WRITE; MMR load leg
  extracted to `memory_instruction_mmr_load`. New leaf, CPU,
  resolution, and metadata tests; STM near-miss test pins the
  `0x7300` sibling; catalog IDs shift 311 to 312 with pins green.
- Validation evidence: `make test` exit 0, ctest 650/650 in
  `build-cmake`, `check_evidence.py` PASS (targets=2, sources=7,
  requirements=801, tests=654), lookup regenerated.
- Decisions: snapshot matcher moved from a static copy in
  `memory_instruction_transaction.c` into the candidate-validate
  module (implements the already-declared public symbol); the
  LOAD leg is exempt from the full-shape value echo in candidate
  validate because the snapshot design stages MMR values on the
  transaction, not the copied shape.
- Knobs: none added, changed, or retired.
- Follow-up: MVMM/MVDK/MVKD rows toward the `0xF00` halt, MCU
  overlay upload map, harness DP/AR/SP seeding.

## 2026-09-19 — P3-S59 lands as 7cdb2ff (MVMM MMR-to-MMR slice)

- Implementation commit `7cdb2ff`: MVMM `0xE7` row with the new
  one-word `MMR4_MMR4` layout stages an absolute DATA READ from
  `0x10+MMRX` with a `STORE_WITH_READ` sidecar to `0x10+MMRY`,
  reusing the MVDM read leg, the MMR-routed issue path, and the
  read-phase MMR store commit block with zero sequencer changes.
  New leaf, CPU (incl. the TI SP-to-AR1 example), resolution-free,
  and metadata tests; repeat eligibility pins Table 2-28
  INELIGIBLE. Catalog IDs shift 312 to 313 with pins green.
- Validation evidence: `make test` exit 0, ctest 653/653 in
  `build-cmake`, `check_evidence.py` PASS (targets=2, sources=7,
  requirements=801, tests=654), lookup regenerated (+256 words
  at `38217`), coverage now 160/205 with strict-complete 45/205.
- Decisions: nibble codes above 8 are rejected with
  INVALID_OPERAND on both leaves (not INVALID_DECODE on the
  checked path) because extraction succeeds structurally and the
  trusted dispatch sweep gates status parity; codes 9-13 would
  otherwise touch routed BK/BRC/RSA/REA/PMST cells the hardware
  form never allows. FORM-T2-26-07 binds COMPLETE/ALL: the one
  row covers the full legal operand space.
- Knobs: none added, changed, or retired.
- Follow-up: MVDK/MVKD class-19 Smem-immediate moves, MVPD/MVDP,
  then the `0xF00` halt re-probe with MVMM in the image path.

## 2026-09-19 — P3-S60 lands as 80ad37b (MVDK Smem-to-absolute slice)

- Implementation commit `80ad37b`: two rows (`MVDK_DIRECT`
  `0x7100`, `MVDK_SMEM` `0x7180`) share
  `INDIRECT_ADDR7_EXTENSION16` and stage the new
  `SMEM_ABSOLUTE_PAIR_WORD16` carrier (Smem DATA leg read
  first, absolute dmad WRITE second, PSHD access convention).
  New pair root mirrors the PORT/stack roots: single-leg Smem
  resolution, absolute dmad planning, per-leg
  MMR-route-or-endpoint issue with observation handoff. New
  leaf, CPU (direct copy, indirect with AR update, bad-mode
  trap), and metadata tests; FORM-T2-26-02 binds
  SUBSET with DIRECT_DP + STANDARD_INDIRECT_MOD0_MOD3.
  Catalog IDs shift 313 to 315 with pins green.
- Validation evidence: `make test` exit 0, ctest 657/657 in
  `build-cmake`, `check_evidence.py` PASS (targets=2, sources=7,
  requirements=801, tests=654), lookup regenerated (+256 words
  at `38473`), coverage now 161/205 (78.537%) with
  strict-complete steady at 45/205.
- Decisions: the absolute resolver now admits plain WRITE
  templates (staged zero, replaced at second-leg issue); two
  absolute-resolution tests move from reject to resolve pins.
  The pair shape validator started inline in `memory_effect.c`
  but tripped the 500-line module gate, so it extracted to
  `memory_effect_smem_absolute_pair` (sibling-module pattern).
  Split decision: the pair root stays one 627-line module
  (mirrors 654-line `stack_pair.c`; under the 800 cap;
  splitting prepare/advance would separate snapshot
  producer/consumer).
- Knobs: none added, changed, or retired.
- Follow-up: MVKD mirror on the same carrier (access READ),
  three-word absolute/long-offset Smem, then the `0xF00` halt
  re-probe.

## 2026-09-19 — P3-S61 lands as 304b1d8 (MVKD absolute-to-Smem slice)

- Implementation commit `304b1d8`: two rows (`MVKD_DIRECT`
  `0x7000`, `MVKD_SMEM` `0x7080`) reuse the S60
  `SMEM_ABSOLUTE_PAIR` carrier with access READ (POPD
  convention): the dmad leg reads first, the Smem leg writes
  second. No resolver, issue, or commit changes — the S60 root
  already implemented both leg orders. New leaf, CPU (TI
  p. 4-113 examples both directions with AR update), and
  metadata tests; FORM-T2-26-05 binds SUBSET with DIRECT_DP +
  STANDARD_INDIRECT_MOD0_MOD3. Catalog IDs shift 315 to 317
  with pins green.
- Validation evidence: `make test` exit 0, ctest 661/661 in
  `build-cmake`, `check_evidence.py` PASS (targets=2, sources=7,
  requirements=801, tests=654), lookup regenerated (+256 words
  at `38729`), coverage now 162/205 (79.024%) with
  strict-complete steady at 45/205.
- Decisions: the pair shape validator derives the Smem-leg
  access as the inverse of the effect access; the shape tail
  gains an explicit READ-template arm (the generic slot aliases
  pair snapshots and would read union garbage). The feared
  `0x73` MVMD/MVKD collision does not exist: MVKD pins `0x70`
  (TI p. 4-113), a clean nibble ladder 0x70/0x71/0x72/0x73.
  Five neighbor near-miss tests pin new MVKD siblings
  (LD_T/MVKD_SMEM, LD_LK16/LD_LK_SHIFT/B_PMAD/CALL_PMAD/RPT_LK
  MVKD_DIRECT, LD_T catalog).
- Knobs: none added, changed, or retired.
- Follow-up: MVPD/MVDP parallel moves, READA/WRITA, then the
  `0xF00` halt re-probe with the full move group in the image
  path.

## 2026-09-19 — P3-S62 lands as f056263 (MVPD program-to-Smem slice)

- Implementation commit `f056263`: two rows (`MVPD_DIRECT`
  `0x7D00`, `MVPD_SMEM` `0x7D80`) on a new
  `SMEM_PROGRAM_PAIR` carrier with access WRITE (PORT
  convention): the PROGRAM leg reads first, the Smem leg
  writes second. New pair root mirrors the S60 root with
  per-leg MMR-route-or-endpoint service; the Smem AR update
  publishes beside the stack pair's. New leaf, CPU (direct
  copy plus MOD3 pre-increment with AR update), catalog, and
  neighbor-row tests; FORM-T2-26-08 binds SUBSET with
  DIRECT_DP + STANDARD_INDIRECT_MOD0_MOD3. Catalog IDs shift
  317 to 319 with pins green.
- Validation evidence: `make test` exit 0, ctest 665/665 in
  `build-cmake`, `check_evidence.py` PASS (targets=2,
  sources=3, requirements=4, tests=3), lookup regenerated
  (+256 words at `38985`), coverage now 163/205 (79.512%)
  with strict-complete steady at 45/205.
- Decisions: the first-drafted rows sat on `0x7800` inside the
  `MACP_SMEM` `0xFE00` mask and were moved to `0x7D00`/`0x7D80`
  (the MACP CPU tests behaviorally pin `0x78`/`0x79` with `S`
  at bit 8); neighbor pins assert `0x78`/`0x79` stay MACP and
  `0x7C` stays undecoded for MVDP. The leaf initially staged
  the S61 `SMEM_ABSOLUTE_PAIR` kind and now stages
  `SMEM_PROGRAM_PAIR`. PROGRAM legs bypass the DATA/IO-only
  endpoint adapter with the same begin/poll/complete sequence
  (DATA_PROGRAM precedent). The indirect CPU test asserts the
  library MOD3 pre-increment (address and post AR `0x0FFF`),
  not TI subtract-AR0 (outside the MOD0--MOD3 model).
  Required split: pair-route classification moves out of
  `memory_instruction_transaction.c` (810 to 767 lines) into
  `memory_instruction_child_route`; the next pair slices add
  no lines there.
- Knobs: none added, changed, or retired.
- Follow-up: MVDP mirror on the same carrier (READ access,
  `0x7C` rows), READA/WRITA, then the `0xF00` halt re-probe
  with the full move group in the image path.

## 2026-09-19 — P3-S63 lands as 4e5472d (MVDP Smem-to-program slice)

- Implementation commit `4e5472d`: two rows (`MVDP_DIRECT`
  `0x7C00`, `MVDP_SMEM` `0x7C80`) reuse the S62
  `SMEM_PROGRAM_PAIR` carrier with access READ (PORT
  convention): the Smem leg reads first, the PROGRAM leg
  writes second. No carrier, root, resolver, issue, or
  commit changes — the S62 root already implements both leg
  orders keyed on the effect access, including the PROGRAM
  second-leg direct endpoint drive. New leaf, CPU (direct
  copy plus MOD2 post-increment with AR update, MOD3-read
  transactional reject, extension-cookie reject), catalog,
  neighbor, and BANZ/RET/RC/MACP/MACD decode-sibling tests;
  FORM-T2-26-04 binds SUBSET with DIRECT_DP +
  STANDARD_INDIRECT_MOD0_MOD3. Catalog IDs shift 319 to 321
  with pins green.
- Validation evidence: `make test` exit 0, ctest 669/669 in
  `build-cmake`, `check_evidence.py` PASS (targets=2,
  sources=3, requirements=4, tests=3), lookup regenerated
  (+256 words at `39241`), coverage now 164/205 (80.000%)
  with strict-complete steady at 45/205.
- Decisions: the indirect CPU test uses MOD2 (post-inc)
  because MOD3 reads are unsupported in the address planner
  (established `UNSUPPORTED_ACCESS` rule for pre-access
  updates on reads); a dedicated CPU test pins the
  transactional reject with no endpoint traffic. The new
  `0x7C` rows absorbed four stale UNKNOWN pins (BANZ/RET/RC
  bit-flips, MACP/MACD invalid lists), converted to MVDP
  sibling pins in matching style.
- Knobs: none added, changed, or retired.
- Follow-up: READA/WRITA, then the `0xF00` halt re-probe with
  the full move group in the image path.

## 2026-09-19 — Correction to P3-S62/S63 (MVPD/MVDP row swap, fec52da)

- Correction commit `fec52da` (no history rewritten): the S62/S63
  rows shipped swapped from recollection (MVPD at `0x7D`,
  MVDP at `0x7C`). The vendored TI text section diagrams are
  unambiguous once character-aligned: MVPD `pmad,Smem`
  (p. 4-117) carries bit8=0 (`0x7C00`/`0x7C80`) and MVDP
  `Smem,pmad` (p. 4-111) carries bit8=1 (`0x7D00`/`0x7D80`);
  the execution prose agrees (`pmad -> PAR` then
  `Smem -> Pmem[PAR]` for MVDP). The earlier collision
  analysis was also inverted: `0x78`/`0x79` were never in
  danger (MACP owns them per its own diagram at bits
  `0111 100S`); the real neighbors are MACD (`0111 101S`,
  `0x7A`/`0x7B`), then MVPD (`0x7C`), MVDP (`0x7D`), READA
  (`0x7E`, one word), WRITA (`0x7F`, one word).
- `fec52da` swaps the four catalog rows, all leaf/CPU/metadata
  test words, the five decode sibling pins (`0x7C00` now
  `MVPD_DIRECT`), and both slice-doc bodies; the lookup is
  regenerated with the same population (`39241`). No carrier,
  leaf-logic, root, inventory, or pin-count changes were
  needed — the swap is address-value-only.
- Validation evidence: `make test` exit 0, ctest 669/669 in
  `build-cmake`, coverage steady at 164/205 (80.000%).
- Lesson recorded: row bases must be read off the vendored
  section diagrams with explicit bit-position alignment,
  never from recollection; the first S62 draft (0x78) and
  the swap both came from memory.
- Follow-up: READA (`0x7E`) and WRITA (`0x7F`) are one-word
  accumulator-A-addressed forms needing their own carrier
  (PAR/RC repeat behavior stays out of scope); then the
  `0xF00` halt re-probe.

## 2026-09-19 — P3-S64 lands as 62f9e36 (READA program-to-Smem slice)

- Implementation commit `62f9e36`: two rows (`READA_DIRECT`
  `0x7E00`, `READA_SMEM` `0x7E80`) on a new
  `SMEM_PROGRAM_A_PAIR` carrier with access WRITE (PORT
  convention): the PROGRAM leg address resolves from
  accumulator A (PAR <- A, low 16 bits) and reads first, the
  Smem leg writes second. The new kind routes through the
  existing SMEM_PROGRAM pair route, root, resolver, and
  commit seams with per-leg MMR-route-or-endpoint service.
  New leaf (DP/A knownness gates, extension-cookie reject),
  arm validator, CPU (direct copy, MOD2 post-increment with
  AR update, bad-mode and A-unknown transactional rejects),
  catalog, neighbor, and BANZD/RCD/RETD/ST decode-sibling
  tests; FORM-T2-26-11 binds SUBSET with DIRECT_DP +
  STANDARD_INDIRECT_MOD0_MOD3. Catalog IDs move 321 to 323
  with pins green.
- Validation evidence: `make test` exit 0 (286/286 contract
  quality, evidence registry PASS, all C binaries, minimal),
  lookup regenerated (+256 words at `39497`), coverage now
  165/205 (80.488%) with strict-complete steady at 45/205.
  CMake ctest not re-run (stale cmake/sanitizer build trees
  removed this session; manifest parity covered by
  check_build_manifests).
- Decisions: the missing child-route mapping (new kind to the
  existing pair route) was the CPU INVALID_STATE root cause
  (ROUTE_NONE skips prepare, so no endpoint traffic issues);
  trusted leaves keep the sibling `(void)instruction`
  contract with no NULL-instruction guard (null test matches
  test_mvdp.c); `memory_effect.h` stands at 517 lines with
  the new arm under the reviewed no-split decision (the
  one-arm-per-kind ABI union cannot split across headers;
  the dual hot-path bound carries the <800 exemption). The
  new `0x7E` rows absorbed five stale UNKNOWN pins
  (BANZD/RCD/RETD bit-flips, ST decode/metadata invalid
  lists), converted to READA sibling pins in matching style.
- Knobs: none added, changed, or retired.
- Follow-up: WRITA (`0x7F`, mirrors READA with access READ),
  then the `0xF00` halt re-probe with the full move group in
  the image path.

## 2026-09-19 — P3-S65 lands as ee7bb59 (WRITA Smem-to-program slice)

- Implementation commit `ee7bb59`: two rows (`WRITA_DIRECT`
  `0x7F00`, `WRITA_SMEM` `0x7F80`) reuse the S64
  `SMEM_PROGRAM_A_PAIR` carrier with access READ (PORT
  convention): the Smem leg reads first, the PROGRAM leg
  writes second with the address from accumulator A
  (PAR <- A, low 16 bits). No carrier, root, resolver,
  issue, or commit changes — the S64 root already implements
  both leg orders keyed on the effect access with the
  A-derived PROGRAM address. New leaf, CPU (direct copy
  plus MOD2 post-increment with AR update, MOD3-read
  transactional reject, extension-cookie and A-unknown
  rejects), catalog, neighbor, and LD/STL/STM
  decode-sibling tests; FORM-T2-26-12 binds SUBSET with
  DIRECT_DP + STANDARD_INDIRECT_MOD0_MOD3. Catalog IDs move
  323 to 325 with pins green.
- Validation evidence: `make test` exit 0 (286/286 contract
  quality, evidence registry PASS, all C binaries, minimal),
  lookup regenerated (+256 words at `39753`), coverage now
  166/205 (80.976%) with strict-complete steady at 45/205.
  CMake ctest not re-run (stale trees removed in S64;
  manifest parity covered by check_build_manifests).
- Decisions: the S64 child-route fix covers WRITA with no
  further root work (first CPU run retired cleanly);
  trusted leaves mirror the `(void)instruction` contract;
  the new `0x7F` rows absorbed four stale UNKNOWN pins
  (LD-shift/STM bit-flips, STL decode/metadata 0x7FFF
  boundaries), converted to WRITA sibling pins in matching
  style.
- Knobs: none added, changed, or retired.
- Follow-up: the `0xF00` halt re-probe with the full move
  group in the image path.

## 2026-09-19 — 0xF00 halt re-probe with the full move group (no behavior change)

- Re-ran the S57 boot probe (`make run-image`, image
  `dsp_full.bin` staged at `/tmp` only, `nokia-ti-lead-rom4`
  profile, `ORDER=be DATA_BASE=0x800`) after the S58–S65
  move-group landings
  (MVDM/MVMD/MVMM/MVDK/MVKD/MVPD/MVDP/READA/WRITA).
- Reset `ENTRY=0xFF80` budget 10: identical to S57 —
  `attempted=4 retired=3 pc=0xF00 opcode=0x0000
  prog_reads=7`, stop INVALID_STATE on the zero overlay
  word (unknown DP), no DATA/IO traffic.
- PROM `ENTRY=0x3000` budget 50: identical to S57 —
  `attempted=1 retired=0`, stop INVALID_STATE on `CALL_PMAD
  0x23CC` (unknown SP).
- Conclusion: both stops are unchanged correct cold-reset
  behavior; no move-group form executes on either path
  before the halt, so the new coverage is unreachable from
  a cold reset. Reaching it still needs the recorded gaps:
  MCU overlay staging at `0xF00` (S56 predicts `MVDM
  0x0000,AR4` at `0xF07`) and harness-side DP/AR/SP
  seeding. Hosted runner selftest PASS (2 vectors) on the
  WRITA-inclusive build.
- Follow-up: overlay staging plus register seeding (harness
  gap); then re-probe again.

## 2026-09-19 — 0xF00 overlay re-probe: halt moves to UNIMPLEMENTED 0x1377 (LDU direct)

- Correction to the previous entry: with block04 staged as
  the program-space overlay (`re/dsp/block04.bin`, 1737
  words at DSP `0x0926`, covering `0x926-0xFEE`), the reset
  probe retires the same STM+B+B, then fetches `0x1377` at
  `0xF00` and stops UNIMPLEMENTED_OPCODE (`attempted=4
  retired=3 prog_reads=7`, still no DATA/IO traffic). The
  PROM vector is unchanged (CALL_PMAD, unknown SP).
- `0x1377` is LDU Smem,dst direct (TI SPRU172C p. 4-78,
  diagram `0001001DIAAAAAAA`, D=1/dst-B, I=0 direct,
  dma `0x77`): the S34 `LDU` row (`0x1290`/`0xFEF8`) covers
  only the I=1 indirect form, so `0x1300-0x137F` is a
  catalog gap sitting directly below the LDU MOD4-MOD10
  fan-out at `0x1380+`. The natural next row is
  `0x1200`/`0xFE80` (D variable, I=0 fixed, 256 encodings).
- Note: retiring past `0xF00` will additionally need DP
  seeding (direct dma with unknown DP rejects), and S56's
  predicted `MVDM` at `0xF07` sits behind this LDU word.
- Staging: `dsp_full.bin` now lives in-repo at
  `probe-images/` (gitignored via `*.bin`, invisible to all
  quality gates); the overlay recipe (block04 at `0x0926`)
  is reproduced above. The emu was used only to locate the
  block files, per the authorized black-box use.
- Follow-up: LDU-direct slice, then DP seeding, then
  re-probe again.

## 2026-09-19 — Correction: faithful cold 0xF00 is loader1, not block04

- The previous entry's block04 overlay was unfaithful: the
  cosim boot doc (used as oracle, no source read) shows the
  MCU stages **loader1** (flash boot-block 25, 180 words,
  first word `0xF7BB`, verified against the descriptor) at
  DSP `0xF00` before reset release; block04 is run-mode
  code placed later. The `0x1377`/LDU-direct identification
  stands as pure TI evidence (p. 4-78 diagram, and
  `0x1300-0x137F` is a real catalog gap below the LDU
  fan-out), but it is NOT the boot path's next form.
- Faithful cold run through this runner
  (`probe-images/dsp_full.bin` + loader1 at `0xF00`,
  `BUDGET=200`): `attempted=7 retired=6` — STM, B, B, then
  three loader1 words including its own SP/PMST setup —
  then `BUS_FAULT` on the first DATA touch at `0x0028`
  (`data_txn=1`). No register seeding was needed to reach
  live DATA traffic. Profile `nokia-ti-lead-rom4` updated
  with the staged-boot recipe and observation.
- Next boot-path need: the `0x0028` map fact (peripheral
  MMR region — device gap, out of scope since S57), then
  loader1's MBOX spin (needs an MCU counterpart, harness
  gap). S56's `MVDM` at `0xF07` prediction stays parked
  behind the warm-boot rehome sequence.
- Follow-up: `0x0028` device-map decision; loader1
  continuation past the MBOX spin needs MCU emulation.

## 2026-09-19 — P3-S66 lands as 546a396 (LDU Smem-direct load slice)

- Implementation commit `546a396`: one row (`LDU_DIRECT`
  `0x1200` under `0xFE80`, D destination at bit 8, I pinned 0
  at bit 7, 7-bit DP-relative direct cookie at bits 6:0)
  zero-extends the DP-relative Smem word into accumulator A
  or B. New checked/trusted `op_ldu_direct` leaf mirroring
  `op_ldm_smem_direct` (same layout, same `DIRECT_WORD16`
  read effect, same zero-extending completion) plus the
  DP-knownness `UNKNOWN_INPUT` gate the MMR-addressed LDM
  leaf does not need. The I=1 indirect half stays with the
  S34 rows. FORM-T2-19-20 binds the row SUBSET with
  `DIRECT_DP` (eighth binding on that form). Catalog IDs
  move 325 to 326 with pins green.
- Validation evidence: `make test` exit 0 (105 C suites ok,
  286/286 contract quality, evidence/knob/boundary/
  manifest/observer checks PASS), lookup regenerated (+256
  `LDU_DIRECT` words, 40009 populated entries), coverage
  holds at 166/205 family (80.976%) with strict-complete
  steady at 45/205. CMake ctest not re-run (stale trees
  removed in S64; manifest parity covered by
  check_build_manifests).
- Decisions: the `0x1234` probe word now decodes as
  `LDU_DIRECT`, so unrelated CPU/observer probe fixtures
  moved to `0x1000` and the LDU_SMEM fixed-bit near-miss
  pins claim the `0x1200` cross-row instead of UNKNOWN;
  three per-form inventory pin tests (multi, circular,
  reverse-carry) now expect the 8-row LDU family shape with
  the `DIRECT_DP` subset; README census counts stay frozen
  (last refreshed S57) per sibling-slice scope.
- Knobs: none added, changed, or retired.
- Follow-up: `0x0028` device-map decision; loader1
  continuation past the MBOX spin needs MCU emulation.

## 2026-09-19 — P3-S67 lands as 76beb20 (SWWSR peripheral-MMR slice)

- Implementation commit `76beb20`: DATA `0x28` routes to a
  new devices-owned SWWSR cell (TI SPRU131 Tables 8-2/10-2,
  Figure 10-2, vendored in-repo): bits 14:0 R/W, bit 15
  reserved with writes masked to zero, reset `0x7FFF`
  known. The cell rides on `arch_state` beside `mmr_file`
  and resets with the documented common subset. The MMR
  router serves `0x28` both directions; `is_routed_address`
  names it explicitly with the `0x1F` CPU window untouched,
  so planner arms needed no changes. New `src/devices/`
  directory (plan-sanctioned peripheral owner) with
  cmake/mk manifests; new `tests/unit/devices/` mirror.
  No opcode, catalog, inventory, knob, or timing changes.
- Validation evidence: `make test` exit 0 (full C suite
  incl. new `devices_swwsr`/`cpu_swwsr`, 286/286 contract
  quality, evidence/knob/boundary/manifest/observer checks
  PASS), coverage steady at 166/205 family with strict
  45/205. RED observed first (route declined `0x28`,
  CPU round trip hit the endpoint). Hosted acceptance:
  faithful loader1 run advances past `0xF05`
  (`attempted=8 retired=7`) to the next explicit stop at
  `0xF07` (`STM` to `0x58` CLKMD, `first_data_fault=0x0058`).
- Decisions: explicit single-address routing instead of
  window widening (neighbors `0x27`/`0x29` stay declined
  with pins); bit-15 mask-on-write models the diagram R
  marking; LDM (not MVMD, whose MMR field is 5-bit) is the
  CPU read-back leg; wait-state timing function stays out
  per the machine-cycle rule.
- Knobs: none added, changed, or retired.
- Follow-up: `0x58` CLKMD slice (predicted next boot stop);
  loader1 MBOX spin still needs MCU emulation.

## 2026-09-19 — P3-S68 lands as 76efb24 (CLKMD peripheral-MMR slice)

- Implementation commit `76efb24`: DATA `0x58` routes to a
  second devices-owned cell, the clock-mode register (TI
  SPRU131 section 8, Figure 8-4, Tables 8-18/8-19, vendored
  in-repo; RE loader1 symbol names the address `clkmd`):
  bits 15:1 R/W PLL configuration, bit 0 PLLSTATUS R with
  writes masked to zero. Reset is CLKMD-pin-dependent
  (Table 8-16; the C542 table marks `0x58` reserved), so no
  reset value is adopted — the cell resets UNKNOWN per
  standing policy and loader1's `STM #0x3146` makes it
  known. Router serves `0x58` both directions with the
  `0x1F` window untouched; planner arms unchanged. No
  opcode, catalog, inventory, knob, or timing changes.
- Validation evidence: `make test` exit 0 (full C suite
  incl. new `devices_clkmd`/`cpu_clkmd`, 286/286 contract
  quality, all checkers PASS), coverage steady at 166/205
  family with strict 45/205. RED observed first (route
  declined `0x58`, CPU retirement hit the endpoint).
  Hosted acceptance: faithful loader1 run advances past
  `0xF07` (`attempted=10 retired=9`, zero data faults) to
  the next explicit stop at `0xF0B` (`RPT #0x140` then
  NOP, `INVALID_STATE` — NOP repeatability is
  UNRESOLVED in the catalog).
- Decisions: reset-unknown (not a Table 8-16 default) since
  strapping is unevidenced for this board; explicit
  single-address routing with `0x57`/`0x59` declined pins;
  PLL lock/status dynamics and clock function out per the
  machine-cycle rule.
- Knobs: none added, changed, or retired.
- Follow-up: RPT+NOP repeat-eligibility (Table 2-28
  evidence for the delay-loop idiom); loader1 MBOX spin
  still needs MCU emulation.

## 2026-09-19 — P3-S69 lands as 634b25c (NOP repeat-eligibility slice)

- Implementation commit `634b25c`: one-field catalog
  change, NOP row `UNRESOLVED` to `ELIGIBLE`. NOP is
  absent from the Table 2-28 nonrepeatable list (TI
  SPRU172C section 2, vendored in-repo), resolved per the
  W52A rule with BITF/CMPM/BITT precedent. No sequencer,
  eligibility, planner, knob, or timing changes.
- Siblings: metadata NOP pin and catalog-contract NOP row
  pin to ELIGIBLE; the two `test_cpu_rpt` UNRESOLVED-body
  cases move to the still-UNRESOLVED `STM #0x2468,T` body
  with identical block semantics; the rpt `read_program`
  serves by address (HOLD re-drives body fetches, MAC
  replay precedent). New eligible-body retirement proof
  (`RPT #2` + NOP retires 4/4, RC 0, phase COMPLETE).
- Validation evidence: `make test` exit 0 (full C suite,
  286/286 contract quality incl. the fixed catalog pin,
  all checkers PASS), coverage steady at 166/205 family
  with strict 45/205. RED observed first (metadata pin,
  CPU retirement). Hosted acceptance: BUDGET=200 spins
  200/200 inside the `0xF09` loop; BUDGET=400 completes
  it (331 attempted, 330 retired, zero data faults) and
  reaches `0xF0C`, stopping UNIMPLEMENTED_OPCODE on
  `0x76F8` (`ST #4,*(0x803)`, absolute ST immediate).
- Decisions: single-row resolution only — other
  UNRESOLVED rows wait for their own Table 2-28 evidence;
  RPTB/RPTBD block repeat stays out.
- Knobs: none added, changed, or retired.
- Follow-up: absolute `ST #lk` slice for `0x76F8`
  (MVDP at `0xF0F` waits behind it); loader1 MBOX spin
  still needs MCU emulation.

## 2026-09-19 — P3-S70 lands as 4eea349 (ST absolute-immediate slice)

- Implementation commit `4eea349`: new `EXACT` row
  `ST_LK_SMEM_ABSOLUTE` (`0x76F8`, 3 words,
  `INDIRECT_ADDR7_ABSOLUTE16_LK`, `INELIGIBLE`) for TI
  SPRU172C section ST syntax 3 (`ST #lk,*(abs)`, `lk ->
  Smem`, no status bits). New checked/trusted
  `op_st_lk_smem_absolute` pair beside the direct leaf,
  staged through MVDM/MVMD absolute address resolution.
  Bundle capacity 2 to 3 with the 3-word extension-fetch
  loop; decoded carrier 20 to 24 bytes (LP64 packet stays
  104, ILP32 root 420 to 424); lookup regenerated (+1
  word, 40010 populated); catalog IDs 326 to 327;
  FORM-T2-20-03 binds the row SUBSET with `ABSOLUTE_DATA`.
  The `ABSOLUTE_DATA` WRITE arm admits plain sidecar-free
  stores (MVMD zero-template gate unchanged). Coverage
  166/205 family (80.976%), strict 45/205.
- Validation evidence: `make test` exit 0 (full C suite
  incl. new decode/ops/metadata/CPU proofs, 286/286
  contract quality, all checkers PASS). RED observed
  first (effect-shape gate, 3-word fetch, carrier pins,
  catalog pins, CPU retirement). Hosted acceptance:
  BUDGET=400 staged loader1 run retires `ST #4,*(0x803)`
  at `0xF0C` (332 attempted, 331 retired, zero data
  faults with the `0x800` RAM window) and reaches `0xF0F`,
  stopping `INVALID_STATE` on `0x7DF8` (MVDP absolute).
- Decisions: minimal `FORM-T2-20-03` binding (`SUBSET` +
  `ABSOLUTE_DATA`, MVDM/MVMD precedent) to satisfy the
  inventory gate while FORM census stays deferred;
  `0x7680` neighbor probe corrected to `ST_IMMEDIATE_SMEM`
  with `0x76F9` as the long-offset probe; trusted
  deferral forges operands (operands authoritative per
  sibling-leaf precedent).
- Knobs: none added, changed, or retired.
- Follow-up: MVDP absolute slice for `0x7DF8`; loader1
  MBOX spin still needs MCU emulation (P3-S71).

## 2026-09-20 — P3-S71 lands as 2873b36 (MCU upload peer slice)

- Implementation commit `2873b36`: hosted MCU upload peer
  (`tools/hosted/mcu_upload_peer.h/.c`, core untouched)
  playing the oracle-verified MBOX ping-pong protocol
  (boot-sequence doc sections 2-3, cross-checked against
  the architecture doc's live trace: UPLOADHEADER
  `0x87B-0x881`, MBOX0 `0x87F`, MBOX1 `0x880`, 512-word
  buffers A `0x900-0xAFF`/B `0xB00-0xCFF`, polarity 1 =
  MCU owns / 0 = ready, DSP ACKs with 1, 64 blocks
  32+32). Init publishes the canonical header, fills
  buffer A from flash block 0, hands it off; each ACK
  refills the other buffer until the 1-64 block limit,
  then holds. Flash block n streams 512 words from bus
  `0x200040 + n * 0x4000` (verified: loader1 block25
  payload at descriptor `0x2996F8 + 0xC`, first word
  `0xF7BB`, byte-identical to the staged DSP image).
  Runner gains trailing `--mcu-upload <flash> <blocks>`;
  `*.fls` joins the gitignored probe images (5110
  v5.30 A staged at `probe-images/`).
- Validation evidence: `make test` exit 0 (new
  `tests/unit/hosted/test_mcu_upload.c` RED-first green:
  header, polarity, ACK advance, flash slices,
  64-iteration 32+32 split, bounded hold, neighbors
  untouched, init rejection; 286+ contract quality and
  all checkers PASS). Live BUDGET=400 staged loader1 run
  with the peer attached reaches the `0xF0F` frontier
  unchanged (332 attempted, 331 retired, `fills_a=1`,
  zero data faults) — MVDP absolute stops the DSP before
  any MBOX traffic, so end-to-end finalize waits on the
  DSP ISA chain.
- Decisions: unit-test-only `hosted` owner arm in the
  boundary gate (hosted peers live under `tools/`,
  outside the scanned production tree); flash geometry
  as explicit peer parameters with oracle production
  values in the runner; no-data peer runs own a zeroed
  `0x800-0x2800` window; stages 4-5 stay out per slice.
- Knobs: none added, changed, or retired.
- Follow-up: MVDP absolute slice for `0x7DF8`, then the
  DSP CRC/finalize chain; full 64-block upload stays
  bounded (~5h at current core speed, correctness first
  per standing direction).

## 2026-09-20 — P3-S72 lands as adc6633 (MVDP absolute-Smem slice)

- Implementation commit `adc6633`: exact row
  MVDP_SMEM_ABSOLUTE `0x7DF8` with
  INDIRECT_ADDR7_ABSOLUTE16_PMAD plus the new
  SMEM_ABSOLUTE_PROGRAM_PAIR sequencer arm
  (`src/sequencer/instruction/memory_instruction_smem_program_pair*.c`,
  split out of the overgrown pair engine; op leaf
  `src/isa/ops/store/op_mvdp_smem_absolute.c`,
  effect `src/isa/ops/memory/memory_effect_smem_absolute_program_pair.c`).
  Exact-before-masked shadow keeps the `0x7D` neighbors
  on MVDP; pair-validator gate extended for the absolute
  arm. Runner bus zero-initialized and gains a
  `prog_writes` counter (hosted-only reporting).
- Validation evidence: `make test` exit 0 (new
  `test_mvdp_smem_absolute_decode/ops/metadata`,
  `test_cpu_mvdp_smem_absolute` RED-first green; masked
  sweep pins hold neighbors; registries and checkers
  PASS). Boot repro past `0xF0F`: BUDGET=400 staged
  loader1 run with the MCU peer retires 333/332 with
  `prog_writes=1` and PC at `0xF12` (`0x7CF8`,
  MVPD-family follower); targeted ENTRY=0xF0F run
  retires the MVDP alone (data_txn=2, smem read plus
  program write, zero data faults).
- Decisions: absolute destination keeps the S70/S62
  exact-hole pattern (`0x76F8`/`0x7CF8`/`0x7DF8`
  within their masked families); bundle capacity 3 for
  the 3-word absolute fetch; runner zero-init fixes a
  real uninitialized-bus defect, not just reporting.
- Knobs: none added, changed, or retired.
- Follow-up: MVPD absolute slice for `0x7CF8` at
  `0xF12`, then the DSP CRC/finalize chain; full
  64-block upload stays bounded per standing direction.

## 2026-09-20 — P3-S73 lands as 4801cdf (MVPD absolute-Smem slice)

- Implementation commit `4801cdf`: exact row
  MVPD_SMEM_ABSOLUTE `0x7CF8` with the S72
  INDIRECT_ADDR7_ABSOLUTE16_PMAD layout, plus the
  `src/isa/ops/store/op_mvpd_smem_absolute.c` leaf
  staging the established SMEM_ABSOLUTE_PROGRAM_PAIR
  arm (kind 26) with WRITE access. No new effect kind,
  no sequencer changes: the pair prepare/root already
  run the PROGRAM leg first and the Smem leg second
  for WRITE access. Exact-before-masked shadow keeps
  the `0x7C` neighbors on MVPD (masked row keeps 127
  of 128). New catalog ID moves `ID_COUNT` 328 to 329
  with pins; `FORM-T2-26-08` binds the row `SUBSET`
  with `ABSOLUTE_DATA`; no MMR, knob, or timing changes.
- Validation evidence: `make test` exit 0 (new
  `test_mvpd_smem_absolute_decode/ops/metadata`,
  `test_cpu_mvpd_smem_absolute` RED-first green; masked
  sweep pins hold neighbors; registries and checkers
  PASS). Boot repro past `0xF12`: BUDGET=400 staged
  loader1 run with the MCU peer retires 337/336 with
  `data_txn=6` and PC at `0xF71` (`0x4EF8`, DST-family
  follower); zero data faults.
- Decisions: reuse the direction-neutral kind-26 arm
  with WRITE access (the access names the Smem-leg
  direction, MVPD convention) rather than a mirrored
  kind; sibling `test_mvpd_smem_metadata` sweep takes
  the S72 shadow-carve (127 of 128).
- Knobs: none added, changed, or retired.
- Follow-up: DST-family absolute slice for `0x4EF8` at
  `0xF71` (`4EF8 0060 FC00`), then the DSP CRC/finalize
  chain; full 64-block upload stays bounded per standing
  direction.

## 2026-09-20 — P3-S74 lands as 0de98ce (DST absolute-Smem slice)

- Implementation commit `0de98ce`: exact row
  DST_SMEM_ABSOLUTE `0x4EF8` (TI SPRU172C DST
  `0100 111S IAAA AAAA`, SPRU179 `dbl(Lmem) = src`,
  S=0 selects A) with the new 2-word
  ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout
  (S[8:8], I[7:7], 0x78 selector, abs16), plus the
  `src/isa/ops/store/op_dst_smem_absolute.c` leaf
  (accumulator+SXM/SST knownness gates, MOD13
  saturation core) staging the new
  LONG32_ABSOLUTE_STORE arm (kind 27: absolute DATA
  address, staged MSW/LSW halves, WRITE) through the
  established long32 pair engine (new resolver
  `address_resolution_long32_absolute_store.c`, new
  candidate shape/liveness
  `memory_candidate_long32_absolute_store.c`, route
  admissions). The MOD13 LONG32 branch splits out of
  `address_resolution.c` into
  `address_resolution_long32_mod13.c` (802 to 759
  lines, SIZE gate holds). New catalog ID moves
  `ID_COUNT` 329 to 330 with pins;
  `FORM-T2-20-01` binds the row `SUBSET` with
  `ABSOLUTE_DATA`; populated first words 40010 to
  40011 (first exact row on a previously unmapped
  word); no MMR, knob, or timing changes.
- Validation evidence: `make test` exit 0 (new
  `test_dst_smem_absolute_decode/ops/metadata`,
  `test_cpu_dst_smem_absolute` RED-first green;
  MOD13 lane untouched and green; registries and
  checkers PASS). Boot repro past `0xF71` with a
  page-0 DATA window (`/tmp/ram_0.bin` at base 0;
  the old `0x800`-base window faulted the `0x60`
  write): BUDGET=400 staged loader1 run with the MCU
  peer retires 339/338 with `data_txn=9` (MSW/LSW
  pair issued, zero data faults) and reaches `0xF17`
  (`0x10F8`, LD-family absolute hole) via the
  implemented `0xFC00` row at `0xF73`. Targeted
  cold-start ENTRY=0xF71 run correctly rejects on
  the unknown accumulator (UNKNOWN_INPUT), proving
  the transactional gate.
- Decisions: S bit decoded through the layout (not
  the family mask) with an S-masked exact-word check
  in extract; MSW/LSW halves alias the single-word
  value/rmw union slots (MOD13 precedent), so the
  kind-27 shape gate checks WRITE access and null
  completion only; ABSOLUTE source admits canonical
  LONG32 in the intrinsic validator (DIRECT_PAGE0
  precedent); MSW-at-base/LSW-above order follows
  the `data_operand_long32` canonical pairing, with
  differential value-order check against the oracle
  as follow-up.
- Knobs: none added, changed, or retired.
- Follow-up: LD-family absolute slice for `0x10F8`
  at `0xF17` (`10F8 087F F844`), then the DSP
  CRC/finalize chain; full 64-block upload stays
  bounded per standing direction.

## 2026-09-20 — P3-S75 lands as 422b14e (LD absolute-Smem slice)

- Implementation commit `422b14e`: exact row
  LD_SMEM_ABSOLUTE `0x10F8` (TI SPRU172C LD
  `0001 000D IAAA AAAA`, D=0 selects A) reusing the
  S74 ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout,
  plus the `src/isa/ops/load/op_ld_smem_absolute.c`
  leaf (SXM knownness gate, S-masked exact-word
  extract) staging the established ABSOLUTE_DATA
  arm (kind 22, READ, sidecar NONE) with an
  SXM-widening accumulator-write read-value
  completion (LD_SMEM completion precedent, C/TC
  preserved). No new effect kind, no sequencer
  changes. New catalog ID moves `ID_COUNT` 330 to
  331 with pins; `FORM-T2-19-02` binds the row
  `SUBSET` with `ABSOLUTE_DATA` (with the S66-style
  carve in the three multi-binding tests);
  populated first words 40011 to 40012; no MMR,
  knob, or timing changes.
- Validation evidence: `make test` exit 0 (new
  `test_ld_smem_absolute_decode/ops/metadata`,
  `test_cpu_ld_smem_absolute` RED-first green;
  MOD5/MOD6 and fixed-mod2 sweeps hold neighbors;
  registries and checkers PASS). Boot repro past
  `0xF17` with the page-0 DATA window: BUDGET=400
  staged loader1 run with the MCU peer retires
  343/342 with `data_txn=12` (MBOX0 poll issued,
  zero data faults) and reaches `0xF1F` (`0x6BF8`,
  ADDM absolute hole `6BF8 0013 0800`) via the
  implemented `0xF844` row at `0xF19` and two
  `0x72xx` moves.
- Decisions: row sits after the LD_SMEM indirect
  family (not adjacent to LD_SMEM) to keep the
  positionally pinned catalog head (`rows[5]` STL,
  `rows[6]` STH) stable; 40-bit SXM widening
  follows the leaf formula (`0xFFFFFF87F0`, not a
  56-bit value); completion is a public leaf entry
  point for direct RED tests.
- Knobs: none added, changed, or retired.
- Follow-up: ADDM absolute slice for `0x6BF8` at
  `0xF1F` (`6BF8 0013 0800`), then the DSP
  CRC/finalize chain; full 64-block upload stays
  bounded per standing direction.
