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
