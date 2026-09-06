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
