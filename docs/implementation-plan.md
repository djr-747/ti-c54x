# ti-c54x implementation plan

Status: the standalone foundation baseline is established and ISA vertical slices
are the active track. P2-W31B1 now implements the transactional DATA endpoint and
persistent memory-instruction root. P2-W31P remains open with structural and
correctness gates green; optimized hosted cohorts are below the 60 ns median
target, while exact three-capture stability acceptance is pending/deferred.
That hosted follow-up did not block P2-W32, which is now implementation-complete
with its local gates green. W31B2 catalog/decode metadata, pure generic MEMORY
effects, CPU end-to-end retirement, focused/full/sanitizer validation, and
independent review are complete; its immediate-READY benchmark follow-up
remains pending.

Execution history is recorded in `docs/development-ledger.md`. This plan describes
current intent; the ledger records completed work and decisions without rewriting
history.

## Objective

Create a small, high-quality, vendor-neutral C11 implementation of the TI C54x DSP
architecture. The library must build and test independently, model DSP-specific
behavior directly, and use mGBA-style macro-generated instruction wrappers and
function-pointer dispatch tables.

## Current work pieces

| Work piece | State | Exit condition |
|---|---|---|
| Repository charter and isolation | Complete | Standalone rules and dependency boundary documented |
| Legacy lineage audit | Complete | Exact imported interpreter base and Osmocom relationship recorded |
| Distribution license decision | Blocked on owner/legal review | New repository license and permitted evidence inputs selected |
| CPU and bus API shell | Complete | Two isolated instances fetch through caller buses and trap unimplemented words |
| 40-bit arithmetic foundation | Complete | C-safe representation and shift/sign helpers pass boundary and exhaustive tests |
| Standalone build and CI | Complete | CMake/CTest, warnings, sanitizers, installation, and consumer smoke tests pass in standalone CI |
| Ownership-mirrored build manifests | Complete | Root Make/CMake files retain policy and orchestration only; source and unit-test registrations live with their owning module, with exactness, parity, reachability, and nested-owner checks |
| Diagnostic isolation | Complete | Optional structured observers compile out and remain outside opcode semantics |
| Source-boundary audit | Complete | Forbidden hot-path dependencies and unregistered knobs are structurally rejected |
| Knob ownership registry | Complete | Every non-architectural switch has class, disposition, owner, tests, and exit criterion |
| Common-core target and evidence registry | Complete | Device-neutral scope, official sources, requirements, owners, and test links are machine checked |
| Architectural register-state foundation | Complete | Private fixed-layout state seeds the documented common reset subset while every unspecified value remains explicitly unknown |
| Address generation | Active | All address effects can be tested without opcode execution |
| Transactional address-effect application | Complete | A private sequencer seam validates a complete result and applies at most one phase-matched AR update with no partial mutation |
| Standard-indirect mode-state policy | Complete | CMPT=0 effects carry and enforce the ARP=0 invariant through owned state access without implicit repair or generic-mode coupling |
| 32-bit single-indirect operand shape | Complete | Long32 MOD0–MOD3 effects carry explicit MSW/LSW addresses, evidenced two-word AR deltas, and pass transactional application without bus or opcode coupling |
| Direct-address source snapshots | Complete | WORD16 CPL=0 DP and non-overflow CPL=1 SP effects carry truthful source snapshots, reject stale state transactionally, and prove DP/SP nonmutation without AR coupling |
| AR0-indexed single-indirect effects | Complete | WORD16 CMPT=0 MOD5/MOD6 effects snapshot distinct ARx and AR0 sources, apply one safe post-update transactionally, and leave wrap and alias behavior unresolved |
| Signed long-offset premodify | Complete | WORD16/LONG32 CMPT=0 MOD13 effects copy the raw signed offset word, form and apply one checked PRE_ACCESS selected-AR update, and leave MOD12 signedness and wrap unresolved |
| Reverse-carry arithmetic and effects | Complete | WORD16 CMPT=0 MOD4/MOD7 effects prove nonescaping MSB-to-LSB AR0 updates while final propagation, selected-AR0 alias ordering, and LONG32 remain unresolved |
| BK architectural state prerequisite | Complete | Private 16-bit BK storage and checked knownness-aware access prove the circular-buffer size dependency without claiming reset, MMR routing, or circular arithmetic |
| Sequencer address-effect decomposition | Complete | Pure effect validation and ordered dependency reads move behind a named sequencer seam before circular effects extend the transaction model |
| Sequencer validation subownership | Complete | Intrinsic effect checks and ordered architectural dependency reads have separate private owners while the validator only orchestrates |
| Circular MOD8/MOD10 WORD16 effects | Complete | Evidence-pinned fixed-step circular arithmetic, BK snapshots, and either one post-access AR update or the BK=0 no-op pass planner and transactional tests |
| Shared-memory transaction semantics | Endpoint integration complete (P2-W31B1) | Word-native requests, explicit byte-lane conversion, resumable reduction, copied DATA endpoints, cancellation, commit-at-READY visibility, and CPU suspension are proven; physical mapping and arbitration remain adapter-owned |
| Explicit WORD16 byte-order codec (P2-W21B) | Complete | Pure BIG/LITTLE two-lane conversion is exhaustive and host-order-independent without becoming a global memory policy |
| Opcode builder and decode | Complete | Exact-row NOP catalog, direct 16-bit first-word decoder, semantic dispatch seam, and checked sequential-PC contract are tested; unknown words remain explicit, while masked encodings and CPU-shell integration remain outside this slice |
| Masked opcode catalog foundation | Complete | The sole catalog accepts exact and masked 16-bit rows, rejects malformed or overlapping patterns deterministically, and owns a checked-in immutable direct lookup artifact; no production masked instruction is registered |
| Private program-PC bridge and CPU NOP transaction | Complete | Preserve the public carrier through one-word fetch, decode and dispatch NOP, stage through a private 16-bit PC seam, and retire transactionally with explicit stop/counter behavior |
| Value-owned operand-field extraction (P2-W19A) | Complete | A pure borrowed-view extractor assembles checked one-to-64-bit raw fields transactionally; no production opcode or fetch behavior changes |
| Transactional extension fetch and width-aware program PC (P2-W19B) | Complete | Allocation-free fixed-capacity extension fetch and explicit 1..255-word PC span/commit are proven before any multiword opcode or RPT integration |
| RPT evidence and instruction-transaction architecture (P2-W20A) | Complete | Immediate-RPT encodings, repeat/IRQ rules, owner hierarchy, staged implementation slices, and unresolved semantics are frozen before code changes |
| Canonical complete-instruction bundle (P2-W20B) | Complete | One decode-owned copied bundle is the checked handoff from CPU extension fetch through complete decode and semantic dispatch while NOP behavior remains unchanged |
| Catalog operand/repeat metadata (P2-W20C) | Complete | Scalar catalog tokens and decode-owned immutable layout sets are proven while production RPT activation remains gated |
| Typed immediate-RPT effect (P2-W20D) | Complete | A pure repeat-family operation converts an already extracted K/lk field into a transactional value-owned effect without production activation |
| Pure single-repeat policy foundation (P2-W20E) | Complete | Three repeat-sequencer leaf owners prove cardinality derivation, preclassified-body admission, and repeat-only request gating without claiming CPU execution |
| RPT CPU integration evidence closure (P2-W20F) | Deferred pending stronger evidence | Resolve RC decrement/final value, single-repeat PC flow/final PC, and already-pending request arbitration without inference from generic pipeline or block-repeat rules |
| P2-W22 immediate status-field LD vertical slice | Implemented — full local and remote gates green | One atomic end-to-end slice for LD #k9,DP, LD #k5,ASM, and LD #k3,ARP; add only the status-field effect and CPU retirement seams these forms require |
| P2-W23 MAX/MIN accumulator vertical slice | Implemented — full local and remote gates green; clean performance record captured at `a14d331` | Four one-word MAX/MIN encodings with signed full-40-bit accumulator selection, explicit tie/C behavior, transactional accumulator effects, and CPU retirement tests; repeatability and machine timing remain unresolved |
| P2-W24 generic accumulator-write effect and CMPL vertical slice | Implemented — full local and remote gates green; clean performance record captured at `7cad341`; repeatability and machine timing remain unresolved | Generic `ACCUMULATOR_WRITE` effect and CMPL four-form vertical slice are proven before further accumulator-family fanout |
| P2-W25 ROL/ROR rotate-through-C vertical slice | Implemented — full local and remote gates green; repeatability remains unresolved | Generic resource-driven input collection and four one-word ROL/ROR forms are proven through the existing `ACCUMULATOR_WRITE` + `C_WRITE` effect path; machine timing remains outside the contract |
| P2-W26 SFTL shifted-accumulator vertical slice | Implemented — full local and remote gates green | SFTL's five-bit signed shift layout, 128 encodings, low-32 shift semantics with explicit C output, and atomic CPU retirement are proven through existing generic effects; repeatability and machine timing remain unresolved |
| P2-W27 AND/OR/XOR register-only logical accumulator vertical slice | Implemented — full local and remote gates green; clean performance record captured at `ef87c94`; no confirmed regression | Three one-word logical accumulator families fit the existing input/effect/retirement seams; 13 catalog rows and 1077 populated first-word encodings after the slice, with repeatability unresolved and no new architecture |
| P2-W28A TC/status foundation | Implemented — full local `make test` and [remote CI](https://github.com/djr-747/ti-c54x/actions/runs/34058489923) green | Private typed TC state seam, STATUS_TC input resource, copied op-input knownness, and value-owned C+TC status delta are proven; all prior accumulator handlers migrated transactionally with no CPU branch |
| P2-W28B ROLTC/SFTC vertical slice | Implemented — targeted checks, independent review, and full local `make test` green; clean performance record captured at `7164ace` (remote CI green) | Register two one-word TC-aware accumulator families through the existing ACCUMULATOR_SOURCE layout and generic retirement path; catalog now has 15 rows and 1081 populated first-word encodings, with reset-runnable coverage still 4/15. No `cpu.c` change, public API, memory, RPT/IRQ, timing, knob, diagnostic, or environment change; repeatability remains unresolved. A minimal catalog-generator schema gap for TC resource tokens was closed centrally. |
| P2-W29A/B T-register foundation and EXP accumulator vertical slice | Implemented — all RED contracts, focused tests, two independent reviews, full local `make test`, and remote CI green; clean performance record captured at `fedbd6a` | Private 16-bit T ownership, candidate-only tagged effect transactions, and EXP_ACC/exp_acc are proven through the existing accumulator-source path. Architecture-needed changes remain serialized GAP reports, never family workarounds; repeatability and timing remain unresolved. |
| P2-W30 two-word immediate AND/OR/XOR accumulator forms | Implemented — focused strict/sanitizer tests, two independent reviews, registry closure, full local gate, and remote CI `34067048536` green; clean catalog-twenty-two performance record captured at `9c22a33` | Six width-two rows add 204 populated first words, taking the catalog to 22 rows/1287 populated words (4/22 reset-runnable). The slice adds only two decode-owned layouts and reuses extension fetch, accumulator input/effect, tagged transaction, and CPU retirement. Common computation and family leaves have mirrored ownership; the new rows are not benchmark-exercised because the public API has no initialized-accumulator bootstrap; repeatability and timing remain unresolved. |
| P2-W31A fixed-MOD2 memory prerequisites | Implemented — focused strict and ASan/UBSan tests, two independent reviews, full local gate, and remote CI `34069533137` green; clean same-catalog-22 performance record captured at `66ef384` | Canonical CPU architectural-state composition, private checked SXM read/knownness, and generic STATUS_SXM input collection are implemented. No opcode or catalog changes, memory effects, adapter/public-ABI guarantees, visibility, byte order, faults, wrap, repeatability, or timing are claimed; those remain later P2-W31B+ work. |
| P2-W31P performance architecture freeze | Implemented — structural/correctness gates green; hosted acceptance pending/deferred | The common CPU path now uses one catalog-derived opcode binding, one trusted value-owned execution packet with a static operand plan, and one trusted PC plan. Checked boundaries, generic resources, effect atomicity, observers, and memory WAIT/RESUME remain intact. Optimized hosted cohorts are below the 60 ns median target; exact three-capture stability remains open and does not block W31B2. |
| P2-W31B transactional data-memory endpoint and fixed-MOD2 load/store pair | Implemented — focused/full/sanitizer gates and two independent reviews green; immediate-READY benchmark follow-up pending | The copied public DATA WORD16 endpoint, adapter/reducer lifecycle, cancellation, persistent root transaction, CPU resume-before-fetch, and compile-time observer isolation are proven. W31B2 registers and decodes `LD *ARx+,dst` plus `STL src,*ARx+`, implements their pure generic MEMORY effects, and proves CPU end-to-end retirement. Catalog coverage is 24 rows/1319 populated first words (4/24 reset-runnable); the immediate-READY benchmark follow-up remains pending. The slice claims no timing, repeatability, AR-boundary wrap, device/MMR/program/IO, dual/parallel, or posted-write semantics. |
| P2-W32 SSBX/RSBX status-bit vertical slice | Implemented — focused tests, independent review, quality gate, focused ASan/UBSan, and full standalone gate green | The TI SPRU172C SSBX/RSBX catalog/decode rows (`0xF5B0`/`0xF4B0`, mask `0xFDF0`), generalized private status-bit write/application seam, pure set/clear handlers, and generic CPU retirement are implemented. All encoded SBIT values, including ST1 reserved bit 10, change and mark known only the selected raw status bit. The slice is one word, has no input resources, is repeatability INELIGIBLE, and adds no memory, timing, RPT/IRQ, knob, environment, diagnostics, or public ABI. Catalog coverage is 26 rows/1383 populated first words with 6/26 reset-runnable. |
| Hosted performance history | Implemented — append-only cohorts retained; exact W31P stability acceptance pending/deferred | Observer-off end-to-end CPU throughput with fixed scenarios, catalog-aware mixed workload, clean-commit append-only history, and advisory same-cohort comparison outside the core; optimized cohorts remain below the 60 ns median target. |
| Instruction timing and resource model | Planned after opcode inventory | Generated opcode metadata and the sequencer account for evidenced CPU cost while device profiles own memory/peripheral waits |
| Emulator integration | Deferred | Provenance resolved and standalone API declared stable |

## ISA implementation fanout policy

ISA work may run in small batches of two or three independent family lanes only
after each lane passes an architecture-fit review. A fitting lane has pinned
evidence, an existing typed input/effect/sequencer path, exclusive family-local
source and test ownership, and no need for opcode-identity logic in the CPU.
The main orchestrator owns architecture decisions, integration, commits, and
shared surfaces; catalog, generated decode, state, sequencer, CPU, and public API
changes are serialized through one integration owner rather than edited by
multiple family lanes.

An implementation lane must stop when an instruction exposes an architecture
gap. Its report identifies the instruction form and evidence locator, the exact
missing contract, the owning subtree, the smallest proposed general seam, the
tests needed to prove it, and any timing or hot-path consequence. The lane must
not hide the gap behind a family-specific CPU branch or silently broaden state,
memory, interrupt, repeat, or timing policy. IRQ/RPT interaction, shared-memory
ordering, and multi-work-unit timing remain serialized cross-cutting work even
when individual opcode semantics appear independent.

For P2-W27, Phase 1 is TDD RED fanout: the catalog/decode lane owns metadata,
layout, and quality fixtures; the logical-operations lane owns only
`src/isa/ops/logical/` and its pure-handler tests; and the CPU-proof lane owns
`tests/unit/cpu/test_cpu_logical_accumulator.c` plus its manifests. None of
these lanes may add production rows, generated artifacts, shared resources, or
CPU behavior before its corresponding test is RED. Phase 2 is CODE_READY:
family-local production may proceed only after the existing layout, resource,
effect, and CPU seams are shown to fit; catalog/decode/generated/shared-surface
changes are serialized by the orchestrator. Any gap in memory, status, state,
effects, sequencing, RPT/IRQ, timing, parallel execution, public API, or CPU
branching is a hard stop and must be reported with its evidence locator,
missing contract, owner, test, and hot-path consequence.

For P2-W28, fanout is split at the architecture seam. Phase A is a serialized
architecture RED/GREEN work piece: the state owner proves private typed TC
read/write and knownness; the CPU-input owner proves a `STATUS_TC` resource and
copied `status_tc/status_tc_known`; and the accumulator-sequencer owner proves
the minimal value-owned C+TC status delta with `PRESERVE`, `WRITE_0`, and
`WRITE_1` actions while migrating every existing accumulator handler. The
transaction remains atomic: validate the complete candidate accumulator and
status result first, then apply both or neither. Phase A explicitly excludes
OVA/OVB, OVM/SXM, public API changes, new effect kinds, memory, RPT/IRQ,
parallel issue, timing, and device behavior. The architecture test ownership is
fixed to `tests/unit/state/status/test_tc_field.c`,
`tests/unit/cpu/input/test_collect_tc.c`, and
`tests/unit/sequencer/accumulator/test_accumulator_status_delta.c`.

Only after Phase A is GREEN does Phase B begin with separate RED lanes for
ROLTC under `src/isa/ops/rotate/` and SFTC under `src/isa/ops/shift/`, plus
family-local CPU proofs. The catalog/decode lane may prepare only its metadata
and layout tests; production rows, generated lookup, dispatch, ancestor/shared
manifests, and shared integration remain serialized by the main orchestrator.
Each family lane may update only the leaf manifest inside its exclusively owned
subtree. Both forms use
the existing accumulator-source layout and generic retirement path. ROLTC
requires selected accumulator and TC inputs, writes the rotated accumulator and
C while preserving TC, and clears guard bits. SFTC does not read pre-TC; it
conditionally leaves the accumulator unchanged while writing TC, otherwise
shifts the low 32 bits left while preserving guards, and preserves C. A lane
must stop and report if either form needs a new layout, opcode-specific CPU
branch, memory, interrupt/repeat, parallel, timing, public-API, or other shared
seam. The expected post-slice catalog is 15 rows and 1081 populated first-word
encodings (encodings, not distinct-instruction count); repeatability and machine
timing remain unresolved until independently evidenced.

### P2-W29: T-register foundation and EXP accumulator vertical slice (planned)

Implementation status: complete at `fedbd6a`. All seven RED contracts, focused
tests, two independent reviews, the full `make test` gate, and remote CI run
`34062796872` are green. The clean catalog-sixteen performance record is
captured; an identical-binary diagnostic demonstrates substantial host noise,
so it does not establish a confirmed regression.

P2-W29 is a demand-driven two-stage slice. It is the next architecture seam only
because EXP needs a private T destination and the existing effect path has no
candidate-only root transaction. No unrelated register, status, memory, repeat,
interrupt, timing, diagnostic, environment, or emulator foundation is admitted.

P2-W29A owns the private T foundation. `src/state/temporary/` owns only the raw
`uint16_t` storage, explicit knownness, reset physical-zero/architectural-unknown
boundary, and checked private read/write contract. `src/isa/ops/op_effect.h`
centrally owns the value-only `TI_C54X_OP_EFFECT_T_REGISTER_WRITE` carrier;
`src/sequencer/temporary/` owns only its transactional application. The shared
architecture lane, serialized and centrally reviewed by the orchestrator, owns
`src/sequencer/effect_transaction.[ch]`: a tagged union stages only the affected
candidate state, validates all staged effects, and commits atomically.
`src/cpu/` stores and resets private T then delegates staging/commit; it does not
branch on opcode identity, copy an unconditional whole state, expose T publicly,
read T as opcode input, route the T MMR, or touch data memory.

P2-W29B begins only after P2-W29A is GREEN. EXP_ACC/`exp_acc` is a one-word,
S-bit-selected accumulator form with value `0xF48E`, mask `0xFEFF`, the existing
`ACCUMULATOR_SOURCE` layout, and `ACCUMULATORS` input resources. The accumulator
family lane owns only `src/isa/ops/accumulator/` and its pure test leaf. The
catalog/decode lanes own tests only; the CPU proof lane owns its test only. The
orchestrator owns production catalog rows, generated lookup, dispatch, parent
manifests, CPU files, and shared integration. EXP leaves the selected source
unchanged and writes signed `-8..31` as a `uint16_t` T value: zero `0000`, both
signed 40-bit extrema `FFF8`, `+1` `001E`, `-1` `001F`, `+2^31`
`FFFF`, `+2^32` `FFFE`, `-2^31` `0000`, `-53` `0019`, and
`0x0785432105 -> FFFC`. The summary one-cycle field is evidence only; no timing
API or claim is introduced. EXP repeatability remains `UNRESOLVED` because
absence from Table 2-28's nonrepeatable list is not positive eligibility.

Ownership and sequencing are explicit: state lane owns only the temporary state
leaf and `tests/unit/state/temporary/`; T-effect lane owns only the temporary
sequencer leaf and its test; effect-transaction RED/GREEN is a serialized shared
architecture work piece; EXP owns only its accumulator leaf and pure tests;
catalog/decode and CPU lanes own tests only; the orchestrator owns shared files.
Any lane that needs a new layout, T input, MMR/data-memory route, OVA/OVB,
OVM/SXM, memory, RPT/IRQ, parallel issue, timing, diagnostics/environment,
allocation, public API, or emulator seam must stop and report the exact evidence,
missing contract, owner, tests, and hot-path consequence as a GAP.

#### P2-W29B: EXP accumulator vertical slice (planned)

This subheading is the immutable FIT/ownership anchor for the EXP family. It is not a
permission to broaden the T foundation: EXP consumes the generic T write effect
and root transaction only after their tests pass. The completed slice has 16
catalog rows and 1083 populated first-word encodings, with reset-runnable
coverage remaining 4/16; no timing or repeatability result is implied.

### P2-W30: two-word immediate logical accumulator vertical slice (planned)

The immutable `(planned)` heading preserves the evidence-registry anchor. P2-W30
is now implemented: the two-word immediate AND, OR, and XOR accumulator forms
have six catalog rows, eight implemented requirements, and eight passing tests.
The forms add 204 populated first words (AND 68, OR 68, XOR 68), taking the
catalog from 16 rows / 1083 populated words to 22 rows / 1287 populated words.
Because every form needs an initialized accumulator input, reset-runnable
coverage remains 4/22.

The central architecture seam is limited to two decode-owned two-word layouts:
`S` bit 9, `D` bit 8, unsigned `SHFT4` bits 3:0, and extension word 1 `lk16`
for the shifted forms; and the same `S`/`D` fields with extension `lk16` and an
implicit shift of 16 for the fixed forms. Existing extension fetch, complete
instruction bundles, the generic program-PC width bridge, `ACCUMULATORS` input,
`ACCUMULATOR_WRITE`, tagged effect transaction, and CPU retirement are reused.
The CPU proof must fetch the extension exactly once at `PC+1`, retire success at
`PC+2`, and roll back atomically on extension failure. At `PC=0xfffd`, retirement
to `0xffff` is valid; at `PC=0xfffe`, the extension may be fetched at `0xffff`
but candidate `PC+2` is rejected with no effect, PC, or retirement mutation.

Ownership mirrors the tree. `src/isa/ops/logical/immediate/common/` owns shared
decoded-operand validation, immediate construction, and parameterized logical-kind
computation; `src/isa/ops/logical/immediate/and/`, `/or/`, and `/xor/` own only their family
leaves. Pure tests mirror those leaves under
`tests/unit/isa/ops/logical/immediate/{and,or,xor}/`; CPU proofs mirror them
under `tests/unit/cpu/isa/logical/immediate/{and,or,xor}/`. The orchestrator
owns catalog metadata, generated lookup, dispatch, parent manifests, CPU shared
surfaces, and integration. A lane must stop and report a GAP if it needs new
memory, status, timing, IRQ/RPT, public API, knob, diagnostic, environment,
allocation, or emulator behavior.

The focused catalog, decode, family, and CPU tests prove the complete path and
its malformed-input rejection. The six new rows are not included in hosted
benchmark streams because initialized accumulator state is not exposed through
the public API; fixed scenarios and the four-row mixed semantic stream remain
unchanged. Semantics use only the selected source
accumulator. The prior destination is not an input even when `D != S`; an
unknown distinct destination is therefore valid because it is overwritten.
`lk16` is unsigned and zero-extended before the selected shift. Status is
preserved. Absence from Table 2-28's
nonrepeatable list is not positive eligibility evidence, so logical-immediate
repeatability remains unresolved; manual two-cycle summaries are evidence only,
not a timing API or claim.

Focused strict and ASan/UBSan tests, two independent reviews, the evidence and
ownership checks, and the full standalone `make test` gate are green. Review
found and closed one catalog-binding weakness: each generated row alias now
rejects its sibling form transactionally instead of accepting either family ID.
Remote CMake, Clang, install, consumer, and sanitizer matrix validation remains
assigned to post-push CI.

### P2-W31A: fixed-MOD2 memory prerequisites (planned)

This immutable `(planned)` heading remains the FIT/ownership anchor for the
memory-oriented slice. P2-W31A is implemented: focused strict and ASan/UBSan
tests, two independent reviews, the full standalone local gate, and remote CI
(`34069533137`) are green. The clean same-catalog-22 performance record is
captured at `66ef384`. This work piece closes only three prerequisites; it makes
no opcode, catalog, or memory ABI claim.

The first prerequisite is canonical CPU state composition. `src/cpu/` must own
one embedded `ti_c54x_arch_state`, which is the sole owner of PC, status, AR0–AR7,
SP, DP, CPL, and CMPT. Duplicate CPU PC/status storage is removed. The private
`arch_state.pc_word` is widened to `ti_c54x_program_word_address` so the existing
32-bit public program-word carrier remains representable; the existing program-PC
bridge continues to enforce the common target's 16-bit validity and does not infer
XPC or extended pages. Initialization/reset seeds the documented common state,
then applies the configured reset carrier. Public inspection, execution, and
observer behavior must remain byte- and semantically equivalent. Accumulators, T,
execution counters, bus descriptors, and observers remain separate owners.

The second prerequisite is a private checked SXM read under
`src/state/status/`. It derives SXM from the existing ST1 representation and
returns explicit value/knownness or a checked failure. This slice intentionally
adds no write accessor: status writes remain separate until a later evidenced
instruction requires them.

The third prerequisite is generic input plumbing. `src/cpu/input/` adds a
`STATUS_SXM` input resource and copies `status_sxm` plus
`status_sxm_known` into the operation-input value. Collection is generic and
opcode-agnostic; no handler consumes SXM yet, and no opcode-specific CPU branch
is part of this work piece.

P2-W31A explicitly leaves opcode and catalog changes, the WAIT adapter and
public ABI, memory effects, shared-memory visibility, byte order, fault
visibility, AR `0xffff` wrap, repeatability, timing, and later instruction
forms to later P2-W31B+ evidence freezes. A lane that needs any of those seams
must report a GAP with its evidence, owner, tests, and hot-path consequence
rather than broaden this prerequisite.

### P2-W31P: trusted execution hot path and release-performance contract (planned)

State: implementation and structural proofs are green; hosted-performance
acceptance remains pending/deferred. The optimized cohorts are below the 60 ns
median target, but the exact three-capture stability rule remains open. This
follow-up no longer blocks W31B2.

The current implementation has clean ownership and transactional semantics. The
hosted audit showed that the common retired-instruction path performed too much
defensive work, so this shared CPU/decode optimization remains the performance
architecture freeze while W31B2 proceeds as the next ISA slice.
It must improve the common path without weakening the checked contracts or making
the library dependent on a particular compiler, linker, host, or emulator.

The frozen shape is:

- `src/isa/catalog/` continues to own the compact generated first-word-to-ID
  lookup and immutable scalar metadata. `src/isa/ops/dispatch/` owns the
  catalog-derived per-ID binding that combines those immutable facts with the
  semantic handler. A CPU iteration performs one first-word ID lookup and one
  per-ID binding selection. It does not repeatedly look up metadata, revalidate
  a layout, or re-extract fields after the packet has been prepared.
- `src/isa/decode/` owns the checked decode boundary and creates one private,
  value-owned trusted execution packet. Packet preparation consumes the selected
  binding and one prevalidated generated/static operand plan, then copies the
  decoded instruction facts, handler, and extracted operand values into the
  packet. The packet retains no data pointer or caller-owned storage; a stored
  function pointer refers only to process-lifetime code. It is prepared once per
  instruction and is not a public mutable object. Checked decode and dispatch
  entry points remain available for forged-input tests; the fast path consumes
  only a packet produced by the private constructor.
- A packet lives only through ordinary semantic dispatch. If it produces a
  MEMORY effect, the existing root transaction copies the decoded instruction,
  input, effect, address, and PC candidates before the packet expires. The root
  retains no packet pointer. Its endpoint context remains the separately
  borrowed caller-owned object copied into the CPU bus descriptor for the CPU
  lifetime; that endpoint lifetime is not packet ownership and is unchanged by
  this work.
- `src/cpu/input/` continues to own generic resource collection. The CPU does not
  add an opcode switch, family-specific resource lookup, or handler-specific field
  extraction. New resources extend the generic collector and its packet contract.
- `src/cpu/execution/` owns the trusted packet loop and one precomputed PC plan.
  PC validation and span arithmetic happen while preparing the plan; retirement
  performs one trusted commit after the semantic effect is proven. The loop keeps
  the existing attempt/retire/stop behavior and never turns opcode identity into
  CPU orchestration logic.
- `src/sequencer/instruction/`, the existing effect transaction, and its owned
  status/accumulator/temporary leaves retain effect atomicity, candidate
  validation, commit ordering, observer boundaries, and resource/effect
  ownership. The memory WAIT/RESUME path is unchanged: a pending transaction is
  resumed before fetch and is never refetched, redecoded, or reissued by the
  optimization.
- `bench/` owns fixed-scenario construction and observer-off captures; `tests/`
  mirrors the owning source subtree for packet, binding, PC-plan, atomicity, and
  regression proofs. No new monolithic `cpu.c`, decoder, or opcode-family file is
  permitted; a source owner approaching the project size review threshold must
  split before adding another concern.

The portable default deliberately does not use a 65,536-entry pointer table or
GNU computed goto. A full pointer table imposes a large, permanently resident
data structure and avoidable relocation/cache pressure for a vendored library;
computed goto would make the default dispatch contract compiler-specific. The
catalog-derived binding and direct generated lookup retain predictable portable C
while allowing a consumer to opt into ordinary compiler IPO/LTO. The standard
consumer build remains free to choose IPO; the core must not require it. The
benchmark release profile may use portable `-O3` plus LTO and must record the
exact flags in every capture.

TDD and validation proceed in explicit phases:

1. RED: capture the current O2/no-LTO and O3/LTO audit, add packet and binding
   contract tests, and add deterministic structural checks for one lookup/one
   binding, one packet-preparation extraction pass, one PC plan, and no
   opcode-specific CPU switch.
2. GREEN: add the generated/static binding and value-owned trusted packet behind
   the existing checked decode/dispatch seams. Prove forged IDs, layouts,
   operands, PC spans, and packets are rejected before the trusted path; prove
   valid packets execute with the same effect, observer, stop, and retirement
   behavior as the checked path.
3. GREEN: route the CPU fast path through the packet and one PC plan while keeping
   memory WAIT/READY/FAULT and cancellation behavior byte-for-byte equivalent at
   the contract level. Add strict, sanitizer, observer-off, and consumer-build
   evidence and retain those checks while W31B2 proceeds.
4. PERF: repeat each fixed scenario in three stable same-host captures for both
   the portable O2/no-LTO diagnostic profile and the advisory portable
   O3/LTO release profile, record compiler/linker/platform metadata, and compare
   against the frozen audit. A set is exactly three consecutive captures with
   identical binary and scenario hashes. Every target scenario in every capture
   must remain below 60 ns and have `(max - min) / median <= 0.15`. A failing
   set is retained as `INCONCLUSIVE`, never silently discarded or selectively
   averaged; a replacement uses a new set ID and records the changed external
   condition.
5. W31B2 execution slice: add immediate-READY `LD *ARx+,dst` and `STL src,*ARx+`
   plus dedicated observer-off benchmark scenarios. Their fixed-scenario medians
   must meet the same target as a follow-up performance requirement before later
   memory fanout. WAIT-capable memory remains a separate contract and is not
   hidden in the benchmark's immediate-READY path.

Acceptance is an implementation-throughput target, not a TI timing claim: on one
same host, both observer-off profiles must record a median below 60 ns per retired
instruction for every fixed scenario in three stable captures. The O2/no-LTO
requirement prevents IPO from hiding a structurally expensive execution path;
O3/LTO proves the intended release profile. This is advisory evidence and is not
a flaky CI threshold. Structural tests must also prove the binding/packet/PC-plan
invariants deterministically, and no result may trade away effect atomicity,
observer isolation, or memory WAIT semantics. There is no runtime knob,
environment read, diagnostic path, A/B switch, or test instrumentation in the
production core for this work; source checks and measurement live in their test
and benchmark owners.

The triggering observations and the exact commands, hashes, host/toolchain
metadata, raw medians, ranges, and acceptance rule are committed in
`bench/audits/p2-w31p-baseline.json`. Those observations freeze the problem;
they do not constitute the later three-capture acceptance set.

### P2-W31B: transactional data-memory endpoint and fixed-MOD2 load/store pair (planned)

State: W31B1 and W31B2 are implemented with focused strict and ASan/UBSan
tests, independent review, and the full standalone local gate green. P2-W31P's
exact hosted stability follow-up remains open but is no longer a prerequisite.
W31B2 added the two catalog/decode rows, pure generic MEMORY effects, and CPU
end-to-end retirement. Only the immediate-READY benchmark follow-up remains
pending. No catalog row or opcode handler was added during W31B1.

The public boundary is deliberately small and vendorable:

- `include/ti_c54x/bus_endpoint.h` owns the generic WORD16 request, observation,
  status, phase, endpoint, and cancellation vocabulary. A request names
  PROGRAM, DATA, or IO space, READ or WRITE, a `uint32_t address_word`, and a
  write value; DATA and IO addresses are checked against `UINT16_MAX`, and a
  READ has `write_value == 0`. The W31B slice copies one `data_endpoint` by
  value into `ti_c54x_bus`; it enforces DATA requests. `bus.h` owns topology
  only. The unused synchronous public data callbacks are retired; synchronous
  program and IO callbacks remain until their own wait-capable work is
  evidenced. There is no attach/detach API.
- An endpoint descriptor contains one borrowed context pointer, one `advance`
  callback, and one `cancel` callback; the caller continues to own the context
  and everything reachable through it for the CPU lifetime. Both callbacks are
  required or the complete descriptor is the canonical all-zero absent value.
  `ti_c54x_bus_word16_endpoint_advance_fn` returns `void` and receives the
  borrowed context, BEGIN or RESUME, a stable nonzero `uint64_t` transaction
  ID, the same immutable request on every phase, and a caller-owned observation
  output that the core zero-initializes before each callback. Observations are exactly
  INVALID, WAIT, READY, or FAULT: WAIT/READY carry `TI_C54X_BUS_OK`, FAULT
  carries a non-OK status, and a read value is meaningful only for READ READY;
  WAIT, FAULT, and WRITE READY carry a zero read value. INVALID is the all-zero
  sentinel and is never a legal callback result. A missing callback for a
  memory instruction reports `BUS_FAULT` plus `TI_C54X_BUS_NO_CALLBACK` before
  an ID is allocated. Malformed callback output leaves the root transaction
  pending and reports internal invalid state; it is not a TI behavior claim.
- `ti_c54x_bus_word16_endpoint_cancel_fn` receives the same borrowed context,
  active ID, and immutable request, and returns
  `TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED` or
  `TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED`; the all-zero
  `TI_C54X_BUS_WORD16_CANCEL_RESULT_INVALID` is malformed. CANCELLED authorizes the
  private reducer to clear the pending transaction. FAILED or INVALID maps to
  `TI_C54X_ERROR_BUS_CANCEL_FAILED`, preserves CPU and transaction state, and
  permits a later reset/deinit call to retry cancellation. Cancel is called
  only for active pending work, never for IDLE, READY, or FAULT.
- External visibility is commit-at-READY. WAIT, FAULT, and cancellation never
  publish a store; the first READY publishes it once before returning. The
  adapter owns physical storage, mapping, arbitration, and byte-lane conversion.
  Posted-device writes, device/MMR behavior, and arbitration policy are outside
  this slice.
- CPU initialization copies the endpoint once. `ti_c54x_cpu_init` requires fresh
  or successfully deinitialized storage; live-storage overwrite/reinitialization
  is retired and no replacement reinit API is added. `ti_c54x_cpu_deinit`
  returns `ti_c54x_error` (ignoring the return remains valid C); `deinit(NULL)`
  is a successful no-op. Reset and deinit cancel a pending WAIT. Cancellation
  failure is explicit and preserves CPU plus pending transaction state. IDs begin
  at one, monotonically increase, never reset or reuse within one CPU lifetime,
  and exhaustion is detected before issue. Reset preserves that sequence.

The execution seam is a persistent root instruction transaction under
`src/sequencer/instruction/`. It composes the existing bus reducer,
`effect_transaction`, address planner, and program-PC bridge. The CPU owns one
root transaction and resumes it before fetching a new word. A first call that
fetches/dispatches and reaches WAIT reports attempted=1, retired=0; a resume
that remains WAIT reports 0/0; a resume that reaches READY reports 0/1. It
never refetches, redecodes, or reissues. `run(0)` does neither begin nor poll.
`TI_C54X_STOP_BUS_WAIT` is a distinct public stop reason; BUS_WAIT emits no
terminal STOP observer, while FETCH is emitted once. A bus fault is
`BUS_FAULT` with no PC, AR, accumulator, or status commit. This is an
instruction work-unit suspension contract, not a machine-cycle API.

The generic memory carrier is defined in a small owned subtree under
`src/isa/ops/memory/` and included as one tagged `MEMORY` arm of
`ti_c54x_op_effect`; there is no second effect channel. It carries a generic
DATA READ/WRITE, the initial `SINGLE_INDIRECT_WORD16` MOD2+ARF address
specification, a store word, and a static process-lifetime completion callback.
It carries neither a resolved address nor AR/ARP/CMPT copies.
`src/sequencer/memory/` resolves that spec against the embedded `arch_state`,
using the old selected AR and staging the post-access AR+1 before issue. The
completion callback receives the copied decoded instruction, copied op-input,
and READY read word, and returns an ordinary next effect; the sequencer never
switches on LD/STL identity or a transform enum. LD owns the callback and uses
captured D plus SXM to emit an accumulator write. STL precomputes the
accumulator low 16 bits and has no completion callback. All PC, address/AR,
counter, and effect candidates are validated before BEGIN so a READY store can
commit atomically. After WRITE READY, no fallible validation or external
callback remains: publication uses only the already-prepared AR, PC, and
retirement assignments. Forged or invalid address, effect, PC, and counter
candidates are rejected before BEGIN. WAIT, FAULT, and cancellation leave
architectural state untouched.

The first evidence-pinned pair is deliberately narrow:

| Form | Encoding contract | Semantic contract |
|---|---|---|
| `LD *ARx+,dst` | value `0x1090`, mask `0xFEF8`; D bit 8, ARF bits 2:0, fixed I=1/MOD2; one program word and 16 first-word encodings | Read the old selected AR, zero-extend the 16-bit word when SXM=0 or sign-extend it into the 40-bit accumulator when SXM=1, then postincrement AR by one word. No encoded shift. |
| `STL src,*ARx+` | value `0x8090`, mask `0xFEF8`; S bit 8, ARF bits 2:0, fixed I=1/MOD2; one program word and 16 first-word encodings | Store the accumulator low 16 bits, then postincrement AR by one word. This unshifted form has no SXM-dependent transform. |

Both forms require CMPT=0 and the existing ARP=0 policy. The old selected AR
is used for the transfer and the post-access AR is committed only with READY.
`AR=0xffff` boundary behavior remains unresolved and is rejected by the current
checked planner. After W31B2 the expected catalog is 24 rows and 1319 populated
first words, with reset-runnable coverage still 4/24; these are lookup
population counts, not a distinct-instruction count or timing claim.

Ownership and TDD order mirror the tree. RED tests are created first, then each
owner goes GREEN without editing another owner’s files:

1. `tests/unit/bus/adapter/test_word16_endpoint_contract.c` proves the copied
   public endpoint shape and canonical callback protocol. The existing
   `tests/unit/bus/adapter/test_word16_exactly_once.c` is activated for the
   external visibility/issue proof; `tests/unit/bus/transaction/` owns reducer
   cancellation.
2. `tests/unit/sequencer/instruction/test_memory_instruction_lifecycle.c` and
   `test_memory_instruction_commit.c` prove persistent
   WAIT/RESUME/READY/FAULT/cancel and publication phases, while
   `tests/unit/sequencer/memory/test_memory_address_resolution.c` proves the
   generic MOD2 address dependency and staged AR result.
3. `tests/unit/cpu/memory/test_cpu_memory_lifecycle.c` and
   `test_cpu_memory_pending.c` prove endpoint copy, resume-before-fetch,
   counter phases, BUS_WAIT, reset/deinit cancellation, re-entry exclusion,
   and monotonic IDs.
4. Catalog and decode owners prove the two rows in
   `tests/unit/isa/catalog/test_fixed_mod2_memory_metadata.c` and
   `tests/unit/isa/decode/test_fixed_mod2_memory_decode.c`.
5. `tests/unit/isa/ops/load/test_ld_smem.c` and
   `tests/unit/isa/ops/store/test_stl_smem.c` prove family-local pure effects;
   `tests/unit/cpu/isa/load/test_cpu_ld_smem.c` and
   `tests/unit/cpu/isa/store/test_cpu_stl_smem.c` prove end-to-end retirement.

The common target does not claim machine-cycle cost, repeatability,
`AR=0xffff` wrap, device/MMR/program/IO spaces, byte-order policy beyond the
adapter contract, dual data reads, parallel issue, posted writes, or any
fault/recovery behavior not represented by the project endpoint protocol.
Those remain explicit GAP or UNRESOLVED work for later evidence freezes.

### P2-W32: SSBX/RSBX status-bit vertical slice (planned)

State: implementation, focused tests, independent no-P0/P1 review, quality
gate, focused ASan/UBSan validation, and the full standalone `make test` gate
are green. The completed non-memory ISA slice is limited to the registered TI
`SPRU172C` SSBX entry (printed p. 4-166) and RSBX entry (printed p. 4-151); no
emulator, device profile, timing table, or inferred status policy is an
authority. The rows are one-word forms with values `0xF5B0`
(SSBX) and `0xF4B0` (RSBX), shared mask `0xFDF0`, `N` at bit 9, and `SBIT` at
bits 3:0. They have no input resources and are marked repeatability
`INELIGIBLE` from the registered instruction boundary.

The slice adds five independently tested ownership pieces:

1. `src/isa/catalog/` registers the two rows and their 32 encodings each;
   `src/isa/decode/` owns the value-owned `N` and `SBIT` extraction.
2. `src/sequencer/status/` generalizes the private status-bit write/application
   route. A candidate changes and marks known only the selected raw ST0 or ST1
   bit, preserving every unrelated raw and known bit and committing atomically.
3. `src/isa/ops/status/` owns pure SSBX/RSBX handlers that produce set/clear
   effects only. `N=1, SBIT=10` is accepted as the encoded ST1 reserved bit;
   this slice does not invent a side effect for its reserved meaning.
4. `src/cpu/` retires both forms through generic decode, input, status-effect,
   and sequential-PC paths. No opcode-specific CPU branch, memory request,
   RPT/IRQ behavior, timing API, knob, environment read, diagnostic callback,
   or public ABI change is permitted.

TDD starts with RED catalog/decode, status-application, pure-handler, and CPU
retirement tests under mirrored owner paths. GREEN may proceed only after the
existing status-field transaction is shown to fit the generalized raw-bit
candidate; a new status route must remain private and transactional. Any need
for status-register policy, reserved-bit behavior, memory, interrupts,
repeat, parallel issue, timing, or public state is a GAP report, not a family
workaround. The expected post-slice catalog is 26 rows and 1383 populated
first words, with reset-runnable coverage likely 6/26; these are lookup
population counts, not a distinct-instruction or timing claim. W31P hosted
stability and W31B2 immediate-READY benchmark follow-ups remain open.

### P2-W16: NOP-seeded opcode architecture

This completed work piece establishes the smallest evidence-backed ISA architecture slice
without integrating it into the public CPU shell. The ownership boundary is
deliberate:

- `src/isa/catalog/` owns one declarative X-macro catalog, and that catalog is
  the only instruction inventory. No second handwritten opcode list or copied
  legacy table is permitted.
- Macro passes generate opcode identity and metadata, a direct 16-bit first-word
  lookup, and a function-pointer semantic dispatch table. The generated views
  remain derived from the one catalog.
- The first registered row is NOP `0xF495` with width one program word. Words
  absent from the catalog classify explicitly as unknown; they do not alias NOP
  or silently select a default handler.
- NOP emits an explicit no-opcode-owned-effect result. Opcode semantics do not
  mutate the program counter, status, memory, buses, devices, or observers.
- `src/sequencer/` alone plans checked private 16-bit sequential PC `+1`. A
  source PC of `0xffff` returns an explicit unresolved-boundary result and
  leaves the input and output state unchanged.
- The public CPU shell is not integrated in this slice: its current carrier is
  32-bit, while high-carrier and XPC policy remain unresolved. The private
  16-bit seam is therefore tested independently of `include/ti_c54x/cpu.h`.
- This slice makes no machine-cycle or pipeline timing claim and adds no memory,
  bus, device, or observer behavior to opcode operations. It does not copy code,
  tables, comments, or tests from legacy emulators or other external projects.

The TDD order is catalog contract and decoder classification first, explicit NOP
semantic no-effect second, and checked sequential-PC planning third. Each owner
has its own test path; cross-owner integration is deferred until the central
sequencer and public CPU-shell contracts are ready.

### P2-W17: masked opcode catalog foundation (active kickoff)

State: implemented. The completed work piece establishes the shared catalog
schema, deterministic lookup generator, checked-in lookup artifact, and their
exhaustive and synthetic quality proofs. Production still registers only the
exact NOP row; no masked instruction is enabled by this slice.

This work piece extends the P2-W16 catalog infrastructure without adding an
operand-bearing or masked production instruction. The exact NOP row, its `0xF495`
encoding, behavior, and public/private APIs remain unchanged. The sole
`src/isa/catalog/` inventory now has an `EXACT` plus `MASKED` schema with these
project-contract rules:

- Every encoding is canonical 16-bit `value`/`mask` data. `EXACT` is represented
  by a full mask; `MASKED` must have a nonzero mask other than `0xffff`, and the
  value may not set any bit outside its mask.
- Widths are one through 255 program words. IDs and handler tokens are unique.
  Pairwise pattern overlap is rejected deterministically; there is
  no priority or declaration-order tie-breaker.
- A deterministic repository-owned generator emits the checked-in immutable
  65,536-entry `uint16_t` direct lookup at
  `generated/isa/catalog/opcode_first_word.inc`. The
  ordinary Make, CMake, and installed-consumer builds consume only checked-in C
  artifacts and remain compiler-only/Python-free. Quality checks regenerate the
  artifact and reject stale output, but runtime has no initialization,
  allocation, or mutable global-table update.
- Operand-field layouts/extraction and extension-word fetch remain separate
  future owners/work pieces. This foundation does not infer variable operand
  fields, multiword fetch semantics, CPU fetch/decode/dispatch/retire
  integration, timing, memory, bus, device, observer, XPC, or external-table
  behavior.

Ownership follows the tree: `src/isa/catalog/` owns the declarative rows and
runtime lookup, `tools/isa/catalog/generate_opcode_lookup.py` owns deterministic
generation, `generated/isa/catalog/` owns the checked-in derivative, and
`tests/contract/quality/isa/catalog/` owns the Python schema, overlap, generator,
and stale-artifact checks. No
implementation file outside the catalog owner may add an opcode row or duplicate
the generated table. Existing TI field/encoding evidence remains architectural
context for later instruction work; the catalog and generator evidence is
`PROJECT_CONTRACT_ONLY` and does not claim TI conformance.

The TDD proof now covers malformed schema, invalid masks/widths, duplicate IDs
or handlers, deterministic pairwise overlap rejection, stale lookup output,
exhaustive 65,536-word mapping, and generated-artifact immutability. The two
Python tests run through `make test` via CTest
`ti_c54x.quality.python`; normal Make, CMake, and installed-consumer builds
consume only checked-in C artifacts and remain compiler-only/Python-free.

### P2-W18: private program-PC bridge and CPU NOP transaction (active kickoff)

State: implemented. This work piece is the first cross-owner execution slice:
it connects the existing public CPU shell to the existing exact NOP decoder and
dispatch seam without adding another instruction or changing the catalog.

The contract is intentionally explicit about what is and is not architectural:

- `src/cpu/` owns instruction-budget orchestration, public-carrier handling,
  caller-bus fetch, decode/dispatch sequencing, stop reasons, and cumulative
  attempted/retired counters. A zero budget performs no fetch or attempt.
- `src/sequencer/` owns the private common 16-bit program-PC bridge. It stages a
  sequential successor as value-owned transaction data; it never truncates a
  public 32-bit carrier or infers XPC/page state. `0xfffe -> 0xffff` is admitted;
  staging a one-word successor from `0xffff` returns an explicit unresolved
  boundary and cannot retire.
- The execution order starts an attempt immediately before the program-bus read,
  passes the exact public 32-bit carrier to that callback, fetches exactly one
  16-bit program word, classifies it, and stops on fetch failure or unknown
  decode while preserving committed state. Bus faults and unknown words count as
  one attempted instruction; zero budget and cumulative-counter preflight failure
  count zero. A known NOP at a carrier above `0xffff` reaches bridge validation
  and stops there without truncation or XPC inference. For an admitted word, the
  CPU dispatches NOP through the generated function-pointer seam, stages the
  private PC, commits PC transactionally, then increments `retired` once. The
  CPU does not expose a partial PC commit.
- `run(N)` is an instruction-retirement budget: it returns after `N` successful
  retirements or the first explicit stop and never overshoots. Cumulative counter
  overflow is an explicit no-mutation stop; silent unsigned wrap is forbidden.
- The public result uses distinct
  `TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS`,
  `TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY`, and
  `TI_C54X_STOP_COUNTER_OVERFLOW` reasons for the three new policy stops;
  malformed internal dispatch or stale-transaction states remain
  `TI_C54X_STOP_INVALID_STATE`.

Ownership mirrors the tree. The implementation seam belongs under
`src/sequencer/program_pc_bridge.[ch]`, CPU orchestration remains under
`src/cpu/`, and the passing tests are split between
`tests/unit/sequencer/test_program_pc_bridge.c` and the CPU-owned execution,
budget, and common-NOP files. The project-contract evidence for this boundary is
anchored by this heading; TI evidence is used only for the existing NOP word,
one-word width, no status effects, and sequential-flow context.

This completed slice excludes operand-field extraction, extension-word fetch, timing or
machine-cycle accounting, shared-memory mutation, data-memory buses, interrupts,
repeat state, new observer/diagnostic behavior, device profiles, emulator
adapters, and all other ISA families. Existing observer behavior is unchanged;
direct Make linkage uses an observer-enabled archive derived from the canonical
source inventory, matching the existing CMake variant and avoiding a second
handwritten source list.

### P2-W19A: value-owned operand-field extraction (active kickoff)

State: implemented. P2-W19 is deliberately split into two atomic contracts:
P2-W19A proves value-owned operand-field extraction first; completed P2-W19B
proves extension-word fetch and width-aware program-PC span/commit. Both precede
repeat instructions, including `RPT`, so operand shape and multiword sequencing are
not smuggled into an opcode handler. Bus reads belong to `src/cpu/` (the
instruction-fetch owner); `src/sequencer/` owns only the resulting width-aware
program-PC span and commit policy.

The 19A contract is a pure, allocation-free checked extractor under
`src/isa/decode/operand_field.[ch]`. Its `ti_c54x_program_words_view` borrows a
caller-owned `program_words` array whose `program_word_count` is the extent; at
least that many `uint16_t` elements must remain alive and unchanged for the call.
The immutable layout similarly borrows a piece array for its declared
`piece_count`, and the output is caller-owned. No pointer is retained after
return, and writes occur only after validation succeeds. A conceptual piece
contains a program-word index, source LSB, width from 1 through 16, and
destination/value LSB from 0 through 63. A field has one or more pieces and a
declared raw width from 1 through 64. Every piece remains inside one source word
and the declared destination width; destination pieces do not overlap and cover
the declared value exactly; source slices for a field do not overlap within a
source word.

The extractor assembles first-word, extension-word, split/non-contiguous, and
64-bit raw values without sign extension or semantic interpretation. Null or
invalid views/layouts/outputs, out-of-range word indexes, invalid widths, overlap,
and gaps leave the output byte-for-byte unchanged. It owns no bus, CPU, state, PC,
effect, timing, memory, interrupt, repeat, observer, diagnostic, file, environment,
heap, or mutable-global behavior. The production opcode inventory, catalog macro
arity, layout binding, handlers, public API, and fetch path remain unchanged.

Ownership mirrors the hierarchy: `src/isa/decode/operand_field.[ch]` owns the
implementation contract; `tests/unit/isa/decode/test_operand_field.c` owns its
executable proof; owner-local Make/CMake registration now owns the test target.
The passing test exhausts valid one-word slice positions and widths, then covers
multiword, extension-word, split/non-contiguous, 64-bit, borrowed-input, and
complete invalid/transactional cases. This is project-contract mechanics evidence
only; it makes no TI encoding or instruction-conformance claim and fetches no
words itself.

Performance boundary: this checked general extractor proves descriptor mechanics
and remains allocation-free, but it is not a requirement that every hot-path
instruction revalidate a descriptor. A future production catalog/layout binding
may validate immutable layouts offline and emit or directly use specialized
extraction code, so descriptor revalidation is not structurally required in the
hot path. No such generator or specialization exists in this work piece.

### P2-W19B: transactional extension fetch and width-aware program PC (active kickoff)

State: implemented and green. This work piece froze the two seams needed before
any multiword opcode, including `RPT`, is allowed to enter CPU retirement. It adds no
production opcode, handler, catalog row, or CPU end-to-end count-two integration.
The implementation and tests must remain standalone, allocation-free, and
project-contract-only; no TI conformance, machine-cycle, memory, IRQ, repeat, or
emulator behavior is claimed here.

`src/cpu/instruction_fetch.[ch]` owns private extension collection only after the
CPU has fetched and decoded the first word and learned `program_word_count`. Its
by-value result has helper capacity for exactly two program words (not an ISA-wide
maximum) and explicitly publishes status, the exact bus status, `program_words[2]`,
and the resulting count. The input copies the first word and exact `uint32_t`
`first_program_word_address` and borrows a bus descriptor/context for the call only;
it retains no pointer. Counts one and two are the only admitted widths: count zero
and every count above two are rejected before any callback. Count one publishes the
first word, makes zero callbacks, and reports `TI_C54X_BUS_OK` because no bus was
attempted. Validation and overflow paths that make zero callbacks—including a null
bus, unsupported width, and address overflow—report `TI_C54X_BUS_OK` in the result
bus-status field because no bus transaction was attempted. Count two checks
`first_program_word_address + 1` in the 32-bit carrier before exactly one callback
at that exact carrier; high carriers are allowed and are never truncated before
`UINT32_MAX` overflow is rejected. A null bus is invalid.
A missing callback for count two maps to a bus fault with
`TI_C54X_BUS_NO_CALLBACK`, while any callback failure preserves its exact non-OK
bus status and maps fetch status to bus fault. Overflow or unsupported width makes
zero callbacks. Every non-OK result publishes no partial bundle (zero count and
zero words), initializes all named result fields deterministically, and is not
retried. This owner does not fetch the first word, count
attempts or retirements, mutate CPU/PC/state, infer common 16-bit/XPC policy, model
timing/waits or memory effects, or emit observers/diagnostics.

`src/sequencer/sequential_pc.[ch]` and `program_pc_bridge.[ch]` generalize the
private staged PC effect to explicit `uint8_t program_word_count` values from 1
through 255. They convert both operands to `uint32_t` before addition for hosts
where `int` is 16 bits, reject count zero, reject any sum above `UINT16_MAX` as
unresolved, and never wrap or narrow before validation. The existing one-word
behavior remains exact: `0xfffe -> 0xffff` is admitted and a
one-word instruction at `0xffff` is unresolved. Valid/stale/malformed effects
remain transactional. The public bridge rejects carriers above `0xffff` without
XPC inference and delegates admitted width arithmetic; it performs no bus I/O.

Current CPU orchestration and P2-W18 NOP ordering remain unchanged. Because no
multiword opcode is registered, the CPU does not yet use count two end-to-end.
RPT work will decide the later orchestration order among bridge span validation,
extension reads, operand extraction, and semantics. `last_fetched_word` policy is
unchanged. Ownership follows the tree: `src/cpu/` owns fetch collection,
`src/sequencer/` owns PC arithmetic and transactional application, and tests live
under `tests/unit/cpu/` and `tests/unit/sequencer/` respectively.

The passing proofs cover count-one/no-read, exact low/high count-two carriers,
`UINT32_MAX` overflow, missing callbacks, every bus-fault status, callback scribble
on failure, no retry, complete-bundle-only publication, deterministic failures, no
pointer retention, exhaustive valid widths and boundaries 1..255, count-one
regression, explicit `0xfffd + 2 = 0xffff` admission and `0xfffe + 2`
unresolved-boundary rejection, high public carriers, malformed/stale effects, and
no mutation. Existing P2-W18 CPU tests remain regression gates. Both new tests are
`PROJECT_CONTRACT_ONLY` and passing under their owner-local runners.

The implementation commit is `3e57e316bdac5cbb469cba2cc5d5060cc45d1dea`. Focused
normal and ASan/UBSan tests passed for both new seams, and the full standalone
`make -B test` gate passed. The next work is evidence-first RPT/IRQ semantics;
this slice does not claim TI conformance, timing, repeat, IRQ, memory mutation,
XPC, or emulator integration.

### P2-W20A: RPT evidence and instruction-transaction architecture (active kickoff)

State: evidence and architecture contract complete. P2-W20B has implemented the
canonical instruction-bundle seam without adding an RPT opcode. Direct RPT handler
or CPU integration work remains gated by the later owner-specific proof slices.

The immediate forms are frozen from TI primary evidence:

- `RPT #K` is one program word with first-word value `0xEC00`, mask `0xFF00`,
  and unsigned `K` in bits 7:0 (`0..255`).
- `RPT #lk` is two program words with exact first word `0xF070` and the complete
  unsigned 16-bit operand in extension word one (`0..65535`).
- The RPT operation loads `RC` with the operand. The following instruction executes
  operand+1 times, giving a range of 1 through 65536 body executions. RPT changes
  no status bits and is itself nonrepeatable.
- Once RPT is decoded, all interrupts including NMI are deferred until every body
  execution completes; literal RS/reset remains admissible. A request still sets
  its IFR flag without immediate acknowledgement, and that flag clears when the
  interrupt is eventually taken. No device source, vector, priority, or wiring is
  selected by this common-core contract.
- Repeat-body eligibility comes from the documented repeatability restrictions;
  handlers and the CPU may not infer eligibility from opcode identity or width.

Implementation is deliberately split along the ownership hierarchy:

1. P2-W20B adds a canonical copied, value-owned instruction-word bundle under
   `src/isa/decode/`. CPU fetch owns only population of that value; neither owner
   retains caller storage.
2. P2-W20C binds immutable operand-layout and repeatability tokens through the sole
   catalog. Layout owners reuse the checked field extractor, and opcode handlers
   never hand-mask instruction words.
3. P2-W20D adds a focused RPT operation and proves both immediate layouts without
   making a partially integrated RPT row executable by the production CPU. It emits
   a discriminated, value-owned `REPEAT_START` effect carrying the exact operand and
   never mutates state, PC, interrupt state, memory, buses, observers, or diagnostics.
   Production catalog activation must be atomic with a generic effect-consumption
   path or an explicit unsupported-effect contract; a transient decode-success then
   `INVALID_STATE` CPU behavior is forbidden.
4. P2-W20E adds `src/sequencer/repeat/` as the pure single-repeat owner. Its internal
   cardinality is at least 17 bits (implemented with `uint32_t`) so `0xffff + 1`
   represents 65536 without wrap. It owns body eligibility and the generic
   interrupt-admission decision, including the reset exception, but not request
   sampling or device wiring.
5. P2-W20F may integrate the generic effect handoff into the CPU only after the
   remaining architectural rules below are resolved. `cpu.c` must never branch on
   the RPT opcode identity; it orchestrates decoded values and sequencer effects.

The directory tree and tests mirror those owners. Each phase is a separate atomic
TDD work piece, no handwritten file becomes a cross-owner monolith, and generated
metadata stays immutable and initialization-free. Machine-cycle timing, shared
memory, profiles, and emulator integration remain outside P2-W20.

Three behaviors remain explicitly unresolved and must not be encoded as convenience
policy: the phase and final value of RC decrement, the mechanism that holds or
reloads the repeated-body PC and its final PC, and arbitration when an interrupt is
already pending before RPT decode. Detailed pipeline and machine-cycle behavior
also remains outside the active timing contract. These gaps do not weaken the
proven cardinality or post-decode interrupt gate; they prevent premature CPU
integration.

#### P2-W20B: canonical complete-instruction bundle (complete)

Implementation commit:
`00700d5dd8bd72b711a5cb8ad1147efd5198a92c`.

This work piece moves the current two-word helper bundle type from CPU ownership to
`src/isa/decode/` and adds a checked complete-instruction binding without changing
the production catalog. The fixed capacity remains explicitly local to the current
helper and is not advertised as the ISA maximum. The bundle contains copied 16-bit
program words plus an explicit count and retains no pointers.

The existing first-word decode remains the only catalog lookup needed for an
attempt. A new completion step validates the first-word result, immutable metadata,
bundle count/capacity, first-word identity/pattern, and full copied bundle before
publishing a value-owned decoded instruction transactionally. Dispatch and NOP
consume that complete value. CPU orchestration calls the existing extension-fetch
owner to populate it, but NOP still performs exactly one bus callback, has identical
stop/counter/PC behavior, and remains the only registered production opcode.

Ownership is split: `src/isa/decode/` owns the type and completion validation;
`src/cpu/` owns fetching/population and execution orchestration; `src/isa/ops/`
owns the complete-instruction handler signature and NOP validation. Tests and build
manifests mirror each owner. The work piece adds no operand layout, RPT row/effect,
repeat state, architectural state integration, IRQ behavior, machine cycles,
memory mutation, observer event, knob, public API, or emulator dependency.

The completed implementation preserves the first-word classifier, asks the CPU
fetch owner to populate extension words, transactionally binds the copied bundle,
and passes that value through generic dispatch. NOP still performs one callback and
one-word retirement; `cpu.c` contains no opcode-identity branch. P2-W20C is next and
must define catalog-bound operand-layout and repeatability metadata without
prematurely activating an instruction whose effect the CPU cannot yet consume.

#### P2-W20C: catalog operand-layout and repeatability metadata (complete)

State: complete in two atomic owner-specific proofs. Production continues to
register only NOP; no RPT first word, handler, effect, or CPU behavior is activated.

P2-W20C1 is complete in implementation commit
`d56a9cc28f183c8e348837e393458660e37091d3`. It extends the sole X-macro
catalog row with two fully namespaced scalar
tokens: a catalog-owned operand-layout identity and a tri-state repeatability class.
The valid repeatability states are `UNRESOLVED`, `ELIGIBLE`, and `INELIGIBLE`;
unresolved is a safe explicit state and never implies admission. Layout identity
reserves `UNKNOWN` as invalid for production rows and provides `NONE` for NOP.
NOP binds `NONE` plus `UNRESOLVED`, because this slice does not infer its body
eligibility from absence in an exclusion summary. The Python schema, every C
X-macro pass, immutable metadata, complete decoded-instruction value, dispatch,
and NOP validation agree on the tokens. The generated 65,536-entry lookup remains
byte-for-byte unchanged and normal builds remain compiler-only. The schema rejects
old arity, unknown/unqualified/non-string tokens, and the reserved layout sentinels;
completion and operations reject forged scalar metadata transactionally.

P2-W20C2 is complete in implementation commit
`4746a8c0a1074bdf3b9419721901a0f8f0808486`. It adds decode-owned immutable
operand-layout-set descriptors and resolution.
A layout set, rather than a single field, is required so future multi-operand
instructions do not force a carrier redesign. The catalog stores only compact IDs,
never decoder-owned pointers. Decode resolves an ID to process-lifetime static data,
validates every field against the instruction program-word width, and routes copied
operand extraction through `operand_field`; handlers never hand-mask instruction
words. Outputs are transactional and no caller pointer is retained.

The directory hierarchy stays explicit: `src/isa/catalog/` owns row syntax, token
identity, immutable metadata, and catalog tests; `src/isa/decode/` owns descriptors,
resolution, extraction, and mirrored decode tests; `src/isa/ops/` only consumes the
completed values. CPU, sequencer, state, timing, buses, observers, diagnostics,
knobs, allocators, environment, and public API are unchanged.

RPT layout identities may exist only as inert decoder metadata during W20C2. They
must remain unreferenced by the production opcode catalog until a typed
`REPEAT_START` effect and generic consumption path can land coherently. RPT
encoding/operation, RC and repeat-PC progression, body admission, interrupt gates,
machine cycles, shared memory, and emulator integration remain outside W20C.

The W20C2 resolver owns three initial identities: `NONE`, inert one-word RPT
immediate-K, and inert two-word RPT immediate-lk. The RPT descriptors are grounded
in the already pinned TI layouts but remain unreachable from production first-word
decode. `operand_field` gains a pure descriptor-validation entry point reused by
both resolution and extraction; W20C2 may not duplicate its coverage, overlap, or
source-index rules. A value-owned layout-set view may borrow process-lifetime static
descriptor storage, but no catalog metadata contains a pointer and no caller storage
is retained. The executable layout proof covers minimum and maximum immediate
values, invalid identities and widths, forged views, multi-operand descriptor shape,
static lifetime, and transactional failures. The generated direct lookup remains
byte-for-byte unchanged. P2-W20D is next and may build the typed value-owned
`REPEAT_START` operation against these layouts, but it may not activate a production
RPT row until generic effect consumption is coherent.

#### P2-W20D: typed immediate-RPT effect (complete)

State: implemented and green in commit
`f4a445ccd8d6bb54d791c86f12066409fc0c6cef`. This project-contract TDD proof does
not implement or close the common TI RPT operation requirement; it prepares the
value boundary that later state/sequencer integration can consume.

`src/isa/ops/repeat/` owns a pure immediate-RPT effect builder and its mirrored
test subtree. Its input is a borrowed, copied `ti_c54x_operand_field_value` already
extracted through one of W20C2's inert K/lk layouts. This avoids inventing a valid
`ti_c54x_decoded_instruction` identity before RPT exists in the catalog and keeps
first-word masks entirely under decode/catalog ownership. The builder accepts only
the proven unsigned 8-bit K and 16-bit lk field shapes, retains no pointer, and
publishes only after full validation. Null pointers use `INVALID_ARGUMENT`; a
nonnull field with an unsupported width or a raw value outside its declared width
uses the new private `INVALID_OPERAND` status without renumbering existing values.

`src/isa/ops/op_effect.h` adds a discriminated `REPEAT_START` payload containing
the exact `uint16_t` operand value. The payload deliberately does not contain
operand+1 cardinality, a repeated-body PC, cycle cost, interrupt state, or a state
pointer; those belong to later sequencer/state owners. Existing NOP and dispatch
paths must initialize the expanded effect deterministically, while CPU continues to
accept only `NONE` and therefore remains unchanged.

The TDD proof covers K and lk composition through the immutable layout extractor,
minimum/interior/maximum values, copied ownership, invalid widths and out-of-range
raw values, nulls, and byte-transactional failure output. The implementation creates
no production RPT catalog row, first-word lookup, generated handler-table entry,
dispatch path, or CPU behavior. The existing common RPT decode and operation
requirements remain `PLANNED`; RC mutation/decrement, operand+1 cardinality,
repeat-body eligibility, IRQ/NMI/IFR/reset admission, repeated-body PC flow, timing,
memory, profiles, and emulator integration remain outside W20D.

The completed module accepts only copied field values, appends an explicit
`INVALID_OPERAND` status without renumbering existing statuses, and commits a named
value-owned effect payload only after validating the 8- or 16-bit shape and raw
range. The operation retains no pointer and imports no catalog, decoded-instruction,
state, bus, observer, or host surface. Source and tests live in mirrored repeat-family
subtrees with owner-local manifests. P2-W20E is next and owns pure single-repeat
cardinality, body admission, and request-admission policy; W20D does not precompute
those decisions.

#### P2-W20E: pure single-repeat policy foundation (active kickoff)

State: implemented and green in commit
`d876f94dfb6ddbd08257f477457ca20ae4f34e68` as three project-contract TDD
proofs. The common TI cardinality,
body-eligibility, and IRQ-gate requirements remain planned until production
integration proves actual body retirement, concrete catalog classifications, and
the active repeat window. This avoids treating a useful pure calculation as broader
execution conformance.

`src/sequencer/repeat/` becomes a dedicated subowner with three leaf modules and a
mirrored test subtree:

- `repeat_cardinality` accepts a copied `uint16_t` operand, widens before adding one,
  and transactionally publishes a `uint32_t` body-execution count. The complete
  input range is valid, including zero to one and `0xffff` to 65536. It owns no
  execution loop, RC, PC, state, effect consumption, or timing.
- `repeat_body_eligibility` consumes only the existing catalog-owned
  `ti_c54x_repeatability_class` scalar. `ELIGIBLE` admits, `INELIGIBLE` rejects, and
  `UNRESOLVED` remains explicitly non-admitting. Forged values fail; the policy may
  not inspect opcode identity, width, operands, addressing forms, or catalog tables.
- `repeat_request_gate` consumes a caller-supplied `uint32_t` count remaining and a
  small gate-local request class: maskable, NMI, or literal RS/reset. Zero means the
  repeat gate is inactive and blocks nothing; counts one through 65536 defer
  maskable and NMI requests while never blocking literal reset. Larger counts and
  forged request kinds fail transactionally. “Not blocked” is deliberately not a
  claim that another interrupt policy must service the request.

Each leaf owns its status/decision types and value contract. Parent sequencer
Make/CMake manifests aggregate `repeat/`; they do not flatten the leaf sources into
an unrelated owner. The first TDD registration creates three planned project tests
under `tests/unit/sequencer/repeat/`, then implementation makes only those contracts
passing. The existing common tests remain planned and separate.

W20E must not import the W20D opcode effect, decode a catalog row, mutate CPU or
architectural state, sample or write IFR, choose sources/vectors/priorities, advance
or reload PC, expose architectural RC phase/final value, add timing, touch memory or
buses, add observers/diagnostics/knobs, read the environment, allocate, or activate
production RPT decode. Pending-request persistence stays with the later
`src/sequencer/interrupt/` owner. RC progression, repeated-body PC flow, and
predecode interrupt arbitration remain explicitly unresolved.

Exit condition: the three project requirements and mirrored tests are passing in
Make and CTest; the full `uint16_t` cardinality domain is proven without wrap;
unresolved repeatability never admits; request decisions cover inactive, first,
interior, and 65536-remaining windows plus invalid inputs; source and test manifests
mirror the new owner; production lookup remains unchanged; and the common TI rows
remain planned without overclaim.

The completed implementation keeps every policy value-only and allocation-free.
Cardinality widens before addition and exhaustively maps the entire `uint16_t`
domain to one through 65536. Body admission preserves `UNRESOLVED` as a distinct
non-admitting result and rejects forged repeatability values transactionally.
Request gating validates counts through 65536, reports only `NOT_BLOCKED` or
`DEFER`, and names the literal RS/reset exception without owning a reset opcode or
device path. Make and CMake aggregate matching repeat-owner manifests, while CPU,
state, opcode effects, production lookup, and every unresolved integration policy
remain unchanged.

#### P2-W20F: CPU integration evidence closure (deferred)

State: evidence review complete; implementation is intentionally deferred. A
second bounded audit of official TI `SPRU172C` Rev C and `SPRU131G` Rev G material
confirmed the existing RPT load, cardinality, repeatability, and post-decode
interrupt gate, but did not close the three execution-order questions required by
the generic CPU transaction.

The additional narrow evidence is useful but insufficient for retirement:

- The RPT entry says RC is inaccessible to software while single-repeat execution
  decrements it. It does not state the decrement edge or final visible RC.
- Repeated MVDK/MVKD behavior tests the current RC value for a conditional address
  increment. That proves an instruction body can depend on repeat progress, so an
  abstract remaining-count loop cannot be assumed to reproduce all architectural
  RC behavior.
- Generic sequential-PC, block-repeat, request-receipt, acknowledgement, latency,
  and pipeline examples do not specify the single-repeat PC hold/reload mechanism,
  final PC, or arbitration of a request already pending before RPT reaches decode.
- GNU binutils 2.42 remains a secondary encoding and operand-shape oracle only. Its
  opcode metadata supplies no RC, PC, IFR, or interrupt-arbitration semantics.

Therefore production RPT rows remain inactive and `cpu.c` gains no repeat branch or
effect consumer. The next admissible normative W20F input is stronger TI evidence
or a versioned original-hardware observation that records initial state, program
words, request timing, externally visible RC/PC results, and provenance. An optional
black-box differential vector may expose a hypothesis or disagreement but cannot
close the common requirement by itself. No such test input may introduce an emulator
build dependency. Until then, shared-memory transaction foundations and other fully
evidenced owner-local work may proceed independently.

### P2-W21: shared-memory transaction foundation

State: P2-W21A, P2-W21B, and P2-W21C implemented and green. External adapter
visibility and arbitration remain planned but are introduced only when a concrete
memory opcode requires them.
This work builds the independent gate required before any instruction may mutate
memory. The core remains word-native and host-endian-neutral; sharing, storage,
device mapping, and arbitration stay outside the CPU instance.

The owner hierarchy is phased so no early value accumulates unrelated policy:

1. P2-W21A creates `src/bus/request/` and the mirrored
   `tests/unit/bus/request/` subtree for one pure WORD16 request value.
2. P2-W21B creates `src/bus/byte_order/` for explicit BE and LE conversion of one
   word to two byte lanes. Program-image encoding, physical shared-memory lanes,
   and snapshot/wire formats remain separate policies rather than one global endian
   flag.
3. P2-W21C creates `src/bus/transaction/` for a copied request, caller-owned
   transaction object/pending token, lifecycle-only state, and a begin/poll/complete
   reducer with protocol-only exactly-once issue and completion across READY, WAIT,
   and FAULT outcomes. External visibility and arbitration remain a separate planned
   adapter owner.
4. Only after that lifecycle is proven may `src/sequencer/memory/` own declarative
   memory effects and ordered visibility. External adapters arbitrate DSP, ARM, DMA,
   and device masters; the core never owns a host mutex or master identity.

The existing synchronous public callbacks and `ti_c54x_bus_status` remain unchanged
during the private foundation. A later one-shot adapter may wrap them, but WAIT is
not appended to the current ABI and no new behavior is routed through `cpu.c` yet.

#### P2-W21A: private WORD16 bus-request contract (active kickoff)

The first slice owns only the canonical command for one architectural 16-bit word.
`src/bus/request/word16_request.[ch]` defines zero-sentinel space and access enums,
an exact `uint32_t address_word`, and a copied `uint16_t write_value_word`. Separate
read and write constructors return a by-value status plus request: PROGRAM preserves
the full 32-bit carrier; DATA and IO reject values above `UINT16_MAX` before any
narrowing; READ always publishes a canonical zero write value; WRITE preserves the
exact input value. Construction reports exactly `OK`, `INVALID_SPACE`, or
`INVALID_ADDRESS`, validating the space discriminant before its address domain.
Invalid construction zeroes every named request field, which leaves both
discriminants invalid; no padding-byte value is promised or tested. A borrowed
request validator retains no pointer and rejects null, zero, forged, range-invalid,
or noncanonical values.

WORD16 is fixed by the type, so W21A adds no redundant transfer-width enum.
Multiword operands will compose separately ordered word requests rather than claim
an accidentally atomic host `uint32_t` transfer. W21A also carries no phase: W21C
binds a copied request only to transaction identity and lifecycle state. A later
`src/sequencer/memory/` owner binds instruction and memory-effect phases, keeping
pure request shape separate from both lifecycle and semantic phase policy.

The planned TDD proof covers all six space/access combinations; zero, maximum, and
full-width program carriers; DATA/IO zero and `0xffff`; rejection of `0x10000` and
larger DATA/IO inputs without truncation; copied write boundaries; canonical read
payload; zero/forged discriminants; invalid-result shape; borrowed-validator nulls;
and named-field zeroing on each failure. Source-boundary checks prove that the
callback-free request owner imports no host I/O surface. Source and test manifests
mirror `bus/request/`; root Make/CMake files only aggregate the new bus owner.

This is a project contract, not TI behavior. It adds no public API, callback, read
completion, byte-order conversion, host memory, raw pointer, transaction token,
phase, wait state, issued/completed state, timing, arbitration, memory map, opcode
effect, CPU/state mutation, observer, diagnostic, knob, environment read, allocator,
mutable global, firmware, or emulator dependency. Exit requires the registered
project test to pass through both supported build systems with current CPU behavior
unchanged.

The completed private leaf is 102 lines of implementation, 77 lines of interface,
and 247 lines of focused proof. Its constructors and validator preserve the frozen
status precedence and address domains, and validation is non-mutating for both valid
and invalid borrowed values. Nested Make/CMake manifests mirror the bus/request
owner; root files add aggregation lines only. The full standalone Make gate passes.
No public callback or CPU execution path consumes the request yet.

#### P2-W21B: explicit WORD16 byte-order codec (implemented)

This slice creates the separate `src/bus/byte_order/` and mirrored
`tests/unit/bus/byte_order/` owners. `word16_codec.[ch]` converts one copied
`uint16_t` word to exactly two value-owned `uint8_t` lanes, or two borrowed readable
lanes to one copied word, under an explicit zero-sentinel `INVALID`, `BIG`, or
`LITTLE` byte order. BIG maps `0x1234` to `{0x12, 0x34}` and LITTLE maps it to
`{0x34, 0x12}`.

Encode and decode return separate by-value result types. Their only statuses are
`OK`, `INVALID_ARGUMENT`, and `INVALID_ORDER`. Encode accepts no pointer; decode
borrows exactly two bytes until return and retains none. Decode validates a null
input before the order, so null plus a forged order reports `INVALID_ARGUMENT`.
Otherwise a zero or forged order reports `INVALID_ORDER`. Every failed result zeroes
its named byte or word payload fields without making a C-padding claim.

The implementation uses unsigned shifts, masks, and explicit fixed-width scalar
conversions. Pointer/type-punning casts, unions, packed structs, native-layout
`memcpy`, and host-order inference are forbidden. There is no mutable or immutable
global "current endian" setting: program-image encoding, physical shared-memory
lanes, and snapshot/wire formats remain three separately owned adapter/profile
policies that select this codec explicitly.

The TDD proof exhausts every `uint16_t` value through encode-then-decode for both
orders, checks exact lane vectors for `0x0000`, `0x1234`, and `0xffff`, covers invalid
and forged orders, proves the decode null-before-order precedence, checks failed
named payload fields, and snapshots borrowed input lanes before and after decoding.
Parent bus manifests aggregate the new child; the already present root bus
aggregation is unchanged.

This is a `PROJECT_CONTRACT` conversion policy, not evidence that C54x, Nokia,
program images, shared RAM, or any device use one physical byte order. It adds no
public API, request change, callback, host memory, transaction, phase, wait,
arbitration, memory effect, CPU/state/ISA behavior, observer, diagnostic, knob,
environment read, allocator, mutable global, firmware, or emulator dependency.
The completed private leaf is 93 lines of implementation, 58 lines of interface,
and 170 lines of focused proof. The proof exhausts both orders and independently
pins external encode and decode vectors, closes null-before-order precedence and
named error-payload behavior, and verifies borrowed-input nonmutation through both
branches. Owner-local Make and CTest registrations mirror the production tree.
Focused normal and sanitizer tests and the full standalone Make gate pass while
W21A, the public ABI, and current CPU behavior remain unchanged.

#### P2-W21C: resumable WORD16 transaction protocol (implemented)

This implementation owns the private `src/bus/transaction/` leaf, mirrored by
`tests/unit/bus/transaction/`. Production leaves are `word16_transaction.[ch]`
and `word16_transaction_validate.[ch]`; `test_word16_transaction.c` proves
lifecycle behavior and `test_word16_transaction_contract.c` proves adversarial
validation. Ownership is explicit in nested Make/CMake manifests and parent
aggregation. The implementation changes no public ABI, CPU path, request type,
codec, or adapter behavior.

The reducer is pure, callback-free, allocation-free, and host-storage-free. The
caller declares transaction storage as canonical `{0}`. Its lifecycle vocabulary
is deliberately not an address-effect phase vocabulary:
`IDLE=0`, `ISSUE_READY`, `IN_FLIGHT`, `COMPLETION_READY`, `COMPLETE`, and `FAULT`.
`PRE_ACCESS` and `POST_ACCESS` remain owned by address effects; future
`src/sequencer/memory/` work binds instruction/memory-effect phases separately.

The transaction object is the caller-owned pending token and copies a fresh,
caller-generated nonzero `uint64_t transaction_id`, a valid W21A request, a staged
read word, and a staged `ti_c54x_bus_status`. The caller must provide a fresh ID
for each reuse while stale observations may exist; generation, wrap, and identity
allocation are external policies. Active transaction storage must not be copied or
used as a second authoritative value while active. Ordinary pointers to the same
storage do not create a second authority. An ID cannot detect a copied state bearing
the same ID or deliberate ID reuse, so those are explicit caller preconditions
rather than implementation claims.

The planned header-level API is:

- `prepare(tx *, borrowed valid request *, id)` copies the request and ID only from
  canonical `IDLE` and enters `ISSUE_READY`.
- `begin(tx *)` returns a by-value issue result with exactly the named fields
  `status`, `transaction_id`, and `request`. Only success enters `IN_FLIGHT` and
  authorizes one external issue; failed issue results zero `transaction_id` and
  every named request field.
- `poll(tx *, borrowed observation *)` accepts the copied ID and zero-sentinel
  `INVALID`, `WAIT`, `READY`, or `FAULT` observation. Repeated canonical `WAIT`
  returns transaction status `WAIT` and remains `IN_FLIGHT` without another begin
  or issue authorization. The first canonical `READY` returns `OK`, or the first
  canonical `FAULT` returns transaction status `FAULT`; either latches one
  observation and enters `COMPLETION_READY`. Later polls are rejected.
- `complete(tx *, id)` returns a by-value result with exactly the named fields
  `status`, `bus_status`, and `read_value_word`. While `IN_FLIGHT`, a matching
  nonzero ID returns transaction status `WAIT` and leaves the transaction unchanged;
  zero and nonmatching IDs still return `INVALID_ARGUMENT` and
  `STALE_TRANSACTION`, respectively. With a matching ID in `COMPLETION_READY`, it
  returns transaction status `OK`, bus status `TI_C54X_BUS_OK`, and a read word only
  for a READ/READY transaction, or transaction status `FAULT`, the exact staged
  non-OK bus status, and a zero read word. It then enters `COMPLETE` or `FAULT`;
  duplicate completion is rejected. Every failed completion result zeroes its
  `bus_status` and `read_value_word` fields.
- `reset(tx *)` clears only canonical `IDLE`, `COMPLETE`, or `FAULT` to zeroed
  `IDLE`; it rejects `ISSUE_READY`, `IN_FLIGHT`, and `COMPLETION_READY`, so it
  cannot imply cancellation. There is no abort operation in this contract.

Statuses are exactly `OK`, `WAIT`, `FAULT`, `INVALID_ARGUMENT`, `INVALID_STATE`,
and `STALE_TRANSACTION`. Null pointers are checked first, then canonical state
and lifecycle, then zero IDs as invalid arguments, then nonmatching nonzero IDs as
stale, and finally observation tag/payload shape. `WAIT` and `READY` observations
carry `TI_C54X_BUS_OK`; `FAULT` accepts only the existing known non-OK
`UNMAPPED`, `FAULT`, or `NO_CALLBACK` public bus statuses. WAIT/FAULT read payloads
are zero, READY writes publish no read word, and READY reads may publish any
`uint16_t`. Every rejected transition preserves all named transaction fields; no
C-padding-byte claim is made.

Exactly-once is a protocol claim only: one issue authorization and one terminal
completion per authoritative token under the caller ownership and unique-ID
preconditions. A separate planned `src/bus/adapter/` owner and
`tests/unit/bus/adapter/test_word16_exactly_once.c` must prove actual callback,
MMIO, or RAM issue and completion counts and physical visibility across wait-state
suspension. Deterministic multi-master arbitration remains another separately owned
later proof. W21C fabricates no adapter count and claims no host memory, device,
timing, observer, diagnostic, environment, global, CPU, request, or codec behavior.

The TDD matrix covers all six space/access combinations, copied request and source
mutation, one begin, repeated WAIT, READY reads `0`, `0x1234`, and `0xffff`, READY
write zero payload, all three public fault statuses, one completion, reset/reuse
with a fresh ID, and stale observations. The adversarial contract proof covers
canonical and forged states/requests, nulls, zero or forged observation kinds and
bus statuses, noncanonical payloads, active-reset rejection, precedence, named
field nonmutation, and zero failed issue/completion payloads. Focused normal and
ASan/UBSan tests pass with runners `ti_c54x.bus_word16_transaction` and
`ti_c54x.bus_word16_transaction_contract`; the full standalone gate is green.

The completion review recorded TDD RED before the production leaves existed,
independent lifecycle/adversarial ownership, no major defect, and closed proof
gaps. Production files are 227/113 lines, headers 115/40, lifecycle test 453,
and adversarial test 499; future growth splits tests rather than exceeding the
review threshold. The implementation commit is `a3edba970cb2143bfb9d3eb437b9882084352a0d`.
No knob or integration behavior was added. Actual callback/MMIO/RAM exactly-once
visibility, arbitration, timing, memory effects, CPU/ISA integration, and public
ABI behavior remain outside this leaf. Follow-up publish/remote verification is
required before the next ISA slice; external adapter work stays deferred until a
concrete memory opcode requires it.

### P2-W22: immediate status-field LD vertical slice (implemented; local verification green)

P2-W22 is one atomic end-to-end ISA vertical slice, not a new foundation phase.
It batches three one-word immediate status-field forms:

- `LD #k9,DP`: value `0xEA00`, mask `0xFE00`, unsigned 9-bit field, `DP := k9`,
  no other status-bit side effects, and repeatability `INELIGIBLE`.
- `LD #k5,ASM`: value `0xED00`, mask `0xFFE0`, raw 5-bit field representing
  `-16..15`, `ASM := raw field`, no other status-bit side effects, and
  repeatability `UNRESOLVED`.
- `LD #k3,ARP`: value `0xF4A0`, mask `0xFFF8`, 3-bit field, `ARP := k3`, no
  other status-bit side effects, and repeatability `INELIGIBLE`.

TI SPRU172C Table 2-19 (printed pages 2-14–2-15) and LD §4.70 (printed pages
4-70–4-71) are the primary form/effect evidence. Table 2-28 (printed pages
2-19–2-21) supplies DP/ARP nonrepeatability; ASM repeatability remains
unresolved. Summary cycle columns are deliberately not adopted as timing
evidence. GNU binutils tag `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c` lines 316–319, is a nonnormative corroboration only;
no implementation, table, comment, or code is copied or vendored, and any
mismatch escalates to TI evidence.

Ownership is deliberately minimal. `src/isa/catalog/` owns exact masked rows,
`src/isa/decode/` owns raw-field extraction, and `src/isa/ops/load/` owns the
three readable semantic handlers and their aggregate target effects;
`src/state/status/` owns only the status-register contract; `src/sequencer/status/`
owns transactional effect application; and `src/cpu/` observes and atomically
retires the effect. `ti_c54x_state` intentionally grows observation fields before
API stability; those fields are not a raw snapshot or persistence ABI and no
stable persistence guarantee is made. The mirrored tests are
`tests/unit/isa/decode/test_ld_status_fields.c`,
`tests/unit/isa/ops/load/test_ld_status_fields.c`,
`tests/unit/state/status/test_status_registers.c`,
`tests/unit/sequencer/status/test_status_field_effect_apply.c`, and
`tests/unit/cpu/test_cpu_ld_status_fields.c`. Local C_EXECUTABLE runners are
`ti_c54x.ld_status_fields_decode`, `ti_c54x.ld_status_fields_ops`,
`ti_c54x.status_registers`, `ti_c54x.status_field_effect_apply`, and
`ti_c54x.cpu_ld_status_fields`; the full local and remote gates are green.
No memory/bus transaction, timing, RPT/IRQ, accumulator, device, or emulator work
is part of this slice; any status-bank split and generic effect commit exist only
because these opcodes require them. The local registry is 91 immutable evidence
records, 133 requirements (107 `IMPLEMENTED`, 9 `PLANNED`, 17 `UNRESOLVED`), and
96 tests (87 `PASSING`, 9 `PLANNED`); the full local repository gate is green and
remote P2-W24 validation is green, with the latest clean hosted performance
record captured at `7cad341`.

### P2-W23: MAX/MIN accumulator vertical slice (planned; implementation not started)

State: implemented; this heading remains the frozen historical evidence anchor.

P2-W23 is a narrow accumulator-only ISA slice. The production rows, state/effect
owners, and public CPU proof now implement the frozen contract:

- `MAX dst` registers `MAX_ACC` as `0xF486/0xFEFF`; `MIN dst` registers
  `MIN_ACC` as `0xF487/0xFEFF`. Each mask leaves only destination bit D variable,
  yielding exactly four one-word encodings: D=0 targets A (`0xF486`/`0xF487`)
  and D=1 targets B (`0xF586`/`0xF587`). TI's operation text selects A on
  `A >= B` for MAX and `A <= B` for MIN, including ties; the selected value is
  written to `dst`, C is 0 for an A selection and 1 for a B selection, and C is
  the only status bit changed. The other accumulator is unchanged.
- A and B are signed, full 40-bit accumulator values including guard bits. Tests
  must cover zero, positive/negative ordering, ties, sign-bit and guard-bit
  boundaries, both destinations, and preservation of the non-destination value.
  The exact four encodings and all mask near-misses are decode-test vectors;
  semantic vectors are exhaustive over destination/operation/tie relation and
  representative full-width accumulator boundaries.
- Ownership mirrors the tree: `src/isa/catalog/` owns the two declarative rows;
  `src/isa/decode/` owns an accumulator destination layout and raw extraction;
  `src/isa/ops/compare/` owns thin MAX/MIN handlers that receive a copied,
  read-only A/B input snapshot, perform the signed compare/select, and return a
  value-owned effect; `src/state/accumulator/` owns canonical full-40-bit A/B
  storage, checked access, and knownness; `src/sequencer/accumulator/` validates
  only the effect payload structure and atomically applies the candidate
  accumulator plus C state once; `src/cpu/` owns checked snapshot acquisition,
  fetch/dispatch/retire integration. Tests mirror those owners
  under `tests/unit/isa/decode/`, `tests/unit/isa/ops/compare/`,
  `tests/unit/state/accumulator/`, `tests/unit/sequencer/accumulator/`, and
  `tests/unit/cpu/`.
- The effect is transactional: handlers do not mutate state and do not place
  source snapshots in the effect. The sequencer does not re-run MAX/MIN
  comparison; it validates only destination, a canonical 40-bit two's-complement
  selected value, and one-bit C before one retirement commit. Structurally
  invalid or rejected effects leave every named field unchanged. Before dispatch,
  the CPU obtains a checked, immutable A/B snapshot; unknown source state stops
  with `INVALID_STATE` without
  retirement or mutation. CPU tests may seed state through the accumulator
  owner's checked internal API before invoking the public execution path;
  no premature public set-state/bootstrap API is introduced. The CPU proof
  checks one attempt/retire, sequential PC +1, destination-only mutation, C
  result, and unknown-word stop.

Review boundary: `src/cpu/cpu.c` is nearing the 500-line review threshold. The
next CPU growth must extract effect routing/retirement transaction into its own
owner; no opcode family may add a new CPU opcode-identity branch.

Performance note: the initial P2-W23 implementation copied accumulator snapshots
and candidate state for every opcode, measuring a 13–25% NOP/LD regression. Catalog
input-resource masks now declare `NONE` for NOP/LD and `ACCUMULATORS` only for
MAX/MIN; CPU snapshot/candidate work follows that generic mask. Two local
bench-compare samples were noisy: one remained +11.7% to +13.7% for NOP/LD,
while the other was within -3.7% to +3.4% versus the hosted cohort; spread
remains high and needs follow-up.

TI `SPRU172C` §4 MAX/MIN (printed pp. 4-99–4-100) is the normative form and
effect evidence; TI `SPRU131G` §2.3 (printed p. 2-8) is the normative
40-bit-accumulator/guard-bit evidence. GNU
binutils 2.42 commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c:358–359`, is nonnormative encoding corroboration only and
must not be copied or vendored. Table 2-28's preamble/list does not positively
authorize MAX/MIN repetition, so repeatability stays `UNRESOLVED`; no machine
cycle or pipeline timing claim is admitted. No memory/bus/device/emulator/RPT/IRQ
behavior is part of this work piece.

### P2-W24: generic accumulator-write effect and CMPL vertical slice (planned)

State: implemented; this heading remains the frozen historical evidence anchor.

P2-W24 is a two-step, CMPL-driven accumulator slice. The first step generalizes
the P2-W23 sequencer payload from a select-named effect to a reusable
`ACCUMULATOR_WRITE` effect. The second step registers and executes CMPL through
that seam. The work is deliberately small enough for a family-local ISA lane,
but the shared effect and CPU retirement surfaces remain integration-owned.

Step 1 freezes the generic payload and ownership contract:

- Rename the select-named effect to `ACCUMULATOR_WRITE`; do not add a CMPL-only
  effect or retain an `ACCUMULATOR_SELECT` semantic type. The payload owns a
  destination (A or B), one canonical full-40-bit value, and an explicit C
  action: `PRESERVE` or `WRITE`.
- Canonical validation is strict and value-owned. `PRESERVE` requires
  `c_value == 0`; `WRITE` requires `c_value` to be exactly 0 or 1. Unknown
  action values, noncanonical 40-bit payloads, invalid destinations, and any
  rejected call leave both accumulators and C unchanged. The sequencer applies
  one validated effect atomically and preserves all unrelated status state.
- The handler remains pure: source snapshots are copied into the input bundle,
  never into the effect. The CPU requests the A/B snapshot only when catalog
  metadata carries the generic `ACCUMULATORS` input-resource mask. No opcode
  identity branch is permitted, and this slice must not grow `src/cpu/cpu.c`;
  effect routing/retirement stays on the existing generic seam.

Step 2 implements CMPL as the first consumer:

- `CMPL` registers value `0xF493` with mask `0xFCFF`. S is bit 9 and D is bit 8,
  producing exactly four one-word forms for source A/B and destination A/B. A
  generic `ACCUMULATOR_SOURCE_DESTINATION` layout exposes S as operand 0 and D
  as operand 1; no CMPL-named field extractor or handler-side bit parsing is
  permitted.
- CMPL writes the full 40-bit one's complement of the selected source
  accumulator to the selected destination. It has no status effects, so its
  effect uses `C PRESERVE` with the canonical `c_value == 0` payload. The
  non-destination accumulator and all unrelated status fields are preserved.
- `src/isa/catalog/` owns the declarative row and `ACCUMULATORS` resource
  metadata; `src/isa/decode/` owns S/D extraction; `src/isa/ops/accumulator/`
  owns the pure CMPL handler; `src/sequencer/accumulator/` owns generic effect
  validation/application; and `src/cpu/` only consumes the generic effect with
  no new opcode-specific path. Tests mirror those owners under
  `tests/unit/isa/decode/`, `tests/unit/isa/ops/accumulator/`,
  `tests/unit/sequencer/accumulator/`, and `tests/unit/cpu/`.
- Decode coverage is exhaustive over all 65,536 first words: the four CMPL
  forms must classify exactly, every fixed-bit near-miss must reject, and
  no unrelated catalog row may be shadowed. Semantic vectors cover all source
  and destination combinations, zero, sign/guard boundaries, and C-preserve
  atomicity.

TI `SPRU172C` §4 CMPL (printed p. 4-32) is the normative encoding and effect
source. `SPRU131G` §2.3 (printed p. 2-8) remains the full 40-bit accumulator and
guard-bit source; CMPL applies one's complement across all forty bits. GNU
binutils 2.42 commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c`, is nonnormative encoding corroboration only and must not
be copied or vendored. Table 2-28 does not positively authorize CMPL repetition,
so repeatability remains `UNRESOLVED`; summary cycle/pipeline columns are not
timing evidence and timing remains `UNRESOLVED`.

Exit condition: satisfied. The generic `ACCUMULATOR_WRITE` contract and its
adversarial payload tests pass, and CMPL's four forms pass exhaustive decode,
pure one's-complement semantics, transactional effect application, and public
CPU retirement tests without a CPU file-growth or opcode-identity exception.
No memory, bus, device, emulator, RPT, IRQ, repeatability, or machine-cycle
claim is added by this work piece.

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

### P2-W26: SFTL shifted-accumulator vertical slice (planned)

State: implemented; the immutable `(planned)` heading remains the historical
evidence anchor. Catalog, decode, shift-operation, CPU, full local repository,
and remote implementation proofs are green.

P2-W26 is a narrow SFTL accumulator slice. It adds no public API, architectural
state, input-resource bit, effect type, sequencer seam, memory behavior, RPT/IRQ
behavior, device behavior, or machine-timing contract. SFTL consumes the
existing `ACCUMULATOR_WRITE` plus `C_WRITE` effect path.

The evidence-backed form and ownership contract is:

- TI `SPRU172C` §4 SFTL (printed p. 4-158, PDF p. 271) defines syntax
  `SFTL src, SHIFT [,dst]`, A/B operands, and a signed shift range of `-16..15`.
  The bit format `1111 00SD 111S HIFT` yields value `0xF0E0` with mask
  `0xFCE0`, one program word, and 128 encoded first words. The catalog row is
  `SFTL_ACC`/`sftl_acc` with `ACCUMULATORS` input only.
- `src/isa/decode/` owns the generic
  `TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout:
  source bit 9 is operand 0, destination bit 8 is operand 1, and raw shift bits
  4:0 are operand 2. The handler sign-extends the five-bit two's-complement
  field; no SFTL-named bit parser is permitted.
- `src/isa/ops/shift/` owns pure semantics. For `q < 0`, let `n = -q`, shift
  selected low32 right by `n`, and set C to source bit `n-1`; for `q == 0`,
  preserve low32 and set C to zero; for `q > 0`, shift low32 left by `q` and
  set C to source bit `32-q`. Guard bits 39:32 clear, and C is the only status
  field changed. Pre-C is not an input. The destination is selected by D;
  source and destination may be A or B.
- `src/cpu/` routes SFTL through the existing generic input/effect/retirement
  path without a production change or opcode branch. Unknown selected source or
  noncanonical input/decode rejects transactionally; unknown unselected state is
  allowed. Non-C status, failed effects, and the PC-`0xffff` boundary remain
  unchanged on rejection. Ownership-mirrored tests live under
  `tests/unit/isa/catalog/`, `tests/unit/isa/decode/`,
  `tests/unit/isa/ops/shift/`, and `tests/unit/cpu/`.

The TDD proof must exhaust all 128 encodings and cover q boundaries `-16`, `-1`,
`0`, `1`, and `15`, low32 zero/one/high-bit/all-ones vectors, guard-only and
mixed raw40 values, carry endpoint cases, source/destination combinations,
known and unknown pre-C values that must not affect execution,
canonical input and decode rejection with output nonmutation, unknown selected
source rollback, allowed unselected unknownness, preserved non-C status, and
one attempt/one retirement/PC+1 for admitted forms. The catalog's post-slice
quality result is expected to be ten rows and 693 populated first-word
encodings; that is not a claim of 693 distinct instructions. The mixed
reset-runnable workload remains four of ten rows.

TI `SPRU172C` §4 SFTL (printed p. 4-158, PDF p. 271) is normative for the
encoding, execution/carry cases, guard clearing, and its statement that C is the
only affected status field. §2.5/Table 2-28 (printed pp. 2-19–2-21) supplies
only a repeatability boundary: SFTL is not positively authorized as repeatable,
so repeatability remains `UNRESOLVED`. Any GNU binutils 2.42 reference at pinned
commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e` is nonnormative corroboration
only and must not be copied or vendored. Summary cycle fields are not admitted
as machine-timing evidence.

Closeout: the catalog now contains ten declarative rows and 693 populated
encoded first words, not 693 distinct instructions; the mixed reset-runnable
coverage remains four of ten rows. No `cpu.c` change, public API, memory,
RPT/IRQ, or timing claim was added.

### P2-W27: AND/OR/XOR register-only logical accumulator vertical slice (planned)

State: implemented; targeted proof and full local and remote gates are green.
Keep the immutable `(planned)` heading as the evidence anchor. The clean
catalog-thirteen performance record is captured at `ef87c94`; it shows no
confirmed regression. This is a hosted implementation observation only, not a
DSP timing claim.
P2-W27 adds three register-only one-word logical accumulator families without
introducing architectural state, input resources, effect kinds, sequencing
seams, public APIs, memory, RPT/IRQ, parallel execution, or timing claims.

The normative forms are TI `SPRU172C` §4 AND (printed p. 4-11), OR (p. 4-123),
and XOR (p. 4-201):

- `AND_ACC_SHIFT`/`and_acc_shift`: value `0xF080`, mask `0xFCE0`, opcode
  `1111 00SD 100S HIFT`, one word, 128 encoded first words.
- `OR_ACC_SHIFT`/`or_acc_shift`: value `0xF0A0`, mask `0xFCE0`, opcode
  `1111 00SD 101S HIFT`, one word, 128 encoded first words.
- `XOR_ACC_SHIFT`/`xor_acc_shift`: value `0xF0C0`, mask `0xFCE0`, opcode
  `1111 00SD 110S HIFT`, one word, 128 encoded first words.

The fixed bits 7:5 are AND=`100`, OR=`101`, XOR=`110`, disjoint from the
existing SFTL=`111` row under the same mask. An omitted destination is only the
assembler alias `D=S`, never an additional catalog row. The post-slice catalog
expectation is 13 rows and 1077 populated first-word encodings; those entries
are encodings rather than instruction count, and reset-runnable coverage stays
four of 13.

`SPRU131G` §2.3.1/§2.3.3 (printed pp. 2-8–2-9) supplies the 40-bit Boolean
ALU, accumulator, and barrel-shifter foundation. Decode reuses
`TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5`: S is bit 9, D
is bit 8, and raw shift bits 4:0 sign-extend to `q=-16..15`. The existing
`ACCUMULATORS` input and generic accumulator-write effect remain the only
resource/effect path.

Pure handlers in `src/isa/ops/logical/` operate on the pre-instruction
destination and shifted source. For `q<0`, logically right-shift the full
40-bit source by `-q`; for `q=0`, use identity; for `q>0`, left-shift the
full-40 source and mask to 40 bits. Combine with `&`, `|`, or `^`. Guard bits
participate, the result is canonical 40-bit with no sign extension, and
`SPRU131G` §4.2.3 (p. 4-12) requires `C_PRESERVE` with all other status
unchanged. Both participating accumulators must be known/canonical; if `S==D`,
unknown data in the unselected accumulator is allowed. Invalid inputs and
effects reject transactionally at the PC boundary without mutation.

Ownership mirrors the repository hierarchy: `src/isa/catalog/` owns rows and
resource metadata; `src/isa/decode/` owns the generic layout/extraction;
`src/isa/ops/logical/` owns pure handlers; `src/cpu/input/` and
`src/sequencer/accumulator/` remain owners of input collection and generic
effects; and `src/cpu/` only orchestrates generic retirement. Phase 1 RED tests
are independently owned at
`tests/unit/isa/catalog/test_logical_accumulator_metadata.c`,
`tests/unit/isa/decode/test_logical_accumulator_decode.c`,
`tests/unit/isa/ops/logical/test_logical_accumulator.c`, and
`tests/unit/cpu/test_cpu_logical_accumulator.c`. Phase 2 production starts only
after those contracts fail for the missing family rows/handlers; the
orchestrator serializes catalog, decode, generated, and shared changes. Any
architecture gap is a hard stop/report under the fanout policy above.

Targeted proof is green across all 384 family encodings and all 65,536 first words,
and exercises q boundaries `-16,-1,0,1,15`, guard-bearing full-40 vectors, every
source/destination combination, fixed-bit collision rejection, the S==D
unknownness rule, canonical/unknown transactional rejection, C and all-status
preservation, exactly one attempt/retirement with PC+1, and PC `0xffff`
rollback. Repeatability remains `UNRESOLVED`: Table 2-28 (printed pp. 2-19–
2-21) does not positively authorize these register forms, and no summary cycle
field is timing evidence. GNU binutils material, if consulted, is
nonnormative corroboration only. The concrete outcome is 13 catalog rows, 1077
populated first words, 384 new logical encodings, and four of 13 rows in the
reset-runnable workload. Existing layout, input, effect, and CPU seams only were
used; `cpu.c`, public API, architectural state, memory, RPT/IRQ, and timing are
unchanged. Canonical 40-bit carrier validation remains distinct from raw input
knownness, and C plus all other status fields are preserved.

### P2-W28: TC-aware status foundation and ROLTC/SFTC vertical slice (planned)

State: both stages are implemented; P2-W28A is fully green locally and remotely,
and P2-W28B targeted, independent review, and full local checks are green. The
work piece is intentionally two-stage because
the opcode forms expose a shared TC status input/output seam that the earlier
C-only accumulator effect could not represent. The first stage is the smallest
demand-driven generalization; the second stage is the opcode proof that consumes
it. No speculative overflow, memory, interrupt, repeat, parallel-issue, device,
public-API, or timing surface is admitted.

#### P2-W28A — shared TC/status foundation (implemented; local and remote green)

The state/status owner added private typed TC read/write accessors with explicit
knownness, backed by the existing ST0 TC bit. The input path added a distinct
`STATUS_TC` resource and a composite `ACCUMULATORS_STATUS_TC` resource, copies
`status_tc/status_tc_known` into `op_input`, and collects TC only when requested.
The accumulator-sequencer owner replaced the C-only output shape with a
value-owned status delta containing independent C and TC actions:
`PRESERVE`, `WRITE_0`, or `WRITE_1`. Existing MAX/MIN, ROL/ROR, SFTL, CMPL, and
logical handlers migrated to that shape without changing their established
semantics. The accumulator transaction validates the candidate accumulator and
the complete status delta before committing both atomically; rejected effects
leave all copied state unchanged.

Closeout: the new seams are private APIs only. The C+TC delta is value-owned,
all prior accumulator handlers are migrated, and `src/cpu/cpu.c`, the public
API, memory, RPT/IRQ, timing, and OVA/OVB behavior are unchanged. Local targeted
tests and the full local `make test` gate are green; remote CI is green at
[run 34058489923](https://github.com/djr-747/ti-c54x/actions/runs/34058489923).

Ownership is explicit and mirrors the tree: `src/state/status/` owns TC access;
`src/isa/catalog/` owns the resource-mask identity; `src/isa/ops/op_input.h`
owns the copied snapshot fields; `src/cpu/input/` owns collection;
`src/isa/ops/op_effect.h` owns the value-only delta type; and
`src/sequencer/accumulator/` owns its validation and atomic application.
Existing family handlers remain in their family subtrees, and
`src/cpu/cpu.c` remains generic orchestration only. The RED tests
are exactly `tests/unit/state/status/test_tc_field.c`,
`tests/unit/cpu/input/test_collect_tc.c`, and
`tests/unit/sequencer/accumulator/test_accumulator_status_delta.c`. Phase A
GREEN also requires the existing accumulator-family tests to pass after
migration. No public header, new effect kind, CPU opcode branch, global state,
allocation, observer, environment read, or hot-path diagnostic is allowed.

#### P2-W28B — ROLTC/SFTC vertical slice

State: implemented; targeted checks, independent review, and the full local
`make test` gate are green.

P2-W28A was GREEN before the rotate and shift family lanes implemented pure
handlers and family-local tests in parallel. Catalog/decode metadata and the
generated lookup were RED-first; production registration and all shared
integration remained serialized by the orchestrator. Both forms are one program
word and use the existing accumulator-source layout. The only reported shared
tooling gap was the catalog schema's missing TC resource-token allowlist entries;
that owner was extended centrally under a focused RED/GREEN schema test, without
changing CPU or execution architecture.

TI `SPRU172C` §4 ROLTC (printed p. 4-144) gives value `0xF492`, mask `0xFEFF`:
the selected accumulator's pre-TC enters bit 0, low32 bits 30:0 move to
bits 31:1, old bit 31 becomes C, guard bits 39:32 clear, and TC is preserved.
The selected accumulator and TC must therefore be known; C is written through
the status delta and TC is preserved. TI §4 SFTC (printed p. 4-157) gives value
`0xF494`, mask `0xFEFF`: if low32 is zero, the accumulator is unchanged and
TC becomes 1; otherwise, when bit31 xor bit30 is zero, low32 shifts left one
with guards preserved and TC becomes 0; when that predicate is nonzero, the
accumulator is unchanged and TC becomes 1. SFTC preserves C and does not read
pre-TC. Guard-only and low32-zero combinations remain explicitly unresolved
where the source does not distinguish them; no inference from summary fields is
permitted.

The catalog result is 15 rows and 1081 populated first-word encodings
(two additional rows with two encodings each), while reset-runnable coverage is
four of 15. Required ownership tests are family-local ROLTC and SFTC handler
tests under `tests/unit/isa/ops/rotate/` and `tests/unit/isa/ops/shift/`,
catalog/decode metadata tests under their owning subtrees, and CPU retirement
proofs under `tests/unit/cpu/`. Repeatability for both forms is unresolved;
summary cycle fields are not timing evidence. If either lane needs a new layout,
status resource beyond P2-W28A, opcode-specific CPU logic, memory, RPT/IRQ,
parallel issue, timing, or public API, it reports a hard GAP with the evidence
locator, missing contract, owner, tests, and hot-path consequence before any
shared edit. Neither family lane reported such an architecture gap; targeted
opcode/CPU tests, catalog quality checks, independent review, and the full
local gate are green.

### Hosted performance history (implemented; local and remote gates green)

Performance capture is hosted tooling, not a standalone-core dependency and not
architectural or silicon timing evidence. The observer-off benchmark measures
end-to-end CPU execution through the public program bus with a monotonic host
clock. `nop`, `ld-dp`, `ld-asm`, and `ld-arp` are stable fixed scenarios;
`mixed-current` follows the production catalog and records its catalog count and
semantic hash. Each of seven recorded samples is preceded by 16 excluded warmup
windows and uses repeated measurement windows with fewer than 65,535 retirements.
Exact compiler/platform/CPU,
build flags, clock, source commit, catalog count, scenario semantic hash, and
benchmark/source hashes are recorded.

History is clean-commit, append-only JSONL. Same-cohort comparison is advisory
only and requires matching CPU/platform/machine/compiler/build flags/clock;
fixed-scenario percentage comparison additionally requires the same scenario
semantic hash. `mixed-current` may change catalog count/hash and is reported as
an evolving, non-like-for-like trend rather than a regression percentage. No flaky
CI throughput threshold is introduced. The hosted surface is `make bench`,
`make bench-smoke`, `make bench-record`, `make bench-compare`, and
capture/compare helpers under
`tools/perf/`. The `TI_C54X_BUILD_BENCHMARKS` ACTIVE TEST/KEEP knob is
default-off, owned by `bench/`, and has no architectural effect; these controls
remain outside core execution. The full local gate is green (Python, C unit,
minimal-consumer, knob, source-boundary, and registry checks), and the remote
GCC, Clang, CMake, install/consumer, sanitizer, and benchmark-smoke gates are
green. The initial clean baseline at `aa0afc0` records 15.62 MIPS for NOP and
11.39 MIPS for `mixed-current`. The catalog-seven record at `7cad341` measures
69.057 ns/14.481 MIPS for NOP, 99.866 ns/10.013 MIPS for `ld-dp`, 100.477
ns/9.953 MIPS for `ld-asm`, 96.845 ns/10.326 MIPS for `ld-arp`, and 89.591
ns/11.162 MIPS for `mixed-current` with reset-runnable coverage of four of
seven catalog rows. Its fixed-scenario deltas versus catalog-six range from
-1.16% to +5.37%, while within-capture spreads range from 7.64% to 63.19%; each
scenario's spread remains retained with the historical record. These are hosted
trend measurements only. The catalog-nine record at `705ac7b` measures
61.839 ns/16.171 MIPS for NOP, 91.592 ns/10.918 MIPS for `ld-dp`, 91.619
ns/10.915 MIPS for `ld-asm`, 96.653 ns/10.346 MIPS for `ld-arp`, and 87.562
ns/11.420 MIPS for `mixed-current` with reset-runnable coverage of four of nine
catalog rows. Its same-cohort fixed-scenario deltas versus catalog-seven range
from -10.45% to -0.20%, with current spreads from 6.05% to 29.42%. The mixed
workload remains evolving and non-comparable because the catalog count changed.
These prior-cohort results establish neither an improvement nor a regression and
are not architectural or silicon timing evidence.

The latest clean catalog-ten record at `cd88596` measures 97.138 ns/10.295 MIPS
for NOP, 102.519 ns/9.754 MIPS for `ld-dp`, 97.918 ns/10.213 MIPS for `ld-asm`,
99.295 ns/10.071 MIPS for `ld-arp`, and 91.728 ns/10.902 MIPS for
`mixed-current` with reset-runnable coverage of four of ten catalog rows.
Relative to catalog-nine, the same-cohort fixed-scenario deltas are +57.08%,
+11.93%, +6.88%, and +2.73%; current spreads are 36.02%, 10.71%, 32.95%, and
10.72%, respectively. Wide host noise/spreads and the absence of fixed-scenario
semantic or CPU-orchestration changes do not establish a confirmed regression or
improvement. The mixed workload remains evolving and non-comparable, and these
hosted measurements are not DSP timing evidence.

The prior clean catalog-thirteen record at `ef87c94` measures 121.420 ns/8.236 MIPS
for NOP, 209.005 ns/4.785 MIPS for `ld-dp`, 202.901 ns/4.929 MIPS for
`ld-asm`, 113.815 ns/8.786 MIPS for `ld-arp`, and 96.359 ns/10.378 MIPS for
`mixed-current`, with reset-runnable coverage of four of 13 catalog rows. Its
fixed-scenario deltas versus catalog-ten are +25.00%, +103.87%, +107.22%, and
+14.62%; current spreads are 51.13%, 10.86%, 5.71%, and 117.11%. Fixed code
paths and CPU orchestration are unchanged; the large deltas and spreads do not
support a confirmed regression. CI is green at
<https://github.com/djr-747/ti-c54x/actions/runs/34055843312>.

The latest clean catalog-fifteen record at `7164ace` measures 109.060 ns/9.169
MIPS for NOP, 182.474 ns/5.480 MIPS for `ld-dp`, 194.054 ns/5.153 MIPS for
`ld-asm`, 111.847 ns/8.941 MIPS for `ld-arp`, and 88.796 ns/11.262 MIPS for
`mixed-current`, with reset-runnable coverage of four of 15 catalog rows. CI is
green at <https://github.com/djr-747/ti-c54x/actions/runs/34059815766>.
Relative to catalog-thirteen, the fixed-scenario deltas are -10.18%, -12.69%,
-4.36%, and -1.73%; current within-capture spreads are 50.31%, 3.83%, 6.90%,
and 112.44%. The mixed spread is 3.58%, but its changed catalog coverage and
semantic hash make it non-comparable. These hosted observations establish no
confirmed improvement or regression and make no DSP or silicon timing claim.

An immediate identical-binary diagnostic rerun (hash `144ffa...`; dirty only
because history was appended) normalized to NOP 66.571 ns/15.022 MIPS,
`ld-dp` 98.326/10.170, `ld-asm` 98.038/10.200, `ld-arp` 96.481/10.365, and
mixed 86.448/11.568. It corroborates host noise only, not a history baseline or
performance claim.

The clean catalog-sixteen record at `fedbd6a` measures 124.251 ns/8.048
MIPS for NOP, 205.652 ns/4.863 MIPS for `ld-dp`, 209.142 ns/4.781 MIPS for
`ld-asm`, 222.837 ns/4.488 MIPS for `ld-arp`, and 206.376 ns/4.846 MIPS for
`mixed-current`, with reset-runnable coverage of four of 16 catalog rows. CI is
green at <https://github.com/djr-747/ti-c54x/actions/runs/34062796872>.
Relative to catalog-fifteen, the comparable fixed-scenario deltas are +13.93%,
+12.70%, +7.78%, and +99.23%; current within-capture spreads are 22.39%, 0.92%,
4.17%, and 8.71%. `mixed-current` is evolving and non-comparable because the
catalog count changed.

A second capture of the identical `def3fe...` binary, with the tree dirty only
because the clean history record had been appended, normalized to NOP 68.561
ns/14.586 MIPS, `ld-dp` 101.587/9.844, `ld-asm` 104.687/9.552, `ld-arp`
106.211/9.415, and mixed 96.438/10.369. Those same-binary changes of -44.82% to
-53.27% versus the clean record are much larger than any code-level distinction
between the captures. The clean catalog-sixteen deltas therefore remain hosted
noise/trend observations and do not establish a confirmed implementation,
architectural, or silicon regression.

The prior clean catalog-twenty-two record at `9c22a33` measured 131.883 ns / 7.582
MIPS (`nop`), 233.294 / 4.286 (`ld-dp`), 240.304 / 4.161 (`ld-asm`), 206.770 /
4.836 (`ld-arp`), and 109.273 / 9.151 (`mixed-current`), with reset-runnable
coverage of 4/22. The clean same-catalog-22 record at `66ef384` has binary hash
prefix `b0c7d039`, the same semantic hashes, and measures 111.770 / 8.947,
116.084 / 8.614, 117.836 / 8.486, 159.492 / 6.270, and 170.433 / 5.867 in
that scenario order. Relative to `9c22a33`, median-cost deltas are -15.25%,
-50.24%, -50.96%, -22.87%, and +55.97%; current spreads are 42.40%, 29.18%,
5.14%, 49.67%, and 48.57%. An identical-binary diagnostic measured 76.978,
119.232, 158.135, 173.612, and 108.285 ns. The unchanged catalog/workloads,
recorded spreads, and same-binary variation establish no confirmed regression or
improvement. The architectural change adds canonical state ownership and one
additional normally-not-taken SXM resource-mask check in the generic collector;
it is not DSP timing.
Remote CI is green at
<https://github.com/djr-747/ti-c54x/actions/runs/34069533137>.

The current `mixed-current` benchmark stream contains only one-word forms. Before
the first multiword instruction is added, its per-window retirement count and
program-word span must be derived together so the stream stays below the
unresolved 16-bit program-PC boundary; this hosted constraint does not claim a
core boundary policy.

## Delivery order

1. Resolve the repository boundary and evidence policy.
2. Establish the opaque lifecycle, bus contract, result/error model, and tests.
3. Prove fixed-width 40-bit numeric primitives without undefined C behavior.
4. Prove address generation and ordered parallel memory effects.
5. Add the declarative opcode inventory and mGBA-style generated dispatcher.
6. Add the central sequencer before implementing opcode families.
7. Prove the shared-memory transaction gate before any opcode family can mutate
   memory.
8. Implement opcode families as requirement-backed vertical slices.
9. Add timing and silicon devices only from pinned evidence.
10. Vendor a release into emulators through separate adapters.

After P2-W21C, speculative foundation expansion is frozen. Work shifts to
end-to-end ISA vertical slices; new infrastructure is introduced only when a
concrete opcode family proves it is required, with related opcodes batched where
evidence and ownership permit.

## Timing and execution-budget gate

Instruction count and machine-cycle cost are separate units and separate API
contracts. Early execution continues to use instruction budgets; a machine-cycle
budget API is not exposed until its common-core timing claims have pinned evidence
and executable tests.

- The eventual timed API uses explicit units. `ti_c54x_cpu_tick()` advances
  exactly one DSP machine cycle and never overshoots; a one-cycle instruction may
  start and retire in that call. `ti_c54x_cpu_run_cycles()` consumes no more than
  its machine-cycle budget, while `ti_c54x_cpu_step_instruction()` continues only
  until the next retirement or an explicit stop. Existing instruction-budget
  entry points are not silently redefined as cycle-budget entry points.
- A multi-cycle instruction is a private resumable sequencer transaction. Budget
  exhaustion leaves its decoded identity, phase, resource classification,
  prepared effects, and any pending bus transaction in the CPU instance; the
  next tick resumes that transaction instead of decoding or applying it again.
  Results distinguish cycles consumed, instructions started, instructions
  retired, in-flight progress, and the exact reason execution stopped.
- Architectural register, status, and PC effects are prepared by semantics and
  committed exactly once at retirement. Bus activity occurs only at an
  evidence-backed sequencer phase; a read result remains staged until retirement,
  while externally visible writes occur exactly once at the evidenced bus-
  completion point. Ending a budget mid-instruction must not expose a partial
  architectural commit or reissue a completed bus side effect.
- Opcode semantic handlers produce architectural effects. They do not increment a
  global cycle counter or embed ad hoc numeric costs.
- The generated opcode inventory provides a named timing/resource classification
  seam alongside decode metadata. Exact values remain absent until evidenced; an
  unknown classification is explicit and cannot silently default to one cycle.
- `src/sequencer/` owns common CPU pipeline, branch/delay, repeat, and resource-
  conflict accounting. Timing that depends on a sequence is tested as a sequence,
  not reduced to a misleading per-opcode scalar.
- `src/bus/` and `src/devices/` profile owners add documented memory, peripheral,
  and wait-state costs. Device costs never leak into the common target.
- A future timed result reports machine cycles with an explicitly named unit.
  Instruction-budget APIs remain available for deterministic semantic execution;
  neither budget is presented as the other.
- An instruction whose timing classification is unknown does not enter the timed
  transaction and never defaults to one cycle. The timed API returns an explicit
  unknown-timing stop, with no instruction side effect; an already in-flight
  transaction can only report states admitted by the contract under which it
  started.
- Per-instance cumulative instruction and machine-cycle counters are `uint64_t`
  and use checked increments. Exhaustion returns an explicit sticky counter-
  overflow stop before beginning another cycle; neither cumulative values nor
  per-call deltas silently wrap.
- A future timed bus profile must support an allocation-free begin/poll/complete
  transaction (or an equivalent explicit READY/WAIT protocol), because the
  current synchronous callback cannot represent wait cycles without overshoot.
  Retry, completion, and fault paths must prove that reads and writes are issued
  and committed at most once.
- Interrupt sampling is a named sequencer boundary but remains unresolved until
  primary interrupt/pipeline evidence pins it. The core must not assume sampling
  on every tick or only at retirement by convenience.
- ARM-to-DSP scheduling belongs solely to an emulator adapter. It uses a reduced
  rational ratio and integer phase accumulator to decide how many exact DSP
  cycles to request per ARM tick; the standalone core never receives an ARM tick,
  host-clock ratio, PLL model, or fractional cycle.
- Timing metadata is static/generated and allocation-free. Runtime accounting uses
  compact typed fields or resource flags passed through the sequencer, with no
  environment lookup, logging hook, string dispatch, or mutable process global in
  the execution path.
- Each base-cost, pipeline-conflict, repeat, branch, bus, and wait-state behavior
  needs a requirement, narrow primary evidence, and direct or sequence-level test.
  Performance benchmarks may measure implementation overhead but cannot substitute
  for architectural timing evidence.

Exit condition: every implemented instruction form has either an evidenced timing
classification or an explicit unknown marker; the sequencer and device profile
compose costs without double counting; instruction and machine-cycle budgets are
provably distinct; one-cycle budgets never overshoot and can resume an in-flight
instruction; retirement and bus effects occur exactly once; counters cannot wrap;
the external rational scheduler introduces no fractional core state; and no
handler-local magic cycle increment exists.

The first architecture target is a device-neutral common C54x core. A concrete
silicon profile is added separately after common CPU semantics are stable; no
device memory map, peripheral, vector table, XPC extension, or timing promise may
leak into the common target without profile-specific evidence and tests.

## Shared-memory transaction and byte-order gate

The CPU core is word-native and host-endian-neutral. It never owns, aliases, or
casts a caller's byte-backed RAM. The caller owns mapped storage and arbitration;
the core exchanges fixed-width architectural values through an explicit bus
transaction boundary.

- Every bus transaction binds a program, data, or I/O word request to a unique
  transaction identity and explicit lifecycle state. A later sequencer owner binds
  that transaction to an evidenced instruction or memory-effect phase. The
  primitive request identifies its architectural word address and read or write
  access; its type fixes WORD16 width, while multiword composition remains explicit.
  A C54x CPU instance does not contain an ARM identity, shared-RAM pointer, device
  map, or host mutex.
- One architectural 16-bit word is the primitive transfer. Multiword values are
  composed from separately named word transfers in evidenced architectural word
  order; they are never treated as an accidentally atomic host `uint32_t`.
- Byte-oriented adapters use explicit load/store conversion helpers. Program-
  image encoding, physical shared-RAM byte-lane order, and snapshot/wire format
  are three separate policies with separate owners and tests. None may inherit
  the compiler host's byte order through a pointer cast, packed struct, union
  pun, or native `memcpy` interpretation.
- The timed bus seam begins one transaction, polls an owned pending token through
  READY/WAIT/FAULT states, and completes it exactly once. Budget exhaustion,
  wait-state polling, or retry must not reissue an MMIO read, duplicate a write,
  or expose half of a multiword transfer.
- Opcode handlers declare value-owned memory effects; they never call memory
  directly. For parallel operations the complete read/write set is available to
  the sequencer before issue, so overlap, read-before-write, resource conflicts,
  and commit order are resolved once from evidence.
- Architectural register/status/PC changes remain staged around memory progress.
  A fault or unsupported transaction returns an explicit outcome under a tested
  rollback/partial-visibility contract; it never relies on incidental callback
  order.
- DSP CPU, ARM CPU, DMA, and device masters are scheduled by the external bus or
  emulator adapter. That owner supplies deterministic arbitration and visibility
  boundaries. Host thread timing is never emulated hardware ordering; shared
  backing storage must be serialized or synchronized by the adapter.
- The common test bus proves request shape, wait/resume, fault propagation,
  exactly-once issue/completion, word order, byte-lane conversion, overlapping
  parallel accesses, and two independently scheduled masters. Profile tests then
  pin device-specific maps, lanes, arbitration, and wait states.

Exit condition: no instruction capable of writing memory is admitted until its
handler emits a declarative memory effect, the sequencer and bus transaction tests
prove exactly-once ordered visibility across suspension, every byte boundary has
an explicit conversion policy, and the device-neutral core remains free of raw
shared-memory pointers and platform arbitration.

## Work-piece identifiers and ledger

Every implementation work piece receives a stable identifier formed from its phase
and sequence, for example `P1-W1`. The identifier appears in the implementation
commit message and its subsequent entry in `docs/development-ledger.md`.

The plan status table is updated when the intended state changes. The ledger is
append-only evidence: it records the actual commit, affected ownership modules,
tests, decisions, registered-knob changes, and remaining work. If an old entry is
wrong, add a correction that names it rather than editing history.

## Isolation gate

The repository must be clonable and testable without an emulator checkout. Firmware
and DSP images are optional external test inputs and are never compiled into the
library. Tests that use them must skip cleanly when no image path is supplied.

No emulator build file or source file will be changed during standalone development.

## Ownership and module-shape gate

Ownership must be visible in both the architecture and the files:

- Every handwritten source and header begins with a short module contract stating
  what the module owns and what it explicitly does not own.
- Public declarations document whether inputs are copied, borrowed, or caller-owned,
  including the lifetime required for callback contexts, memory windows, trace
  sinks, CPU storage, and serialization buffers.
- The CPU owns architectural and sequencer state. The caller owns CPU storage, bus
  context, mapped memory, trace context, and external image data unless a specific
  API says otherwise.
- `cpu.c` owns instruction-lifecycle orchestration only. It must not accumulate ALU,
  address-generation, opcode-family, device, serialization, or platform behavior.
- Arithmetic, status, addressing, bus mapping, decode, sequencing, on-chip devices,
  tracing, serialization, and each opcode family remain independently testable
  modules with narrow internal interfaces.
- Handwritten files are reviewed for splitting as they approach 500 lines. Exceeding
  800 lines requires an explicit recorded justification. Generated opcode/table
  output is exempt but kept in a dedicated generated directory.
- No module reaches through another module's private state. Cross-module mutation
  occurs through named internal interfaces or explicit execution-effect objects.

Exit condition: the architecture review can assign every mutable field, callback,
and side effect to exactly one owning module, and the source tree contains no general
“miscellaneous” or growing all-purpose execution file.

### Required ownership hierarchy

The source and test trees mirror each other:

```text
include/ti_c54x/             stable public contracts
  address.h                  scalar program/data/I/O word-address carriers
  cpu.h
  bus.h
  state.h
  trace.h

src/
  cpu/                       instance lifecycle and top-level orchestration
  foundation/                fixed-width arithmetic and status primitives
  bus/                       bus routing and mapped-window fast paths
  address/                   effective-address plans and AR side effects
  isa/
    catalog/                 sole declarative X-macro opcode inventory and generated views
    decode/                  opcode metadata, operand decoding, disassembly
    ops/                     one directory/file group per semantic family
  sequencer/                 retire, PC/XPC, delay, repeat, XC, IRQ, IDLE
  devices/                   documented optional on-chip peripherals by device
  state/                     architectural serialization and state views
  debug/                     optional structured tracing and breakpoints

generated/                   reproducible opcode/table output only

tests/
  support/                   dependency-free test harness and shared generators
  unit/                      mirrors each src ownership directory
  contract/                  public API, ABI, and standalone-consumer tests
  differential/             normalized external-oracle transcripts
  fuzz/                      bounded decoder, arithmetic, and sequencer fuzzers
```

Rules:

- Public headers express stable contracts, never private layouts.
- A module's private header stays beside its implementation, not in a general
  `src/internal/` dumping ground.
- Tests for `src/<owner>/` live under `tests/unit/<owner>/`; cross-module behavior
  belongs under `tests/contract/`.
- Cross-module headers are introduced only for a named architectural seam and list
  both producer and consumer ownership.
- Generated declarations and dispatch tables are never hand-edited or mixed into an
  opcode semantic source file.
- Root build files own toolchain policy, global options, aggregate targets,
  packaging, and inclusion order only. Source lists and unit-test registrations
  live in small fragments or `CMakeLists.txt` files under the same `src/<owner>/`
  and `tests/unit/<owner>/` directories as the code they register. Shared test-
  target construction may live in one `tests/cmake/` helper, but it must not
  become a second flat target inventory.
- Both direct Make and CMake consume that ownership hierarchy and must continue
  producing the same library, observer-disabled artifact, test executables, CTest
  names, install package, and consumer surface. A manifest split is structural;
  it does not justify changing compile options, target behavior, or test scope.

## Diagnostic isolation gate

Diagnostics are optional observers of architectural events, not opcode behavior.

- Production library code never calls `getenv`, `setenv`, or equivalent process
  configuration APIs. A hosted executable may read its environment once before
  `ti_c54x_cpu_init` and convert it to explicit configuration.
- Enabled observation classes are stored as one copied, per-instance `uint64_t`
  event mask in cold/debug state. No mutable process-global flag or per-feature
  boolean is read from the execution path. `TEST` and `A/B` controls never share this
  mask because they may select behavior rather than merely observe it.
- Hot call sites use one internal macro with short-circuit semantics, conceptually:

  ```text
  if (TI_C54X_OBSERVE_ENABLED(cpu, EVENT_BIT) && event_specific_condition) { ... }
  ```

  When observation is disabled at build time, `TI_C54X_OBSERVE_ENABLED` is a constant
  false expression and the compiler removes the site and event-specific condition.
- The macro and mask are owned by `src/debug/`. No opcode-family module defines its
  own gate, reads diagnostic configuration, formats text, or invokes host logging.
- Opcode handlers return architectural execution effects. The sequencer/debug seam
  may convert completed effects into compact structured events after retirement.
- The core never owns output files, streams, sockets, format strings, or logging
  policy. A caller-owned sink and context receive structured events only.
- Observer configuration is non-architectural cold state: it is not included in CPU
  snapshots, does not survive reinitialization unless the caller supplies it again,
  and cannot be changed from inside an observer callback.
- Observer callbacks receive immutable event values and no implicit CPU pointer.
  Re-entering or mutating a running CPU from a callback is forbidden and tested.
- A repository structural test scans `include/` and `src/` and rejects environment
  APIs, stdio/file APIs, mutable diagnostic globals, platform names, or diagnostics
  included directly by opcode semantic modules.

Exit condition: diagnostic-off builds contain no diagnostic branches or callback
symbols in the instruction hot path, diagnostic-on builds read a single instance
mask, and the structural boundary test passes.

## Knob ownership and lifecycle gate

Non-architectural switches are exceptional and centrally registered in
`docs/knobs.md`. A knob includes runtime configuration, a build option, a test hook,
an observation bit, or an implementation-selection switch. It may not be introduced
as an anonymous environment variable, preprocessor branch, static boolean, or magic
configuration field.

Each knob has two independent labels:

- **Class `TEST`:** builds or enables tests, examples, sanitizers, validation, or
  controlled fault/input injection. Behavioral test hooks are unavailable in normal
  production builds and cannot falsify an architectural result.
- **Class `OBSERVE`:** records structured facts without changing architectural
  state, memory effects, timing, interrupts, or scheduling.
- **Class `A/B`:** selects two explicit implementations or models for comparison.
  Both paths share the same contract and tests; an A/B knob cannot hide unresolved
  ISA semantics in a stable release.

And one lifecycle disposition:

- **`KEEP`:** supported configuration with a stable owner and compatibility policy.
- **`TEMP`:** bounded experiment or migration aid with a named exit criterion. It
  cannot enter the stable public API.
- **`RETIRE`:** compatibility input awaiting removal; new call sites are forbidden,
  and the registry states the removal release/condition and replacement.

Lifecycle transitions are explicit: `TEMP` must become `KEEP` or `RETIRE`; `KEEP`
may become `RETIRE`; `RETIRE` is removed after its recorded compatibility condition.
Removal preserves the retired identifier/bit in a tombstone list for the current ABI
major so persisted host configuration cannot silently acquire a different meaning.

Every registry entry records identifier, class, disposition, owning module, default,
compile/runtime scope, architectural effect, test coverage, and exit/removal
criterion. The source-boundary audit rejects knobs absent from the registry and
rejects `TEST`/`TEMP` knobs in installed public headers.

The long-term source of truth is a small declarative registry under `config/`, from
which enum identifiers, allowed compile definitions, documentation checks, and the
retired-ID tombstone list are generated or verified. The Markdown registry remains
the human review surface. Direct `#ifdef` feature names and ad-hoc configuration
fields that are absent from that registry fail CI.

The 64-bit hot mask is reserved for `OBSERVE` event classes only. Its bit allocation
is stable within an ABI major, statically checked for uniqueness/exhaustion, copied
once during CPU initialization, and read through the single compile-time-aware
macro. `TEST` controls live only in test-build configuration; `A/B` controls use
typed cold configuration with an explicit comparison owner and never appear as an
anonymous hot-path bit.

Exit condition: every switch is discoverable from the registry, owned by exactly one
module, tested according to class, and has an explicit keep or removal decision.
