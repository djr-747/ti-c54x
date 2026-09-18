# ti-c54x implementation plan

Status: the standalone foundation baseline is established and ISA vertical slices
are the active track. P2-W31B1 now implements the transactional DATA endpoint and
persistent memory-instruction root. P2-W31P remains open with structural and
correctness gates green; optimized hosted cohorts are below the 60 ns median
target, while exact three-capture stability acceptance is pending/deferred.
That hosted follow-up did not block P2-W32, which is implementation-complete
with its local gates green. P2-W33 is implementation-complete with focused,
review, sanitizer, quality, and final full-gate validation green.
P2-W34 is the implemented evidence-fit slice for fixed-MOD2 LDU. W31P hosted
stability and W31B immediate-READY benchmark captures remain open and
nonblocking.
P2-W35 is implementation-complete as the evidence-fit fixed-MOD2 LD-to-T slice;
its catalog, decode, pure effect, and generic CPU contracts are implemented with
focused strict/sanitizer, quality, review, and full-gate validation green.
W31B2 catalog/decode metadata, pure generic MEMORY effects, CPU end-to-end
retirement, focused/full/sanitizer validation, and independent review are
complete; its immediate-READY benchmark follow-up remains pending.
P2-W36 is implementation-complete as the evidence-fit fixed-MOD2 immediate-store
slice; its catalog, decode, pure effect, and generic CPU contracts are green.
W31P hosted stability and W31B immediate-READY benchmark follow-ups remain open
and nonblocking. P2-W37 is implementation-complete as the evidence-fit
fixed-MOD2 memory-to-status slice for `LD Smem,DP` and `LD Smem,ASM`; its
repeatability boundary and generic MEMORY/status-field ownership are recorded
below. P2-W38 is now implementation-complete as the evidence-fit fixed-MOD2
`LD Smem,16,dst` slice; its exact SXM transform, shared load core, fixture
corrections, and validation evidence are recorded below.
P2-W39 is implementation-complete as the generic T-input prerequisite plus
bounded fixed-MOD2 `ST T,*ARx+` slice; its catalog, decode, pure effect, generic
CPU retirement, validation, and closure evidence are recorded below.
P2-W40 is implementation-complete as the authoritative ISA instruction-form
inventory and coverage-reporting contract. Its audit metadata is deliberately
separate from production decode: TI SPRU172C Chapter 2 Tables 2-1 through 2-26
establish a 205-form summary/source-universe checkpoint, while
common/profile/addressing scope remains explicit and unresolved; the current
catalog reports only the provisional 35/205 family progress and strict 25/205
complete-form lower bound.
P2-W41 is implementation-complete as the bounded fixed-MOD2
`LD Smem,TS,dst` vertical slice. Its catalog, decode, pure effect, and generic
CPU contracts are green with combined STATUS_SXM+T_REGISTER input, documented
signed TS transforms, and transactional rejection of the project-defined
unsupported count zone. No new resource bit, state, effect, layout, address,
CPU branch, public ABI, or timing seam was added.
P2-W42A is implementation-complete as the standard-indirect MOD0–3 memory
transaction seam; it deliberately leaves the catalog and opcode handlers
unchanged while the generic MEMORY effect, resolver, and root transaction
contracts are tightened.
P2-W42B is implementation-complete as the bounded fan-out of that seam across the ten existing
fixed-MOD2 Smem families; it replaces those rows with neutral MOD-bearing
forms without adding opcode identities.
P2-W43A is implementation-complete as the generic indexed WORD16 MEMORY seam
only; it adds no catalog row, opcode handler, or instruction-family claim. Its
three RED contracts are GREEN, covering the effect, resolver, and transaction
behaviors below. P2-W43B is implementation-complete as the twenty-row indexed
MOD5/MOD6 per-family expansion from the explicit primary composition audit.
The catalog projection is ID_COUNT=56 with 2,103 populated first-word lookup
entries and 55 catalog bindings; inventory family progress remains 35/205
(17.073%) and strict complete-form coverage remains 25/205 (12.195%).
P2-W44A is implementation-complete as the generic circular WORD16 MEMORY
seam; it adds no catalog rows or handlers. P2-W44B is implementation-complete
as the bounded twenty-row MOD8/MOD10 family fan-out: ID_COUNT is 76, the
populated first-word lookup has 2,343 entries, and there are 75 catalog
bindings. The six owner-local RED contracts and authoritative full suite are
GREEN.
P2-W45A is implementation-complete as the generic reverse-carry WORD16 MEMORY
seam, with no catalog rows, handlers, or CPU opcode branch. P2-W45B is
implementation-complete as the evidence-first reverse-carry MOD4/MOD7
per-family composition; its twenty rows produce ID_COUNT=96, lookup 2,583, and
95 bindings, with focused, quality, sanitizer, review, and full-gate validation
green.

P2-W46A is implementation-complete as the private allocation-free WORD16
same-address read-modify-write seam inside the existing MEMORY effect carrier.
P2-W46B is implementation-complete: the three neutral MOD0–3
ANDM/ORM/XORM rows are composed through generic catalog, decode, effects, CPU,
and inventory seams. Its dedicated observer-off benchmark executable, capture,
comparator, and quality contract, clean baseline artifact, and baseline-relative
comparison are green. The baseline is recorded at
`bench/audits/p2-w46b-rmw-baseline.json` with semantic hash
`3b39187c40a4fd43`; performance remains a first-class acceptance axis without
implying TI timing. P2-W47B1 is implementation-complete for the generic
LONG32 MEMORY MOD13 carrier and root seam. P2-W47B2A completes the typed
PMST.SST state/input prerequisite without widening the copied input snapshot.
P2-W47B2B completes the DLD/DST catalog, checked/trusted semantics, and
inventory activation. P2-W47B2C completes CPU lifecycle coverage and the
observer-off full-CPU opcode benchmark; W47B is closed. P2-W48A implements the
newly evidenced observable RC lifecycle as a typed, transactional repeat-progress
seam. P2-W48B implements the post-decode maskable-request IFR latch/defer/take
lifecycle and documented alternate clear paths. Neither slice changes the current
CPU hot path. P2-W48C corrects the unsupported DLD repeatability classification
to `UNRESOLVED`; no catalog row is yet positively evidenced `ELIGIBLE`. Runtime
RPT activation still waits on single-repeat PC flow, predecode interrupt
arbitration, and positive body-eligibility evidence. P2-W49A is
implementation-complete as the shared bit-32 ADD/SUB arithmetic, OVM input,
typed OVA/OVB/OVM status, and atomic sticky-overflow effect foundation. W49B
owns register-only ADD/SUB activation and its public-CPU performance proof.
P2-W50A and P2-W50B are implementation-complete: the dual-WORD16 MOD0
transaction seam is now exercised by evidenced dual-memory ADD/SUB forms. The
current projection is 120 catalog rows, 3,543 populated first-word lookup
entries, 120 bindings, 44/205 family coverage, and 27/205 strict-complete
coverage. W50B's hosted benchmark is a characterization and optimization target;
it records no absolute host threshold or TI machine-cycle claim.
P2-W51A is now implementation-complete as the durable dual-indirect planning
checkpoint: modes 0, 1, and 2 resolve from direct AR2–AR5 selectors with
same-AR Xmod precedence, while the requests-only persistent snapshot remains
32 bytes. Root size, CPU wiring, catalog projection, and opcode activation are
unchanged; W51B owns atomic commit/publication before any broader activation.
P2-W51B is implementation-complete as the dual-memory root commit/publication
seam: the persistent snapshot carries both value-owned commit descriptors,
shadow validation runs before issue and at every resume boundary, and address
updates publish atomically with the ordinary effect and PC. Root sizes remain
frozen at 336/308; broader dual-mode opcode activation is the next seam.
P2-W51C is implementation-complete as the dual ADD/SUB MOD0–MOD2 activation:
sixteen non-MOD0/MOD0 syntax-6 rows decode, stage, and retire through the
W51B root with atomic address publication. Catalog, lookup, binding, family,
and strict-complete coverage are 120, 3,543, 120, 44/205, and 27/205; MOD3
neighborhoods stay undecoded and the MOD0 benchmark roster stays frozen.

### P3: 5110 DSP-image execution focus (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for
re-sequencing remaining work by what the 5110 DSP image needs. The
append-only reconstruction hypotheses, black-box observations, and downstream
speech acceptance boundary are maintained outside this repository.

The input is a mnemonic census (counts only, no sibling source, tables,
comments, or structure copied) over the 21 big-endian disassembly
blocks in the sibling `re/dsp-5110` set (`block*.asm`, 36,202 static
instruction words), joined against `config/isa/inventory.json` on the
normalized uppercase mnemonic on 2026-09-10. Headline: 21,850 static
words (60.4%) use mnemonics with no COMPLETE/SUBSET form, and 14,352
(39.6%) use mnemonics with at least one covered form. Mnemonic-level
coverage is an upper bound only: a covered mnemonic (LD, ST, ADD,
SUB) may still use exact syntaxes with no implemented form, and
static words include data disassembled as code, so dynamic
reachability remains unproven.

Priority waves by static count (numbers are census facts, not
implementation targets):

1. Control flow (~6,500 words): BC 1,754, CALL 1,278, RET 763, XC
   523, B 499, CC 74, BANZ 74, BACC 14, CALA 7, RETE 7, FB 4, FCALL
   3, RETF 1; plus delayed-slot variants with no inventory form at
   all: CALLD 707, BCD 303, BD 222, RETD 174, BANZD 66, CCD 29,
   RCD 20, FBD/CALAD/BACCD/RETED (few each).
2. Immediate/status stores (~5,500): STM 4,260, STH 780, STLM 487.
3. Repeat machinery (~1,500): RPT 653, RPTBD 378 (no form), RC 251,
   RPTB 177, RPTZ 36. Runtime RPT activation still waits on the
   recorded P2-W48C prerequisites (single-repeat PC flow, predecode
   interrupt arbitration, positive body-eligibility evidence).
4. MAC/MPY data path (~1,600): MAC 515, MPY 301, MPYA 188, MAS 146,
   MACR 87, MACA 64, SQUR/SQURA/SQDST/SQURS ~140, MPYR 51, MPYU 34,
   DELAY 24, small counts down to FIRS 1. LTD/MACD keep their
   recorded delay-seam dependency.
5. MMR/stack moves (~2,600): LDM 232, MVMM 288, MVDK 207, MVDM 205,
   MVKD 204, MVMD 139, MVPD 19, MVDP 5, POPM 447, PSHM 430, PSHD 24,
   POPD 18, MVDD 408. LDM/ST MMR keep the recorded device-seam
   dependency.
6. Bit/field ops (~1,200): BITF 714, CMPM 367, CMPS 40, BITT 23,
   BIT 21, SACCD 39, SRCCD 13, STRCD 8.
7. Shifts and misc (~2,300): SFTA 553, NORM 119, SAT 36, NEG 138,
   ABS 82, MAR 473, ADD/SUB satellites (ADDM 186, DADD 134, ADDS 73,
   DSUB 49, DADST 48, SUBC 39, DSADT 34, ADDC 33, SUBS 31, SUBB 26,
   DRSUB 21, DSUBT 7, ABDST 4), FRAME 26, READA 47, WRITA 46, IDLE
   12, TRAP 73, INTR 2.
8. Port I/O (~500, new seam): PORTW 322, PORTR 186. No port seam
   exists yet; it is admitted as future work with no timing claim.

Already covered at mnemonic level (14,352 words): LD 4,078, SUB
1,675, ADD 1,402, STL 1,367, ST 1,308, NOP 880, AND 789, ANDM 389,
ORM 372, OR 357, DST 254, SSBX 200, DLD 188, ROL 187, LDU 186, RSBX
178, SFTL 148, XOR 136, MAX 65, MIN 55, EXP 51, LDR 24, ROR 22, plus
small counts (XORM, CMPL, ROLTC, CMPR).

Structural findings admitted by this focus. The delayed-branch
family (~1,900 words) is real TI syntax (CALLD/RPTBD appear in the
vendored SPRU172C text, e.g. §4 CALL and RPTBD sections) but has no
inventory form, so the 205-form denominator needs a scope audit for
delayed variants before it can measure branch readiness. The image
`.bin` bytes are not ingested as test vectors: permitted use of DSP
images stays open under the P0-W4 owner/legal follow-up, and
emulator integration stays deferred per repository policy. Slice
order follows waves 1–8 above; remaining LD-family MOD variants
yield to wave-1 control flow.

### P3 slice order (approved 2026-09-16)

This heading records the approved continuation order for the P3
5110-DSP-image track. Slice 0 lands or parks the tree state that
predates approval; each later slice keeps the standing gates (TDD
RED-first, evidence plus requirement plus executable test, atomic
commit followed by a ledger-only commit). If a slice's TI evidence
is missing it stays `UNRESOLVED` and the next evidenced slice moves
up; order flexes, gates do not. Coverage is measured only by
`config/isa/inventory.json` bindings (family x/205, strict-complete
y/205); populated-first-word lookup counts are never ISA coverage.

| # | Slice | Census wave | Exit |
|---|---|---|---|
| 0 | Land or park in-flight `RETF` plus RTN register | 1 tail | Clean tree via atomic commit plus ledger commit, or documented park plus revert |
| 1 | 205-denominator delayed-variant scope audit | cross-cutting | Documented denominator decision for branch-readiness claims |
| 2 | `XC` conditional execution | 1 tail | Catalog, decode, effect, CPU, inventory binding |
| 3 | Far branches `FB`/`FCALL` (plus `FBD`/`FRET` if evidenced) | 1 tail | Same four-contract shape |
| 4 | Accumulator-addressed `BACC`/`CALA` (plus `BACCD`/`CALAD` if evidenced) | 1 tail | Same four-contract shape |
| 5 | `RC`/`RCD` conditional returns | 1 tail | Same shape over the delayed-return seam |
| 6 | MMR/device routing seam, no catalog rows | prerequisite | Sealed seam with RED contracts, W43A pattern |
| 7 | `STM` MMR store (4,260 static words) | 2 | Four contracts over the slice-6 seam |
| 8 | `STH` plus `STLM` status stores | 2 | Same shape |
| 9 | RPT activation: single-repeat PC flow, predecode IRQ arbitration, body eligibility | 3 prerequisite | Closes the P2-W48C items; `RPT`/`RPTB`/`RPTZ` follow, `RPTBD` after delayed-branch composition |
| 10 | MAC/MPY datapath seam plus `MAC`/`MPY` first | 4 | Seam plus first families; `MPYA`/`MAS`/`MACR` follow |
| 11 | MMR/stack moves in static-count order | 5 | Family slices over the slice-6 seam |
| 12 | Bit/field ops, shifts/misc, port seam | 6–8 | Closes the census tail; ports get a seam-first treatment |

### P3 denominator audit (closed 2026-09-16)

Tables 2-12 through 2-16 hold 24 family forms and no
delayed-variant row (Table 2-12, for example, is exactly
B/BACC/BANZ/BC/FB/FBACC), so the 205-form denominator needs
no expansion for delayed variants: the seven implemented
delayed rows (`BD`, `BCD`, `BANZD`, `CALLD`, `CCD`, `RETD`,
`RETED`) already share their family form through a `_DELAYED`
addressing-subset token, and every future delayed row
(`RPTBD` under `FORM-T2-16-04`, `FBD` under `FORM-T2-12-05`,
`RETFD` under `FORM-T2-15-06`, `BACCD` under `FORM-T2-12-02`,
`CALAD` under `FORM-T2-13-01`, `RCD` under `FORM-T2-15-03`)
composes the same way. The contract
`test_inventory_delayed_family_binding.py` locks this: any
delayed-only form ID fails the gate before it can move the
denominator. Branch readiness is therefore measurable as
family coverage over the 24 branch/repeat rows, currently
8/24 `SUBSET` (`B`, `BANZ`, `BC`, `CALL`, `CC`, `RET`,
`RETE`, `RETF`).

Correction to the 2026-09-11 wave-1 audit entry: `RC`
(`FORM-T2-15-03`, Table 2-15) is the conditional-return
instruction, not a repeat-counter reference — it parallels
the `BC`/`BCD` branch and `CC`/`CCD` call pairs with its
delayed companion `RCD`. The census count of 251 `RC` words
moves from wave 3 (repeat machinery) to the wave-1 tail, and
slice 5 above covers `RC`/`RCD` together. `XC`
(`FORM-T2-18-07`) stays wave-1 tail by control-flow
ownership, not by table neighborhood.

### P3 seam-first refocus (2026-09-16)

The 5110 ISA program is re-sequenced seam-first: each
architectural seam below is the forcing function, and
mnemonic slices compose on it. Slices stay one family per
commit, but a seam lands before its first family. Order
follows 5110 static weight behind each seam:

| Seam | Unlocks (static words) | Status |
|---|---|---|
| S1 MMR/device routing (MMR file, ST/LD MMR routing, knownness) | `STM` 4,260 + MMR/stack moves ~1,400 + `STH`/`STLM` 1,267 | next after W77B/W78 |
| S2 RPT activation (single-repeat PC flow, predecode IRQ arbitration, body eligibility — the W48C prerequisites) | `RPT`/`RPTB`/`RPTBD`/`RPTZ` ~1,244 | landed `6cf9924` (2026-09-16) |
| S3 MAC/MPY datapath (accumulator x T, repeat interplay) | MAC family ~1,550 | seam `598cc61` + MAC row + HOLD tails landed (2026-09-16); MPY row next |
| S4 conditional execution (skip/NOP following words, uninterruptible window vs arbitration) | `XC` 523 + its follow-word coverage | after S3 |
| S5 accumulator-addressed branch target | `BACC`/`CALA` + delayed companions ~30 | with S1 or S4 |
| S6 port I/O seam | `PORTR`/`PORTW` 508 | last |

Arch completeness criterion: every 5110-census mnemonic
executes except far addressing (`FB`/`FCALL`/`FRET`/`FRETE`/`FCALA`
and delayed companions), which needs the XPC/far-memory
model the baseline explicitly keeps in later profiles. A
lane that hits a missing seam stops and opens the seam
slice; no family-local workaround is admitted.

- [P3-S1: MMR/device routing seam (implemented; local verification green)](plan/p3-s1-mmr-device-routing-seam.md)

- [P3-S7: STM immediate-MMR-store vertical slice (implemented; local verification green)](plan/p3-s7-stm-immediate-mmr-store-vertical-slice.md)

- [P3-S2: RPT activation seam (implemented; local verification green)](plan/p3-s2-rpt-activation-seam.md)

- [P3-S3: MAC/MPY datapath seam (implemented; local verification green)](plan/p3-s3-mac-mpy-datapath-seam.md)

- [P2-W38: fixed-MOD2 LD Smem,16 vertical slice (planned)](plan/p2-w38-fixed-mod2-ld-smem-16-vertical-slice.md)

- [P2-W39: generic T input and fixed-MOD2 ST T vertical slice (planned)](plan/p2-w39-generic-t-input-and-fixed-mod2-st-t-vertical-slice.md)

- [P2-W40: authoritative ISA instruction-form inventory (planned)](plan/p2-w40-authoritative-isa-instruction-form-inventory.md)

- [P2-W41: fixed-MOD2 LD Smem,TS,dst vertical slice (planned)](plan/p2-w41-fixed-mod2-ld-smem-ts-dst-vertical-slice.md)

- [P2-W42A: standard-indirect MOD0–3 memory transaction seam (planned)](plan/p2-w42a-standard-indirect-mod0-3-memory-transaction-seam.md)

- [P2-W42B: standard-indirect MOD0–3 Smem family expansion (planned)](plan/p2-w42b-standard-indirect-mod0-3-smem-family-expansion.md)

- [P2-W43A: generic indexed WORD16 MEMORY seam (planned)](plan/p2-w43a-generic-indexed-word16-memory-seam.md)

- [P2-W43B: indexed MOD5/MOD6 per-family composition (planned)](plan/p2-w43b-indexed-mod5-mod6-per-family-composition.md)

- [P2-W44A: generic circular WORD16 MEMORY seam (planned)](plan/p2-w44a-generic-circular-word16-memory-seam.md)

- [P2-W44B: circular MOD8/MOD10 per-family composition (planned)](plan/p2-w44b-circular-mod8-mod10-per-family-composition.md)

- [P2-W45A: generic reverse-carry WORD16 MEMORY seam (planned)](plan/p2-w45a-generic-reverse-carry-word16-memory-seam.md)

- [P2-W45B: reverse-carry MOD4/MOD7 per-family composition (planned)](plan/p2-w45b-reverse-carry-mod4-mod7-per-family-composition.md)

- [P2-W46A: generic WORD16 same-address read-modify-write seam (planned)](plan/p2-w46a-generic-word16-same-address-read-modify-write-seam.md)

- [P2-W46B: ANDM/ORM/XORM neutral MOD0–3 composition (planned)](plan/p2-w46b-andm-orm-xorm-neutral-mod0-3-composition.md)

- [P2-W47A: generic LONG32 pair transaction seam (planned)](plan/p2-w47a-generic-long32-pair-transaction-seam.md)

- [P2-W47B: DLD/DST LONG32 MOD13 composition (planned)](plan/p2-w47b-dld-dst-long32-mod13-composition.md)

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
| ISA instruction-form inventory (P2-W40) | Implemented — focused tests and checker green | Freeze the authoritative 205-form TI summary denominator and separate audit metadata from production decode; schema/count/catalog-binding/report contracts preserve explicit coverage and scope states, keep unresolved forms in the denominator, and report only the provisional 35/205 and 25/205 progress measures |
| P2-W41 fixed-MOD2 LD Smem,TS,dst vertical slice | Implemented — focused strict/sanitizer tests, quality 207, clean dual review, and full standalone gate green | Add the 0x1490/0xFEF8 fixed-MOD2 TS load through the existing ACCUMULATOR_ARF, SXM/T input, MEMORY, ACCUMULATOR_WRITE, address-resolution, and CPU retirement seams. Valid signed six-bit counts shift the SXM-widened WORD16 in canonical 40-bit space; project-unsupported 0x20..0x2f rejects transactionally before the data bus. Catalog-family progress is 35/205 (17.073%); strict complete-form coverage remains 25/205 (12.195%). |
| P2-W42A standard-indirect MOD0–MOD3 memory transaction seam | Implemented — focused CTest contracts, evidence checker, and diff validation green to date | Generalize the existing generic MEMORY effect, address resolver, and root memory-instruction transaction to MOD0–MOD3 without changing the catalog or opcode handlers. MOD3 read effects reject structurally before endpoint issue; MOD0 has no AR commit, MOD1/MOD2 are POST, and MOD3 writes are PRE metadata with commit-at-READY atomicity. |
| P2-W43B indexed MOD5/MOD6 per-family composition | Implemented — five focused CTests, inventory Python 3/3, and independent ASan/UBSan gate green | Register 20 indexed catalog rows (ID_COUNT 56; lookup 2,103), append the three private MOD4 layouts, and add seven load plus three store MOD5/MOD6 wrappers through the W43A indexed MEMORY seam. Keep 55 catalog bindings, 35/205 family coverage, 25/205 strict complete coverage, and no ABI/effect/address/state/CPU/timing expansion. |
| P2-W44A generic circular WORD16 MEMORY seam | Implemented — focused RED→GREEN C executable runners green; no catalog rows or handlers | Define typed CIRCULAR_WORD16 MOD8/MOD10 read/write effects, ordered CMPT→ARP→selected AR→BK resolution, existing circular planner reuse, BK=0 NONE/no-update, stale/unknown rejection, and atomic WAIT/READY/FAULT/cancel transactionality. Catalog remains ID_COUNT 56, lookup 2,103, and 55 bindings; coverage remains 35/205 family and 25/205 strict. W44B's 20-row fan-out remains projected at ID_COUNT 76, lookup 2,343, and 75 bindings. |
| P2-W44B circular MOD8/MOD10 per-family composition | Implemented — six focused C/Python runners GREEN; full suite pending | Register twenty bounded MOD8/MOD10 rows over the ten existing Smem families, reuse private MOD4 layouts and the W44A CIRCULAR_WORD16 seam, and preserve the combined inventory token and unchanged coverage. ID_COUNT is 76, lookup 2,343, and bindings 75; unsupported modes/boundaries remain excluded. |
| P2-W45A generic reverse-carry WORD16 MEMORY seam | Implemented — three focused C runners GREEN; full suite pending | Define private typed REVERSE_CARRY_WORD16 MOD4/MOD7 effect, ordered CMPT→ARP→selected AR→AR0 resolution, exact live snapshots, boundary rejection, and atomic WAIT/FAULT/cancel/READY transactionality. ARF is 1..7 only; no BK, root transaction, CPU/catalog/opcode/public ABI/timing/device/RPT changes. W45B remains projected at ID_COUNT 96, lookup 2,583, and 95 bindings. |
| P2-W45B reverse-carry MOD4/MOD7 per-family composition | Implemented — six focused C/Python runners GREEN, quality 215, fresh ASan/UBSan 5/5, clean review, and full standalone gate GREEN | Register twenty bounded MOD4/MOD7 rows over the ten existing Smem families, reuse MOD4 layouts and the W45A seam, reject ARF0 transactionally, and preserve the combined inventory token and unchanged coverage. ID_COUNT is 96, lookup 2,583, and bindings 95; unsupported alias/boundary scope remains excluded. |
| P2-W46A generic WORD16 same-address RMW seam | Implemented — focused transaction/effect and structural performance runners GREEN | Allocation-free AND/OR/XOR carrier and one-read/one-write transaction resolve once, use fresh phase IDs, validate direct stale snapshots without replanning, and publish AR/PC only after WRITE READY. Carrier size is frozen on supported 32/64-bit ABIs; no bus-exclusion or TI timing claim is made. |
| P2-W46B ANDM/ORM/XORM neutral MOD0–3 composition | Implemented — six focused C/Python runners, clean baseline, and baseline comparison GREEN | Register three two-word MOD0–3 rows (0x6880/0x6980/0x6A80, mask 0xFFE0), reject MOD3 before endpoint, and reuse W46A through generic decode/effects/CPU/inventory. ID_COUNT 99, lookup 2,679, bindings 98; family 38/205, strict 25/205. The observer-off benchmark is advisory, with clean baseline artifact `bench/audits/p2-w46b-rmw-baseline.json`, semantic hash `3b39187c40a4fd43`, and no TI timing claim. |
| P2-W47A generic LONG32 pair transaction seam | Implemented — focused shape/read/write CTests 3/3, hot-path quality 6/6, focused sanitizer 3/3, and authoritative full gate GREEN | Complete the generic sequencer-only LONG32 `{first=MSW,second=LSW}` reducer and validation split over the canonical private WORD16 transaction seam. Preserve deterministic MSW→LSW order, copied fixed-size state, fresh increasing IDs, WAIT/FAULT/cancel/read-publish/partial-write semantics, malformed-pair rejection, and allocation-free/no-replan hot-path checks. Native LP64 size is 32/88 bytes and supported ILP32 is 32/80; no catalog/opcode/CPU/public-ABI/effect-carrier change. W47B defers executable opcode benchmarking until evidence-backed DLD/DST composition. |
| P2-W47B1 generic LONG32 MEMORY MOD13 root | Implemented — focused and sanitizer matrices 10/10 GREEN | Add the fixed-size LONG32 MEMORY carrier, checked CMPT=0 MOD13 resolver, and discriminated root transaction over W47A. Freeze LP64 address/effect/root sizes at 24/40/336 and ILP32 at 24/36/308; preserve copied MSW/LSW values, two fresh IDs, WAIT/fault/cancel/stale recovery, partial physical write visibility, deferred AR/PC commit, and the direct WORD16 fast path. Catalog rows, DLD/DST semantics, SST collection, CPU opcode tests, inventory, and benchmark remain W47B2. |
| P2-W47B2A typed PMST.SST input seam | Implemented — focused normal, sanitizer, and LP64/ILP32 layout checks GREEN | Add typed SST read/write/knownness and copied input collection while retaining the 48-byte snapshot. Common reset leaves SST unknown, SXM/SST remain independent resources, and invalid collection is nonmutating. DLD/DST semantic rejection and execution remain W47B2B/C. |
| P2-W47B2B DLD/DST MOD13 catalog and semantics | Implemented — focused catalog/decode/semantic and inventory tests plus authoritative full gate GREEN | Activate two 16-encoding rows and a four-field two-word layout; keep trusted execution single-pass; implement DLD SXM completion and DST copied raw-40 SST/SXM saturation; bind both as MOD13 subsets. CPU bus lifecycle and opcode benchmarking remain W47B2C. |
| P2-W47B2C DLD/DST CPU and performance closure | Implemented — focused CPU/sanitizer/performance tests and authoritative full gate GREEN | Prove READY/WAIT/FAULT execution, stable resume identity, deferred publication, partial DST visibility, unknown-input rejection, and an observer-off full-CPU benchmark while retaining single-pass trusted dispatch and fixed carrier sizes. |
| P2-W50A dual-WORD16 MOD0 read foundation | Implemented — focused CTest 5/5, Python hot-path 7/7, and full-gate validation green | Establish the fixed-size dual-WORD16 MOD0 address/effect/pair/root seams: copy old AR2–AR5 addresses, serialize X then Y over one endpoint, preserve WAIT identity, and publish only after both READY observations. No opcode row, CPU branch, or TI timing claim. |
| P2-W50B dual-memory ADD/SUB MOD0 activation | Implemented — focused CTest 4/4, Python quality/performance checks, benchmark smoke, and full-gate validation green | Activate `ADD_DUAL_MOD0`/`SUB_DUAL_MOD0` (`0xA000/0xA200`, `0xFECC`) through the generic trusted binding and W50A transaction. Preserve fixed carriers, observer-off execution, and no-refetch/no-replan structure; one recorded hosted characterization measured ~1.75–1.81 µs/instruction, with no absolute threshold or TI cycle claim. |
| P2-W51A dual-indirect MOD0–MOD2 planning checkpoint | Implemented — focused RED→GREEN address and resolution contracts; no catalog/CPU/root activation | Direct AR2–AR5 selectors, MOD0/MOD1/MOD2 old-address and post-update planning, same-AR Xmod precedence, 32-byte requests-only snapshot, and MOD3 deferral. |
| P2-W51B dual-memory root commit/publication contract | Implemented — RED→GREEN root contract plus carrier widening; focused, sanitizer, and full-gate validation green | Extend the persistent dual snapshot to 48 bytes with two value-owned commit descriptors, validate mode/selector/address/live-state identity, publish distinct-AR updates atomically only after both reads and ordinary-effect staging, commit one update for same-AR Xmod precedence, keep WAIT/FAULT non-publishing, and freeze root sizes at 336/308 with no opcode activation. |
| P2-W51C dual ADD/SUB MOD0–MOD2 activation | Implemented — RED→GREEN activation plus inventory projection; focused, sanitizer, and full-gate validation green | Activate the sixteen non-MOD0/MOD0 syntax-6 ADD/SUB rows through the W51B root with per-row first words, shared parameterized checked/trusted leaves, per-row inventory SUBSET bindings, and no CPU branch, layout, timing, or benchmark-roster change. |
| Private program-PC bridge and CPU NOP transaction | Complete | Preserve the public carrier through one-word fetch, decode and dispatch NOP, stage through a private 16-bit PC seam, and retire transactionally with explicit stop/counter behavior |
| Value-owned operand-field extraction (P2-W19A) | Complete | A pure borrowed-view extractor assembles checked one-to-64-bit raw fields transactionally; no production opcode or fetch behavior changes |
| Transactional extension fetch and width-aware program PC (P2-W19B) | Complete | Allocation-free fixed-capacity extension fetch and explicit 1..255-word PC span/commit are proven before any multiword opcode or RPT integration |
| RPT evidence and instruction-transaction architecture (P2-W20A) | Complete | Immediate-RPT encodings, repeat/IRQ rules, owner hierarchy, staged implementation slices, and unresolved semantics are frozen before code changes |
| Canonical complete-instruction bundle (P2-W20B) | Complete | One decode-owned copied bundle is the checked handoff from CPU extension fetch through complete decode and semantic dispatch while NOP behavior remains unchanged |
| Catalog operand/repeat metadata (P2-W20C) | Complete | Scalar catalog tokens and decode-owned immutable layout sets are proven while production RPT activation remains gated |
| Typed immediate-RPT effect (P2-W20D) | Complete | A pure repeat-family operation converts an already extracted K/lk field into a transactional value-owned effect without production activation |
| Pure single-repeat policy foundation (P2-W20E) | Complete | Three repeat-sequencer leaf owners prove cardinality derivation, preclassified-body admission, and repeat-only request gating without claiming CPU execution |
| RPT CPU integration evidence closure (P2-W20F) | Deferred pending stronger evidence | W48A closes observable post-body RC progression/final zero; exact internal decrement ordering, single-repeat PC flow/final PC, and already-pending request arbitration remain unresolved without inference from generic or block-repeat rules |
| P2-W48A observable RC progression lifecycle | Implemented — focused state/repeat contracts and full 65,536-body progression GREEN | Add typed RC access and a fixed-size transactional REPEAT_START/body-retirement reducer; preserve exact count, stale/unknown rejection, terminal RC zero, and zero current CPU hot-path impact without activating RPT decode or PC replay |
| P2-W48B repeat-gated pending IFR latch | Implemented — focused state/interrupt contracts and authoritative full gate GREEN | Add typed one-bit IFR mutation and a fixed-size maskable-request receive/defer/take lifecycle with separate reset, IFR-write-one, and INTR clears; preserve unrelated flags and zero current CPU hot-path impact without claiming NMI mapping, priority, or predecode arbitration |
| P2-W48C DLD repeatability correction | Implemented — focused metadata/admission tests and authoritative full gate GREEN | Correct DLD_LONG32_MOD13 from unsupported INELIGIBLE to UNRESOLVED while retaining DST and the other evidenced negatives; do not infer ELIGIBLE from absence in a negative list or change execution/coverage/hot-path behavior |
| P2-W49A bit-32 ADD/SUB overflow and status foundation | Implemented — focused semantic/layout tests, supported ILP32 probe, evidence gate, semantic review, and interleaved observer-off benchmark GREEN | Provide modulo-40 ADD/SUB facts, bit-32 carry/no-borrow, signed-32 overflow direction, typed OVM/OVA/OVB, copied OVM input, and atomic sticky overflow effects without activating an instruction row or materially regressing the current CPU path |
| P2-W49B register-only ADD/SUB syntax-9 activation | Implemented — focused C/Make/CMake, Python/generator/inventory, six-pair W49A comparison, and dedicated observer-off benchmark evidence GREEN; no TI timing claim | Activate ADD_ACC_SHIFT (0xF400/0xFCE0) and SUB_ACC_SHIFT (0xF420/0xFCE0) through the existing ACCUMULATOR_SOURCE_DESTINATION_SHIFT5, W49A arithmetic/status, trusted binding, and generic CPU retirement seams; keep repeatability unresolved, project +2 rows/256 lookup entries/2 bindings and 42/205 family + 27/205 strict coverage, and require performance parity without any TI timing claim |
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
| P2-W33 CMPR CC,ARx status-compare vertical slice | Implemented — focused strict, seven focused ASan/UBSan, quality 186 Python, two independent no-P0/P1/P2 reviews, and final full `make test` green | The TI SPRU172C CMPR CC,ARx row (`0xF4A8`, mask `0xFCF8`) has 32 one-word encodings, copied generic AUXILIARY_REGISTERS input, pure unsigned EQ/LT/GT/NEQ comparison against AR0, TC result through STATUS_BIT_WRITE at ST0 bit 12, and generic CPU retirement. Full integration caught and corrected two stale UNKNOWN near-miss fixtures. The selected ARx and AR0 must be known or the handler returns UNKNOWN_INPUT; all other raw/known state is preserved. CMPR is explicitly nonrepeatable. No public API, memory, timing, IRQ/RPT, device, or opcode-specific CPU branch is added. Catalog coverage is 27 rows/1415 populated first words with 6/27 reset-runnable. |
| P2-W34 fixed-MOD2 LDU vertical slice | Implemented — focused catalog/decode/effects/CPU contracts passing; performance follow-ups remain open and nonblocking | Add the TI SPRU172C one-word `LDU *ARx+,dst` row derived from opcode diagram `0001_001D_IAAAAAAA`: value `0x1290`, mask `0xFEF8`, D bit 8, ARF bits 2:0, 16 encodings, `ACCUMULATOR_ARF` layout, no input resources, and repeatability `UNRESOLVED`. Reuse the W31B fixed-MOD2 MEMORY effect/completion and CPU contract: CMPT=0, ARP=0, old-AR transfer before postincrement, and AR=0xffff rejection by the checked planner. LDU loads Smem into dst15:0, clears dst39:16, ignores SXM, and reports no status-bit effects. No new resource/effect/address kind, public ABI, CPU opcode branch, timing, IRQ/RPT, device/MMR, environment, diagnostics, or allocation is added. Realized coverage is 28 rows/1431 populated first words with 6/28 reset-runnable. |
| P2-W35 fixed-MOD2 LD-to-T vertical slice | Implemented — focused catalog/decode/effects/CPU contracts, sanitizers, quality, review, and full gate green; performance follow-ups remain open and nonblocking | Add the TI SPRU172C §4.70 one-word `LD Smem,T` fixed-MOD2 row derived from `0011_0000_IAAAAAAA`: value `0x3090`, mask `0xFFF8` (bit 8 fixed zero), ARF bits 2:0, eight encodings, and a private generic `TI_C54X_OPERAND_LAYOUT_ARF` layout. Reuse the W31B MEMORY effect/completion and existing `T_REGISTER_WRITE`: transfer the old selected AR WORD16 into T before postincrement, with CMPT=0/ARP=0 and the checked AR=0xffff boundary. Repeatability is `UNRESOLVED`; no timing claim. `operand_layout.c` is 518 lines, remains cohesive, and is well below the 800-line threshold; the no-split decision is recorded. No new public T/MMR API, CPU branch, resource, address kind, IRQ/RPT, device, environment, diagnostics, or allocation behavior was added. Realized coverage is 29 rows/1439 populated first words with 6/29 reset-runnable. |
| P2-W36 fixed-MOD2 immediate-store vertical slice | Implemented — focused catalog/decode/effects/CPU contracts passing; sanitizer, quality, review, and full gate green | Add the TI SPRU172C Store T/TRN/immediate `ST #lk,*ARx+` two-word row (printed pp. 4-166–4-167), composed from syntax-3 `0111_0110_IAAAAAAA` and registered MOD2: value `0x7690`, mask `0xFFF8`, bit 8 fixed zero, ARF bits 2:0, eight encodings, `program_word_count=2`, and private `TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16`. Copy word 1 as raw `lk`; reuse existing extension fetch, width-aware PC, generic MEMORY WRITE with `write_value_word` and `complete=NULL`, and the checked CMPT=0/ARP=0 old-AR transfer then postincrement route. Status Bits are None. Repeatability is `UNRESOLVED`; no timing claim. Candidate `LD Smem,DP/ASM` also fits but is deferred; `ST T` is blocked on T input, `STH` on saturation, and ALU/control forms on shared seams. No public ABI, CPU production change or opcode branch, new resource/effect/address kind, IRQ/RPT, device/MMR, environment, diagnostics, allocation, or repeatability behavior is claimed. Realized coverage is 30 rows/1447 populated first words with 6/30 reset-runnable. |
| P2-W37 fixed-MOD2 memory-to-status vertical slice | Implemented — focused strict/sanitizer tests, quality, clean dual review, and full clean gate green | Add the TI SPRU172C §4.70 one-word `LD Smem,DP` and `LD Smem,ASM` fixed-MOD2 rows (printed pp. 4-69–4-71): values `0x4690` and `0x3290`, mask `0xFFF8` (bit 8 fixed zero), ARF bits 2:0, eight encodings each, and the existing private `TI_C54X_OPERAND_LAYOUT_ARF` layout with no input resources. Reuse the W31B MEMORY read/completion and status-field effect/writer: read data from the old selected-AR address, mask it to `0x01ff` for DP or `0x001f` for ASM, then postincrement and retire through generic WAIT/READY/FAULT and sequential-PC paths under CMPT=0/ARP=0. Table 2-28 marks DP repeatability `INELIGIBLE`; ASM repeatability remains `UNRESOLVED`. No `LD Smem,ARP` row, new resource/effect/address kind, CPU opcode branch, public API, timing, IRQ/RPT, device/MMR, environment, diagnostics, or allocation behavior was added. The catalog preserves unique handler tokens through thin DP/ASM wrappers over one shared internal core. The pure leaf is 283 lines; `operand_layout.c` remains 535 lines and unchanged, with the reviewed no-split decision in force. Realized coverage is 32 rows/1463 populated first words with 6/32 reset-runnable. |
| P2-W38 fixed-MOD2 LD Smem,16 vertical slice | Implemented — focused strict/sanitizer tests, quality 186, dual review, independent CMake CTest 178/178, and final clean `make test` green | Add one TI SPRU172C §4.66 syntax-3 fixed-MOD2 `LD Smem,16,dst` row: `0x4490`/`0xFEF8`, D bit 8, ARF bits 2:0, sixteen encodings, `ACCUMULATOR_ARF`, `STATUS_SXM`, and repeatability `UNRESOLVED`. Reuse MEMORY read/completion, ACCUMULATOR_WRITE, fixed-MOD2 addressing, and generic CPU WAIT/READY/FAULT sequential-PC retirement. Semantics are the evidenced SXM-controlled sign/zero extension followed by logical left shift 16 in canonical 40-bit space, preserving C/TC/OV and unrelated state. The unique `ld_smem_shift16` handler extends the shared `op_ld_smem` core; no new layout, resource, effect, address, state, CPU, public API, or source-manifest seam was added. The D-bit and sibling-ID fixture corrections are recorded in the detailed slice text above. `op_ld_smem.c` is 357 lines, its header 44, and `operand_layout.c` remains 535 lines with no split. Realized coverage is 33 rows/1479 populated first words with 6 reset-runnable; W31P/W31B performance follow-ups remain open and nonblocking. |
| P2-W39 generic T input and fixed-MOD2 ST T vertical slice | Implemented — focused strict plus legacy input/store regressions, ASan/UBSan, quality 186, semantic/integration review, independent CMake CTest 183/183, and final clean `make test` green; performance follow-ups remain open and nonblocking | Add the project `T_REGISTER` input snapshot contract and the one-word fixed-MOD2 `ST T,*ARx+` catalog/decode/effect/CPU vertical slice at `0x8C90`/`0xFFF8`, reusing `OPERAND_LAYOUT_ARF`, generic MEMORY WRITE, fixed-MOD2 addressing, and WAIT/READY/FAULT retirement. T input bit 32 is copied only when requested; unknown T rejects transactionally without a memory effect. The pure effect writes exact T to the old selected-AR WORD16 address before postincrement with no status-bit effects. No new layout, effect, address, state, public API, source-manifest, timing, or CPU opcode branch seam is admitted. Realized coverage is 34 rows, 1487 populated first words, ID_COUNT 35, 6 reset-runnable, and four aggregate fixed-MOD2 memory rows; repeatability remains unresolved. |
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

- [P2-W29: T-register foundation and EXP accumulator vertical slice (planned)](plan/p2-w29-t-register-foundation-and-exp-accumulator-vertical-slice.md)

- [P2-W30: two-word immediate logical accumulator vertical slice (planned)](plan/p2-w30-two-word-immediate-logical-accumulator-vertical-slice.md)

- [P2-W31A: fixed-MOD2 memory prerequisites (planned)](plan/p2-w31a-fixed-mod2-memory-prerequisites.md)

- [P2-W31P: trusted execution hot path and release-performance contract (planned)](plan/p2-w31p-trusted-execution-hot-path-and-release-performance-contract.md)

- [P2-W31B: transactional data-memory endpoint and fixed-MOD2 load/store pair (planned)](plan/p2-w31b-transactional-data-memory-endpoint-and-fixed-mod2-load-store-pair.md)

- [P2-W32: SSBX/RSBX status-bit vertical slice (planned)](plan/p2-w32-ssbx-rsbx-status-bit-vertical-slice.md)

- [P2-W33: CMPR CC,ARx vertical slice (planned)](plan/p2-w33-cmpr-cc-arx-vertical-slice.md)

- [P2-W34: fixed-MOD2 LDU vertical slice (planned)](plan/p2-w34-fixed-mod2-ldu-vertical-slice.md)

- [P2-W35: fixed-MOD2 LD-to-T vertical slice (planned)](plan/p2-w35-fixed-mod2-ld-to-t-vertical-slice.md)

- [P2-W36: fixed-MOD2 immediate-store vertical slice (planned)](plan/p2-w36-fixed-mod2-immediate-store-vertical-slice.md)

- [P2-W37: fixed-MOD2 memory-to-status vertical slice (planned)](plan/p2-w37-fixed-mod2-memory-to-status-vertical-slice.md)

- [P2-W16: NOP-seeded opcode architecture](plan/p2-w16-nop-seeded-opcode-architecture.md)

- [P2-W17: masked opcode catalog foundation (active kickoff)](plan/p2-w17-masked-opcode-catalog-foundation.md)

- [P2-W18: private program-PC bridge and CPU NOP transaction (active kickoff)](plan/p2-w18-private-program-pc-bridge-and-cpu-nop-transaction.md)

- [P2-W19A: value-owned operand-field extraction (active kickoff)](plan/p2-w19a-value-owned-operand-field-extraction.md)

- [P2-W19B: transactional extension fetch and width-aware program PC (active kickoff)](plan/p2-w19b-transactional-extension-fetch-and-width-aware-program-pc.md)

- [P2-W20A: RPT evidence and instruction-transaction architecture (active kickoff)](plan/p2-w20a-rpt-evidence-and-instruction-transaction-architecture.md)

- [P2-W49A: bit-32 ADD/SUB overflow and status foundation (planned)](plan/p2-w49a-bit-32-add-sub-overflow-and-status-foundation.md)

- [P2-W49B: register-only ADD/SUB syntax-9 activation (planned)](plan/p2-w49b-register-only-add-sub-syntax-9-activation.md)

- [P2-W50A: dual-WORD16 MOD0 read foundation (planned)](plan/p2-w50a-dual-word16-mod0-read-foundation.md)

- [P2-W50B: dual-memory ADD/SUB MOD0 activation (planned)](plan/p2-w50b-dual-memory-add-sub-mod0-activation.md)

- [P2-W51A: dual-indirect MOD0-MOD2 planning checkpoint (planned)](plan/p2-w51a-dual-indirect-mod0-mod2-planning-checkpoint.md)

- [P2-W51B: dual-memory root commit/publication contract (planned)](plan/p2-w51b-dual-memory-root-commit-publication-contract.md)

- [P2-W51C: dual ADD/SUB MOD0–MOD2 activation (planned)](plan/p2-w51c-dual-add-sub-mod0-mod2-activation.md)

- [P2-W52A: COMMON repeat-truth closure (planned)](plan/p2-w52a-common-repeat-truth-closure.md)

- [P2-W52B: dual MOD3 AR0/BK planning checkpoint (planned)](plan/p2-w52b-dual-mod3-ar0-bk-planning-checkpoint.md)

- [P2-W52C: dual MOD3 ADD/SUB activation (planned)](plan/p2-w52c-dual-mod3-add-sub-activation.md)

- [P2-W53: LD Smem[,SHIFT],dst vertical slice (planned)](plan/p2-w53-ld-smem-shift-dst-vertical-slice.md)

- [P2-W54: LD src[,SHIFT],dst vertical slice (planned)](plan/p2-w54-ld-src-shift-dst-vertical-slice.md)

- [P2-W55: LD #K,dst vertical slice (planned)](plan/p2-w55-ld-k-dst-vertical-slice.md)

- [P2-W56: LD src,ASM[,dst] vertical slice (planned)](plan/p2-w56-ld-src-asm-dst-vertical-slice.md)

- [P2-W57: LD #lk,16,dst vertical slice (planned)](plan/p2-w57-ld-lk-16-dst-vertical-slice.md)

- [P2-W58: LD #lk[,SHFT],dst vertical slice (planned)](plan/p2-w58-ld-lk-shft-dst-vertical-slice.md)

- [P2-W59: LD Xmem,SHFT,dst vertical slice (planned)](plan/p2-w59-ld-xmem-shft-dst-vertical-slice.md)

- [P2-W60: LDR Smem,dst vertical slice (planned)](plan/p2-w60-ldr-smem-dst-vertical-slice.md)

- [P2-W61: LDR Smem,dst indexed MOD5/MOD6 vertical slice (planned)](plan/p2-w61-ldr-smem-dst-indexed-mod5-mod6-vertical-slice.md)

- [P2-W62: B pmad unconditional-branch vertical slice (planned)](plan/p2-w62-b-pmad-unconditional-branch-vertical-slice.md)

- [P2-W63: BC pmad,cond conditional-branch vertical slice (planned)](plan/p2-w63-bc-pmad-cond-conditional-branch-vertical-slice.md)

- [P2-W64: stack-pointer input seam vertical slice (planned)](plan/p2-w64-stack-pointer-input-seam-vertical-slice.md)

- [P2-W65A: stack-push MEMORY transaction seam (planned)](plan/p2-w65a-stack-push-memory-transaction-seam.md)

- [P2-W65B: CALL pmad vertical slice (planned)](plan/p2-w65b-call-pmad-vertical-slice.md)

- [P2-W66A: stack-pop MEMORY transaction seam (planned)](plan/p2-w66a-stack-pop-memory-transaction-seam.md)

- [P2-W66B: RET vertical slice (planned)](plan/p2-w66b-ret-vertical-slice.md)

- [P2-W67A: delayed-branch MEMORY transaction seam (planned)](plan/p2-w67a-delayed-branch-memory-transaction-seam.md)

- [P2-W67B: RETD vertical slice (planned)](plan/p2-w67b-retd-vertical-slice.md)

- [P2-W67C: CALLD vertical slice (planned)](plan/p2-w67c-calld-vertical-slice.md)

- [P2-W68: BD delayed-branch vertical slice (planned)](plan/p2-w68-bd-delayed-branch-vertical-slice.md)

- [P2-W69: BCD delayed-conditional-branch vertical slice (planned)](plan/p2-w69-bcd-delayed-conditional-branch-vertical-slice.md)

- [P2-W70: BANZ auxiliary-register-branch vertical slice (planned)](plan/p2-w70-banz-auxiliary-register-branch-vertical-slice.md)

- [P2-W71: BANZD delayed auxiliary-register-branch vertical slice (planned)](plan/p2-w71-banzd-delayed-auxiliary-register-branch-vertical-slice.md)

- [P2-W72: CCD delayed-conditional-call vertical slice (planned)](plan/p2-w72-ccd-delayed-conditional-call-vertical-slice.md)

- [P2-W73: CC non-delayed-conditional-call vertical slice (planned)](plan/p2-w73-cc-non-delayed-conditional-call-vertical-slice.md)

- [P2-W74: RETE interrupt-return vertical slice (planned)](plan/p2-w74-rete-interrupt-return-vertical-slice.md)

- [P2-W75: RETED delayed-interrupt-return vertical slice (planned)](plan/p2-w75-reted-delayed-interrupt-return-vertical-slice.md)

- [P2-W77: RC conditional-return vertical slice (planned)](plan/p2-w77-rc-conditional-return-vertical-slice.md)

- [P2-W78: RCD delayed-conditional-return vertical slice (planned)](plan/p2-w78-rcd-delayed-conditional-return-vertical-slice.md)

- [P3-S3j: MPYA-dst register-only vertical slice (implemented; local verification green)](plan/p3-s3j-mpya-dst-register-only-vertical-slice.md)

- [P3-S3k: SQUR-Smem square vertical slice (implemented; local verification green)](plan/p3-s3k-squr-smem-square-vertical-slice.md)

- [P3-S3l: SQUR-Adst register-only vertical slice (implemented; local verification green)](plan/p3-s3l-squr-adst-register-only-vertical-slice.md)

- [P3-S3m: SQURA/SQURS accumulate-squares vertical slice (implemented; local verification green)](plan/p3-s3m-squra-squrs-accumulate-squares-vertical-slice.md)

- [P3-S3n: MPYU unsigned-multiply vertical slice (implemented; local verification green)](plan/p3-s3n-mpyu-unsigned-multiply-vertical-slice.md)

- [P3-S3o: MACA Smem accumulate-A slice (implemented; local verification green)](plan/p3-s3o-maca-smem-accumulate-a-slice.md)

- [P3-S3p: MACA T,src register-only slice (implemented; local verification green)](plan/p3-s3p-maca-t-src-register-only-slice.md)

- [P3-S3q: MACP Smem,Pmad program-coefficient slice (implemented; local verification green)](plan/p3-s3q-macp-smem-pmad-program-coefficient-slice.md)

- [P3-S3r: MACD Smem,Pmad delay-write slice (implemented; local verification green)](plan/p3-s3r-macd-smem-pmad-delay-write-slice.md)

- [P3-S3s: MACSU Xmem,Ymem dual slice (implemented; local verification green)](plan/p3-s3s-macsu-xmem-ymem-dual-slice.md)

- [P3-S3t: MACR Xmem,Ymem dual lock-in slice (implemented; local verification green)](plan/p3-s3t-macr-xmem-ymem-dual-lock-in-slice.md)

- [P3-S3u: MASR Smem and dual lock-in slice (implemented; local verification green)](plan/p3-s3u-masr-smem-and-dual-lock-in-slice.md)

- [P3-S3v: MACAR Smem lock-in slice (implemented; local verification green)](plan/p3-s3v-macar-smem-lock-in-slice.md)

- [P3-S3w: MASA Smem and T slice (implemented; local verification green)](plan/p3-s3w-masa-smem-and-t-slice.md)

- [P3-S3x: MASAR T lock-in slice (implemented; local verification green)](plan/p3-s3x-masar-t-lock-in-slice.md)

- [P3-S3y: MPYR Smem lock-in slice (implemented; local verification green)](plan/p3-s3y-mpyr-smem-lock-in-slice.md)

- [P3-S3z: ADD Smem slice (implemented; local verification green)](plan/p3-s3z-add-smem-slice.md)

- [P3-S4: XC conditional-execution slice (implemented; local verification green)](plan/p3-s4-xc-conditional-execution-slice.md)

- [P3-S5: accumulator-addressed push seam (implemented; local verification green)](plan/p3-s5-accumulator-addressed-push-seam.md)

- [P3-S8: BACC accumulator-addressed branch family slice (implemented; local verification green)](plan/p3-s8-bacc-accumulator-addressed-branch-family-slice.md)

- [P3-S9: CALA accumulator-addressed call family slice (implemented; local verification green)](plan/p3-s9-cala-accumulator-addressed-call-family-slice.md)

- [P3-S10: STH syntax-1 Smem store slice (implemented; local verification green)](plan/p3-s10-sth-syntax-1-smem-store-slice.md)

- [P3-S11: STH by-ASM Smem store slice (implemented; local verification green)](plan/p3-s11-sth-by-asm-smem-store-slice.md)

- [P3-S12: STH by-SHFT Xmem store slice (implemented; local verification green)](plan/p3-s12-sth-by-shft-xmem-store-slice.md)

- [P3-S13: STH by-SHIFT Smem store slice (implemented; local verification green)](plan/p3-s13-sth-shift-smem-store-slice.md)

- [P3-S14: SUB Smem slice (implemented; local verification green)](plan/p3-s14-sub-smem-slice.md)

- [P3-S15: AND Smem slice (implemented; local verification green)](plan/p3-s15-and-smem-slice.md)

- [P3-S16: OR Smem slice (implemented; local verification green)](plan/p3-s16-or-smem-slice.md)

- [P3-S17: XOR Smem slice (implemented; local verification green)](plan/p3-s17-xor-smem-slice.md)

- [P3-S18: STH MOD0-MOD3 second-word disambiguation seam (planned)](plan/p3-s18-sth-mod0-3-second-word-seam.md)

- [P3-S6: port I/O seam (planned)](plan/p3-s6-port-io-seam.md)

- [P3-S19: SUB Smem rows (implemented; local verification green)](plan/p3-s19-sub-smem-rows-slice.md)

- [P3-S20: AND Smem rows (implemented; local verification green)](plan/p3-s20-and-smem-rows-slice.md)

- [P3-S21: OR Smem rows (implemented; local verification green)](plan/p3-s21-or-smem-rows-slice.md)

- [P3-S22: XOR Smem rows (implemented; local verification green)](plan/p3-s22-xor-smem-rows-slice.md)

- [P3-S23: SUBS Smem leaf (implemented; local verification green)](plan/p3-s23-subs-smem-leaf-slice.md)

- [P3-S24: SUBB Smem leaf (implemented; local verification green)](plan/p3-s24-subb-smem-leaf-slice.md)

- [P3-S25: SUBB Smem rows (implemented; local verification green)](plan/p3-s25-subb-smem-rows-slice.md)

- [P3-S26: SUBS Smem rows (implemented; local verification green)](plan/p3-s26-subs-smem-rows-slice.md)

- [P3-WAVE5: ADD/SUB leaf fan-out (implemented; local verification green)](plan/p3-wave5-add-sub-leaves-slice.md)

- [P3-S27: ADD/SUB #lk rows (planned)](plan/p3-s27-add-sub-lk-rows-slice.md)

- [P3-WAVE6: MPY/MAC #lk leaf fan-out (planned)](plan/p3-wave6-mpy-mac-leaves-slice.md)

- [P3-S28: ADD/SUB Xmem rows (planned)](plan/p3-s28-add-sub-xmem-rows-slice.md)

- [P3-S29: ADD/SUB src-ASM rows (planned)](plan/p3-s29-add-sub-src-asm-rows-slice.md)

- [P3-S30: MPY/MAC #lk rows (planned)](plan/p3-s30-mpy-mac-lk-rows-slice.md)

- [P3-S31: ADD/SUB Smem SHIFT rows (planned)](plan/p3-s31-add-sub-smem-shift-rows-slice.md)

- [P3-S32: ADDM RMW-ADD seam (planned)](plan/p3-s32-addm-rmw-add-seam-slice.md)

- [P2-W21: shared-memory transaction foundation](plan/p2-w21-shared-memory-transaction-foundation.md)

- [P2-W22: immediate status-field LD vertical slice (implemented; local verification green)](plan/p2-w22-immediate-status-field-ld-vertical-slice.md)

- [P2-W23: MAX/MIN accumulator vertical slice (planned; implementation not started)](plan/p2-w23-max-min-accumulator-vertical-slice.md)

- [P2-W24: generic accumulator-write effect and CMPL vertical slice (planned)](plan/p2-w24-generic-accumulator-write-effect-and-cmpl-vertical-slice.md)

- [P2-W25: generic input collection and ROL/ROR vertical slice (planned)](plan/p2-w25-generic-input-collection-and-rol-ror-vertical-slice.md)

- [P2-W26: SFTL shifted-accumulator vertical slice (planned)](plan/p2-w26-sftl-shifted-accumulator-vertical-slice.md)

- [P2-W27: AND/OR/XOR register-only logical accumulator vertical slice (planned)](plan/p2-w27-and-or-xor-register-only-logical-accumulator-vertical-slice.md)

- [P2-W28: TC-aware status foundation and ROLTC/SFTC vertical slice (planned)](plan/p2-w28-tc-aware-status-foundation-and-roltc-sftc-vertical-slice.md)

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

Each slice owns one `docs/plan/<id>-<slug>.md` doc. Its `###` heading is the
FIT/ownership anchor: never reword the heading, only advance its status
parenthetical from `(planned)` to `(implemented; local verification green)`
when the slice lands. The slice link list in this plan mirrors the same
parenthetical per link. Commit split: the implementation commit carries code,
tests, registry/evidence, and generated files only; the follow-up ledger-only
commit flips the parenthetical in both the slice-doc heading and the plan link
and appends the ledger entry.

The plan status table is updated when the intended state changes. The ledger is
append-only evidence: it records the actual commit, affected ownership modules,
tests, decisions, registered-knob changes, the status flip, and remaining work.
If an old entry is wrong, add a correction that names it rather than editing
history.

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
