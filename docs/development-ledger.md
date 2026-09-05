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
