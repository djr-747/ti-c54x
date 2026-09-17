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
speech acceptance boundary are maintained separately in
`docs/dsp-reconstruction-hypothesis-evidence-ledger.md`.

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

### P3-S1: MMR/device routing seam (planned)

This immutable `(planned)` heading is the durable S1 FIT/ownership
anchor. S1 is a sealed architectural seam with no catalog rows, no
opcode identities, no handlers, and no CPU opcode branch, following
the W43A pattern. TI SPRU131G §3.3.4/Table 3-3 (printed p. 3-27)
maps the CPU memory-mapped registers to DATA-space addresses
`0x00`–`0x1F`: `IMR 00`, `IFR 01`, reserved `02`–`05`, `ST0 06`,
`ST1 07`, `AL 08`, `AH 09`, `AG 0A`, `BL 0B`, `BH 0C`, `BG 0D`,
`T 0E`, `TRN 0F`, `AR0`–`AR7 10`–`17`, `SP 18`, `BK 19`, `BRC 1A`,
`RSA 1B`, `REA 1C`, `PMST 1D`, `XPC 1E` (far/XPC deferred per the
arch criterion, declined), reserved `1E`–`1F`. TI SPRU172C §4
`STM #lk, MMR` (printed pp. 4-175–4-176) stores the second
program word into the MMR file or DP0 with the nine MSBs of the
effective address cleared, so DATA-space WORD16 traffic at
`0x00`–`0x1F` must route to register state, never to plain
endpoint memory.

Scope: the `src/bus/mmr/` MMR file owns the Table 3-3 map with
per-address owner dispatch versus private instance-owned
16-bit storage plus knownness for the five registers with no
architectural owner (`IMR`, `TRN`, `BRC`, `RSA`, `REA`);
reserved `02`–`05` and `XPC` decline and fall through to the
endpoint exactly as today. Registers with live owners route to
checked whole-word access: `T`/`ARn`/`SP`/`BK`/`IFR` through
existing accessors, `ST0`/`ST1`/`PMST` and accumulator words
`AL`/`AH`/`AG`/`BL`/`BH`/`BG` through new whole-word owner
accessors (single-sourced knownness stays with the owner:
whole-word status knownness requires the full known mask,
accumulator-word knownness follows the accumulator known
bit). The `src/sequencer/memory/` resolver gates MMR READ
knownness at prepare time (transactional reject on unknown,
mirroring the `STACK_POP_RTN` precedent) and the
`src/sequencer/instruction/` root transaction serves routed
requests locally with no endpoint traffic, preserving atomic
WAIT/READY/FAULT/cancel behavior. No timing, IRQ/RPT, wrap,
machine-cycle, public-ABI, or new public CPU snapshot claim
is admitted. No knob is added.

Ownership: `REQ-PROJECT-P3-S1-MMR-FILE` (`src/bus/mmr/`),
`REQ-PROJECT-P3-S1-MMR-RESOLVER` (`src/sequencer/memory/`),
`REQ-PROJECT-P3-S1-MMR-TRANSACTION`
(`src/sequencer/instruction/`), and
`REQ-PROJECT-P3-S1-MMR-OWNER-ACCESS` (`src/state/`) carry
the project contract over TI `Table 3-3` map evidence plus
the reused `BK`/`T` MMR records. The four RED lanes are
`tests/unit/bus/test_mmr_route.c` (CTest
`ti_c54x.mmr_route`), `tests/unit/sequencer/memory/test_mmr_address_resolution.c`
(CTest `ti_c54x.mmr_resolver`),
`tests/unit/sequencer/instruction/test_mmr_memory_transaction.c`
(CTest `ti_c54x.mmr_transaction`), and owner-local accessor
cases under `tests/unit/state/` (CTest
`ti_c54x.mmr_owner_access`). Exit: sealed seam with all four
lanes green, existing endpoint behavior preserved for
declined addresses, and slice 7 (`STM`) unblocked.

### P3-S7: STM immediate-MMR-store vertical slice (planned)

This immutable `(planned)` heading is the durable S7 FIT/ownership
anchor. S7 composes the first family on the S1 seam: TI SPRU172C
§4 `STM #lk, MMR` (printed pp. 4-175–4-176) stores the second
program word into the MMR file or DP0 with the nine MSBs of the
effective address cleared regardless of `DP`. Word 0 is
`0111_011I_AAAAAAAA` (one row `STM` at `0x7700` under mask
`0xFF00`, 256 encodings, two program words): `I=0` selects the
7-bit direct form (`0x00`–`0x1F` route through S1, `0x20`–`0x7F`
reach DP0 endpoint RAM) while `I=1` selects the indirect form
and rejects as deferred at the op leaf, mirroring the
unevidenced-condition precedent. Status bits are none;
repeatability stays `UNRESOLVED` (`STM` is absent from Table
2-28); inputs are none. The new `DIRECT` memory address kind
carries the 7-bit address in an appended `direct_dma7` byte
(the ABI asserts guard the unchanged sizes) and resolves
through a new DP-independent page-0 planner (the `DIRECT_DP`
planner wrongly requires live `DP==dp9`), with a new
`DIRECT7_EXTENSION16` layout (`I` flag, `addr7`, `lk16`).
`FORM-T2-20-14` lands `SUBSET` with the `STM` symbol (the
deferred indirect form keeps it from `COMPLETE`).
The four RED executable contracts are
`tests/unit/isa/catalog/test_stm_metadata.c`
(CTest `ti_c54x.stm_metadata`),
`tests/unit/isa/decode/test_stm_decode.c`
(CTest `ti_c54x.stm_decode`),
`tests/unit/isa/ops/store/test_stm.c`
(CTest `ti_c54x.stm_ops`), and
`tests/unit/cpu/isa/store/test_cpu_stm.c`
(CTest `ti_c54x.cpu_stm`), plus the address-layer planner
lane and the `SUBSET` inventory binding. Realized closure
is 163 catalog rows, `ID_COUNT=163`, 7,620 lookup entries,
family coverage 62/205 (30.244%), and 33/205 strict
complete-form coverage; 339 evidence records, 527
requirements, and 418 tests (418 `PASSING`, 0 `PLANNED`).
Landing also added the page-0 `DIRECT` liveness arm to
candidate validation with a CMPT-independence CPU proof,
and re-pinned the two `ST #lk,Smem` near-miss flips
(`0x0100`) that now resolve into the `STM` row.

### P3-S2: RPT activation seam (planned)

This immutable `(planned)` heading is the durable S2 FIT/ownership
anchor. S2 activates single-repeat on the W48A/W48B seam: TI
SPRU172C §4 `RPT` (printed pp. 4-146–4-147) loads `RC` with
the iteration count and repeats the following instruction
`n + 1` times with no status effects; `RPT` itself is
nonrepeatable and `RC` is inaccessible while decrementing.
Two rows land: `RPT_K` at `0xEC00` under mask `0xFF00`
(`K` in bits 7:0, one program word, `FORM-T2-16-02`) and
`RPT_LK` exact at `0xF070` (16-bit extension, two program
words, `FORM-T2-16-03`), both `INELIGIBLE` with no inputs,
reusing the `RPT_IMMEDIATE_K`/`RPT_IMMEDIATE_LK` layouts and
the `op_rpt_immediate` core behind checked and trusted
leaves. `RPT Smem` (`FORM-T2-16-01`) stays `UNIMPLEMENTED`:
its memory-source operand needs address-resolution and
read integration that this slice does not admit.

The CPU owns `ti_c54x_repeat_progress` plus the W48B
pending latch and a body-PC word (delayed-branch pattern).
The `REPEAT_START` route starts progress (`RC` load plus
cardinality), rejects `RPT` inside delay slots and nested
`RPT` while `ACTIVE`, and retires sequentially onto the
body. The predecode point, taken only while `ACTIVE` and
before extension fetch, affirms request deferral through
the gate (latched maskable requests persist; take stays
with the future vector slice and `RS`/reset preempts by
construction) and admits only `ELIGIBLE` bodies —
`UNRESOLVED` blocks and `INELIGIBLE` rejects, both as
transactional `INVALID_STATE` stops with the body
unretired and `PC` frozen. A new `repeat_single` driver
under `src/sequencer/repeat/` owns start, predecode
(arbitration plus admission), and body-retired (`HOLD`
versus `ADVANCE`) decisions; `run.c` wires start plus
predecode now and owns no repeat policy. No catalog row is
`ELIGIBLE` today, so end-to-end replay unblocks with the
first positively-evidenced `ELIGIBLE` classification plus
its retirement-tail `HOLD` call sites; the `HOLD`/`ADVANCE`
flow itself is proven at the driver seam. No vector,
take-path, NMI-source, timing, or `RPTB`/`RPTZ`/`RPTBD`
claim is admitted.
The RED executable contracts are
`tests/unit/isa/catalog/test_rpt_k_metadata.c` and
`test_rpt_lk_metadata.c`
(CTests `ti_c54x.rpt_k_metadata`, `ti_c54x.rpt_lk_metadata`),
`tests/unit/isa/decode/test_rpt_k_decode.c` and
`test_rpt_lk_decode.c`
(CTests `ti_c54x.rpt_k_decode`, `ti_c54x.rpt_lk_decode`),
`tests/unit/isa/ops/repeat/test_rpt_k.c` and
`test_rpt_lk.c`
(CTests `ti_c54x.rpt_k_ops`, `ti_c54x.rpt_lk_ops`),
`tests/unit/sequencer/repeat/test_repeat_single_flow.c`
(CTest `ti_c54x.repeat_single_flow`), and
`tests/unit/cpu/isa/repeat/test_cpu_rpt.c`
(CTest `ti_c54x.cpu_rpt`), plus the `SUBSET`
`FORM-T2-16-02`/`FORM-T2-16-03` inventory bindings.
Realized closure is 165 catalog rows, `ID_COUNT=165`,
7,877 lookup entries, family coverage 64/205 (31.220%), and
33/205 strict complete-form coverage; 341 evidence records,
537 requirements, and 426 tests (426 `PASSING`, 0
`PLANNED`). Landing also split the run.c repeat wiring into
`src/cpu/execution/repeat_activate.c` (run.c 795 lines) and
re-pinned five neighbor near-miss flips plus the W20F
no-row pins that now resolve into the `RPT` rows.

### P3-S3: MAC/MPY datapath seam (planned)

This immutable `(planned)` heading is the durable S3 FIT/ownership
anchor. S3 builds the multiply-accumulate datapath on the S2 repeat
seam: TI SPRU172C §4 `MAC[R]` (printed pp. 4-81–4-83) syntax 1
`MAC[R] Smem, src` executes `(Smem) x (T) + (src)` into `src`
(or `dst`), one program word, with opcode bits 15:10 `001010`
(R bit 9, S bit 8, I bit 7, Smem field 6:0); the operation is
affected by `FRCT` and `OVM` and affects `OVdst` (or `OVsrc`
when `dst` is unspecified); the `R` suffix rounds by adding
`0x8000` and clearing bits 15:0. Table 2-28 (p. 2-20) lists no
MAC/MPY-family instruction, so the Smem syntaxes are repeatable
by omission — the recorded basis for the future `ELIGIBLE`
classification and the S2 `HOLD`-tail replay. The TI Example 1
vector (`MAC *AR5+, A` with `A` = `00 0000 1000`, `T` =
`0400`, `mem[0100]` = `1234`, `FRCT` = 0 yielding `A` = `00
0048 E000`) is the seam's independent oracle.

The seam is a new pure module under `src/isa/ops/multiply/`
owning the checked/trusted multiply-accumulate core over a
copied raw40 accumulator, `u16` T, `u16` multiplicand, `FRCT` /
`OVM` snapshot bits, a rounding flag, and an add/sub/store
select: it stages accumulator plus overflow effects and owns no
addressing, catalog rows, CPU sequencing, or timing. Slices: S3a
lands the seam with no rows; S3b lands the `MAC` Smem row
(`FORM-T2-04-01`, Table 2-4 ordinal 1) with metadata, decode,
ops, CPU retirement, `ELIGIBLE` classification, and an `RPT
MAC` end-to-end replay; S3c lands `MPY` Smem
(`FORM-T2-03-01`); `MPYA`/`MAS`/`MACR` then follow in census
order. The RED executable contract for the seam is
`tests/unit/isa/ops/multiply/test_multiply_core.c` (CTest
`ti_c54x.multiply_core`), proving the TI Example 1 vector,
the `FRCT` shift, `R` rounding, `OVM` saturation with `OVdst`,
and unknown-T/unknown-src transactional stops. Deferred: dual
Xmem/Ymem syntax 2 (needs dual-operand addressing), `#lk`
syntaxes 3–4 (long-constant words), `MACA`/`MACD`/`MACP`
(accumulator-A/program-memory operands plus the delay seam),
the remaining wave-4 families, and any PMST product-shift
claim beyond the `FRCT` behavior the `MAC` section states.

### P2-W38: fixed-MOD2 LD Smem,16 vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The bounded one-word `LD Smem,16,dst` form from TI SPRU172C
§4.66 (printed pp. 4-65–4-69) is implemented as
`LD_SMEM_SHIFT16_MOD2` with unique handler token `ld_smem_shift16`, value
`0x4490`, mask `0xFEF8`, D bit 8, ARF bits 2:0, sixteen encodings, the existing
`ACCUMULATOR_ARF` layout, `STATUS_SXM` input, and repeatability `UNRESOLVED`.
The exact SXM transform is WORD16 zero-extension when SXM=0 or sign-extension
when SXM=1 to canonical 40 bits, followed by logical left shift 16 within 40
bits: `0000` is zero in both modes, `7fff` is `0x007fff0000`, `8000` is
`0x0080000000`/`0xff80000000`, `ffff` is `0x00ffff0000`/`0xffffff0000`, and
the printed `FEDC`/SXM=1 vector is `0xfffedc0000`. C, TC, OV, and unrelated
state remain unchanged. The unique handler extends the shared `op_ld_smem`
core; existing MEMORY read/completion, ACCUMULATOR_WRITE, fixed-MOD2
addressing, and generic CPU WAIT/READY/FAULT sequential-PC paths are reused
under CMPT=0/ARP=0. No new layout, resource, effect, address, state, CPU,
public API, or source-manifest seam was added. The TDD fixture corrections
record that D bit 8 is an operand (not invalid), `0x4490 ^ 0x0200 = 0x4690`
collides with the W37 DP sibling so `0x0208`/`0x4698` are used, and stale W37
ID-count and aggregate fixed-MOD2 row-count expectations were updated from
33→34 and 2→3. STH, LDR, LD#K, ST T, and TS remain deferred for their recorded
gaps. Focused strict, ASan/UBSan, quality-186, dual-review, and independent
CMake full CTest 178/178 and final clean `make test` gates are green. The
`op_ld_smem.c` leaf is 357 lines and its
header is 44 lines, both below 500; `operand_layout.c` remains 535 lines with
no split. Realized coverage is 33 rows and 1479 populated first-word encodings
with 6 reset-runnable. W31P/W31B performance follow-ups remain open and
nonblocking.

### P2-W39: generic T input and fixed-MOD2 ST T vertical slice (planned)

This immutable `(planned)` heading remains the FIT/ownership anchor for the
evidence freeze; the W39 implementation is now complete. TI SPRU172C Store
T/TRN/immediate (printed pp. 4-166–4-168)
defines the one-word `ST T,*ARx+` form with syntax-1 opcode diagram
`1000_1100_IAAAAAAA`, which composes with the registered SPRU131G §5.5.3
fixed-MOD2 `*ARx+` rule as value `0x8C90`, mask `0xFFF8`, fixed bit 8, ARF bits
2:0, eight encodings, and the existing `OPERAND_LAYOUT_ARF`. The operation
copies the old 16-bit T value to the old selected-AR WORD16 address before
postincrement and has no status-bit effects; its printed `*AR7-` example stores
`T=0x4210` at old `AR7=0x0321` then leaves `AR7=0x0320`.

The project contract adds a `T_REGISTER` input resource (bit 32) carrying copied
`t_register_raw16` and `t_register_known` fields from private T state. T is not
read when unrequested; malformed or unknown input is transactional, and unknown
T remains `known=false` for semantic rejection. The `st_t_smem` leaf emits a
pure MEMORY WORD16 WRITE with exact copied T, `complete=NULL`, old-AR address
before postincrement, and no status effects. Generic fixed-MOD2 address
resolution and CPU WAIT/READY/FAULT/sequential-PC retirement are reused without
a CPU opcode branch. No new layout, effect, address, architectural-state,
public API, source-manifest, timing, or machine-cycle seam was added.

Five RED lanes established the input, catalog, decode, effects, and CPU
retirement contracts. Focused strict plus legacy input/store regressions,
focused ASan/UBSan, quality-186, semantic review, integration review,
independent fresh CMake CTest 183/183, and authoritative clean `make test` are
green; integration corrected two stale `ID_COUNT34` fixtures to `ID_COUNT35`.
Realized coverage is 34 rows, 1487 populated first-word encodings, ID_COUNT 35,
6 reset-runnable rows, and four aggregate fixed-MOD2 memory rows. Repeatability
remains `UNRESOLVED` because Table 2-28 absence is not positive eligibility
evidence. STH, LDR, LD#K, and TS remain deferred for their recorded gaps;
W31P/W31B performance follow-ups remain open and nonblocking.


### P2-W40: authoritative ISA instruction-form inventory (planned)

This heading is the immutable FIT/ownership anchor for the inventory audit. The
authoritative SPRU172C summary/source-universe checkpoint is §§2.1–2.4, Tables
2-1 through 2-26 (printed pp. 2-2–2-18): 205 syntax forms, 126 normalized
mnemonic tokens, and 13 parallel bundles, with the pinned per-table vector
`[13,13,10,22,6,15,5,5,5,6,5,6,5,2,6,5,5,7,21,14,4,2,4,2,5,12]`.

W40 adds only audit metadata and quality contracts. `config/isa/inventory.json`
will describe one record per syntax form, normalized mnemonic and parallel
bundle metadata, explicit `COMPLETE`/`SUBSET`/`UNIMPLEMENTED` coverage, and
explicit `COMMON_CANDIDATE`/`PROFILE_ONLY`/`OUT_OF_SCOPE`/`UNRESOLVED` scope;
205 is not yet an exact common-ISA denominator until those scope decisions are
evidenced.

`opcode_catalog.def` remains the sole production opcode value/mask source;
inventory records must bind to catalog identity rather than duplicate opcode
authority. Unresolved candidate forms remain in the provisional denominator,
and any exclusion needs explicit evidence. The report is provisional and must
state 35/205 = 17.073%
catalog-family progress and the strict 25/205 = 12.195% complete-form lower
bound; populated-first-word ratios are not ISA coverage.

The planned implementation owns `tools/isa/inventory/schema.py` and
`tools/quality/check_isa_inventory.py`, with RED-first contract tests under
`tests/contract/quality/isa/inventory/`. This work has no runtime, hot-path, or
public-ABI change; timing and profile-specific forms remain outside this slice.

Implementation is complete. The value-owned JSON inventory carries all 205
stable table/ordinal form locators, the exact 126-mnemonic and 13-parallel-form
checkpoints, 35 catalog bindings, and 170 explicitly unimplemented forms. The
current bindings classify 25 forms as `COMPLETE` and ten fixed-MOD2 Smem forms
as `SUBSET`; covered forms are `COMMON_CANDIDATE`, while the remaining 170 keep
scope `UNRESOLVED`. The validator derives table counts from form records,
requires contiguous stable IDs, checks form/binding coverage bidirectionally,
and compares every binding with the sole production catalog without copying
opcode values or masks. Report arithmetic counts distinct form IDs, so aliases
cannot inflate progress. The checker is an explicit Make and CTest quality
gate, and 21 focused schema/count/binding/report tests are green.

Execution history is recorded in `docs/development-ledger.md`. This plan describes
current intent; the ledger records completed work and decisions without rewriting
history.

### P2-W41: fixed-MOD2 LD Smem,TS,dst vertical slice (planned)

This heading is the FIT/ownership anchor for the W41 activation. TI SPRU172C
§4.66 (printed pp. 4-65–4-69) defines the one-word `LD Smem,TS,dst` form with
opcode diagram `0001_010D_IAAAAAAA`. The project composes that form with the
registered fixed-MOD2 `*ARx+` rule, producing a row with value `0x1490`, mask
`0xFEF8`, D bit 8, ARF bits 2:0, and sixteen encodings. The existing
`ACCUMULATOR_ARF` layout and a combined
`STATUS_SXM+T_REGISTER` input mask are reused. `TS` is the signed six-bit
`T[5:0]` shift count with documented encodings 00..1F = 0..31 and 30..3F =
-16..-1; raw 20..2F is outside the documented range and must reject
`INVALID_OPERAND` transactionally.

The pure effect reads one WORD16 from the old selected-AR address before
postincrement, widens according to SXM, then applies the signed TS transform to
the canonical 40-bit accumulator result. Pinned boundary vectors include
`FEDC,T=8,SXM=1 -> 0xfffffedc00`; `8000,T=0x30 -> 0` (SXM=0) or
`0xffffffffff` (SXM=1); `T=0x3f -> 0x4000`/`0xffffffc000`; and
`T=0x1f,7fff -> 0xff80000000`. The transform is modulo-40 for positive left
shifts and arithmetic-right for negative shifts of the widened signed value.
TS is distinct from `SHIFT`/`ASM`; OVM and OVdst do not apply, and C, TC, OVM,
OVA/OVB, and unrelated state other than the selected destination accumulator
remain unchanged. Repeatability remains
`UNRESOLVED`; no timing, IRQ/RPT, device/MMR, public-ABI, or opcode-specific
CPU branch claim is admitted.

Four RED requirements/tests cover catalog, decode, pure effect, and generic CPU
retirement; the combined input mask is exercised within effect/CPU coverage.
Expected post-implementation closure is 35 catalog rows, 1503 populated
first-word encodings, ID_COUNT 36, five aggregate fixed-MOD2 memory rows, six
reset-runnable rows, inventory touched 35/205 (17.073%), and strict complete
25/205 (12.195%).

Implementation is complete. Four RED contracts first failed on the absent
opcode/resource/handler and unknown CPU instruction, then pass through one
catalog-derived handler and the generic fixed-MOD2 memory transaction. T[5:0]
valid counts cover 0..31 and -16..-1 with high T bits ignored; the project-only
0x20..0x2f zone rejects before a data-bus request and also rejects completion
transactionally. The source remains cohesive at 491 lines after explicit
review, below the 500-line split trigger; further growth requires a split.
Focused strict and ASan/UBSan tests, the 207-test quality gate, independent
semantic/integration reviews, and the authoritative standalone `make test`
gate are green.

### P2-W42A: standard-indirect MOD0–3 memory transaction seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W42A
activation. W42A is a generic sequencer/memory seam only: the catalog and all
opcode handlers remain unchanged. The existing
`SINGLE_INDIRECT_WORD16` memory effect accepts MOD0 through MOD3, with MOD3
read effects structurally rejected before address resolution or endpoint
issue. The W42B load-handler slice will select the `INVALID_OPERAND` status
before an endpoint request. The
resolver uses the existing planner and records address order as MOD0 `NONE`
(no AR commit), MOD1/MOD2 `POST`, and MOD3 `PRE` (writes only). `PRE` is
address-order metadata; it never exposes an auxiliary-register mutation before
the root transaction reaches `READY`.

The TI address-mode boundaries used to scope the seam are the existing
`E-TI-C54X-SPRU131G-S5.5.3-P5-13` and
`E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` records; they are not evidence for bus,
fault, or transaction behavior.

The root memory-instruction transaction remains atomic: `WAIT`, `FAULT`, and
cancel preserve architectural state, and `READY` commits the selected AR only
when the target AR commit and the program-counter/effect commits are all
accepted. No public ABI, machine-cycle/timing, device/MMR, CPU opcode branch,
new effect kind, new address kind, or new architectural state kind is admitted.
The three RED tests are owned by `src/isa/ops/memory`, `src/sequencer/memory`,
and `src/sequencer/instruction` respectively. Implementation is complete: each
RED contract now runs GREEN through the corresponding generic seam, covering
MOD0–MOD3 validation, address-order planning, and atomic WAIT/FAULT/cancel/
READY behavior. Focused strict and ASan/UBSan CTest execution, the 207-test
quality gate, independent semantic/integration reviews, the authoritative
standalone `make test`, the evidence checker, and repository diff validation
are green; no catalog row, opcode handler, or public ABI changed. A reviewed
retain decision keeps
`memory_instruction_transaction.c` cohesive at 523 lines as the root
transaction owner; candidate validation will be split if growth approaches
approximately 700 lines and must be split before 800 lines.

W42A is staged before W42B's opcode fan-out. W42B must make an explicit,
neutral symbol and MOD-layout decision before catalog rows or handlers are
added; its corrected candidate masks are `0xFEE0` and `0xFFE0`, not the earlier
incorrect values. The combined W42A/W42B lookup projection is 1503 → 1863
populated first words (+360 matched words), consisting of +272 legal encodings
and +88 known-but-rejected MOD3 reads. These lookup counts are planning
projections only: inventory percentages remain 35/205 (17.073%) catalog-family
progress and 25/205 (12.195%) strict complete-form coverage until complete
forms are actually bound and implemented.

### P2-W42B: standard-indirect MOD0–3 Smem family expansion (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W42B
activation. W42B broadens the ten existing fixed-MOD2 standard-indirect Smem
families by replacing, rather than adding, their `_MOD2` catalog rows with
neutral symbols: `LD_SMEM`, `LD_SMEM_TS`, `STL_SMEM`, `LDU_SMEM`,
`LD_T_SMEM`, `ST_IMMEDIATE_SMEM`, `LD_DP_SMEM`, `LD_ASM_SMEM`,
`LD_SMEM_SHIFT16`, and `ST_T_SMEM`. Their exact MOD0 bases and masks are,
respectively, `0x1080/0xFEE0`, `0x1480/0xFEE0`, `0x8080/0xFEE0`,
`0x1280/0xFEE0`, `0x3080/0xFFE0`, `0x7680/0xFFE0`, `0x4680/0xFFE0`,
`0x3280/0xFFE0`, `0x4480/0xFEE0`, and `0x8C80/0xFFE0`.

The broadened forms use explicit neutral layouts without changing existing
operand indices: `ACCUMULATOR_ARF_MOD` carries D/source, ARF, MOD;
`ARF_MOD` carries ARF, MOD; and `ARF_EXTENSION16_MOD` carries ARF,
EXTENSION16, MOD. Mode bits are [4:3], ARF is [2:0], and D is bit 8.
Handlers consume decoded mode fields and never parse raw instruction words.
Loads admit MOD0/MOD1/MOD2 and return `INVALID_OPERAND` for MOD3 before any
MEMORY effect or endpoint request; stores admit MOD0–MOD3 through the W42A
transaction seam. No new effect kind, address kind, CPU branch, public ABI,
device/MMR route, timing contract, or architectural state is added.

Status: COMPLETE. The five W42B requirements and executable tests are closed;
the implementation retains the project-contract boundary and makes no new
public ABI or timing promise.

The expansion retains `ID_COUNT=36` and projects the direct first-word lookup
from 1503 to 1863 (+360): 480 broadened encodings comprise 392 legal forms and
88 known-but-rejected load MOD3 forms, for +272 legal and +88 rejected words.
Inventory symbols become neutral while all ten remain `SUBSET` with
`addressing_subset=STANDARD_INDIRECT_MOD0_MOD3`; family progress remains
35/205 (17.073%) and strict complete-form coverage remains 25/205 (12.195%)
through W42B because other Smem addressing forms remain outside this slice.
The load implementation is split across `op_ld_smem.c` (~393 lines) and
`op_ld_smem_common.c` (~145 lines), keeping each module below the review
threshold. `operand_layout.c` is ~577 lines and remains cohesive for now; it
must be split before further growth approaches 800 lines. W42A completion and
lookup projections are preserved, and no duplicate/conflicting row semantics
are admitted.

### P2-W43A: generic indexed WORD16 MEMORY seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W43A
activation. W43A is a generic transaction prerequisite only; it does not add
catalog rows, opcode identities, handlers, or a CPU opcode branch. The private
address kind is `INDEXED_AR0_WORD16`, with an explicit MOD5/MOD6 discriminator.
The resolver reads CMPT and ARP, then the selected AR and AR0 from the copied
private `arch_state`; it reuses the existing indexed planner and candidate/
commit structures. MOD5 uses the old selected ARx as the WORD16 address and
schedules ARx−AR0 after access, rejecting ARx<AR0; MOD6 uses ARx+AR0 and
rejects unsigned overflow. Selected AR0 aliasing is rejected, and AR0 never
mutates. Both READ and WRITE are admitted by this seam.

The root transaction remains atomic: WAIT, FAULT, and cancel preserve all
architectural state; READY validates both selected-AR and AR0 snapshots and
commits the selected AR, program PC, and MEMORY effect together. No timing,
device/MMR, IRQ, RPT, wrap, machine-cycle, public-ABI, or new public CPU
snapshot claim is admitted. No new `op_input` or CPU snapshot field is planned.
The three RED lanes are project-contract tests for the MEMORY effect shape,
resolver, and root transaction, owned respectively by `src/isa/ops/memory/`,
`src/sequencer/memory/`, and `src/sequencer/instruction/`, with owner-local
proposed test files `test_indexed_word16_memory_effect.c`,
`test_indexed_word16_memory_resolver.c`, and
`test_indexed_word16_memory_transaction.c`; implementation should register
the CTest runners `ti_c54x.indexed_word16_memory_effect`,
`ti_c54x.indexed_word16_memory_resolver`, and
`ti_c54x.indexed_word16_memory_transaction` when each lane closes.

The existing `memory_instruction_transaction.c` root owner was 523 lines before
implementation; W43A split it into a 335-line root owner and a 268-line
candidate validator. The indexed resolver is 169 lines. Existing
indexed-addressing TI and project-contract evidence is reused; no new evidence
is admitted by activation.

Status: COMPLETE. RED→GREEN focused strict and ASan/UBSan CTest execution,
independent semantic/integration review, the authoritative standalone `make
test`, the evidence checker, and repository diff validation are green. No
public ABI, CPU snapshot, timing, device/MMR, IRQ, RPT, wrap, catalog,
opcode-handler, or new architectural-state claim was added; inventory coverage
is unchanged. The 335-line root, 268-line validator, and 169-line resolver are
retained as cohesive modules and will be reviewed before approaching the
repository's 500/800-line thresholds.

### P2-W43B: indexed MOD5/MOD6 per-family composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W43B
activation. TI SPRU131G Figure 5-7 and Table 5-3 (printed pp. 5-10–5-11)
establish the single-indirect composition of family opcode fields with D where
present, ARF bits [2:0], and MOD bits [4:3] under CMPT=0/ARP=0; Table 5-4
(printed p. 5-13) defines MOD5 as old ARx for the operand followed by ARx−AR0
and MOD6 as old ARx followed by ARx+AR0. The composition is pinned only for
the ten existing SPRU172C family diagrams/pages: `LD Smem,dst`, `LD
Smem,TS,dst`, `STL src,Smem`, `LDU Smem,dst`, `LD T,Smem`, `ST immediate,Smem`,
`LD Smem,DP`, `LD Smem,ASM`, `LD Smem,16,dst`, and `ST T,Smem`.

W43B freezes exactly twenty private IDs and unique wrapper tokens, one MOD5
and one MOD6 row per family: `LD_SMEM_MOD5/6`, `LD_SMEM_TS_MOD5/6`,
`STL_SMEM_MOD5/6`, `LDU_SMEM_MOD5/6`, `LD_T_SMEM_MOD5/6`,
`ST_IMMEDIATE_SMEM_MOD5/6`, `LD_DP_SMEM_MOD5/6`, `LD_ASM_SMEM_MOD5/6`,
`LD_SMEM_SHIFT16_MOD5/6`, and `ST_T_SMEM_MOD5/6`, with handler tokens using
the corresponding `_mod5`/`_mod6` suffix. Their exact base/mask pairs, in
the same family order, are `0x10A8/0xFEF8` and `0x10B0/0xFEF8`,
`0x14A8/0xFEF8` and `0x14B0/0xFEF8`, `0x80A8/0xFEF8` and `0x80B0/0xFEF8`,
`0x12A8/0xFEF8` and `0x12B0/0xFEF8`, `0x30A8/0xFFF8` and `0x30B0/0xFFF8`,
`0x76A8/0xFFF8` and `0x76B0/0xFFF8`, `0x46A8/0xFFF8` and `0x46B0/0xFFF8`,
`0x32A8/0xFFF8` and `0x32B0/0xFFF8`, `0x44A8/0xFEF8` and `0x44B0/0xFEF8`,
and `0x8CA8/0xFFF8` and `0x8CB0/0xFFF8`.

The three appended private decode layouts are `ACCUMULATOR_ARF_MOD4`,
`ARF_MOD4`, and `ARF_EXTENSION16_MOD4`; they preserve existing operand
indices while carrying the indexed mode discriminator. Each family wrapper
consumes decoded fields and delegates its existing pure transform to the W43A
`INDEXED_AR0_WORD16` MEMORY READ/WRITE seam, including old-AR addressing,
selected-AR post-update, and atomic WAIT/READY/FAULT/cancel behavior. No new
effect/address/state kind, public ABI, opcode-specific CPU branch, timing,
device/MMR, IRQ/RPT, wrap, or architectural-state claim is admitted.

Inventory remains audit metadata only: the ten subset bindings use the
multi-addressing token `STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6`, with family
and strict coverage percentages unchanged until complete-form evidence exists.
The planned catalog projection is `ID_COUNT=56` and 2,103 populated first-word
lookup entries (+240); these are implementation targets, not realized coverage.
The six RED lanes are owner-local project tests for catalog metadata/lookup,
decode/layout, indexed load effects, indexed store effects, CPU integration,
and inventory multi-binding quality. Planned CTest runners are
`ti_c54x.indexed_standard_indirect_metadata`,
`ti_c54x.indexed_standard_indirect_decode`,
`ti_c54x.indexed_standard_indirect_load_effects`,
`ti_c54x.indexed_standard_indirect_store_effects`,
`ti_c54x.cpu_indexed_standard_indirect`, and the existing
`ti_c54x.quality.python` discovery runner for the inventory lane; each test
path and owning module is recorded in the evidence registry.

Implementation closure: all six W43B RED lanes are GREEN. The catalog,
decode/layout, indexed load, indexed store, CPU integration, and inventory
multi-binding fixtures passed their recorded runners; a fresh ASan+UBSan
configure/build passed the five focused CTests (5/5) with no diagnostics, and
the inventory Python lane passed 3/3. The catalog projection
is ID_COUNT=56 with 2,103 populated first-word lookup entries and 55 bindings;
the ten indexed form bindings retain the
`STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6` addressing token, so inventory family
coverage remains 35/205 (17.073%) and strict complete-form coverage remains
25/205 (12.195%). The implementation fixed the stale W42B/legacy ID_COUNT
fixtures and added the TS pure prevalidation path before the memory endpoint;
these corrections are regression fixtures, not new ISA scope. Operand
ownership review retains `src/isa/decode/operand_layout.c` at 620 lines as a
cohesive owner and requires a split before 800 lines. The current load/store
owners remain below that threshold: `op_ld_smem.c` 393 lines,
`op_ld_smem_common.c` 308, `op_ld_smem_indexed.c` 317, and
`op_store_indexed_common.c` 243. No public ABI, new effect/address/state
kind, CPU branch, timing, device/MMR, or IRQ/RPT expansion was added by this
closure; sanitizer output is validation evidence only.

Closure accounting is 165 evidence records; 263 requirements (226
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 217 tests (209 PASSING, 8 PLANNED);
and 7 sources. Inventory coverage remains 35/205 = 17.073% catalog-family
progress and 25/205 = 12.195% strict complete-form coverage. W43A seam reuse
is the only address/effect integration admitted; no new ABI, timing, device,
CPU branch, or architectural state is planned.

### P2-W44A: generic circular WORD16 MEMORY seam (planned)

This immutable `(planned)` heading remains the FIT/ownership anchor for the
W44A activation. W44A is now implementation-complete. The private typed
`CIRCULAR_WORD16` specification admits CMPT=0 WORD16
MOD8/MOD10 reads and writes without catalog rows or opcode handlers. The
operation-layer spec carries only copied ARF and typed circular-mode inputs;
the resolved address effect carries copied AR/BK snapshots. Resolution reads
dependencies in strict CMPT, ARP, selected AR, then BK order, admits selected
AR0–AR7, preserves BK=0 as a valid NONE/no-update result, and applies the
registered positive-BK wrap domain. Unknown or stale dependencies reject.
The root candidate and existing generic transaction preserve WAIT, READY,
FAULT, and cancel transactionality, with READY atomically committing any
selected-AR update, sequential PC, and completed effect; no root transaction
source change was needed.

The three owner-local RED contracts are separated across
`src/isa/ops/memory/`, `src/sequencer/memory/`, and
`src/sequencer/instruction/`, with tests mirrored under those owners. RED then
GREEN validation passed the three recorded C executable runners:
`ti_c54x.circular_word16_memory_effect`,
`ti_c54x.circular_word16_address_resolution`, and
`ti_c54x.circular_word16_memory_transaction` (each via `make_goal=test`).
The implementation does not claim LONG32 scaling, MOD9/MOD11, MOD14,
CMPT=1/dual-indirect, initial-boundary policy beyond the existing planner, BK
MMR routing, timing, device/MMR, IRQ/RPT, public ABI, or catalog scope.

Current owner sizes are `memory_effect.c` 78 lines,
`address_resolution.c` 222 lines, `memory_instruction_candidate_validate.c`
316 lines (confirmed current count), and
`memory_instruction_transaction.c` 335 lines unchanged. No split is needed;
the candidate validator remains under the 500-line review threshold and must
be reviewed before crossing it.

W44B remains the later twenty-row MOD8/MOD10 family fan-out, projected at
`ID_COUNT=76`, 2,343 populated first-word lookup entries, and 75 catalog
bindings. Those projection values are intentionally excluded from W44A's
completed implementation and coverage accounting.

Closure accounting is 166 evidence records; 266 requirements (229
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 220 tests (212 PASSING, 8 PLANNED);
and 7 sources. Catalog identity remains ID_COUNT=56 with 2,103 populated
first-word lookup entries and 55 bindings; inventory coverage remains 35/205 =
17.073% catalog-family progress and 25/205 = 12.195% strict complete-form
coverage.

### P2-W44B: circular MOD8/MOD10 per-family composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W44B
activation. Figure 5-7 and Table 5-3 establish the common single-indirect
field composition, while Table 5-4 fixes MOD8 as old ARx followed by circular
ARx−1 and MOD10 as old ARx followed by circular ARx+1 for fixed WORD16 steps.
Composed only with the ten existing SPRU172C Smem diagrams, the bounded rows
are exactly:

`LD_SMEM_MOD8/10` `0x10C0/0x10D0` mask `0xFEF8`;
`LD_SMEM_TS_MOD8/10` `0x14C0/0x14D0` mask `0xFEF8`;
`STL_SMEM_MOD8/10` `0x80C0/0x80D0` mask `0xFEF8`;
`LDU_SMEM_MOD8/10` `0x12C0/0x12D0` mask `0xFEF8`;
`LD_T_SMEM_MOD8/10` `0x30C0/0x30D0` mask `0xFFF8`;
`ST_IMMEDIATE_SMEM_MOD8/10` `0x76C0/0x76D0` mask `0xFFF8`;
`LD_DP_SMEM_MOD8/10` `0x46C0/0x46D0` mask `0xFFF8`;
`LD_ASM_SMEM_MOD8/10` `0x32C0/0x32D0` mask `0xFFF8`;
`LD_SMEM_SHIFT16_MOD8/10` `0x44C0/0x44D0` mask `0xFEF8`; and
`ST_T_SMEM_MOD8/10` `0x8CC0/0x8CD0` mask `0xFFF8`. Handler tokens use the
corresponding `_mod8`/`_mod10` suffixes.

W44B is now implementation-complete. RED-first catalog and decode contracts
initially failed on absent MOD8/MOD10 identities, and the inventory contract
reported 55 rather than 75 bindings; the corrected catalog, generated lookup,
inventory, load/store wrappers, and CPU integration are GREEN. The catalog
remains at the exact 76 IDs with 2,343 populated lookup entries and 75
bindings. Family coverage remains 35/205 (17.073%) and strict complete-form
coverage remains 25/205 (12.195%).

W44B reuses the existing `ACCUMULATOR_ARF_MOD4`, `ARF_MOD4`, and
`ARF_EXTENSION16_MOD4` layouts and the completed W44A CIRCULAR_WORD16 MEMORY
effect, resolver, and transaction seams. Selected AR0–AR7 remain admitted;
resolution orders CMPT, ARP, selected AR, then BK, with BK=0 as NONE/no-update
and BK>0 as the positive-BK POST-access wrap update. Inventory remains audit
metadata only: the ten stable forms use
`STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6_MOD8_MOD10`, with family and strict
coverage unchanged at 35/205 (17.073%) and 25/205 (12.195%). No
`operand_layout.c` edit is planned; a new status-load circular file and new
store circular common file are preferred owner splits, while central catalog
and inventory edits remain serialized.

Unsupported and out of scope are MOD9/MOD11, MOD14, CMPT=1/dual-indirect,
LONG32 circular scaling, initial-boundary policy beyond the registered planner,
timing/repeatability, BK MMR/device routing, public ABI, new architectural
kinds, IRQ/RPT, and broader catalog scope. Six project requirements and
owner-local tests cover catalog, decode, seven load effects, three store
effects, CPU integration, and inventory multi-binding; all six now have
passing runners. The corrected near-miss fixtures replace stale 0x0040
unknown-word assumptions with explicit circular-sibling expectations. The common load and LD_T load
implementation is 472 lines and must not grow further without a split; the
original status-load file remains 482 lines and the circular status path is
split into its dedicated file. `operand_layout.c` remains 620 lines with the
prior split-before-800 decision.

Closure accounting is 168 evidence records; 272 requirements (235
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 226 tests (218 PASSING, 8 PLANNED);
and 7 sources. The authoritative standalone `make test` runner is GREEN after
the full masked-row near-miss audit.

### P2-W45A: generic reverse-carry WORD16 MEMORY seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W45A
activation. W45A is now implementation-complete. Existing registered TI
MOD4/MOD7 and reverse-carry primitive, planner, and application records are reused. W45A freezes a private typed
`REVERSE_CARRY_WORD16` memory address kind for MOD4/MOD7; ARF admits only 1..7
and AR0 aliasing remains unresolved/rejected. Unrelated fields are canonical
zero and prevalidation completes before architectural reads.

Resolution is ordered CMPT, ARP, selected AR, then AR0; AR0 must be known,
nonzero, and one-hot. The planner uses the old selected AR as the operand
address and stages a POST update to the selected AR only. No modulo or wrap
inference is made; boundary propagation that escapes the documented domain is
rejected. Candidate validation requires exact live snapshots, with WAIT,
FAULT, cancel, stale, and no-refetch behavior preserved and READY committing
the selected-AR update, sequential PC, and MEMORY effect atomically.

W45A owns only the private memory effect, address-resolution, and candidate
validation seams. It adds no BK, root transaction, CPU, catalog, opcode, public
ABI, timing, device, or RPT behavior. Three project requirements/tests
cover effect shape, address resolution, and memory transaction ownership. RED
replay covered an absent kind/field compile failure, valid effect rejection by
the resolver, and transaction prepare rejection; all three are now GREEN.
The production sizes are `memory_effect.c` 95 lines, `memory_effect.h` 76,
`address_resolution.c` 293, `memory_instruction_candidate_validate.c` 354,
its header 29, and tests 219/411/426 lines. Review closure strengthened
rejection output zeroing, forged commit rejection, and module-contract wording.
No root transaction, CPU,
catalog/opcode, public ABI, timing, device, or RPT change was made.
W45B remains a later twenty-row MOD4/MOD7 fan-out, projected at ID_COUNT=96,
lookup 2,583, and 95 bindings; it is not activated here.

Closure accounting is 169 evidence records; 275 requirements (238
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 229 tests (221 PASSING, 8 PLANNED);
and 7 sources. Fresh ASan/UBSan validation passed 15/15, independent reviews
found no production defect after their test/contract findings were closed, and
the authoritative standalone `make test` gate is GREEN. W45B is the next
audited but inactive reverse-carry fan-out.

### P2-W45B: reverse-carry MOD4/MOD7 per-family composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for W45B. The
ten existing Smem families compose exactly twenty private MOD4/MOD7 rows:

`LD_SMEM_MOD4/7` `0x10A0/0x10B8` mask `0xFEF8`;
`LD_SMEM_TS_MOD4/7` `0x14A0/0x14B8` mask `0xFEF8`;
`STL_SMEM_MOD4/7` `0x80A0/0x80B8` mask `0xFEF8`;
`LDU_SMEM_MOD4/7` `0x12A0/0x12B8` mask `0xFEF8`;
`LD_SMEM_SHIFT16_MOD4/7` `0x44A0/0x44B8` mask `0xFEF8`;
`LD_T_SMEM_MOD4/7` `0x30A0/0x30B8` mask `0xFFF8`;
`ST_IMMEDIATE_SMEM_MOD4/7` `0x76A0/0x76B8` mask `0xFFF8` (two words);
`LD_DP_SMEM_MOD4/7` `0x46A0/0x46B8` mask `0xFFF8`;
`LD_ASM_SMEM_MOD4/7` `0x32A0/0x32B8` mask `0xFFF8`; and
`ST_T_SMEM_MOD4/7` `0x8CA0/0x8CB8` mask `0xFFF8`. Handler tokens use
`_mod4`/`_mod7` suffixes and the existing MOD4 layouts/resources/word counts.

W45B reuses the typed W45A reverse-carry seam with CMPT=0/ARP=0, known
nonzero one-hot AR0, old selected-AR address, and POST selected-AR update.
ARF1..7 executes; ARF0 encodings decode but handlers reject transactionally
before endpoint issue because alias semantics remain unresolved. No BK or
wrap inference is admitted, and no operand-layout, root-transaction, CPU
branch, public ABI, timing, device, or RPT change is included. Inventory uses
`STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`; family and strict
coverage remain 35/205 (17.073%) and 25/205 (12.195%).

Closure accounting is 170 evidence records; 281 requirements (244
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 235 tests (227 PASSING, 8 PLANNED);
and 7 sources. W45B's six owner-local RED lanes are GREEN: the authoritative
standalone `make test` is GREEN, quality validation covers 215 tests, fresh
post-fix ASan/UBSan validation is 5/5 without diagnostics, and independent
review is clean. No new knob was added; the family and strict coverage remain
35/205 (17.073%) and 25/205 (12.195%).

### P2-W46A: generic WORD16 same-address read-modify-write seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for W46A. The
seam remains private and value-owned inside the existing
`TI_C54X_OP_EFFECT_MEMORY` carrier: a compact typed transform enum (`AND`,
`OR`, `XOR`) and copied WORD16 operand describe one same-address read followed
by one write. No indirect callback, allocation, new effect kind, CPU opcode
branch, or public ABI is admitted. Address resolution runs once; the cached old
address is reused for both bus phases, each phase receives a fresh transaction
ID, and the transform runs exactly once on the copied read word.

The transaction must preserve WAIT/FAULT/cancel and stale semantics. Resume
paths validate direct live snapshots (CMPT/ARP, selected AR, and AR0/BK when
the selected mode needs them, plus PC) instead of rerunning the planner or
rereading the endpoint. Architectural address/PC effects commit only after
WRITE READY. This is core architectural commit atomicity; exclusion against
an external bus master between the read and write is explicitly unsupported
until separately evidenced. Helpers are split before the 500-line review
threshold.

Performance is part of the seam contract: keep the carrier/candidate fixed-size
where feasible, retain an allocation-free hot path, and add a structural
one-read/one-write gate. W46B owns the later executable observer-off RMW
benchmark. That benchmark is non-history and must not redefine the five frozen
performance scenarios or claim machine-cycle timing.

Implementation preserves the 40-byte MEMORY carrier and 48-byte generic effect
on supported 64-bit ABIs (36/40 bytes on supported 32-bit ABIs). The root
transaction remains 464 lines after splitting value transforms and paired-write
construction into a private helper. Address resolution runs once; READY/WAIT,
FAULT, cancel, stale-state, candidate-forgery, endpoint-callback mutation, and
second-ID exhaustion paths are executable. Direct live validation never calls
the address resolver, and AR/PC publish only after WRITE READY. The cohesive RMW
transaction test is about 600 lines; it has been reviewed and remains unsplit
for this single lifecycle contract, with a mandatory split before 800 lines.

Closure accounting is 175 evidence records; 290 requirements (253
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 244 tests (236 PASSING, 8 PLANNED);
and 7 sources. W46A's two focused C runners and six-test structural performance
lane are GREEN. W46B's five focused C/Python runners, dedicated benchmark quality
contract, clean baseline artifact, and baseline comparison are GREEN. No
external-master exclusion or TI timing claim is added.

### P2-W46B: ANDM/ORM/XORM neutral MOD0–3 composition (planned)

This immutable `(planned)` heading remains the FIT/ownership anchor for W46B.
The primary TI SPRU172C instruction pages are ANDM printed p4-13, ORM p4-125,
and XORM p4-203. The implemented catalog rows are
`ANDM_SMEM_MOD0_3` base `0x6880`, `ORM_SMEM_MOD0_3` base `0x6980`, and
`XORM_SMEM_MOD0_3` base `0x6A80`, all mask `0xFFE0`, two program words,
existing `ARF_EXTENSION16_MOD` layout, no input resources or status effects,
and ineligible for repeatability.

MOD0/1/2 are admitted under the current safe boundary policy. MOD3 decodes for
metadata but the handlers reject it before endpoint issue because these forms
require a read and MOD3 is write-only. The rows reuse W46A and generic decode,
memory effects, CPU retirement, and audit-only inventory binding; all five
implementation requirements and their focused runners are green. Direct,
absolute, long-offset, CMPT1, indexed, circular, reverse, MMR/device, wrap,
and timing expansion remain deferred. The projection is three additional
rows/96 populated first-word entries: ID_COUNT=99, lookup=2,679, bindings=98;
inventory family progress is 38/205 (18.537%) while strict complete-form
coverage remains 25/205 (12.195%). The dedicated observer-off benchmark cohort
has an executable, capture, comparator, quality contract, clean baseline artifact,
and baseline-relative comparison. The baseline is
`bench/audits/p2-w46b-rmw-baseline.json` (semantic hash `3b39187c40a4fd43`);
the benchmark remains advisory and makes no TI timing claim.

### P2-W47A: generic LONG32 pair transaction seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W47A
activation. W47A is a generic sequencer-only forcing function: it adds no
catalog row, opcode handler, CPU branch, public ABI, or MEMORY effect-carrier
field. It reuses the canonical LONG32 descriptor with `{first=MSW,
second=LSW}` ordering and the private WORD16 reducer, so the pair seam proves
multiword lifecycle behavior without widening an existing architectural
carrier.

The activation contract freezes these decisions before implementation:

- Request order is deterministic: first/MSW, then second/LSW. Descriptors and
  values are copied into the fixed-size private reducer. Each word receives a
  caller-supplied fresh, nonzero, strictly increasing transaction ID;
  `WAIT` resumes the same request with the same ID.
- A first-word fault issues no second request. A second-word fault produces no
  pair read result and no architectural commit. Cancel applies only to the
  active word; a failed cancel preserves reducer state.
- Reads publish only after both words are `READY`. Existing DATA endpoints may
  make each write externally visible: the first write is not rolled back when
  the second fails or is cancelled, while later architectural commit remains
  deferred. W47A makes no external-master exclusion or all-or-none physical
  write claim.
- Malformed, noncanonical, or overlapping pairs reject before endpoint issue.

W47A is implementation-complete. Its private reducer and validation split
preserve the canonical MSW→LSW order, copied descriptors/values, caller-supplied
fresh increasing IDs, same-ID WAIT, first/second FAULT behavior, active-word
cancellation, read publication only after both READY observations, and the
existing endpoint's necessarily partial write visibility. The first write is
not rolled back if the second faults or is cancelled; no external-master
exclusion or all-or-none physical-write claim is made. Malformed,
noncanonical, and overlapping pairs reject before issue.

The four RED lanes are GREEN: shape, read lifecycle, write lifecycle, and
structural hot-path performance. The reducer is allocation-free, fixed-size,
never replans, and constructs exactly two child requests (one in `prepare`, one
in the first-word transition); canonical validation uses direct field checks.
The quality/performance lane is 6/6; native LP64 freezes the request and
transaction at 32/88 bytes, and supported ILP32 at 32/80 bytes. Focused strict
sanitizer validation is 3/3 and the authoritative standalone full gate is
GREEN. No catalog, opcode, CPU, public-ABI, or effect-carrier change was made.
An executable opcode benchmark is deferred to W47B.

W47A closure accounting is 176 evidence records; 294 requirements (257
`IMPLEMENTED`, 8 `PLANNED`, 29 `UNRESOLVED`); 248 tests (240 `PASSING`, 8
`PLANNED`); and 7 sources. The current ISA snapshot remains 98 catalog rows,
2,679 populated first-word entries, 38/205 family progress, and 25/205
strict-complete forms.

### P2-W47B: DLD/DST LONG32 MOD13 composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W47B
activation. The TI DLD and DST entries define the long-word read/write families
and their one-extension-word long-offset forms. Composed with the pinned
CMPT=0 MOD13, LONG32 delta, word-pairing, and repeat-policy records, this slice
freezes exactly two two-word rows: `DLD_LONG32_MOD13` base `0x56E8`, mask
`0xFEF8`, and `DST_LONG32_MOD13` base `0x4EE8`, mask `0xFEF8`. Their full
patterns are `0101_011D_1110_1AAA` and `0100_111S_1110_1AAA`; D/S is bit 8,
ARF is bits 2:0, MOD13 is bits 6:3, and the second word is the copied `lk`
extension. Each admits sixteen ARF encodings, uses the new private
`ACCUMULATOR_ARF_EXTENSION16_MOD4` layout, and is ineligible for single-repeat
execution. The activation does not claim TI bus order, external pair atomicity,
rollback, or machine-cycle timing.

DLD consumes `STATUS_SXM` and reads first/MSW then second/LSW into `dst31:0`;
SXM=0 clears `dst39:32`, while SXM=1 sign-extends bit 31. C16 is not an input
resource; project vectors must prove the C16×SXM lane descriptions converge on
the same stored 40-bit pattern. DST consumes `ACCUMULATORS`, `STATUS_SXM`, and
`STATUS_SST`; the full source remains unchanged and SST/SXM saturation is
applied before first/MSW then second/LSW store. SST=0 stores source bits31:0;
SST=1 clamps according to the pinned raw-40/signed-40 rules. Unknown SST or
SXM rejects transactionally before endpoint issue.

W47B composes both rows over the private W47A reducer and generic MOD13
LONG32 MEMORY path. A fixed-size discriminated child union copies operands and
source snapshots, issues exactly two children, and never replans, refetches, or
recomputes source values. Pre-access AR/PC candidates commit only after pair
completion; per-word physical write visibility remains the W47A contract. Static
completion callbacks may remain process-lifetime function pointers but own no
mutable context, formatting, or fan-out. Root and validator helpers split before
file-size thresholds, and an observer-off full-CPU benchmark is required.

W47B1 is implementation-complete for the generic fixed-size LONG32 MEMORY
carrier, checked CMPT=0 MOD13 resolver, and discriminated root transaction.
Focused and sanitizer matrices are 10/10 GREEN. Native LP64 freezes the address
specification/effect/root at 24/40/336 bytes; supported ILP32 freezes them at
24/36/308. The root issues two copied requests directly through the pair reducer,
preserves active IDs and the sequence watermark across WAIT/fault/cancel/stale
paths, and commits AR/PC only after both words are READY. The established WORD16
path does not call the LONG32 preparation helper. The 494-line root and 782-line
cohesive lifecycle test are closed to further W47B growth; W47B2 owns its opcode,
semantic, CPU, quality, and benchmark work in their existing modules.

W47B1 accounting is 181 evidence records; 304 requirements (259 `IMPLEMENTED`,
16 `PLANNED`, 29 `UNRESOLVED`); 258 tests (242 `PASSING`, 16 `PLANNED`); and 7
sources. Catalog and inventory counts remain unchanged until W47B2 activates the
two production rows.

W47B2A completes the typed PMST.SST state/input prerequisite without widening
the 48-byte copied opcode-input snapshot on LP64 or ILP32. Common reset leaves
SST unknown; checked typed writes establish knownness; collection keeps SST and
SXM independently selectable and stages output transactionally. Unknown-status
rejection before endpoint issue remains in the DLD/DST semantic and CPU slices.
Accounting is 181 evidence records; 304 requirements (260 `IMPLEMENTED`, 15
`PLANNED`, 29 `UNRESOLVED`); 258 tests (243 `PASSING`, 15 `PLANNED`); and 7
sources. Catalog, lookup, binding, and ISA coverage counts remain unchanged.

W47B2B activates the exact two-word DLD/DST MOD13 rows and their four-field
layout. Checked semantics validate complete decode; trusted semantics consume
the single pre-extracted D/S, ARF, `lk`, and MOD13 packet without resolving or
extracting the layout again. DLD publishes canonical SXM zero/sign extension
only after its MSW/LSW completion. DST snapshots and saturates the selected
raw-40 accumulator once before staging copied MSW/LSW writes. Inventory now
binds both rows as bounded MOD13 subsets: 100 catalog rows, `ID_COUNT=101`,
2,711 lookup entries, 100 bindings, 40/205 family coverage, and unchanged
25/205 strict coverage. Accounting is 181 evidence records; 304 requirements
(264 `IMPLEMENTED`, 11 `PLANNED`, 29 `UNRESOLVED`); 258 tests (247 `PASSING`,
11 `PLANNED`); and 7 sources. CPU READY/WAIT/FAULT execution and the
observer-off benchmark remain W47B2C.

W47B2C closes the activation with full public-CPU execution proofs. DLD and DST
each cover READY, first/second WAIT and first/second FAULT, two-word fetch,
stable resume identity, fresh paired transaction IDs, no refetch, deferred
AR/PC publication, and parity-derived LONG32 word order. DLD additionally
proves D/S selection, SXM lanes, unknown-SXM rejection, and C16 independence;
DST proves copied raw-40 sources, the SST/SXM saturation boundary matrix,
unknown-source/status rejection, and first-write physical visibility when the
second write faults. The dedicated observer-off benchmark runs both exact rows
through the public CPU with semantic counts and ordering checked around every
timed window. Its capture schema records host/compiler/build provenance and
timing spread but intentionally freezes no host threshold or TI timing claim.
Structural tests retain single trusted-packet extraction, static dispatch,
allocation/I/O/environment exclusion, and LP64/ILP32 carrier sizes. Accounting
is 181 evidence records; 304 requirements (267 `IMPLEMENTED`, 8 `PLANNED`, 29
`UNRESOLVED`); 258 tests (250 `PASSING`, 8 `PLANNED`); and 7 sources. Catalog,
lookup, binding, and ISA-coverage counts remain 100, 2,711, 100, 40/205, and
25/205 respectively.

Inventory rows `FORM-T2-19-01` (DLD) and `FORM-T2-20-01` (DST) now bind the
production symbols as `LONG_OFFSET_MOD13_CMPT0` subsets. Strict complete
coverage remains unchanged because each row implements only part of its full
mnemonic family.

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

### P2-W33: CMPR CC,ARx vertical slice (planned)

This implementation-complete bounded non-memory ISA slice has green focused,
review, sanitizer, quality, and final full-gate validation; full integration
caught and corrected two stale UNKNOWN near-miss fixtures. Its evidence boundary is the
registered TI `SPRU172C` CMPR entry (printed p. 4-34) plus the registered
repeatability boundary in §2.5/Table 2-28 (printed pp. 2-19–2-21). CMPR is a
one-word `CC,ARx` form with value `0xF4A8`, mask `0xFCF8`, `CC` in bits 9:8,
and `ARx` in bits 2:0, yielding 32 encodings. `CC` selects unsigned EQ, LT,
GT, or NEQ comparison of selected ARx against AR0. `TC` is set to one iff the
condition is true and otherwise cleared; every other raw and known status bit
is preserved. CMPR is explicitly nonrepeatable (`INELIGIBLE`).

The project fit is intentionally narrow. `src/cpu/input/` copies a generic
`AUXILIARY_REGISTERS` snapshot containing all eight `uint16_t` AR values and
per-register knownness, collected only through existing checked architectural
state reads. The pure handler returns `UNKNOWN_INPUT` unless AR0 and selected
ARx are known. It emits the existing `STATUS_BIT_WRITE` effect targeting ST0
bit 12, and the generic CPU path retires it without an opcode-specific branch.
All other state is preserved transactionally. No public API, memory, timing,
IRQ/RPT, device, or environment surface is introduced.

TDD ownership is split across catalog, decode, CPU-input, compare-handler, and
CPU retirement tests under mirrored paths. The catalog/decode tests prove the
32 masked encodings and CC/ARx fields; the input test proves all-eight AR
snapshot copying and unknownness; the pure-effect test proves unsigned
comparison and TC-only mutation; the CPU test proves generic retirement and
PC progression. Any need for a new status policy, memory, interrupt/repeat,
parallel issue, timing, public state, or opcode-specific CPU branch is a GAP
report rather than a CMPR workaround. Expected post-slice coverage is 27 rows,
1415 populated first words, and 6/27 reset-runnable; those counts are now
realized. W31P hosted stability and W31B2 immediate-READY benchmark follow-ups
remain open.

### P2-W34: fixed-MOD2 LDU vertical slice (planned)

This implemented bounded memory ISA slice has an evidence boundary of the
registered TI `SPRU172C` LDU entry (printed p. 4-79), the registered standard
indirect MOD2 rule in `SPRU131G` §5.5.3 (printed p. 5-13), and the registered
Table 2-28 repeatability boundary (printed pp. 2-19–2-21). LDU is a one-word
`*ARx+,dst` form derived from opcode diagram `0001_001D_IAAAAAAA`, with value
`0x1290`, mask `0xFEF8`, `D` at bit 8, `ARF` at bits 2:0, 16 encodings, and
the `ACCUMULATOR_ARF` layout. It has no input resources. Repeatability remains
`UNRESOLVED`: absence from the forbidden list does not establish eligibility.

The project fit reuses the W31B fixed-MOD2 MEMORY effect/completion and generic
CPU retirement contract. CMPT=0 and ARP=0 are frozen; the old selected AR is
used for transfer before its postincrement, and the checked planner continues
to reject AR=0xffff. LDU loads Smem into destination bits 15:0, clears bits
39:16, ignores SXM, and reports no status-bit effects. No new resource, effect,
or address kind is introduced, and no public ABI, opcode-specific CPU branch,
timing, IRQ/RPT, device/MMR, environment, diagnostics, or allocation behavior
is claimed. The open W31P hosted stability and W31B immediate-READY benchmark
captures remain nonblocking follow-ups.

TDD ownership was split across catalog, decode, pure LDU MEMORY
effect/completion, and generic CPU retirement tests under mirrored paths; all
four executable contracts are now passing. Any
need for an SXM read, a new memory/address seam, or a repeatability decision is
a GAP report rather than an LDU workaround. Catalog coverage is now 28 rows,
1431 populated first words, and 6/28 reset-runnable. Repeatability remains
unresolved.

### P2-W35: fixed-MOD2 LD-to-T vertical slice (planned)

This implemented evidence-fit slice is bounded by the TI `SPRU172C` §4.70 LD
entry (printed pp. 4-70–4-71), the registered standard-indirect MOD2 rule in
`SPRU131G` §5.5.3 (printed p. 5-13), and the registered Table 2-28
repeatability boundary (printed pp. 2-19–2-21). The one-word `LD Smem,T` form
uses opcode diagram `0011_0000_IAAAAAAA`; after fixed-MOD2 composition its row
is value `0x3090`, mask `0xFFF8`, with bit 8 fixed zero, ARF bits 2:0, and
exactly eight encodings. It requires a private generic
`TI_C54X_OPERAND_LAYOUT_ARF` decode layout and no input resources. Repeatability
is `UNRESOLVED`: absence from Table 2-28's forbidden list is not positive
eligibility evidence, and the separately forbidden fixed long-offset or
absolute forms are excluded.

The project fit reuses the W31B fixed-MOD2 MEMORY effect/completion and the
existing `T_REGISTER_WRITE` effect. Under CMPT=0 and ARP=0, the old selected AR
WORD16 is transferred into T before its postincrement; the checked planner's
AR=0xffff boundary remains in force. The `*AR3+` example transfers data
`0xFEDC` from the old AR3 address `0x0300`, then advances AR3 to `0x0301`. No
public T/MMR API, new resource,
address kind, CPU opcode branch, timing, IRQ/RPT, device, environment,
diagnostic, or allocation behavior is claimed.

TDD ownership is split across catalog, decode, pure load MEMORY completion, and
generic CPU retirement tests under mirrored paths; all four executable contracts
are passing. `src/isa/decode/` owns the new private layout contract; no family
workaround may hide a shared seam. The current `operand_layout.c` is 518 lines
and remains cohesive, so the reviewed no-split decision keeps it well below the
800-line threshold. Realized coverage is 29 rows, 1439 populated first words,
and 6/29 reset-runnable. Repeatability remains unresolved. W31P hosted stability
and W31B immediate-READY benchmark follow-ups remain open and nonblocking.

### P2-W36: fixed-MOD2 immediate-store vertical slice (planned)

State: implementation-complete. Focused catalog/decode/effects/CPU contracts,
sanitizer checks, quality, independent review, and the full standalone gate are
green.

This activated evidence-fit slice is bounded by the TI `SPRU172C` Store
T/TRN/immediate entry (printed pp. 4-166–4-167), the registered
standard-indirect MOD2 rule in `SPRU131G` §5.5.3 (printed p. 5-13), and the
registered Table 2-28 repeatability boundary (printed pp. 2-19–2-21). Syntax 3
defines two-word `ST #lk,Smem` with opcode diagram
`0111_0110_IAAAAAAA`; fixed-MOD2 composition yields value `0x7690`, mask
`0xFFF8`, fixed bit 8, ARF bits 2:0, and exactly eight first words. A planned
private `TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16` layout extracts that ARF and
copies word 1 as the raw 16-bit `lk`. Repeatability remains `UNRESOLVED`:
absence from Table 2-28 is not positive eligibility evidence, and its separate
long-offset or absolute prohibition does not decide this `*ARx+` form.

The project fit reuses generic extension fetch, width-aware sequential PC,
fixed-MOD2 address resolution, and the W31B MEMORY transaction. The pure store
leaf stages a WORD16 WRITE with `write_value_word=lk` and `complete=NULL`.
Under CMPT=0 and ARP=0 the write targets the old selected AR address; READY
then commits postincrement and PC+2, WAIT resumes the same transaction without
refetch, and FAULT publishes neither architectural change. No public ABI,
opcode-specific CPU branch, new input resource, effect kind, address kind,
timing, IRQ/RPT, device/MMR, environment, diagnostic, or allocation behavior
is authorized.

TDD ownership is split across catalog, decode, pure immediate-store effect,
and generic CPU retirement tests under mirrored paths; all four executable
contracts are passing. The current `operand_layout.c` is 535 lines and remains
cohesive; the two-field descriptor has a recorded no-split review and remains
well below the 800-line split threshold. Realized coverage is 30 rows, 1,447
populated first words, and 6/30 reset-runnable. `LD Smem,DP/ASM` remains a later strict-fit
candidate. `ST T`, `STH`, and the audited ALU/control forms remain blocked by
their T-input, saturation/OVM/OV, or PC/SP ownership gaps. P2-W31P hosted
stability and the W31B immediate-READY benchmark follow-ups remain open and
nonblocking.

### P2-W37: fixed-MOD2 memory-to-status vertical slice (planned)

This activated evidence-fit slice is bounded by the TI `SPRU172C` §4.70 LD
entry (printed pp. 4-69–4-71), the registered standard-indirect MOD2 rule in
`SPRU131G` §5.5.3 (printed p. 5-13), and the registered Table 2-28
repeatability boundary (printed pp. 2-19–2-21). The one-word `LD Smem,DP` and
`LD Smem,ASM` forms use the existing fixed-MOD2 `ARF` layout: values `0x4690`
and `0x3290`, mask `0xFFF8`, fixed bit 8, ARF bits 2:0, and exactly eight
encodings each. No `LD Smem,ARP` syntax is registered. Both forms have no input
resources. Table 2-28 makes DP repeatability `INELIGIBLE`; ASM repeatability is
`UNRESOLVED` because absence is not positive eligibility evidence.

The project fit reuses the W31B MEMORY WORD16 read/completion, existing
status-field effect/writer, old-AR-address read before postincrement, and generic
CPU WAIT/READY/FAULT and sequential-PC paths under CMPT=0 and ARP=0. Completion
masks the copied read to `0x01ff` before `STATUS_FIELD_WRITE` DP or to `0x001f`
before `STATUS_FIELD_WRITE` ASM. The checked planner's AR=0xffff boundary
remains in force. No new layout, resource, effect, address kind, CPU branch,
public API, timing, IRQ/RPT, device/MMR, environment, diagnostics, or allocation
behavior is claimed. The current `operand_layout.c` is 535 lines and remains
unchanged; its reviewed no-split decision remains below the 800-line threshold.

TDD ownership is split across catalog, decode, pure load/status-field completion,
and generic CPU retirement tests under mirrored paths; all four executable
contracts pass through strict and sanitizer runners, with quality and clean dual
review green. The implementation preserves unique catalog handler tokens through
thin DP/ASM wrappers over one shared internal core. Realized coverage is 32 rows,
1463 populated first words, and 6/32 reset-runnable; the pure leaf is 283 lines,
while `operand_layout.c` remains 535 lines and unchanged under its reviewed
no-split decision. W31P hosted stability and W31B immediate-READY benchmark
follow-ups remain open and nonblocking; the final clean full gate is green.

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

State: evidence review remains open and CPU integration is intentionally deferred.
The original bounded audit of official TI `SPRU172C` Rev C and `SPRU131G` Rev G
material confirmed RPT load, cardinality, repeatability, and the post-decode
interrupt gate. P2-W48A's later primary-source correction closes the observable
RC value progression, but not the exact internal decrement ordering, repeated-body
PC behavior, or already-pending request arbitration required by the generic CPU
transaction.

The additional narrow evidence is useful but insufficient for retirement:

- The RPT entry says RC is inaccessible to software while single-repeat execution
  decrements it. READA and WRITA operation pseudocode later closes the observable
  sequence and final zero, while leaving the exact internal decrement ordering
  unstated.
- Repeated MVDK/MVKD behavior tests the current RC value for a conditional address
  increment. W48A therefore keeps architectural RC authoritative and validates it
  against the remaining-body invariant rather than substituting an unverified
  abstract counter.
- Generic sequential-PC, block-repeat, request-receipt, acknowledgement, latency,
  and pipeline examples do not specify the single-repeat PC hold/reload mechanism,
  final PC, or arbitration of a request already pending before RPT reaches decode.
- GNU binutils 2.42 remains a secondary encoding and operand-shape oracle only. Its
  opcode metadata supplies no RC, PC, IFR, or interrupt-arbitration semantics.

Therefore production RPT rows remain inactive and `cpu.c` gains no repeat branch.
The next admissible normative W20F input is stronger TI evidence or a versioned
original-hardware observation for repeated-body PC flow and predecode request
arbitration. An optional black-box differential vector may expose a hypothesis or
disagreement but cannot close the common requirement by itself. No such test input
may introduce an emulator build dependency.

#### P2-W48A: observable RC progression lifecycle (implemented)

The earlier W20F ledger conclusion is corrected additively by stronger passages in
TI SPRU172C Rev C. READA (printed pp. 4-136–4-137) and WRITA (printed
pp. 4-196–4-197) explicitly perform their transfer, increment PAR, and write
`RC-1` while RC is nonzero, then perform a terminal transfer at RC zero without
another RC update. FIRS (printed p. 4-59) independently corroborates the nonzero
RC decrement loop. Paired with RPT's operand load and operand-plus-one cardinality,
this proves the observable sequence and final zero without inventing a clock edge.

`src/state/repeat_counter_register.[ch]` owns checked typed RC reads and writes
plus the existing dedicated knownness bit. `src/sequencer/repeat/repeat_progress.[ch]`
consumes the existing value-owned `REPEAT_START` effect into a fixed-size,
caller-owned progress value. Start loads the exact operand into RC and widens before
forming the one-through-65,536 body count. Each caller-reported successful body
retirement first proves active phase, count bounds, RC knownness, and
`RC == remaining - 1`; it then decrements nonzero RC once or preserves terminal
zero, and reaches COMPLETE only when the remaining count becomes zero. Every
rejected start or retirement preserves both architectural state and progress.

Mirrored state and repeat tests prove checked RC access, zero/one/two/`0xffff`
progressions, the full 65,536-body completion, unrelated-state preservation, and
transactional null, unsupported-effect, active-restart, forged, unknown, stale,
and extra-completion rejection. The reducer is O(1), allocation-free, value-owned,
and performs no I/O, callback, observation, diagnostics, environment access, or
mutable-global work. It is not yet called by `run.c`, so this slice adds no branch
or carrier growth to the current trusted CPU path.

The existing exact-phase `REQ-COMMON-SINGLE-REPEAT-RC-PROGRESSION` remains
`UNRESOLVED`; the new narrower observable-progression requirement is implemented.
Repeated-body PC flow and predecode interrupt arbitration remain unresolved.
Production RPT catalog rows, body replay, request service, timing, and public API
changes remain outside W48A.

#### P2-W48B: repeat-gated pending IFR latch (implemented)

TI SPRU131G separates request receipt from acknowledgement: receipt sets the
corresponding IFR flag even when the request cannot yet be taken. Its single-repeat
rule defers maskable requests received after RPT decode until every body execution
completes. Section 6.10.1 also identifies reset, explicit write-one-to-IFR, and
`INTR` as clear paths in addition to normal interrupt take. W48B therefore does
not retain the earlier overbroad statement that only normal take can clear IFR.

`src/state/interrupt_flag_register.[ch]` owns knownness-aware reads, full writes,
and checked one-bit set and clear operations over the existing 16-bit IFR storage.
It preserves unrelated flags and every other architectural field. The fixed-size,
caller-owned `src/sequencer/interrupt/repeat_pending_request.[ch]` reducer admits
exactly one caller-selected maskable IFR bit. Receipt latches the bit before consulting the
existing repeat gate; an active gate reports deferral without clearing either the
bit or pending latch. An explicit take is rejected while the gate is active and,
after the remaining-body count reaches zero, clears the matching bit and latch.
Separate reset, IFR-write-one, and `INTR` operations clear matching pending state.

Focused common and project tests prove receipt-before-deferral, preservation at
remaining counts one and 65,536, eligibility and normal take at zero, unrelated
IFR-bit preservation, alternate clears, authoritative reset from unknown IFR, and
transactional null, validation-dependent unknown, forged, stale, duplicate,
invalid-mask, and invalid-count rejection. The reducer calls the existing repeat
request gate rather than duplicating it. It allocates nothing, retains no pointer,
and performs no I/O, observer, diagnostic, environment, or mutable-global work. No
CPU, `run.c`, catalog, trusted packet, public ABI, or benchmark binary changes, so
current CPU execution gains no branch or carrier.

NMI representation, concrete bit assignment, request-priority selection, vector
entry, PC behavior, and requests already pending before RPT reaches decode remain
outside W48B. Production RPT activation therefore remains gated by repeated-body
PC flow and predecode arbitration evidence. Closure accounting is 187 immutable
evidence records; 309 requirements (273 `IMPLEMENTED`, 7 `PLANNED`, 29
`UNRESOLVED`); and 263 tests (256 `PASSING`, 7 `PLANNED`). Catalog and ISA
coverage counts remain unchanged.

#### P2-W48C: DLD repeatability correction (implemented)

A complete audit of the current 100 catalog rows against TI SPRU172C §2.5/Table
2-28 and SPRU131G §6.7/Table 6-18 finds 16 directly supported `INELIGIBLE`
classifications, 83 unresolved classifications, and no positively authorized
`ELIGIBLE` row. The tables enumerate nonrepeatable instructions but do not state
that every omitted form is repeatable. Chapter 3 repeat-execution timing classes
cannot fill that gap because they include instructions separately documented as
nonrepeatable.

`DLD_LONG32_MOD13` was the one unsupported negative: DLD is absent from both
negative lists, and its SPRU172C §4.42 entry does not state a repeat rule. W48C
therefore changes DLD from `INELIGIBLE` to `UNRESOLVED`, not `ELIGIBLE`.
`DST_LONG32_MOD13` remains `INELIGIBLE`, as the negative tables explicitly list
the long-word store. The DLD semantic validator and exact metadata/admission tests
preserve this asymmetry. Both values remain non-admitting through the existing
tri-state repeat-body policy.

The correction changes no opcode identity, encoding, program-word count, layout,
input resource, semantic result, generated lookup population, inventory binding,
CPU routing, carrier, or benchmark binary. It therefore adds no normal or memory
instruction hot-path work. Closure accounting is 188 immutable evidence records;
310 requirements (274 `IMPLEMENTED`, 7 `PLANNED`, 29 `UNRESOLVED`); and 264
tests (257 `PASSING`, 7 `PLANNED`). Catalog and ISA coverage remain 100 rows,
`ID_COUNT=101`, 2,711 lookup entries, 100 bindings, 40/205 family coverage, and
25/205 strict coverage.

### P2-W49A: bit-32 ADD/SUB overflow and status foundation (planned)

TI SPRU131G describes a 40-bit ALU and accumulator lane while defining carry
and borrow at bit 32. Arithmetic overflow for the common C54x mode is measured
against the signed 32-bit result range: with OVM clear, the full modulo-40 ALU
result is retained; with OVM set, a positive or negative overflow is replaced
by `00 7FFF FFFFh` or `FF 8000 0000h`. The destination accumulator's OVA or
OVB flag is set on overflow and otherwise remains sticky. W49A establishes
these shared rules before any ADD or SUB catalog form is activated.

`src/foundation/arithmetic40_addsub.[ch]` owns checked, allocation-free ADD and
SUB primitives over canonical 40-bit operands. Each produces one modulo-40
result, the exact low-32 carry or no-borrow result, and an explicit signed-32
overflow direction. The foundation does not choose OVM storage policy or mutate
status. It uses defined unsigned arithmetic for the lane and sufficiently wide
signed arithmetic for overflow classification, including the signed-40 extrema.

The private status owner adds typed knownness-aware OVM, OVA, and OVB access.
The generic input-resource mask widens from eight to sixteen bits before OVM
occupies bit `0x0080`, retaining representable invalid and future resource bits
without widening the copied 48-byte opcode-input snapshot or trusted packet.
The accumulator effect's status delta gains a destination-overflow action that
may preserve or set the selected OVA/OVB flag but may not implicitly clear it.
The existing accumulator-plus-status candidate remains the atomic commit unit;
no new effect kind or opcode-specific CPU branch is introduced.

Mirrored tests cover modulo-40 wrap, bit-32 carry/no-borrow independent of guard
bits, signed-32 positive and negative overflow including signed-40 extremes,
typed field knownness and transactional writes, OVM collection, sticky A/B
overflow effects, malformed rejection, and LP64/ILP32 carrier-size stability.
W49A adds no catalog row, handler, timing claim, runtime repeat behavior, bus
work, public ABI, allocator, I/O, environment access, observer, diagnostic, or
mutable process state. W49B composes the separately evidenced register-only
ADD/SUB syntax-9 forms through this seam and benchmarks the resulting public CPU
path.

W49A is implementation-complete. Its supported ILP32 probe freezes metadata,
decoded instruction, input snapshot, trusted packet, and outer effect sizes at
24/20/48/76/40 bytes; native LP64 remains 24/20/48/104/48. Six interleaved
same-host observer-off benchmark pairs against the frozen W48C binary found no
material regression: median deltas were -1.34% NOP, -2.96% LD-DP, -1.21%
LD-ASM, +1.23% LD-ARP, and -3.62% mixed-current, with semantic hashes unchanged.
Closure accounting is 192 immutable evidence records; 314 requirements (278
`IMPLEMENTED`, 7 `PLANNED`, 29 `UNRESOLVED`); and 269 tests (262 `PASSING`, 7
`PLANNED`). Catalog and inventory counts remain unchanged.

### P2-W49B: register-only ADD/SUB syntax-9 activation (planned)

This immutable `(planned)` anchor records the bounded arithmetic vertical slice;
the status is now implemented. The bounded
forms are `FORM-T2-01-09` ADD `src [, SHIFT] [, dst]` and `FORM-T2-02-09` SUB
`src [, SHIFT] [, dst]`, both one program word and class 1. Their catalog rows
are `ADD_ACC_SHIFT` at value `0xF400`, mask `0xFCE0`, and `SUB_ACC_SHIFT` at
value `0xF420`, mask `0xFCE0`. S bit 9 selects source A/B, D bit 8 selects
destination A/B, and the existing
`ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout exposes raw SHIFT bits 4:0 as
the signed range -16 through 15. Syntax-10 ASM forms, all memory forms, and
sign-suppressed variants remain outside this piece.

SPRU172C §§ADD and SUB (printed pp. 4-4–4-7 and 4-187–4-190) define the
execution expressions `(dst or src) + (src << SHIFT) -> dst` and
`dst - (src << SHIFT) -> dst`; the shifter runs before the ALU. SPRU131G
§4.4 (printed pp. 4-17–4-18) supplies the 40-bit accumulator shifter, signed
immediate count, and SXM sign/zero fill. W49A owns the checked modulo-40
result, bit-32 carry/no-borrow, signed-32 overflow direction, OVM/OVA/OVB
access, and atomic sticky-overflow effect. W49B composes these facts into
read-before-write accumulator semantics: source/destination aliasing uses the
original source value, OVM clear retains the modulo-40 result, OVM set clamps
positive/negative overflow to `00 7FFF FFFFh`/`FF 8000 0000h`, and no overflow
preserves the destination's sticky OVA/OVB flag.

The semantic requirement is deliberately split from decode, trusted binding,
CPU retirement, inventory projection, and performance. Focused executable tests
cover
all 128 S/D/SHIFT encodings, fixed-bit near misses, SHIFT -16/-1/0/+1/+15,
SXM sign and zero fill, source/destination aliasing, bit-32 carry/no-borrow
independent of guard bits, signed-32 overflow with OVM clear/set, sticky
destination OVA/OVB preservation/set, malformed decoded inputs, unknown required
inputs, and PC+1 atomic retirement. Repeatability remains `UNRESOLVED`: Table
2-28 does not positively authorize omitted forms, so this slice must not
activate runtime RPT.

The acceptance axis includes an observer-off public-CPU benchmark against the
clean W49A baseline. The implementation preserves the trusted packet and outer
effect-carrier sizes, remains allocation-free, adds no opcode-specific CPU
branch, and makes no TI timing, device, profile, memory, interrupt, or public-ABI
claim. Focused C/Make/CMake and Python/generator/inventory checks are GREEN. A
six-pair interleaved comparison against W49A found fixed-scenario paired median
deltas of +3.51% NOP, +4.79% LD-DP, +0.19% LD-ASM, and +0.70% LD-ARP amid wider
host spread, with matching semantic hashes and retirement counts. The dedicated
observer-off ADD/SUB benchmark is also GREEN at 70.569/68.553 ns per instruction
on this host with semantic hash `dd671e98fa2ca1dd`. The realized catalog is 102
rows with ID_COUNT 103,
2,967 populated first-word lookup entries, 102 inventory bindings, family
coverage 42/205 (20.488%), and strict complete-form coverage 27/205
(13.171%). The implementation commit is recorded in the development ledger as
`5cbacda4076a43616be014ba0f528d7e95228e73`. The implemented registry records are
`REQ-COMMON-P2-W49B-ADD-SUB-ACC-SHIFT5-DECODE`,
`REQ-COMMON-P2-W49B-ADD-SUB-ACC-SHIFT5-EFFECTS`,
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-TRUSTED-BINDING`,
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-CPU-RETIREMENT`,
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-INVENTORY`, and
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-HOT-PATH-PERFORMANCE`; the separate
`REQ-COMMON-P2-W49B-ADD-SUB-ACC-SHIFT5-REPEATABILITY` remains unresolved.

### P2-W50A: dual-WORD16 MOD0 read foundation (planned)

This immutable `(planned)` anchor records the architecture-forcing seam now
implemented and green. Six W50A requirements and six executable tests are
closed; the anchor title remains `(planned)` so its historical locator is
stable.
SPRU131G §5.5.4 (printed pp. 5-19–5-22) defines dual data-memory operations as
one-word, indirect-only forms: X memory is read on the D bus and Y memory on the
C bus. The encoded fields are `Xmod[7:6]`, `Xar[5:4]`, `Ymod[3:2]`, and
`Yar[1:0]`; selectors `00..11` choose `AR2..AR5`. MOD0 reads the old selected
AR without updating it. When both sides select one AR with differing modes,
`Xmod` controls; this foundation admits only the evidence-fit MOD0/MOD0 case.
MMR/device behavior, physical dual-bus arbitration, and machine-cycle timing
remain excluded. W50A admits only MOD0/MOD0 and makes no opcode, catalog, or
physical timing claim.

The implementation is intentionally generic and fixed-size. `src/address/`
owns the checked dual-MOD0 address plan with two copied old-AR WORD16 addresses,
selector validation, and no post-access AR candidate. The address foundation
uses `STATE_POLICY_NONE`: only selected AR2..AR5 snapshots are live dependencies,
so unknown or nonzero CMPT/ARP does not block resolution. Full dual resolution
is prepare-local; the persistent root snapshot retains only two canonical
requests. The memory effect owns a compact dual-WORD16 read arm that copies
encoded X/Y modes and AR selectors plus a static pair-completion callback;
transaction IDs and pair state stay in the generic reducer. The pair reducer
deterministically serializes X then Y over one endpoint, retains child identity
across WAIT, cancels on FAULT or failed terminal transitions, and publishes both
words only after both READY observations. This one-endpoint serializer is a
hosted deterministic serialization choice, not a TI machine-cycle or dual-bus
timing claim. The foundation is CMPT/ARP-independent, performs no AR update,
and uses generic cold pair dispatch; no CPU opcode branch was added.

The supported carrier sizes are frozen at address spec 24 bytes, memory effect
40/36 bytes (LP64/ILP32), outer op effect 48/40 bytes, dual reducer 80/72
bytes, and persistent root transaction 336/308 bytes. The common
`memory_instruction_transaction.c` remains the 521-line owner because common
WORD16 orchestration and local hot checks stay together; pair lifecycles are
split into `memory_instruction_dual.c`, capped at 477 lines. The 769-line root
integration test remains one cohesive lifecycle matrix below the 800-line
review threshold; further cases should split into focused files.

No opcode row, catalog binding, decode layout, or CPU opcode branch is activated
in W50A. The six TDD records are
`tests/unit/address/test_indirect_dual_mod0.c`,
`tests/unit/sequencer/memory/test_dual_address_resolution.c`,
`tests/unit/isa/ops/memory/test_dual_word16_memory_effect.c`,
`tests/unit/sequencer/instruction/test_dual_word16_read_transaction.c`,
`tests/unit/sequencer/instruction/test_dual_memory_instruction_transaction.c`,
and `tests/contract/quality/perf/test_dual_word16_hot_path.py`. They are green
under focused CTest 5/5 and Python hot-path 7/7; the legacy DLD/DST structural
suite is also 7/7. The performance contract shows fixed-size, allocation-free
state, no replan/refetch or host-side I/O, and observer-off structural checks
without making a TI cycle claim. The project record and six implemented
requirements/tests are
`E-PROJECT-P2-W50A-DUAL-WORD16-MOD0-FOUNDATION-FIT-OWNERSHIP-CONTRACT`,
`REQ-COMMON-P2-W50A-DUAL-WORD16-MOD0-ADDRESS-PLANNING`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-ADDRESS-RESOLUTION`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-EFFECT-CARRIER`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-PAIR-TRANSACTION`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-ROOT-INTEGRATION`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-HOT-PATH-PERFORMANCE`, and their
corresponding `T-*` records in `config/evidence.json`.

### P2-W50B: dual-memory ADD/SUB MOD0 activation (planned)

This immutable `(planned)` anchor records the bounded opcode activation over
W50A. TI SPRU172C defines syntax 6 `ADD Xmem,Ymem,dst` and
`SUB Xmem,Ymem,dst` as one-word fixed-left-16 dual-memory operations. Their
full-form values/masks are `0xA000/0xFE00` and `0xA200/0xFE00`; W50B fixes
both encoded mode fields to MOD0 and therefore registers only
`ADD_DUAL_MOD0` at `0xA000/0xFECC` and `SUB_DUAL_MOD0` at
`0xA200/0xFECC`. The decode-owned
`DUAL_DESTINATION_XAR_YAR` layout exposes D bit 8, Xar bits 5:4, and Yar bits
1:0. Raw X/Y selectors `0..3` map to AR2..AR5 through the W50A address seam.
All non-MOD0 variants remain outside this slice.

SPRU131G section 4.2.1 establishes that the DB and CB WORD16 operands are
independently zero-extended when SXM is zero or sign-extended when SXM is one
into the 40-bit ALU input. Each extended value is shifted left 16 before the
shared W49A modulo-40 ADD/SUB operation. Carry and borrow are evaluated at bit
32. The fixed-16 exception in Table 4-1 makes C directional: ADD may set but
not clear C, while SUB may clear but not set C. Thus ADD stages C=1 only on a
carry and otherwise preserves C; SUB stages C=0 only on a borrow and otherwise
preserves C. OVM zero retains the modulo-40 result; OVM one saturates signed-32
overflow to `00 7FFF FFFFh` or `FF 8000 0000h`, and only the selected
destination OVA/OVB flag is preserved or set sticky.

The catalog input contract is exactly auxiliary registers plus C, SXM, and
OVM; no accumulator value or carry-in participates in the arithmetic. Checked
and trusted leaves emit one generic dual READ effect with a static completion
callback. The existing W50A resolver, X-then-Y reducer, root transaction,
ACCUMULATOR_WRITE effect, status transaction, and sequential PC retirement own
the rest of execution. No opcode-specific CPU branch, carrier widening,
allocator, environment access, I/O, formatting, observer work, public ABI,
device behavior, physical parallelism, or machine-cycle claim is added.

Executable acceptance covers all 64 restricted encodings, fixed-bit near
misses, operand extraction, checked/trusted binding, X/Y selector mapping,
SXM extension, modulo-40 arithmetic, directional C, OVM saturation, sticky
destination overflow, READY/WAIT/FAULT pair lifecycle, no-update AR behavior,
atomic PC retirement, inventory projection, and observer-off public-CPU
performance. The hosted benchmark uses four ADD/SUB by destination scenarios,
two deterministic DATA reads per retirement, fixed semantic hashes, and
same-cohort advisory comparison without an absolute nanosecond or TI-cycle
threshold. W50A carrier sizes and generic no-refetch/no-replan structure remain
frozen.

Both inventory forms remain `SUBSET` because W50B covers only MOD0/MOD0:
`FORM-T2-01-06` and `FORM-T2-02-06` bind through
`DUAL_INDIRECT_MOD0_MOD0`. The expected realized catalog is 104 rows,
`ID_COUNT=105`, 3,031 populated first words, and 104 bindings; family coverage
rises to 44/205 while strict complete-form coverage stays 27/205.
Repeatability remains `UNRESOLVED`: omission from the negative repeat tables
is not positive eligibility evidence.

W50B is implementation-complete. The catalog, decode, semantics, CPU lifecycle,
inventory, and performance contracts are all GREEN: focused CTest is 4/4, the
observer-off benchmark smoke and Python quality checks pass, and the authoritative
full local gate is recorded by the closeout ledger. The benchmark retires
320,000 instructions per scenario with exactly two DATA reads per retirement;
one hosted characterization produced medians of approximately
1,749.8–1,811.9 ns/instruction across the four ADD/SUB destination scenarios.
These values are volatile characterization only, not a release threshold or TI
timing claim, and no benchmark history artifact is created. The dual benchmark
remains a visible optimization target because its
generic two-read transaction path is materially slower than register-only
arithmetic.

The 499-line benchmark remains a cohesive fixed-roster owner below the 500-line
review trigger; any additional scenarios must be split into a separate capture
module before growth. `operand_layout.c` remains a cohesive descriptor owner
below the 800-line hard limit; future layout growth requires an explicit split.
The next forcing seam is W51 coordinated non-MOD0 dual addressing/update modes,
including same-AR Xmod precedence and atomic two-AR commit, while preserving the
trusted-path and dual-read performance constraints.

### P2-W51A: dual-indirect MOD0-MOD2 planning checkpoint (planned)

This immutable `(planned)` heading is the durable W51A FIT/ownership anchor.
W51A extends the pure dual-indirect WORD16 planner and the sequencer resolution
seam without activating an opcode row or changing the CPU, catalog, inventory,
root carrier, or public API. The primary evidence is SPRU131G §5.5.4/Table 5-8
(printed pp. 5-19–5-22): modes 0, 1, and 2 retain the old address, stage a
post-access decrement of one, or stage a post-access increment of one, and the
encoded Xar/Yar fields directly select AR2–AR5. SPRU179C ADD Example 4
(printed p. 4-7) independently demonstrates that `*AR2+` paired with `*AR2−`
leaves the X-side increment effective. MOD3 stays deferred because its AR0/BK
circular dependency and the existing CMPT1 dual boundary are not yet resolved.

`src/address/` owns the value-only mode planner. It accepts modes 0, 1, and 2,
uses copied old AR values for both operands, stages at most one post-access AR
update for a same-selector alias, and gives Xmod precedence. Directly encoded
Xar/Yar selectors do not read CMPT or ARP; those controls belong to other
addressing families. Boundary rejection remains transactional, with no partial
X/Y effect on error. The new COMMON requirement is exercised by
`tests/unit/address/test_indirect_dual_modes.c`.

`src/sequencer/memory/` resolves the pair against one frozen architectural
snapshot and publishes two canonical DATA READ requests plus value-owned commit
candidates. W51A deliberately preserves the requests-only persistent snapshot
at exactly 32 bytes (`sizeof(ti_c54x_dual_memory_address_snapshot) == 32`);
atomic commit/publication of the staged updates is the next W51B seam. Existing
`tests/unit/sequencer/memory/test_dual_address_resolution.c` now covers the
MOD1/MOD2 and same-AR behavior while retaining W50A's reverse-linked project
resolution requirement.

W51A's RED→GREEN focused evidence covers all nine MOD0/MOD1/MOD2 X/Y mode pairs,
old-address request order, decrement/increment candidates, same-AR Xmod
precedence, transactional boundary rejection, and independence from CMPT/ARP.
The observer-off W50B smoke remains green with semantic hash
`b40823092c0aa605`. A dirty-tree W51A sample measured 733.988–882.744
ns/instruction; this is volatile host characterization only, with no regression
threshold, release gate, TI-cycle claim, or benchmark-history artifact.

W51A closure accounting is 206 immutable evidence records; 337 requirements
(299 `IMPLEMENTED`, 7 `PLANNED`, 31 `UNRESOLVED`); and 287 tests (280 `PASSING`,
7 `PLANNED`). Catalog, lookup, binding, family, and strict-complete coverage
remain 104, 3,031, 104, 44/205, and 27/205. The next documented seam is W51B:
persist both prevalidated update candidates and publish the root transaction
atomically, after which broader dual-mode opcode activation can proceed.

### P2-W51B: dual-memory root commit/publication contract (planned)

This immutable `(planned)` heading is the durable W51B FIT/ownership anchor.
W51B owns the generic root transaction seam only; it does not activate an opcode
row, alter CPU wiring, widen the public API, or change catalog/inventory
projection. The W51A architectural mode/update evidence remains the source for
MOD0/MOD1/MOD2 old-address behavior, direct Xar/Yar selector mapping to AR2–AR5,
and same-AR Xmod precedence. All lifecycle and ABI assertions in this slice
are project contracts.

The persistent dual snapshot must carry exactly two value-owned commit
descriptors alongside its copied requests. Before endpoint issue and again at
the commit boundary, the root validates the copied mode/selector/address
shadows against the live architectural state; malformed modes, selectors,
addresses, or drift reject transactionally without partial output. Distinct
selected ARs publish both post-access updates atomically, and only after both
DATA reads are READY and the ordinary effect staging succeeds. If X and Y
select one AR, Xmod precedence yields one commit. WAIT, FAULT, cancellation,
and other nonterminal paths remain non-publishing. Native LP64/ILP32 root
sizes stay frozen at 336/308 bytes, and no broader dual-mode opcode activation
is part of W51B.

The executable contract lives at
`tests/unit/sequencer/instruction/test_dual_memory_instruction_w51b_contract.c`
with CTest name `ti_c54x.dual_memory_instruction_w51b_contract` and is green:
the 48-byte snapshot, atomic distinct-AR publication, single X-owned alias
commit, WAIT/FAULT non-publication, drift rejection, and frozen 336/308 root
sizes are all proven. The dual effect carrier admits MOD0/MOD1/MOD2 with MOD3
still deferred, and the READY commit lives in the split
`memory_instruction_dual_commit` owner after the 500-line quality bound forced
`memory_instruction_dual.c` back to 448 lines. The W51B closure registry is
208 immutable evidence records, 338 requirements (300 `IMPLEMENTED`, 7
`PLANNED`, 31 `UNRESOLVED`), and 288 tests (281 `PASSING`, 7 `PLANNED`).
Catalog, lookup, binding, family, and strict-complete coverage remain 104,
3,031, 104, 44/205, and 27/205. The next documented seam is broader dual-mode
opcode activation through this root.

### P2-W51C: dual ADD/SUB MOD0–MOD2 activation (planned)

This immutable `(planned)` heading is the durable W51C FIT/ownership anchor.
W51C activates the eight non-MOD0/MOD0 dual-indirect MOD0/MOD1/MOD2
combinations for syntax-6 `ADD Xmem,Ymem,dst` and `SUB Xmem,Ymem,dst`
through the W51B root; it adds no addressing mode, alters no CPU wiring,
widens no public API, and changes no timing or machine-cycle claim. The
W50B syntax-6 first-word evidence remains the source for the D[8],
Xar[5:4], and Yar[1:0] positions and the ADD/SUB bit-9 discriminator; the
W51A mode evidence remains the source for MOD0/MOD1/MOD2 old-address and
post-access update behavior with same-AR Xmod precedence. A clean-decode
probe proves the sixteen new first-word neighborhoods are currently
unclaimed, and MOD3 neighborhoods stay undecoded and deferred. All
lifecycle, eligibility, and ABI assertions in this slice are project
contracts; repeatability remains `UNRESOLVED` for every new row.

Each new row keeps mask `0xFECC`, one program word, thirty-two encodings
(D by XAR by YAR), the existing `DUAL_DESTINATION_XAR_YAR` layout,
`UNRESOLVED` repeatability, and the `AUXILIARY_REGISTERS_STATUS_C_SXM_OVM`
resources. ADD rows are
`ADD_DUAL_M{m}_{n}` at `0xA000|(m<<6)|(n<<2)` and SUB rows are
`SUB_DUAL_M{m}_{n}` at `0xA200|(m<<6)|(n<<2)` for `(m,n)` in `{0,1,2}^2`
minus `(0,0)`; the existing `ADD_DUAL_MOD0`/`SUB_DUAL_MOD0` rows keep their
names. Checked and trusted leaves share one core parameterized by
operation and both modes; per-row wrappers and completion validators prove
the exact decoded identity and stage the row's modes without an
opcode-specific CPU branch. Inventory binds all nine combos per form as
`SUBSET` under per-row `DUAL_INDIRECT_MOD{m}_MOD{n}` addressing; both forms
remain `SUBSET` while MOD3 is deferred. The MOD0 benchmark roster is frozen,
so no new hosted scenarios are added: the unchanged MOD0 smoke re-runs as
the shared-path characterization.

Ownership follows the W50B split: `REQ-COMMON-P2-W51C-DUAL-MODES-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W51C-DUAL-MODES-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W51C-DUAL-MODES-EFFECTS`
(`src/isa/ops/arithmetic/`) carry the TI architectural evidence, while
`REQ-PROJECT-P2-W51C-DUAL-MODES-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`), `REQ-PROJECT-P2-W51C-DUAL-MODES-CPU-RETIREMENT`
(`src/cpu/`), and `REQ-PROJECT-P2-W51C-DUAL-MODES-INVENTORY`
(`tools/quality/`) carry the project plan anchor. Dispatch and inventory
are already green through their shared generic proofs (the all-ID dispatch
test and the nine-combo binding projection); catalog, decode, effects, and
retirement stay planned.

The RED executable contracts are
`tests/unit/isa/catalog/test_add_sub_dual_modes_metadata.c` with CTest name
`ti_c54x.add_sub_dual_modes_metadata`,
`tests/unit/isa/decode/test_add_sub_dual_modes_decode.c` with CTest name
`ti_c54x.add_sub_dual_modes_decode`,
`tests/unit/isa/ops/arithmetic/test_add_sub_dual_modes.c` with CTest name
`ti_c54x.add_sub_dual_modes`, and
`tests/unit/cpu/isa/arithmetic/test_cpu_add_sub_dual_modes.c` with CTest
name `ti_c54x.cpu_add_sub_dual_modes`; all four are green, and dispatch and
inventory were already green through their shared generic proofs. The W51C
closure registry is 210 immutable evidence records, 344 requirements (306
`IMPLEMENTED`, 7 `PLANNED`, 31 `UNRESOLVED`), and 292 tests (285 `PASSING`,
7 `PLANNED`). Catalog, lookup, binding, family, and strict-complete
coverage are 120, 3,543, 120, 44/205, and 27/205. The MOD0 benchmark smoke
re-runs unchanged as the shared-path characterization with semantic hash
`b40823092c0aa605`; the next documented work is the repeat-machinery phase.

### P2-W52A: COMMON repeat-truth closure (planned)

This immutable `(planned)` heading is the durable W52A FIT/ownership anchor.
W52A closes the five `PLANNED` COMMON repeat requirements with executable
TI-evidence-bound contracts over the existing W20D/W20E/W48A/W48B leaves; it
adds no production RPT catalog row, no CPU repeat branch, no public API, and
no timing claim. The W20F deferral stays in force: exact internal RC
decrement ordering, repeated-body PC flow/final PC, and predecode request
arbitration remain `UNRESOLVED`, and `decode_first_word(0xEC00)` and
`decode_first_word(0xF070)` stay explicitly unknown. Full first-word
value/mask metadata pins for `RPT #K` (`0xEC00`/`0xFF00`) and `RPT #lk`
(exact `0xF070`) are deferred to the later RPT activation slice that
registers production rows.

The five contracts prove the TI claims through production paths. Decode
pins the exhaustive `0xEC00`–`0xECFF` K neighborhood (bits 7:0, width 8,
one word), the exact-`0xF070` plus full-range 16-bit extension word (width
16, two words), width-mismatch rejection, and the explicit unknown-word
pins. Operation chains the op leaf into `repeat_progress_start` to prove
the exact decoded operand reaches architectural RC for both widths with no
status payload, plus transactional malformed rejection and borrowed-field
preservation. Cardinality pins the evidenced examples (`#2` to three
executions, `#0` to one, `#0xffff` to 65,536) with widen-before-add and
null rejection. The IRQ gate pins maskable-plus-NMI deferral at remaining
counts one and 65,536, literal RS/reset admission at both endpoints, full
admission at zero, and transactional invalid-kind/count rejection.
Eligibility pins the admission tri-state, sweeps all 120 registered rows to
a valid admission, spot-pins documented `INELIGIBLE` rejections and the
DLD/DST asymmetry, and audits the twenty W49B/W50B/W51C rows: none matches
a positively listed Table 2-28 nonrepeatable form, so all twenty stay
`UNRESOLVED` (blocked, never inferred eligible or ineligible).

Ownership mirrors the requirement owners: `REQ-COMMON-RPT-IMMEDIATE-DECODE`
(`src/isa/decode/`), `REQ-COMMON-RPT-IMMEDIATE-OPERATION`
(`src/isa/ops/repeat/`), `REQ-COMMON-SINGLE-REPEAT-CARDINALITY`,
`REQ-COMMON-SINGLE-REPEAT-IRQ-GATE`, and
`REQ-COMMON-SINGLE-REPEAT-BODY-ELIGIBILITY` (`src/sequencer/repeat/`) carry
the TI architectural evidence. The RED executable contracts are
`tests/unit/isa/decode/test_rpt_immediate_decode.c` with CTest name
`ti_c54x.rpt_immediate_decode_common`,
`tests/unit/isa/ops/repeat/test_rpt_immediate.c` with
CTest name `ti_c54x.rpt_immediate_operation_common`,
`tests/unit/sequencer/repeat/test_single_repeat_cardinality.c` with CTest
name `ti_c54x.repeat_cardinality_common`,
`tests/unit/sequencer/repeat/test_single_repeat_irq_gate.c` with CTest
name `ti_c54x.repeat_request_gate_common`, and
`tests/unit/sequencer/repeat/test_single_repeat_body_eligibility.c` with
CTest name `ti_c54x.repeat_body_eligibility_common`; all five are green.
The W52A closure registry is 211 immutable evidence records, 344
requirements (311 `IMPLEMENTED`, 2 `PLANNED`, 31 `UNRESOLVED`), and 292
tests (290 `PASSING`, 2 `PLANNED`). Catalog, lookup, binding, family, and
strict-complete coverage are unchanged at 120, 3,543, 120, 44/205, and
27/205. The next documented work is AR0/BK indexed/circular addressing
including MOD3 dual, then reset/MMR edges.

### P2-W52B: dual MOD3 AR0/BK planning checkpoint (planned)

This immutable `(planned)` heading is the durable W52B FIT/ownership anchor.
W52B admits dual-indirect MOD3 (`*ARx+0%`) through the address planner, the
sequencer resolution seam, and the root commit validator without activating
an opcode row or changing the CPU, catalog, inventory, or public API. The
primary evidence is TI SPRU131G Table 5-8 (MOD3 function `addr = ARx` with
post-access `ARx = circ(ARx + AR0)`), §5.5.4.2 (BK=0 gives plain indexed
`ARx + AR0` addition), §5.5.4.3 (nonzero BK gives §5.5.3.4 circular
addressing), §5.5.3.4 step rules (signed AR0 step with magnitude at most BK
and first access inside the buffer), and Tables 5-6/5-7 plus §5.5.5 (Xar/Yar directly select
AR2–AR5 in a format with no ARF field, bounding the compatibility-mode
question). Same-selector aliases keep the evidenced Xmod precedence with
one X-owned update; indexed overflow and out-of-buffer circular starts
reject as boundaries with no wrap inference; unknown AR0/BK sources reject
at resolution; identical specs resolve identically under any CMPT/ARP
state. The persistent dual snapshot widens 48 to 52 bytes with copied
AR0/BK shadows so the root validator can recheck MOD3 commits, while the
root carriers hold at 336/308 because the candidate union already dominates
the dual snapshot arm.

Ownership: `REQ-COMMON-P2-W52B-DUAL-MOD3-PLAN` (`src/address/`),
`REQ-COMMON-P2-W52B-DUAL-MOD3-RESOLVE` (`src/sequencer/memory/`), and
`REQ-COMMON-P2-W52B-DUAL-MOD3-ROOT` (`src/sequencer/instruction/`) carry
the TI architectural evidence. The RED executable contracts are
`tests/unit/address/test_indirect_dual_mod3.c` with CTest name
`ti_c54x.indirect_dual_mod3`,
`tests/unit/sequencer/memory/test_dual_mod3_resolution.c` with CTest name
`ti_c54x.dual_mod3_resolution`, and
`tests/unit/sequencer/instruction/test_dual_mod3_root.c` with CTest name
`ti_c54x.dual_mod3_root`; all three are green. The W52B closure registry
is 217 immutable evidence records, 347 requirements (314 `IMPLEMENTED`, 2
`PLANNED`, 31 `UNRESOLVED`), and 295 tests (293 `PASSING`, 2 `PLANNED`).
Catalog, lookup, binding, family, and strict-complete coverage are
unchanged at 120, 3,543, 120, 44/205, and 27/205. The next documented work
is W52C MOD3 dual ADD/SUB activation, then reset/MMR edges.

### P2-W52C: dual MOD3 ADD/SUB activation (planned)

This immutable `(planned)` heading is the durable W52C FIT/ownership anchor.
W52C activates the fourteen MOD3-involving syntax-6 dual-memory ADD/SUB
rows at `0xA000|(m<<6)|(n<<2)` and `0xA200|(m<<6)|(n<<2)` for `(m,n)` in
`{0,1,2,3}^2` minus the nine MOD0–MOD2 combinations, each with mask
`0xFECC`, one program word, thirty-two encodings, the existing
`DUAL_DESTINATION_XAR_YAR` layout, `UNRESOLVED` repeatability, and
`AUXILIARY_REGISTERS_STATUS_C_SXM_OVM` resources. Checked and trusted
leaves share the operation/mode-parameterized core with per-row identity
validation and completion validators; macro-driven dispatch binds all rows
with no opcode-specific CPU branch and no public ABI change. MOD3 sides
retire through the generic W52B root with AR0/BK snapshots, atomic per-row
address publication, single X-owned commits for same-AR aliases, and
destination routing. The fourteen neighborhoods were probed CLEAN before
activation; none matched a positively listed Table 2-28 nonrepeatable
form, so all fourteen stay `UNRESOLVED`.

Ownership: `REQ-COMMON-P2-W52C-DUAL-MOD3-CATALOG` (`src/isa/catalog/`),
`REQ-COMMON-P2-W52C-DUAL-MOD3-DECODE` (`src/isa/decode/`), and
`REQ-COMMON-P2-W52C-DUAL-MOD3-EFFECTS` (`src/isa/ops/arithmetic/`) carry
the TI architectural evidence;
`REQ-PROJECT-P2-W52C-DUAL-MOD3-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W52C-DUAL-MOD3-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W52C-DUAL-MOD3-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_add_sub_dual_mod3_metadata.c` with CTest name
`ti_c54x.add_sub_dual_mod3_metadata`,
`tests/unit/isa/decode/test_add_sub_dual_mod3_decode.c` with CTest name
`ti_c54x.add_sub_dual_mod3_decode`,
`tests/unit/isa/ops/arithmetic/test_add_sub_dual_mod3.c` with CTest name
`ti_c54x.add_sub_dual_mod3`, and
`tests/unit/cpu/isa/arithmetic/test_cpu_add_sub_dual_mod3.c` with CTest
name `ti_c54x.cpu_add_sub_dual_mod3`; all four are green. The W52C closure
registry is 219 immutable evidence records, 353 requirements (320
`IMPLEMENTED`, 2 `PLANNED`, 31 `UNRESOLVED`), and 299 tests (297
`PASSING`, 2 `PLANNED`). Coverage is 134 catalog rows, `ID_COUNT=135`,
3,991 lookup entries, 134 trusted bindings, family coverage above 44/205,
and unchanged 27/205 strict complete-form coverage. The next documented
work is reset/MMR edges.

### P2-W53: LD Smem[,SHIFT],dst vertical slice (planned)

This immutable `(planned)` heading is the durable W53 FIT/ownership anchor.
W53 activates Table 2-19 form `FORM-T2-19-05`, TI SPRU172C §4 LD syntax 4
`LD Smem[,SHIFT],dst` (printed pp. 4-66–4-67): word0
`0110_1111_IAAAAAAA` (value `0x6F80`, mask `0xFFE0`, mode bits [4:3], ARF
bits [2:0]) with word1 `0000_110D_010S_HIFT` (D bit 8, five-bit signed
SHIFT in bits [4:0], `-16 v SHIFT v 15`). Semantics is
`dst = SXM-widened Smem << SHIFT` in canonical 40-bit space: SXM selects
sign/zero extension, OVM saturation applies on overflow, and OVdst is
affected; C, TC, and unrelated state stay unchanged. Loads admit
MOD0/MOD1/MOD2 through the W42A seam and reject MOD3 `INVALID_OPERAND`
before any MEMORY effect, following the W42B precedent; Table 2-28 lists
only `LD ARP`/`LD DP`, so this form stays `UNRESOLVED` repeatability. A
dedicated `D/ARF/MOD/SHIFT5` operand layout carries the decoded fields;
handlers never parse raw instruction words. No new effect kind, address
kind, CPU branch, public ABI, timing, or device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W53-LD-SMEM-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W53-LD-SMEM-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W53-LD-SMEM-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W53-LD-SMEM-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W53-LD-SMEM-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W53-LD-SMEM-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_smem_shift_metadata.c` with CTest name
`ti_c54x.ld_smem_shift_metadata`,
`tests/unit/isa/decode/test_ld_smem_shift_decode.c` with CTest name
`ti_c54x.ld_smem_shift_decode`,
`tests/unit/isa/ops/load/test_ld_smem_shift.c` with CTest name
`ti_c54x.ld_smem_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_smem_shift.c` with CTest name
`ti_c54x.cpu_ld_smem_shift`; all four are green. The W53 closure
registry is 224 immutable evidence records, 359 requirements (328
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 303 tests (303
`PASSING`, 0 `PLANNED`). Coverage is 135 catalog rows, `ID_COUNT=136`,
4,023 lookup entries, 135 trusted bindings, family coverage 45/205
(21.951%), and unchanged 27/205 strict complete-form coverage. The row
is not reset-runnable (Smem reads need a known AR).

### P2-W54: LD src[,SHIFT],dst vertical slice (planned)

This immutable `(planned)` heading is the durable W54 FIT/ownership anchor.
W54 activates Table 2-19 form `FORM-T2-19-11`, TI SPRU172C §4 LD syntax
10 `LD src[,SHIFT],dst` (printed pp. 4-66–4-68): word
`1111_01SD_010S_HIFT` (value `0xF440`, mask `0xFCE0`, source in bit 9,
destination in bit 8, five-bit signed SHIFT in bits 4-0), reusing the
existing `ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout,
`UNRESOLVED` repeatability, and `ACCUMULATORS_STATUS_SXM_OVM` resources. Semantics
is `dst = src40 << SHIFT` in canonical 40-bit space: no SXM widening
applies to the 40-bit source, but right shifts follow the general C54x
shift rule shared with the ADD core (SXM=0 logical, SXM=1 arithmetic);
OVM saturation applies on guard-vs-bit31 overflow with sticky
destination overflow, and C/TC stay preserved. The D bit is
authoritative for the writeback and OV target, so D==S encodings
naturally produce the documented OVsrc case. Table 2-28 lists only
`LD ARP`/`LD DP`, so this form stays `UNRESOLVED` repeatability. No
memory is touched: no AR dependency, no fetch beyond the single word,
no new effect kind, address kind, CPU branch, public ABI, timing, or
device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W54-LD-SRC-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W54-LD-SRC-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W54-LD-SRC-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W54-LD-SRC-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W54-LD-SRC-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W54-LD-SRC-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_src_shift_metadata.c` with CTest name
`ti_c54x.ld_src_shift_metadata`,
`tests/unit/isa/decode/test_ld_src_shift_decode.c` with CTest name
`ti_c54x.ld_src_shift_decode`,
`tests/unit/isa/ops/load/test_ld_src_shift.c` with CTest name
`ti_c54x.ld_src_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_src_shift.c` with CTest name
`ti_c54x.cpu_ld_src_shift`; all four are green. The W54 closure
registry is 229 immutable evidence records, 365 requirements (334
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 307 tests (307
`PASSING`, 0 `PLANNED`). Coverage is 136 catalog rows, `ID_COUNT=137`,
4,151 lookup entries, 136 trusted bindings, family coverage 46/205
(22.439%), and 28/205 strict complete-form coverage (`FORM-T2-19-11`
is `COMPLETE`). The row is not reset-runnable (accumulators start
unknown). W54 also corrects the resource token to
`ACCUMULATORS_STATUS_SXM_OVM`: accumulator-only inputs must be
collected or every execution rejects `UNKNOWN_INPUT`.

### P2-W55: LD #K,dst vertical slice (planned)

This immutable `(planned)` heading is the durable W55 FIT/ownership anchor.
W55 activates Table 2-19 form `FORM-T2-19-07`, TI SPRU172C §4 LD syntax
6 `LD #K, dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume pp. 4-67–4-69): word
`1110_100D_KKKK_KKKK` (value `0xE800`, mask `0xFE00`, destination in
bit 8, unsigned 8-bit K in bits 7-0, `0 v K v 255`), one program word,
a new `ACCUMULATOR_IMMEDIATE_K8` layout, `UNRESOLVED` repeatability
(Table 2-28 lists only `LD ARP`/`LD DP`), and `NONE` input resources.
Semantics is `dst = K` zero-extended to 40 bits: TI Example 5
(`LD #248, B` with SXM=1 yields `00 0000 00F8`) proves the constant
ignores SXM, and with no SHIFT or ASM shift there is no OVM/OV effect,
so no status inputs are collected and C/TC/OVA/OVB stay preserved. No
memory is touched: no AR dependency, no fetch beyond the single word,
no new effect kind, address kind, CPU branch, public ABI, timing, or
device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W55-LD-K-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W55-LD-K-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W55-LD-K-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W55-LD-K-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W55-LD-K-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W55-LD-K-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_k_metadata.c` with CTest name
`ti_c54x.ld_k_metadata`,
`tests/unit/isa/decode/test_ld_k_decode.c` with CTest name
`ti_c54x.ld_k_decode`,
`tests/unit/isa/ops/load/test_ld_k.c` with CTest name
`ti_c54x.ld_k`, and
`tests/unit/cpu/isa/load/test_cpu_ld_k.c` with CTest name
`ti_c54x.cpu_ld_k`; all four are green. The W55 closure
registry is 234 immutable evidence records, 371 requirements (340
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 311 tests (311
`PASSING`, 0 `PLANNED`). Coverage is 137 catalog rows, `ID_COUNT=138`,
4,663 lookup entries, 137 trusted bindings, family coverage 47/205
(22.927%), and 29/205 strict complete-form coverage (`FORM-T2-19-07`
is `COMPLETE`). The row is reset-runnable (no inputs required).

### P2-W56: LD src,ASM[,dst] vertical slice (planned)

This immutable `(planned)` heading is the durable W56 FIT/ownership anchor.
W56 activates Table 2-19 form `FORM-T2-19-10`, TI SPRU172C §4 LD syntax
9 `LD src, ASM[, dst]` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67): word
`1111_01SD_1000_0010` (value `0xF482`, mask `0xFCFF`, source in bit 9,
destination in bit 8; omitted dst defaults to src),
reusing the `ACCUMULATOR_SOURCE_DESTINATION` layout, `UNRESOLVED`
repeatability (Table 2-28 lists only `LD ARP`/`LD DP`), and a new
`ACCUMULATORS_STATUS_SXM_OVM_ASM` resource combination built on a new
`STATUS_ASM` input bit. Semantics is `dst = src << ASM` where the
5-bit ASM field is read signed (-16..15): SXM selects logical versus
arithmetic right shift exactly as in W54, OVM saturates on guard-bit
overflow with sticky destination overflow, and C/TC stay preserved.
No memory is touched: no AR dependency, no fetch beyond the single
word, no new effect kind, address kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W56-LD-SRC-ASM-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W56-LD-SRC-ASM-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W56-LD-SRC-ASM-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W56-LD-SRC-ASM-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W56-LD-SRC-ASM-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W56-LD-SRC-ASM-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_src_asm_metadata.c` with CTest name
`ti_c54x.ld_src_asm_metadata`,
`tests/unit/isa/decode/test_ld_src_asm_decode.c` with CTest name
`ti_c54x.ld_src_asm_decode`,
`tests/unit/isa/ops/load/test_ld_src_asm.c` with CTest name
`ti_c54x.ld_src_asm`, and
`tests/unit/cpu/isa/load/test_cpu_ld_src_asm.c` with CTest name
`ti_c54x.cpu_ld_src_asm`; all five are green (four RED contracts plus
the ASM carrier proof). The W56 closure
registry is 239 immutable evidence records, 378 requirements (347
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 316 tests (316
`PASSING`, 0 `PLANNED`). Coverage is 138 catalog rows, `ID_COUNT=139`,
4,667 lookup entries, 138 trusted bindings, family coverage 48/205
(23.415%), and 30/205 strict complete-form coverage (`FORM-T2-19-10`
is `COMPLETE`). The row is not reset-runnable (accumulators start
unknown). W56 adds the shared `STATUS_ASM` input bit (snapshot
48→56 bytes, transaction 336→344 bytes, twelve 512-byte CPU test
storages to 1024) with an independent carrier proof.

### P2-W57: LD #lk,16,dst vertical slice (planned)

This immutable `(planned)` heading is the durable W57 FIT/ownership anchor.
W57 activates Table 2-19 form `FORM-T2-19-09`, TI SPRU172C §4 LD syntax
8 `LD #lk, 16, dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67, which agrees on all ten LD opcode
diagrams): word0 `1111_000D_0110_0010` (value `0xF062`, mask `0xFEFF`,
destination in bit 8, all other first-word bits fixed, two encodings),
word1 is the copied raw 16-bit `lk` in range -32768..32767, two program
words, a new `ACCUMULATOR_IMMEDIATE_LK16` layout carrying destination
plus `lk16`, `UNRESOLVED` repeatability (Table 2-28 lists only `LD ARP`/
`LD DP`), and `STATUS_SXM` input resources. Semantics is
`dst = SXM-widen(lk) << 16` in canonical 40-bit space: SXM=0
zero-extends `lk`, SXM=1 sign-extends bit 15, followed by logical left
shift 16. Pinned vectors mirror the fixed-16 W38 precedent: `0000` is
zero in both modes, `7fff` is `0x007fff0000`, `8000` is
`0x0080000000`/`0xff80000000`, and `ffff` is
`0x00ffff0000`/`0xffffff0000`. C, TC, and OV stay preserved with no OVM
input: the fixed-16 shift follows W38, not the variable-SHIFT
saturation path. Unknown SXM rejects transactionally before any effect.
No memory is touched: no AR dependency, one extension fetch at `PC+1`
with retirement at `PC+2` (the `PC=0xfffe` span edge rejects with no
mutation), no new effect kind, address kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W57-LD-LK16-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W57-LD-LK16-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W57-LD-LK16-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W57-LD-LK16-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W57-LD-LK16-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W57-LD-LK16-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_lk16_metadata.c` with CTest name
`ti_c54x.ld_lk16_metadata`,
`tests/unit/isa/decode/test_ld_lk16_decode.c` with CTest name
`ti_c54x.ld_lk16_decode`,
`tests/unit/isa/ops/load/test_ld_lk16.c` with CTest name
`ti_c54x.ld_lk16`, and
`tests/unit/cpu/isa/load/test_cpu_ld_lk16.c` with CTest name
`ti_c54x.cpu_ld_lk16`; all four are green. The W57 closure
registry is 244 immutable evidence records, 384 requirements (353
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 320 tests (320
`PASSING`, 0 `PLANNED`). Coverage is 139 catalog rows, `ID_COUNT=140`,
4,669 lookup entries, 139 trusted bindings, family coverage 49/205
(23.902%), and 31/205 strict complete-form coverage (`FORM-T2-19-09`
is `COMPLETE`). The row is reset-runnable (SXM is reset-known, no
memory or accumulator input required).

### P2-W58: LD #lk[,SHFT],dst vertical slice (planned)

This immutable `(planned)` heading is the durable W58 FIT/ownership anchor.
W58 activates Table 2-19 form `FORM-T2-19-08`, TI SPRU172C §4 LD syntax
7 `LD #lk[,SHFT],dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67, which agrees on all ten LD opcode
diagrams): word0 `1111_000D_0010_SHFT` (value `0xF020`, mask `0xFEF0`,
destination in bit 8, unsigned 4-bit SHFT in bits 3:0 spanning 0..15,
all other first-word bits fixed, 32 encodings), word1 is the copied
raw 16-bit `lk` in range -32768..32767, two program words, a new
`ACCUMULATOR_IMMEDIATE_SHIFT4_LK16` layout carrying destination,
shift, plus `lk16`, `UNRESOLVED` repeatability (Table 2-28 lists only
`LD ARP`/`LD DP`), and `STATUS_SXM_OVM` input resources. Omitted
SHFT assembles as 0; the assembler note redirecting SHFT=0,
0≤lk≤255 to syntax 6 is assembler behavior and does not narrow
decode. Semantics is `dst = SXM-widen(lk) << SHFT` in canonical
40-bit space: SXM=0 zero-extends `lk`, SXM=1 sign-extends bit 15,
followed by logical left shift by the unsigned SHFT. Guard-bit
overflow engages OVM saturation with sticky destination overflow
following the variable-shift W53/W54 precedent; C and TC stay
preserved. Unknown SXM/OVM rejects transactionally before any
effect. No memory is touched: no AR dependency, one extension fetch
at `PC+1` with retirement at `PC+2`, no new effect kind, address
kind, CPU branch, public ABI, timing, or device/MMR claim is
admitted. The prerequisite `operand_layout` table split (sets file
vs plans/resolver file, both below 500 lines) lands first as a
behavior-preserving refactor.

Ownership: `REQ-COMMON-P2-W58-LD-LK-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W58-LD-LK-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W58-LD-LK-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W58-LD-LK-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W58-LD-LK-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W58-LD-LK-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_lk_shift_metadata.c` with CTest name
`ti_c54x.ld_lk_shift_metadata`,
`tests/unit/isa/decode/test_ld_lk_shift_decode.c` with CTest name
`ti_c54x.ld_lk_shift_decode`,
`tests/unit/isa/ops/load/test_ld_lk_shift.c` with CTest name
`ti_c54x.ld_lk_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_lk_shift.c` with CTest name
`ti_c54x.cpu_ld_lk_shift`; all four are green. The W58 closure
registry is 249 immutable evidence records, 390 requirements (359
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 324 tests (324
`PASSING`, 0 `PLANNED`). Coverage is 140 catalog rows, `ID_COUNT=141`,
4,701 lookup entries, 140 trusted bindings, family coverage 50/205
(24.390%), and 32/205 strict complete-form coverage (`FORM-T2-19-08`
is `COMPLETE`). The row is reset-runnable (SXM/OVM are reset-known,
no memory or accumulator input required).

### P2-W59: LD Xmem,SHFT,dst vertical slice (planned)

This immutable `(planned)` heading is the durable W59 FIT/ownership anchor.
W59 activates Table 2-19 form `FORM-T2-19-06`, TI SPRU172C §4 LD syntax
5 `LD Xmem, SHFT, dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67, which agrees on all ten LD opcode
diagrams): word0 `1001_010D_XXXX_SHFT` (value `0x9400`, mask `0xFE00`,
destination in bit 8, unsigned 4-bit SHFT in bits 3:0 spanning 0..15,
one program word, 512 encodings). Per SPRU131G Figure 5-11/Table 5-6
(printed pp. 5-19–5-20) the XXXX field is Xmod in bits 7:6 plus Xar
in bits 5:4; Xar selects AR2–AR5 (Table 5-7) and Xmod selects
`*ARx`/`*ARx-`/`*ARx+`/`*ARx+0%` circular (Table 5-8). §5.5.4.4
confirms single-operand dual-format instructions expose only Xmem
with Xmod/Xar defining the mode; the Xmem read travels the D bus.
A new `ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4` layout carries destination,
Xmod, Xar, plus SHFT; repeatability is `UNRESOLVED` (Table 2-28
lists only `LD ARP`/`LD DP`); input resources are a new
`AUXILIARY_REGISTERS_STATUS_SXM_OVM` combination (no carry is read).
Semantics is `dst = SXM-widen(Xmem) << SHFT` in canonical 40-bit
space with OVM saturation and sticky destination overflow on
guard-bit overflow following the variable-shift precedent; C and TC
stay preserved. The AR update (post-access decrement/increment for
MOD1/MOD2, circular AR0/BK commit for MOD3) needs an X-only dual
resolution: the pair machinery always issues two reads and two
commits, and single-indirect translation is rejected (spurious
CMPT0/ARP0 policy, wrong MOD3). W59A therefore adds one address
kind reusing the pair planner with Y self-aliased, packaged as a
single WORD16 read through the existing single-read child — one
D-bus DATA read, X commit only. Unknown AR/SXM/OVM rejects
transactionally. One program word retires at `PC+1`; no new effect
kind, CPU branch, public ABI, timing, or device/MMR claim is
admitted beyond the single X-only address kind.

Ownership: `REQ-COMMON-P2-W59-LD-XMEM-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W59-LD-XMEM-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W59-LD-XMEM-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W59-LD-XMEM-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W59-LD-XMEM-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W59-LD-XMEM-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_xmem_shift_metadata.c` with CTest name
`ti_c54x.ld_xmem_shift_metadata`,
`tests/unit/isa/decode/test_ld_xmem_shift_decode.c` with CTest name
`ti_c54x.ld_xmem_shift_decode`,
`tests/unit/isa/ops/load/test_ld_xmem_shift.c` with CTest name
`ti_c54x.ld_xmem_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_xmem_shift.c` with CTest name
`ti_c54x.cpu_ld_xmem_shift`; all four are green. The W59 closure
registry is 255 immutable evidence records, 396 requirements (365
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 328 tests (328
`PASSING`, 0 `PLANNED`). Coverage is 141 catalog rows, `ID_COUNT=142`,
5,213 lookup entries, 141 trusted bindings, family coverage 51/205
(24.878%), and 33/205 strict complete-form coverage (`FORM-T2-19-06`
is `COMPLETE`). The row is not reset-runnable (AR contents start
unknown).

### P2-W60: LDR Smem,dst vertical slice (planned)

This immutable `(planned)` heading is the durable W60 FIT/ownership anchor.
W60 activates Table 2-19 form `FORM-T2-19-19`, TI SPRU172C §4 LDR
syntax `LDR Smem, dst` (printed p. 4-78; cross-checked against the
SPRU179C algebraic volume p. 4-78, which agrees on opcode,
execution, and worked example): word0 `0001_011D_IAAAAAAA`
(value `0x1680`, mask `0xFEE0`, destination in bit 8, MOD0--MOD2
single-indirect Smem byte mirroring the `LD_SMEM` row, one program
word, 64 encodings with MOD3 patterns admitted at decode and
rejected transactionally at effect validation as write-only).
The existing `ACCUMULATOR_ARF_MOD` layout carries destination,
ARF, and MOD; repeatability is `UNRESOLVED` (Table 2-28 lists
only `LD ARP`/`LD DP`); input resources are `STATUS_SXM` (status
bits say SXM only: no OVM input, no saturation, no OV effect).
Semantics is `dst = (SXM-widen(Smem) << 16) + 0x8000` with bits
14:0 cleared, i.e. guard is the SXM sign extension, bits 31:16
are Smem verbatim, and bits 15:0 are always `0x8000`; C and TC
stay preserved. The checked/trusted leaves reuse the Smem common
identity/mode validation and single-indirect staging with an
LDR-specific rounding completion. Unknown SXM rejects
transactionally. One program word retires at `PC+1`; no new
layout, address kind, effect kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted. MOD4--MOD15 Smem
variants stay deferred to later slices following the
W43B/W44B pattern, so `FORM-T2-19-19` lands as `SUBSET`.

Ownership: `REQ-COMMON-P2-W60-LDR-SMEM-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W60-LDR-SMEM-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W60-LDR-SMEM-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W60-LDR-SMEM-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W60-LDR-SMEM-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W60-LDR-SMEM-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ldr_smem_metadata.c` with CTest name
`ti_c54x.ldr_smem_metadata`,
`tests/unit/isa/decode/test_ldr_smem_decode.c` with CTest name
`ti_c54x.ldr_smem_decode`,
`tests/unit/isa/ops/load/test_ldr_smem.c` with CTest name
`ti_c54x.ldr_smem`, and
`tests/unit/cpu/isa/load/test_cpu_ldr_smem.c` with CTest name
`ti_c54x.cpu_ldr_smem`; all four are green. The W60 closure
registry is 260 immutable evidence records, 402 requirements (371
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 332 tests (332
`PASSING`, 0 `PLANNED`). Coverage is 142 catalog rows, `ID_COUNT=143`,
5,277 lookup entries, 142 trusted bindings, family coverage 52/205
(25.366%), and 33/205 strict complete-form coverage (`FORM-T2-19-19`
is `SUBSET`). The row is reset-runnable (SXM is reset-known,
no memory or accumulator input required).

### P2-W61: LDR Smem,dst indexed MOD5/MOD6 vertical slice (planned)

This immutable `(planned)` heading is the durable W61 FIT/ownership anchor.
W61 activates two Table 2-19 rows under `FORM-T2-19-19`, TI SPRU172C §4
LDR with AR0-indexed Smem per SPRU131G Table 5-4 (MOD5 `*ARx-AR0`
with `ARx = ARx - AR0`, MOD6 `*ARx+AR0` with `ARx = ARx + AR0`,
borrow/carry boundary rejected): `LDR_MOD5` at first-word value
`0x16A8` and `LDR_MOD6` at `0x16B0`, both under mask `0xFEF8`
with the `ACCUMULATOR_ARF_MOD4` layout (destination, ARF, 4-bit
mode), one program word, `UNRESOLVED` repeatability, and
`STATUS_SXM` resources, 16 encodings each. Semantics reuses the
W60 rounding completion (`dst = (SXM-widen(Smem) << 16) +
0x8000`, C/TC/OV preserved, no OVM input) over the existing
`INDEXED_AR0_WORD16` transaction seam with selected-AR
post-update; AR0 must be known and unknown SXM rejects
transactionally. One program word retires at `PC+1`; no new
layout, address kind, effect kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted. The form stays
`SUBSET` (MOD4/MOD7 reverse-carry and MOD8--MOD15 remain
deferred).

Ownership: `REQ-COMMON-P2-W61-LDR-INDEXED-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W61-LDR-INDEXED-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W61-LDR-INDEXED-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W61-LDR-INDEXED-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W61-LDR-INDEXED-CPU-RETIREMENT` (`src/cpu/`),
and `REQ-PROJECT-P2-W61-LDR-INDEXED-INVENTORY`
(`tools/quality/`) carry the project contract. Following the
W43B shared-family convention, the RED executable contracts
extend the shared indexed tables instead of adding new files:
`tests/unit/isa/catalog/test_indexed_standard_indirect_metadata.c`
(CTest `ti_c54x.indexed_standard_indirect_metadata`, rows 20→22),
`tests/unit/isa/decode/test_indexed_standard_indirect_decode.c`
(CTest `ti_c54x.indexed_standard_indirect_decode`),
`tests/unit/isa/ops/load/test_indexed_standard_indirect_load_effects.c`
(CTest `ti_c54x.indexed_standard_indirect_load_effects`), and
`tests/unit/cpu/memory/test_cpu_indexed_standard_indirect.c`
(CTest `ti_c54x.cpu_indexed_standard_indirect`); all four are
green. The W61 closure
registry is 263 immutable evidence records, 408 requirements (377
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 336 tests (336
`PASSING`, 0 `PLANNED`). Coverage is 144 catalog rows,
`ID_COUNT=145`, 5,309 lookup entries, 144 trusted bindings,
family coverage 52/205 (25.366%), and 33/205 strict
complete-form coverage (`FORM-T2-19-19` stays `SUBSET`; W61 adds
rows to an already-covered family, so family coverage is unchanged
from W60). The rows
are reset-runnable (SXM is reset-known, no memory or accumulator
input required).

### P2-W62: B pmad unconditional-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W62 FIT/ownership anchor.
W62 activates one Table 2-12 row under `FORM-T2-12-01`, TI SPRU172C §4
`B[D]` (printed p. 4-14, cross-checked against the SPRU179C algebraic
volume `goto pmad`, class 29A, §4-14, which agrees on opcode diagram,
`PC = pmad(15–0)` execution, and the worked example): one row `B_PMAD`
at first-word value `0xF073` under mask `0xFFFF` (bits
`1111_00Z0_0011_0011` with Z=0; Z=1 is the deferred delayed `BD`
variant and decodes `UNKNOWN_WORD`), two program words, one encoding.
The new `BRANCH_PMAD16` layout carries the copied 16-bit pmad from
the second bundle word and no first-word operands. Repeatability is
`INELIGIBLE` on the manual's explicit "not repeatable" note (printed
p. 4-14) plus the Table 2-28 absence boundary; input resources are
`NONE` (status-bits note says none: no input, no saturation, no
status effect). Semantics is `PC = pmad` with all other state
preserved; the worked example pins `B 2000h` with `PC=1F45` to
`PC=2000`.

The slice adds the first PC-target seam: a new
`TI_C54X_OP_EFFECT_PC_WRITE` kind carrying one copied `uint16_t`
pmad, a `src/isa/ops/branch/op_b_pmad.[ch]` pure checked/trusted
leaf (per-row identity validation, transactional output discipline),
a `src/sequencer/program_pc_target.[ch]` planner that validates and
stages the target in the bridge's transactional style, and a `run.c`
route that commits the target atomically with the staged effect
transaction instead of the sequential bridge plan. No MEMORY
endpoint, no new address kind, no status/accumulator/T effect, no
public ABI, timing, device/MMR, or delay-slot claim is admitted. The
form lands `SUBSET` (delayed `BD` deferred to the delay seam).

Ownership: `REQ-COMMON-P2-W62-B-PMAD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W62-B-PMAD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W62-B-PMAD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W62-B-PMAD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W62-B-PMAD-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W62-B-PMAD-INVENTORY` (`tools/quality/`) carry the
project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_b_pmad_metadata.c`
(CTest `ti_c54x.b_pmad_metadata`),
`tests/unit/isa/decode/test_b_pmad_decode.c`
(CTest `ti_c54x.b_pmad_decode`),
`tests/unit/isa/ops/branch/test_b_pmad.c`
(CTest `ti_c54x.b_pmad`), and
`tests/unit/cpu/isa/branch/test_cpu_b_pmad.c`
(CTest `ti_c54x.cpu_b_pmad`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule (the future BD
slice updates this test). Projected closure is 145 catalog rows,
`ID_COUNT=146`, 5,310 lookup entries, 145 trusted bindings, family
coverage 53/205 (25.854%), and 33/205 strict complete-form coverage
(`FORM-T2-12-01` stays `SUBSET`); 268 evidence records, 414
requirements (383 `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and
340 tests (340 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P2-W63: BC pmad,cond conditional-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W63 FIT/ownership anchor.
W63 activates one Table 2-12 row under `FORM-T2-12-04`, TI SPRU172C §4
`BC[D]` (printed pp. 4-18–4-20, cross-checked against the SPRU179C
algebraic volume `if (cond(s)) goto pmad`, class 31A, §4-18, which
agrees on the opcode diagram, conditional `PC = pmad(15–0)` execution,
and the worked examples): one row `BC_PMAD` at first-word value
`0xF800` under mask `0xFF00` (bits `1111_10Z0` fixed with Z=0 plus
the 8-bit condition code in bits 7:0; Z=1 is the deferred delayed
`BCD` variant and decodes `UNKNOWN_WORD`), two program words, 256
encodings. The new `BRANCH_COND_PMAD16` layout carries the copied
condition code and the second-word pmad. Repeatability is
`INELIGIBLE` on the manual's explicit "not repeatable" note (printed
p. 4-19); input resources are the new
`ACCUMULATORS_STATUS_C_TC` combination. Semantics: if the condition
holds, `PC = pmad`; else `PC = PC+2`; no status effect for the
implemented conditions. Worked examples pin `BC 2000h,AGT` with
`A=00 0000 0053,PC=1000` to `PC=2000` and with `A=FF FFFF FFFF` to
`PC=1002`.

The slice implements the 16 image-evidenced single conditions over
1,752 of the 1,754 static uses: TC (`0011_0000`), NTC (`0010_0000`),
C (`0000_1100`), NC (`0000_1000`), and the twelve accumulator
compares AEQ/ANEQ/AGT/AGEQ/ALT/ALEQ (`0100_0101/0100/0110/0010/0011/
0111`) and BEQ/BNEQ/BGT/BGEQ/BLT/BLEQ (`0100_1101/1100/1110/1010/
1011/1111`). Accumulator compares are full-40-bit two's-complement
signed project-interpretation pins (guard bits included; TI shows no
guard-nonzero vector): EQ/NEQ/GT/GEQ/LT/LEQ against zero. BIO/NBIO
(external-pin seam), UNC, AOV/BOV/ANOV/BNOV (OVA/OVB effect seam),
multi-condition combinations, and delayed `BCD` stay deferred; their
codes reject `INVALID_OPERAND` transactionally. Knownness is demanded
only for consumed inputs (TC cond ignores unknown accumulators).
The taken path reuses the W62 `PC_WRITE` route; the not-taken path
emits effect `NONE` through the proven sequential bridge (no `run.c`
change). The form lands `SUBSET`.

Ownership: `REQ-COMMON-P2-W63-BC-PMAD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W63-BC-PMAD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W63-BC-PMAD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W63-BC-PMAD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W63-BC-PMAD-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W63-BC-PMAD-INVENTORY`
(`tools/quality/`) carry the project contract. The four RED
executable contracts are
`tests/unit/isa/catalog/test_bc_pmad_metadata.c`
(CTest `ti_c54x.bc_pmad_metadata`),
`tests/unit/isa/decode/test_bc_pmad_decode.c`
(CTest `ti_c54x.bc_pmad_decode`),
`tests/unit/isa/ops/branch/test_bc_pmad.c`
(CTest `ti_c54x.bc_pmad`), and
`tests/unit/cpu/isa/branch/test_cpu_bc_pmad.c`
(CTest `ti_c54x.cpu_bc_pmad`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule. Projected closure
is 146 catalog rows, `ID_COUNT=147`, 5,566 lookup entries, 146
trusted bindings, family coverage 54/205 (26.341%), and 33/205
strict complete-form coverage (`FORM-T2-12-04` stays `SUBSET`); 273
evidence records, 420 requirements (389 `IMPLEMENTED`, 0 `PLANNED`,
31 `UNRESOLVED`), and 344 tests (344 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P2-W64: stack-pointer input seam vertical slice (planned)

This immutable `(planned)` heading is the durable W64 FIT/ownership anchor.
W64 adds the stack-pointer input seam that the W65 `CALL` row will
consume, mirroring the SST seam precedent (`d21b884`): one new input
resource bit `TI_C54X_INPUT_RESOURCES_STACK_POINTER`, snapshot fields
`stack_pointer_raw16` plus `stack_pointer_known`, a `collect.c` arm
reading the architectural SP through
`ti_c54x_arch_state_read_sp`, and KNOWN_MASK inclusion. The
op-input carrier ABI grows accordingly (LP64 and ILP32 static
asserts plus their stale messages are updated to the verified
sizes). No catalog row, layout, handler, effect, inventory form, or
CPU retirement change is admitted; SP commit and the stack
transaction belong to W65.

Ownership: `REQ-PROJECT-P2-W64-SP-INPUT-SEAM` (`src/cpu/input/`,
`src/isa/ops/`) carries the project contract. The RED executable
contracts are `tests/unit/cpu/input/test_collect_stack_pointer.c`
(CTest `ti_c54x.collect_stack_pointer`) plus the updated carrier
ABI asserts in
`tests/unit/cpu/input/test_collect_ovm_status.c`. Projected closure
is 273→275 evidence records, 421 requirements (390 `IMPLEMENTED`,
0 `PLANNED`, 31 `UNRESOLVED`), and 345 tests (345 `PASSING`, 0
`PLANNED`) with catalog, lookup, bindings, and family coverage
unchanged. These numbers are projections until the slice lands.

### P2-W65A: stack-push MEMORY transaction seam (planned)

This immutable `(planned)` heading is the durable W65A FIT/ownership
anchor. W65A is a generic project-contract seam only, mirroring the
W42A precedent: it adds no catalog row, opcode handler, layout, CPU
opcode branch, inventory form, or TI claim. It gives the W65B `CALL`
row a stack transaction that pushes one WORD16 through the existing
MEMORY root with commit-at-READY atomicity.

The seam adds one address kind
`TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16` with an all-zero spec
(arf zero; every mode field zero), one pure planner leaf
`src/address/stack_push.[ch]` mapping old SP to the TOS DATA WORD16
at `SP-1` with a `PRE_ACCESS` SP update to `SP-1` (the access address
is the post-update register, mirroring the MOD3 precedent), one
update target `TI_C54X_ADDRESS_UPDATE_SP` through the
intrinsic/dependency/validate/apply chain (mode-independent: no
CMPT/ARP/CPL read; unknown live SP rejects; live SP must match the
frozen snapshot), a `STACK_PUSH` resolution arm issuing the DATA
WRITE request, and root prepare/commit support: prepare stages the
live return address (bridge successor) into the frozen effect copy
(the pure handler cannot know PC; the shape check requires request
value == frozen effect value), commit writes SP and the frozen pmad
target atomically at READY with the existing WAIT/FAULT/resume
machinery unchanged. 16-bit SP arithmetic wraps mod 65536
(`SP=0` pushes at `0xFFFF` with SP after `0xFFFF`); the manual states
no stack-overflow status, so wrap is an explicit
project-interpretation pin, not a TI claim. No `run.c` change is
admitted; the existing MEMORY route retires the push.

Ownership: `REQ-PROJECT-P2-W65A-STACK-PUSH-PLANNER`
(`src/address/`), `REQ-PROJECT-P2-W65A-STACK-PUSH-VALIDATE`
(`src/sequencer/` intrinsic/dependency/validate/apply),
`REQ-PROJECT-P2-W65A-STACK-PUSH-MEMORY`
(`src/isa/ops/memory/`, `src/sequencer/memory/`), and
`REQ-PROJECT-P2-W65A-STACK-PUSH-ROOT`
(`src/sequencer/instruction/`) carry the project contract. The three
RED executable contracts are
`tests/unit/address/test_stack_push_planner.c`
(CTest `ti_c54x.stack_push_planner`),
`tests/unit/sequencer/memory/test_stack_push_resolution.c`
(CTest `ti_c54x.stack_push_resolution`), and
`tests/unit/sequencer/instruction/test_stack_push_root.c`
(CTest `ti_c54x.stack_push_root`), driven with hand-built effects and
bundles; no catalog identity is consumed. Projected closure is 277
evidence records, 425 requirements (394 `IMPLEMENTED`, 0 `PLANNED`,
31 `UNRESOLVED`), and 348 tests (348 `PASSING`, 0 `PLANNED`) with
catalog, lookup, bindings, and family coverage unchanged. These
numbers are projections until the slice lands.

### P2-W65B: CALL pmad vertical slice (planned)

This immutable `(planned)` heading is the durable W65B FIT/ownership
anchor. W65B activates one Table 2-13 row under `FORM-T2-13-02`, TI
SPRU172C §4 `CALL[D]` (printed pp. 4-27–4-28, cross-checked against
the SPRU179C algebraic volume `call pmad`, class 29B, §4-27, which
agrees on `SP-1 → SP`, `PC+2 → TOS`, `pmad → PC`, and the worked
example): one row `CALL_PMAD` at first-word value `0xF074` under mask
`0xFFFF` (bits `1111_00Z0_0111_0100` with Z=0; Z=1 word `0xF274` is
the deferred delayed `CALLD` variant and decodes `UNKNOWN_WORD`),
two program words, one encoding. (An early research misread of the
diagram as `0xF034` was corrected against the `re/dsp` disassembly,
which renders `f074` as `call`; `0xF074` is unclaimed by every
existing row.) The row reuses the `BRANCH_PMAD16` layout, declares
`STACK_POINTER` input resources (consuming the W64 seam; `collect.c`
arms it generically), and is `INELIGIBLE` on the manual's explicit
"not repeatable" note plus the Table 2-28 absence boundary. The
handler demands known SP (`INVALID_OPERAND` otherwise) and stages a
MEMORY `STACK_PUSH` WRITE with a zero value placeholder; the W65A
root derives the `PC+2` return address at prepare and commits SP,
TOS, and `PC = pmad` atomically. The worked example pins `CALL
3333h` with `PC=0025,SP=1111` to `TOS[1110]=0027,SP=1110,PC=3333`.
The form lands `SUBSET` (delayed `CALLD` deferred to the delay
seam).

Ownership: `REQ-COMMON-P2-W65B-CALL-PMAD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W65B-CALL-PMAD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W65B-CALL-PMAD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W65B-CALL-PMAD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W65B-CALL-PMAD-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W65B-CALL-PMAD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_call_pmad_metadata.c`
(CTest `ti_c54x.call_pmad_metadata`),
`tests/unit/isa/decode/test_call_pmad_decode.c`
(CTest `ti_c54x.call_pmad_decode`),
`tests/unit/isa/ops/branch/test_call_pmad.c`
(CTest `ti_c54x.call_pmad`), and
`tests/unit/cpu/isa/branch/test_cpu_call_pmad.c`
(CTest `ti_c54x.cpu_call_pmad`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule (the future CALLD
slice updates this test). Projected closure is 147 catalog rows,
`ID_COUNT=148`, 5,567 lookup entries, 147 trusted bindings, family
coverage 55/205 (26.829%), and 33/205 strict complete-form coverage
(`FORM-T2-13-02` stays `SUBSET`); 279 evidence records, 431
requirements (400 `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and
352 tests (352 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P2-W66A: stack-pop MEMORY transaction seam (planned)

This immutable `(planned)` heading is the durable W66A FIT/ownership
anchor. W66A is a generic project-contract seam only, mirroring the
W65A precedent: it adds no catalog row, opcode handler, layout, CPU
opcode branch, inventory form, or TI claim. It gives the W66B `RET`
row a stack transaction that pops one WORD16 through the existing
MEMORY root with commit-at-READY atomicity.

The seam adds one address kind
`TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16` with an all-zero spec
(arf zero; every mode field zero), one pure planner leaf
`src/address/stack_pop.[ch]` mapping old SP to the TOS DATA WORD16
at old SP with a `POST_ACCESS` SP update to `SP+1` mod 65536 (the
access address is the pre-update register, mirroring the MOD0
post-increment shape), a `POST_ACCESS` shape through the existing
`ADDRESS_UPDATE_SP` target (the source kind stays `STACK_PUSH`:
the snapshot is identical and only the phase differs), a
`STACK_POP` resolution arm issuing the DATA READ request with no
mode read, and root prepare/commit support: prepare proves any
16-bit TOS value is plannable against the live PC carrier before
any bus issue, and commit plans the absolute target from the
transaction's own READ observation and commits SP plus the TOS
target atomically at READY. `SP=0xFFFF` pops to `SP=0x0000`; like
the push wrap this is an explicit project-interpretation pin, not
a TI claim. The READ value is never routed through the ordinary
accumulator/status completion: the handler stages `complete ==
NULL` and the pop commit consumes the observation directly. No
`run.c` change is admitted; the existing MEMORY route retires the
pop.

Ownership: `REQ-PROJECT-P2-W66A-STACK-POP-PLANNER`
(`src/address/`), `REQ-PROJECT-P2-W66A-STACK-POP-VALIDATE`
(`src/sequencer/` intrinsic POST shape),
`REQ-PROJECT-P2-W66A-STACK-POP-MEMORY`
(`src/isa/ops/memory/`, `src/sequencer/memory/`), and
`REQ-PROJECT-P2-W66A-STACK-POP-ROOT`
(`src/sequencer/instruction/`) carry the project contract. The
three RED executable contracts are
`tests/unit/address/test_stack_pop_planner.c`
(CTest `ti_c54x.stack_pop_planner`),
`tests/unit/sequencer/memory/test_stack_pop_resolution.c`
(CTest `ti_c54x.stack_pop_resolution`), and
`tests/unit/sequencer/instruction/test_stack_pop_root.c`
(CTest `ti_c54x.stack_pop_root`), driven with hand-built effects
and bundles; no catalog identity is consumed. Projected closure
is 284 evidence records, 435 requirements (404 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 355 tests (355 `PASSING`, 0
`PLANNED`) with catalog, lookup, bindings, and family coverage
unchanged. These numbers are projections until the slice lands.

### P2-W66B: RET vertical slice (planned)

This immutable `(planned)` heading is the durable W66B FIT/ownership
anchor. W66B activates one Table 2-15 row under `FORM-T2-15-04`, TI
SPRU172C §4 `RET[D]` (printed p. 4-139, cross-checked against the
SPRU179C instruction list `[d]return`, "Unconditional return",
which agrees on the mnemonic family): one row `RET` at first-word
value `0xFC00` under mask `0xFFFF` (bits `1111_11Z0_0000_0000`
with Z=0; Z=1 word `0xFE00` is the deferred delayed `RETD`
variant and decodes `UNKNOWN_WORD`), one program word, one
encoding, no operands. (Both words are unclaimed by every
existing row.) The row reuses the `OPERAND_LAYOUT_NONE` layout,
declares `STACK_POINTER` input resources (consuming the W64 seam;
`collect.c` arms it generically), and is `INELIGIBLE` on the
manual's explicit "not repeatable" note plus the Table 2-28
absence boundary. The handler demands known SP
(`INVALID_OPERAND` otherwise) and stages a MEMORY `STACK_POP`
READ with a zero value and null completion; the W66A root reads
TOS at old SP and commits `SP+1` and `PC = TOS` atomically. The
worked example pins `RET` with `PC=2112,SP=0300,TOS[0300]=1000`
to `PC=1000,SP=0301`. The form lands `SUBSET` (delayed `RETD`
deferred to the delay seam; `RETE`/`RETF` stay `UNRESOLVED`).

Ownership: `REQ-COMMON-P2-W66B-RET-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W66B-RET-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W66B-RET-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W66B-RET-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W66B-RET-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W66B-RET-INVENTORY` (`tools/quality/`) carry the
project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_ret_metadata.c`
(CTest `ti_c54x.ret_metadata`),
`tests/unit/isa/decode/test_ret_decode.c`
(CTest `ti_c54x.ret_decode`),
`tests/unit/isa/ops/branch/test_ret.c`
(CTest `ti_c54x.ret`), and
`tests/unit/cpu/isa/branch/test_cpu_ret.c`
(CTest `ti_c54x.cpu_ret`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule (the future
RETD slice updates this test). Projected closure is 148 catalog
rows, `ID_COUNT=149`, 5,568 lookup entries, 148 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-15-04` stays `SUBSET`); 289
evidence records, 441 requirements (410 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 359 tests (359 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W67A: delayed-branch MEMORY transaction seam (planned)

This immutable `(planned)` heading is the durable W67A FIT/ownership
anchor. W67A is a generic project-contract seam only, mirroring the
W65A/W66A precedent: it adds no catalog row, opcode handler,
layout, CPU opcode branch, `run.c` change, inventory form, or TI
claim. It gives the W67B `RETD` and W67C `CALLD` rows a stack
transaction that defers only the PC redirect while committing the
stack update immediately.

TI semantics (SPRU172C §4 `RET[D]`, printed p. 4-139: TOS→PC with
SP+1 while the two following words execute; §4 `CALL[D]`,
printed p. 4-27: delayed execution pushes `(PC)+4`, i.e. the
address after the two delay-slot words, with the §4-27 worked
example pushing `0029` for `CALLD` at `0025`; SPRU131G §6.3.1
states the two following words execute and forbids a
PC-discontinuity inside them) split the commit: the SP decrement
or increment and the TOS WRITE publish at the delayed branch's
own retirement, and only the PC transfer waits for two program
words (one 2-word instruction or two 1-word instructions). The
§4-27 Execution block and its worked example are normative for
the `PC+4` return address; the Example 7-5 pipeline narrative in
SPRU131G §7 labels the early return register `a3` loosely and is
not adopted for the pushed value.

The seam adds two address kinds,
`TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16` and
`TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16`, with the
same all-zero spec and access shapes as the immediate kinds
(WRITE-only with the staged return address; READ-only with zero
value and null completion). Planning and resolution are shared
with the immediate arms: the pop observes old SP with a
`POST_ACCESS` update, the push observes old SP with a
`PRE_ACCESS` update. The root prepare proves the sequential
successor plus, for pushes, the `PC+4` return address: the staged
TOS WRITE carries successor+2, rejected as `INVALID_CANDIDATE`
when it exceeds `0xFFFF` (a delayed call whose return address
leaves 16-bit program space is malformed; TI code always has two
slot words after the call). The pop prepare proves TOS-target
plannability as before. Commit sets SP plus the TOS value
atomically at READY exactly like the immediate path, but writes
PC to the sequential successor instead of the branch target and
reports the deferred target (TOS for pops, the frozen pmad for
pushes) in the transaction result. The result carries
`deferred_branch_armed` plus the 16-bit deferred PC; ordinary
retirements leave it disarmed. The pure value
`src/sequencer/delayed_branch/delayed_branch.[ch]` owns the
two-word countdown with saturating consume (a slot fill wider
than the remaining words still takes the branch: malformed code
gets a total rule, not a halt) and replace-on-arm. The CPU
instance owns one such value, cleared by init and reset; no
`run.c` change is admitted (the W67B slice wires adopt/advance).

Ownership: `REQ-PROJECT-P2-W67A-DELAYED-BRANCH-VALUE`
(`src/sequencer/delayed_branch/`),
`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-VALIDATE`
(`src/sequencer/` intrinsic shape),
`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-MEMORY`
(`src/isa/ops/memory/`, `src/sequencer/memory/`), and
`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-ROOT`
(`src/sequencer/instruction/`) carry the project contract. The
three RED executable contracts are
`tests/unit/sequencer/delayed_branch/test_delayed_branch.c`
(CTest `ti_c54x.delayed_branch`),
`tests/unit/sequencer/instruction/test_delayed_stack_pop_root.c`
(CTest `ti_c54x.delayed_stack_pop_root`), and
`tests/unit/sequencer/instruction/test_delayed_stack_push_root.c`
(CTest `ti_c54x.delayed_stack_push_root`), driven with hand-built
effects and bundles; no catalog identity is consumed. The root
proofs split pop/push into one file per kind (each under 500
lines), mirroring the W65A/W66A layout. Projected
closure is 291 evidence records, 445 requirements (414
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 362 tests
(362 `PASSING`, 0 `PLANNED`) with catalog, lookup, bindings,
and family coverage unchanged. These numbers are projections
until the slice lands.

### P2-W67B: RETD vertical slice (planned)

This immutable `(planned)` heading is the durable W67B FIT/ownership
anchor. W67B activates one Table 2-15 row under `FORM-T2-15-04`, TI
SPRU172C §4 `RET[D]` (printed p. 4-139): one row `RETD` at
first-word value `0xFE00` under mask `0xFFFF` (bits
`1111_11Z0_0000_0000` with Z=1), one program word, one encoding,
no operands. The row reuses the `OPERAND_LAYOUT_NONE` layout,
declares `STACK_POINTER` input resources, and is `INELIGIBLE` on
the manual's explicit "not repeatable" note. The handler demands
known SP (`INVALID_OPERAND` otherwise) and stages a MEMORY
`STACK_POP_DELAYED` READ with a zero value and null completion;
the W67A root reads TOS at old SP, commits `SP+1` atomically,
advances PC to the one-word successor, and reports TOS deferred.
The W67B `run.c` wiring adopts the report into the CPU-owned
countdown (replacing any pending branch) and, after every later
retirement that arms nothing new, either disarms on an inner
PC-discontinuity (a non-delayed stack branch or `PC_WRITE`
effect wins; TI forbids these in delay slots per SPRU131G
§6.3.1 printed p. 6-6) or consumes the retired word count,
taking the deferred branch (PC=TOS) at zero with saturating
consume for malformed fills. WAIT/FAULT/budget-exhaustion leave
the countdown untouched in the instance; reset clears it. The
form lands `SUBSET` (`RETE`/`RETF` stay `UNRESOLVED`; `CALLD`
waits for W67C). This slice updates the W66B `test_ret_decode.c`
deferred-word pin (0xFE00 now decodes `RETD`) under the
cross-slice hazard rule, including the bit-9 near-miss entry.

Ownership: `REQ-COMMON-P2-W67B-RETD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W67B-RETD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W67B-RETD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W67B-RETD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W67B-RETD-CPU-RETIREMENT` (`src/cpu/`, owning
the run-loop adopt/advance wiring),
and `REQ-PROJECT-P2-W67B-RETD-INVENTORY` (`tools/quality/`)
carry the project contract. The four RED executable contracts
are `tests/unit/isa/catalog/test_retd_metadata.c`
(CTest `ti_c54x.retd_metadata`),
`tests/unit/isa/decode/test_retd_decode.c`
(CTest `ti_c54x.retd_decode`),
`tests/unit/isa/ops/branch/test_retd.c`
(CTest `ti_c54x.retd`), and
`tests/unit/cpu/isa/branch/test_cpu_retd.c`
(CTest `ti_c54x.cpu_retd`). Projected closure is 149 catalog
rows, `ID_COUNT=150`, 5,569 lookup entries, 149 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-15-04` stays `SUBSET`); 297
evidence records, 451 requirements (420 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 366 tests (366 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W67C: CALLD vertical slice (planned)

This immutable `(planned)` heading is the durable W67C FIT/ownership
anchor. W67C activates one Table 2-13 row under `FORM-T2-13-02`, TI
SPRU172C §4 `CALL[D]` (printed pp. 4-27–4-28, already pinned by the
W65B TI records): one row `CALLD` at first-word value `0xF274`
under mask `0xFFFF` (bits `1111_00Z0_0111_0100` with Z=1), two
program words, one encoding. The row reuses the `BRANCH_PMAD16`
layout, declares `STACK_POINTER` input resources, and is
`INELIGIBLE` on the manual's explicit "not repeatable" note. The
handler demands known SP (`INVALID_OPERAND` otherwise) and stages a
MEMORY `STACK_PUSH_DELAYED` WRITE with a zero value placeholder;
the W67A root stages `PC+4` into TOS (the CALLD worked example
pushes `0029` for `CALLD` at `0025`), commits SP and TOS, advances
PC to the sequential successor, and reports pmad deferred. The
W67B `run.c` countdown adopts the report unchanged, so no `run.c`
change is admitted by this slice. The form stays `SUBSET` (a
`COMPLETE` form remains one production catalog row per the
inventory schema; `CALL_PMAD` keeps `ABSOLUTE_PMAD_NONDELAYED`
and `CALLD` takes `ABSOLUTE_PMAD_DELAYED`). This slice updates the
W65B `test_call_pmad_decode.c` deferred-word pin (0xF274 now
decodes `CALLD`) under the cross-slice hazard rule, including the
bit-9 near-miss entry.

Ownership: `REQ-COMMON-P2-W67C-CALLD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W67C-CALLD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W67C-CALLD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence
(reusing the W65B `CALL[D]` opcode, semantics, and nonrepeatable
records; no new TI record is needed);
`REQ-PROJECT-P2-W67C-CALLD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W67C-CALLD-CPU-RETIREMENT` (`src/cpu/`, owning
the no-change adoption of the W67B countdown wiring), and
`REQ-PROJECT-P2-W67C-CALLD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_calld_metadata.c`
(CTest `ti_c54x.calld_metadata`),
`tests/unit/isa/decode/test_calld_decode.c`
(CTest `ti_c54x.calld_decode`),
`tests/unit/isa/ops/branch/test_calld.c`
(CTest `ti_c54x.calld`), and
`tests/unit/cpu/isa/branch/test_cpu_calld.c`
(CTest `ti_c54x.cpu_calld`). Projected closure is 150 catalog
rows, `ID_COUNT=151`, 5,570 lookup entries, 150 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-13-02` stays `SUBSET`); 299
evidence records, 457 requirements (426 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 370 tests (370 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W68: BD delayed-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W68 FIT/ownership
anchor. W68 activates one Table 2-12 row under `FORM-T2-12-01`, TI
SPRU172C §4 `B[D]` (printed p. 4-14, already pinned by the W62 TI
records): one row `BD` at first-word value `0xF273` under mask
`0xFFFF` (bits `1111_00Z0_0011_0011` with Z=1), two program
words, one encoding. The row reuses the `BRANCH_PMAD16` layout,
declares no input resources, and is `INELIGIBLE` on the manual's
explicit "not repeatable" note. The slice adds the first
non-MEMORY deferred-branch seam, mirroring the W62 combined
seam-plus-row shape: a new `TI_C54X_OP_EFFECT_DELAYED_PC_WRITE`
kind carrying one copied 16-bit pmad, a pure handler emitting it
with all other state preserved, and a `run.c` route that
validates the absolute target through the existing target
planner, commits the sequential successor, retires, and arms the
CPU-owned countdown with pmad (replacing any pending countdown;
the arming instruction consumes no slot words itself). No MEMORY
endpoint, no new address kind, no status/accumulator/T effect,
no public ABI, timing, or device claim is admitted. The form
stays `SUBSET` (`B_PMAD` keeps `ABSOLUTE_PMAD_NONDELAYED`
while the new row takes `ABSOLUTE_PMAD_DELAYED`; the delayed
conditional `BCD` waits for a later slice reusing this route
unchanged). This slice updates the W62 `test_b_pmad_decode.c`
deferred-word pin (0xF273 now decodes `BD`) under the
cross-slice hazard rule, including the bit-9 near-miss entry.

Ownership: `REQ-COMMON-P2-W68-BD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W68-BD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W68-BD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence
(reusing the W62 `B[D]` opcode, semantics, and nonrepeatable
records; no new TI record is needed);
`REQ-PROJECT-P2-W68-BD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W68-BD-CPU-RETIREMENT` (`src/cpu/`, owning
the new delayed-PC route), and
`REQ-PROJECT-P2-W68-BD-INVENTORY` (`tools/quality/`) carry the
project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_bd_metadata.c`
(CTest `ti_c54x.bd_metadata`),
`tests/unit/isa/decode/test_bd_decode.c`
(CTest `ti_c54x.bd_decode`),
`tests/unit/isa/ops/branch/test_bd.c`
(CTest `ti_c54x.bd`), and
`tests/unit/cpu/isa/branch/test_cpu_bd.c`
(CTest `ti_c54x.cpu_bd`). Projected closure is 151 catalog
rows, `ID_COUNT=152`, 5,571 lookup entries, 151 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-12-01` stays `SUBSET`); 301
evidence records, 463 requirements (432 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 374 tests (374 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W69: BCD delayed-conditional-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W69 FIT/ownership
anchor. W69 activates one Table 2-12 row under `FORM-T2-12-04`, TI
SPRU172C §4 `BC[D]` (printed pp. 4-18–4-20, already pinned by the
W63 TI records): one row `BCD` at first-word value `0xFA00`
under mask `0xFF00` (bits `1111_10Z0` fixed with Z=1 plus the
8-bit condition code in bits 7:0), two program words, 256
encodings. The row reuses the `BRANCH_COND_PMAD16` layout,
declares the `ACCUMULATORS_STATUS_C_TC` input combination, and is
`INELIGIBLE` on the manual's explicit "not repeatable" note. The
pure handler evaluates the sixteen image-evidenced single
conditions exactly as the W63 leaf (shared branch-condition
helper, no semantic fork), emitting taken
`DELAYED_PC_WRITE` versus not-taken `NONE`; deferred codes
reject `INVALID_OPERAND` and consumed-input-only knownness is
preserved. The W68 `run.c` delayed-PC route retires taken
branches and the proven sequential bridge retires not-taken
ones, so no `run.c` change is admitted by this slice. No MEMORY
endpoint, no new effect kind, no status/accumulator/T effect, no
public ABI, timing, or device claim is admitted. The form stays
`SUBSET` (`BC_PMAD` keeps `ABSOLUTE_PMAD_COND` while the new row
takes `ABSOLUTE_PMAD_COND_DELAYED`; multi-condition combinations
stay deferred). This slice updates the W63
`test_bc_pmad_decode.c` deferred-word pin (0xFA00 now decodes
`BCD`) under the cross-slice hazard rule, including the bit-9
near-miss entry.

Ownership: `REQ-COMMON-P2-W69-BCD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W69-BCD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W69-BCD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence
(reusing the W63 `BC[D]` opcode, semantics, and nonrepeatable
records; no new TI record is needed);
`REQ-PROJECT-P2-W69-BCD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W69-BCD-CPU-RETIREMENT` (`src/cpu/`, owning
the no-change adoption of the W68 delayed-PC route), and
`REQ-PROJECT-P2-W69-BCD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_bcd_metadata.c`
(CTest `ti_c54x.bcd_metadata`),
`tests/unit/isa/decode/test_bcd_decode.c`
(CTest `ti_c54x.bcd_decode`),
`tests/unit/isa/ops/branch/test_bcd.c`
(CTest `ti_c54x.bcd`), and
`tests/unit/cpu/isa/branch/test_cpu_bcd.c`
(CTest `ti_c54x.cpu_bcd`). Projected closure is 152 catalog
rows, `ID_COUNT=153`, 5,827 lookup entries, 152 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-12-04` stays `SUBSET`); 303
evidence records, 469 requirements (438 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 378 tests (378 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W70: BANZ auxiliary-register-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W70 FIT/ownership
anchor. W70 activates one Table 2-12 row under `FORM-T2-12-03`, TI
SPRU172C §4 `BANZ[D]` (printed pp. 4-16–4-17): one row `BANZ`
at first-word value `0x6C00` under mask `0xFD00` (bits
`0110_11Z0` fixed with Z=0 plus the 8-bit Sind field), two
program words, 256 encodings. The row reuses the `BRANCH_PMAD16`
pmad half with a new 8-bit Sind operand, declares the
`AUXILIARY_REGISTERS` input combination, and is `INELIGIBLE`
on the manual's explicit "not repeatable" note. Semantics per
the manual: the Sind addressing side effect always applies
(`*ARx-` post-decrements even when the branch is not taken),
then taken `ARx != 0` selects pmad else sequential advance;
under the repo CMPT=0 operating condition with explicit ARX
selection (no ARP read). The slice adds the indirect-branch
seam, mirroring the W65A shape: a new
`TI_C54X_OP_EFFECT_BRANCH_AUXILIARY` kind carrying the AR
index, value-after, update flag, taken flag, delay flag, and
copied pmad; a pure handler evaluating taken/not-taken over the
image-evidenced `*ARx`/`*ARx-` Sind subset (I=1, MOD0/MOD1;
all other Sind combinations reject `INVALID_OPERAND` as
deferred) gated on known ARx; and a `run.c` route that
validates the absolute target, gates CMPT=0, commits the AR
write when flagged, retires PC immediate or deferred per the
taken/delay flags, and arms the CPU-owned countdown for
taken-delayed. No MEMORY endpoint, no new address kind, no
status/accumulator/T effect, no public ABI, timing, or device
claim is admitted. The form lands `SUBSET` (delayed `BANZD`
waits for W71 reusing this seam unchanged). This slice needs
three new TI records (opcode diagram, always-applied Sind side
effect with worked examples, nonrepeatable note).

Ownership: `REQ-COMMON-P2-W70-BANZ-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W70-BANZ-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W70-BANZ-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W70-BANZ-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W70-BANZ-CPU-RETIREMENT` (`src/cpu/`, owning
the new auxiliary-branch route), and
`REQ-PROJECT-P2-W70-BANZ-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_banz_metadata.c`
(CTest `ti_c54x.banz_metadata`),
`tests/unit/isa/decode/test_banz_decode.c`
(CTest `ti_c54x.banz_decode`),
`tests/unit/isa/ops/branch/test_banz.c`
(CTest `ti_c54x.banz`), and
`tests/unit/cpu/isa/branch/test_cpu_banz.c`
(CTest `ti_c54x.cpu_banz`). Projected closure is 153 catalog
rows, `ID_COUNT=154`, 6,083 lookup entries, 153 trusted
bindings, family coverage 57/205 (27.805%), and 33/205 strict
complete-form coverage (`FORM-T2-12-03` lands `SUBSET`); 318
evidence records, 475 requirements (444 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 382 tests (382 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W71: BANZD delayed auxiliary-register-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W71 FIT/ownership
anchor. W71 activates one Table 2-12 row under `FORM-T2-12-03`,
TI SPRU172C §4 `BANZ[D]` (printed pp. 4-16–4-17, TI records
reused from W70 so no new TI record is needed): one row `BANZD`
at first-word value `0x6E00` under mask `0xFF00` (bit 9 set
selects the delayed variant), two program words, 256 encodings.
The row reuses the W70 `BRANCH_SIND_PMAD16` layout with
`AUXILIARY_REGISTERS` inputs and `INELIGIBLE` repeatability.
Semantics per the manual: the Sind addressing side effect always
applies (`*ARx-` post-decrements even when the branch is not
taken), then taken `ARx != 0` defers pmad through the two-word
delay slots while not-taken advances sequentially at once; under
the repo CMPT=0 operating condition with explicit ARX selection
(no ARP read). The slice adds a `BANZD` leaf emitting
`BRANCH_AUXILIARY` with the delay flag set over the same
image-evidenced `*ARx`/`*ARx-` Sind subset (I=1, MOD0/MOD1;
all other Sind combinations reject `INVALID_OPERAND` as
deferred) gated on known ARx; shared Sind evaluation hoists
into a branch-Sind helper with the `BANZ` leaf refactored onto
it at zero behavior change. The W70 `run.c` auxiliary-branch
route already retires taken-delayed (sequential advance plus
countdown arm) and not-taken (sequential advance), so no
`run.c` change is admitted; W71 covers the taken-delayed arm
path the W70 slice exercised only by construction. No MEMORY
endpoint, no new layout, effect, address kind, status/
accumulator/T effect, public ABI, timing, or device claim is
admitted. The form stays `SUBSET` with `BANZ` and `BANZD`
symbols. This slice needs no new TI records.

Ownership: `REQ-COMMON-P2-W71-BANZD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W71-BANZD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W71-BANZD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W71-BANZD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W71-BANZD-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-delayed retirement proof), and
`REQ-PROJECT-P2-W71-BANZD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_banzd_metadata.c`
(CTest `ti_c54x.banzd_metadata`),
`tests/unit/isa/decode/test_banzd_decode.c`
(CTest `ti_c54x.banzd_decode`),
`tests/unit/isa/ops/branch/test_banzd.c`
(CTest `ti_c54x.banzd`), and
`tests/unit/cpu/isa/branch/test_cpu_banzd.c`
(CTest `ti_c54x.cpu_banzd`). Projected closure is 154 catalog
rows, `ID_COUNT=155`, 6,339 lookup entries, 154 trusted
bindings, family coverage 57/205 (27.805%), and 33/205 strict
complete-form coverage (`FORM-T2-12-03` stays `SUBSET`); 310
evidence records, 481 requirements (450 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 386 tests (386 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W72: CCD delayed-conditional-call vertical slice (planned)

This immutable `(planned)` heading is the durable W72 FIT/ownership
anchor. W72 activates one Table 2-13 row under `FORM-T2-13-03`,
TI SPRU172C §4 `CC[D]` (printed pp. 4-29–4-31): one row `CCD`
at first-word value `0xFB00` under mask `0xFF00` (bit 9 set
selects the delayed variant; the non-delayed `CC` stays
deferred), two program words, 256 encodings. The row reuses the
`BRANCH_COND_PMAD16` layout (copied 8-bit condition plus
16-bit pmad), declares the new `ACCUMULATORS_STATUS_C_TC`
plus `STACK_POINTER` input combination, and is `INELIGIBLE` on
the manual's explicit "not repeatable" note. Semantics per the
manual: the sixteen image-evidenced single conditions evaluate
through the shared branch-condition helper (BIO/NBIO, UNC,
overflow, and multi-condition combinations reject
`INVALID_OPERAND` as deferred); taken branches stage the
`CALLD`-shaped `MEMORY` stack-push-delayed effect (the root
transaction pushes PC+4 past the slots, plans the absolute
pmad target, and arms the CPU-owned countdown) while not-taken
branches emit `NONE` and retire sequentially at once. Taken
branches additionally gate known SP (`INVALID_OPERAND` per the
`CALLD` leaf convention); not-taken branches consume
condition inputs only. The `CALLD` `MEMORY` route and the
sequential bridge retire both outcomes, so no `run.c` change
is admitted. No new layout, effect, address kind,
status/accumulator/T effect, public ABI, timing, or device
claim is admitted. The form lands `SUBSET` with a lone `CCD`
symbol under an `ABSOLUTE_PMAD_COND_DELAYED` binding (shared
with `BCD`: conditional plus delayed). This slice needs three
new TI records (opcode diagram, delayed PC+4 push with worked
examples, nonrepeatable note).

Ownership: `REQ-COMMON-P2-W72-CCD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W72-CCD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W72-CCD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W72-CCD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W72-CCD-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-deferral retirement proof), and
`REQ-PROJECT-P2-W72-CCD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_ccd_metadata.c`
(CTest `ti_c54x.ccd_metadata`),
`tests/unit/isa/decode/test_ccd_decode.c`
(CTest `ti_c54x.ccd_decode`),
`tests/unit/isa/ops/branch/test_ccd.c`
(CTest `ti_c54x.ccd`), and
`tests/unit/cpu/isa/branch/test_cpu_ccd.c`
(CTest `ti_c54x.cpu_ccd`). Projected closure is 155 catalog
rows, `ID_COUNT=156`, 6,595 lookup entries, 155 trusted
bindings, family coverage 58/205 (28.293%), and 33/205 strict
complete-form coverage (`FORM-T2-13-03` lands `SUBSET`); 312
evidence records, 487 requirements (456 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 390 tests (390 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P2-W73: CC non-delayed-conditional-call vertical slice (planned)

This immutable `(planned)` heading is the durable W73 FIT/ownership
anchor. W73 activates one Table 2-13 row under `FORM-T2-13-03`,
TI SPRU172C §4 `CC[D]` (printed pp. 4-29–4-31): one row `CC`
at first-word value `0xF900` under mask `0xFF00` (bit 9 clear
selects the non-delayed variant; the delayed `CCD` shipped in
W72), two program words, 256 encodings. The row reuses the
`BRANCH_COND_PMAD16` layout (copied 8-bit condition plus
16-bit pmad), declares the W72 `ACCUMULATORS_STATUS_C_TC`
plus `STACK_POINTER` input combination, and is `INELIGIBLE` on
the manual's explicit "not repeatable" note. Semantics per the
manual: the sixteen image-evidenced single conditions evaluate
through the shared branch-condition helper (BIO/NBIO, UNC,
overflow, and multi-condition combinations reject
`INVALID_OPERAND` as deferred); taken branches stage the
`CALL`-shaped `MEMORY` stack-push effect (the root transaction
pushes PC+2, plans the absolute pmad target, and retires
PC=pmad at once with no countdown) while not-taken branches
emit `NONE` and retire sequentially at once. Taken branches
additionally gate known SP (`INVALID_OPERAND` per the `CALL`
leaf convention); not-taken branches consume condition inputs
only. The `CALL` `MEMORY` route and the sequential bridge
retire both outcomes, so no `run.c` change is admitted. No new
layout, effect, address kind, status/accumulator/T effect,
public ABI, timing, or device claim is admitted. The form
stays `SUBSET` with `CCD` and `CC` symbols, the new row under
an `ABSOLUTE_PMAD_COND` binding (shared with `BC_PMAD`:
conditional non-delayed). This slice needs no new TI records:
opcode diagram, semantics, and nonrepeatable note are the W72
records `E-TI-C54X-SPRU172C-S4-CC-OPCODE-P4-29`,
`E-TI-C54X-SPRU172C-S4-CC-SEMANTICS-P4-29-4-31`, and
`E-TI-C54X-SPRU172C-S4-CC-NONREPEATABLE-P4-31`.

Ownership: `REQ-COMMON-P2-W73-CC-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W73-CC-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W73-CC-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W73-CC-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W73-CC-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-immediate retirement proof), and
`REQ-PROJECT-P2-W73-CC-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_cc_metadata.c`
(CTest `ti_c54x.cc_metadata`),
`tests/unit/isa/decode/test_cc_decode.c`
(CTest `ti_c54x.cc_decode`),
`tests/unit/isa/ops/branch/test_cc.c`
(CTest `ti_c54x.cc`), and
`tests/unit/cpu/isa/branch/test_cpu_cc.c`
(CTest `ti_c54x.cpu_cc`). Projected closure is 156 catalog
rows, `ID_COUNT=157`, 6,851 lookup entries, 156 trusted
bindings, family coverage unchanged at 58/205 (28.293%), and
33/205 strict complete-form coverage (`FORM-T2-13-03` stays
`SUBSET`); 317 evidence records, 493 requirements (462
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 394 tests
(394 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P2-W74: RETE interrupt-return vertical slice (planned)

This immutable `(planned)` heading is the durable W74 FIT/ownership
anchor. W74 activates one Table 2-15 row under `FORM-T2-15-05`,
TI SPRU172C §4 `RETE[D]` (printed p. 4-139): one row `RETE`
at first-word value `0xF4EB` under mask `0xFFFF` (bit 9 clear
selects the non-delayed variant; the delayed `RETED` at
`0xF6EB` stays deferred), one program word, one encoding, no
operands. The row reuses the `OPERAND_LAYOUT_NONE` layout,
declares `STACK_POINTER` inputs (the INTM write consumes no
input), and is `INELIGIBLE` on the manual's explicit "not
repeatable" note. Semantics per the manual: `PC=TOS`,
`SP+1`, `0→INTM` (ST1 bit 11), affecting INTM only; the
worked example pins `PC 01C3/SP 2001/TOS 0110` to `PC
0110/SP 2002/ST1 xCxx→x4xx`. The pure leaf stages a `MEMORY`
`STACK_POP` READ with a zero value and an INTM-clearing
completion; the root commits the completion through the
generic pop-completion seam (new: pops honor a non-null
completion exactly like loads) before the pop architecture
commit, so no `run.c` change and no new effect kind are
admitted. The form lands `SUBSET` with a lone `RETE` symbol
under a `STACK_POP_NONDELAYED` binding shared with `RET`.
This slice needs three new TI records (opcode diagram,
TOS/SP/INTM semantics with worked example, nonrepeatable
note).

Ownership: `REQ-COMMON-P2-W74-RETE-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W74-RETE-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W74-RETE-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W74-RETE-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W74-RETE-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the INTM-clear retirement proof), and
`REQ-PROJECT-P2-W74-RETE-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_rete_metadata.c`
(CTest `ti_c54x.rete_metadata`),
`tests/unit/isa/decode/test_rete_decode.c`
(CTest `ti_c54x.rete_decode`),
`tests/unit/isa/ops/branch/test_rete.c`
(CTest `ti_c54x.rete`), and
`tests/unit/cpu/isa/branch/test_cpu_rete.c`
(CTest `ti_c54x.cpu_rete`). Projected closure is 157 catalog
rows, `ID_COUNT=158`, 6,852 lookup entries, 157 trusted
bindings, family coverage 59/205 (28.780%), and 33/205
strict complete-form coverage (`FORM-T2-15-05` lands
`SUBSET`); 322 evidence records, 499 requirements (468
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 398 tests
(398 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P2-W75: RETED delayed-interrupt-return vertical slice (planned)

This immutable `(planned)` heading is the durable W75 FIT/ownership
anchor. W75 activates one Table 2-15 row under `FORM-T2-15-05`,
TI SPRU172C §4 `RETE[D]` (printed p. 4-139): one row `RETED`
at first-word value `0xF6EB` under mask `0xFFFF` (bit 9 set
selects the delayed variant; the non-delayed `RETE` shipped
in W74), one program word, one encoding, no operands. The row
reuses the `OPERAND_LAYOUT_NONE` layout, declares
`STACK_POINTER` inputs, and is `INELIGIBLE` on the manual's
explicit "not repeatable" note. Semantics per the manual with
delayed-branch sequencing: `PC=TOS`, `SP+1`, `0→INTM` (ST1
bit 11); SP and the INTM clear commit at retire while the TOS
redirect alone defers past the two delay-slot words (the same
eager-commit/deferred-redirect split the `RETD` slice proves
for SP). The pure leaf stages a `MEMORY` `STACK_POP_DELAYED`
READ with the shared INTM-clearing completion owned once in
`op_rete_common`; the root commits it through the generic
pop-completion seam, so no `run.c` change, no new effect
kind, and no new address kind are admitted. The form stays
`SUBSET` with `RETE` and `RETED` symbols under a
`STACK_POP_DELAYED` binding shared with `RETD`. This slice
needs no new TI records: opcode (with the Z delayed bit),
semantics, and nonrepeatable note are the W74 records
`E-TI-C54X-SPRU172C-S4-RETE-OPCODE-P4-139`,
`E-TI-C54X-SPRU172C-S4-RETE-SEMANTICS-P4-139`, and
`E-TI-C54X-SPRU172C-S4-RETE-NONREPEATABLE-P4-139`.

Ownership: `REQ-COMMON-P2-W75-RETED-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W75-RETED-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W75-RETED-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W75-RETED-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W75-RETED-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the delayed INTM-clear retirement proof),
and `REQ-PROJECT-P2-W75-RETED-INVENTORY` (`tools/quality/`)
carry the project contract. The four RED executable
contracts are `tests/unit/isa/catalog/test_reted_metadata.c`
(CTest `ti_c54x.reted_metadata`),
`tests/unit/isa/decode/test_reted_decode.c`
(CTest `ti_c54x.reted_decode`),
`tests/unit/isa/ops/branch/test_reted.c`
(CTest `ti_c54x.reted`), and
`tests/unit/cpu/isa/branch/test_cpu_reted.c`
(CTest `ti_c54x.cpu_reted`). Projected closure is 158 catalog
rows, `ID_COUNT=159`, 6,853 lookup entries, 158 trusted
bindings, family coverage unchanged at 59/205 (28.780%), and
33/205 strict complete-form coverage (`FORM-T2-15-05` stays
`SUBSET`); 324 evidence records, 505 requirements (474
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 402 tests
(402 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P2-W77: RC conditional-return vertical slice (planned)

This immutable `(planned)` heading is the durable W77 FIT/ownership
anchor. W77 activates one Table 2-15 row under `FORM-T2-15-03`,
TI SPRU172C §4 `RC[D]` (printed pp. 4-133–4-134): one row `RC`
at first-word value `0xFC00` under mask `0xFF00` (bits
`1111_11Z0` fixed with Z=0 plus the 8-bit condition field),
one program word, 256 encodings. The `0xFC00` word keeps its
`RET` identity under the W77A single-word shadow seam (the
unconditional word behaves identically either way); the
other 255 encodings decode as `RC`. The row adds the 1-word
`BRANCH_COND8` layout, declares the
`ACCUMULATORS_STATUS_C_TC_STACK_POINTER` input combination,
and is `INELIGIBLE` on the manual's explicit "not
repeatable" note. Semantics per the manual: the shared
condition seam evaluates the sixteen image-evidenced single
conditions (BIO/NBIO, UNC, overflow, and multi-condition
combinations reject as deferred, exactly like `BC`/`CC`);
taken stages a `MEMORY` `STACK_POP` READ through the
`RET`-proven pop seam gated on known SP, while not-taken
emits `NONE` for sequential advance with no endpoint
traffic; no status-bit effects either way. No `run.c`
change, no new effect kind, and no new address kind are
admitted. The form lands `SUBSET` with the `RC` symbol under
the shared `STACK_POP_NONDELAYED` binding (delayed `RCD`
waits for W78 reusing this seam unchanged). This slice needs
three new TI records (opcode diagram, TOS/SP semantics with
worked example, nonrepeatable note).

Ownership: `REQ-COMMON-P2-W77-RC-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W77-RC-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W77-RC-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W77-RC-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W77-RC-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-pop/not-taken-sequential
retirement proof), and `REQ-PROJECT-P2-W77-RC-INVENTORY`
(`tools/quality/`) carry the project contract. The four RED
executable contracts are
`tests/unit/isa/catalog/test_rc_metadata.c`
(CTest `ti_c54x.rc_metadata`),
`tests/unit/isa/decode/test_rc_decode.c`
(CTest `ti_c54x.rc_decode`),
`tests/unit/isa/ops/branch/test_rc.c`
(CTest `ti_c54x.rc`), and
`tests/unit/cpu/isa/branch/test_cpu_rc.c`
(CTest `ti_c54x.cpu_rc`). Projected closure is 160 catalog
rows, `ID_COUNT=161`, 7,109 lookup entries, 160 trusted
bindings, family coverage 61/205 (29.756%), and 33/205
strict complete-form coverage (`FORM-T2-15-03` lands
`SUBSET`); 329 evidence records, 511 requirements (480
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 406 tests
(406 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P2-W78: RCD delayed-conditional-return vertical slice (planned)

This immutable `(planned)` heading is the durable W78 FIT/ownership
anchor. W78 activates the delayed companion of W77 under the same
`FORM-T2-15-03`, TI SPRU172C §4 `RC[D]` (printed pp. 4-133–4-134):
one row `RCD` at first-word value `0xFE00` under mask `0xFF00`
(bits `1111_11Z0` with Z=1 plus the 8-bit condition field),
one program word, 256 encodings. The `0xFE00` word keeps its
`RETD` identity under the W77A single-word shadow seam; the
other 255 encodings decode as `RCD`. The row reuses the
`BRANCH_COND8` layout, declares the
`ACCUMULATORS_STATUS_C_TC_STACK_POINTER` input combination,
and is `INELIGIBLE` on the same explicit note. Semantics per
the manual: the shared condition seam evaluates the sixteen
image-evidenced single conditions (deferred codes reject as
in W77); taken stages a `MEMORY` `STACK_POP_DELAYED` READ
through the `RETD`-proven delayed-pop seam gated on known
SP, while not-taken emits `NONE`; the two delay-slot words
execute through the unchanged delayed-branch countdown and
the tested conditions are unaffected by the slots, with no
status-bit effects either way. No `run.c` change, no new
effect kind, and no new address kind are admitted. The form
stays `SUBSET` with the `RCD` symbol added under the shared
`STACK_POP_DELAYED` binding. This slice needs three new TI
records (delayed opcode diagram with Z=1, slot semantics,
nonrepeatable note).

Ownership: `REQ-COMMON-P2-W78-RCD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W78-RCD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W78-RCD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W78-RCD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W78-RCD-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the delayed taken-pop/not-taken
retirement proof), and `REQ-PROJECT-P2-W78-RCD-INVENTORY`
(`tools/quality/`) carry the project contract. The four RED
executable contracts are
`tests/unit/isa/catalog/test_rcd_metadata.c`
(CTest `ti_c54x.rcd_metadata`),
`tests/unit/isa/decode/test_rcd_decode.c`
(CTest `ti_c54x.rcd_decode`),
`tests/unit/isa/ops/branch/test_rcd.c`
(CTest `ti_c54x.rcd`), and
`tests/unit/cpu/isa/branch/test_cpu_rcd.c`
(CTest `ti_c54x.cpu_rcd`). Projected closure is 161 catalog
rows, `ID_COUNT=162`, 7,364 lookup entries, 161 trusted
bindings, family coverage unchanged at 61/205 (29.756%),
and 33/205 strict complete-form coverage
(`FORM-T2-15-03` stays `SUBSET`); 334 evidence records, 517
requirements (486 `IMPLEMENTED`, 0 `PLANNED`, 31
`UNRESOLVED`), and 410 tests (410 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.

### P3-S3j: MPYA-dst register-only vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MPYA dst` form from TI SPRU172C §MPYA
(printed pp. 4-104–4-105), dual-sourced against TI SPRU179C §MPYA
(printed pp. 4-106–4-107, algebraic `dst = T * hi(A)`), is implemented
as `MPYA_DST` with value `0xF48C`, mask `0xFEFF`, D bit 8, two
encodings, the reused `MAX_MIN_ACC` layout, resources
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and repeatability
`ELIGIBLE`. Word 0 is `1111_010D_1000_1100`: D=0 writes A, D=1 writes
B. A single `MASKED` row carries both encodings (two exact rows under
mask `0xFEFF` would collide on the unmasked D bit); the destination
rides the word through the reused single-destination-bit layout,
following the `MAX_ACC`/`MIN_ACC` compare-family precedent — no new
layout, resource, effect, address, state, CPU, public API, or
source-manifest seam is added. Execution is `(T) x (A(32-16))` into
`dst` through the existing multiply core with STORE select and no
rounding (`hi(A)` feeds the core exactly as the S3d Smem leaf does,
with T and multiplicand roles swapped); T and source A are unchanged,
C/TC are preserved, and `OVdst` follows the core overflow. The TI
Example 2 vector (`MPYA B` with `A` = `FF 8765 1111`, `T` = `4567`,
`FRCT` = 0 yielding `B` = `FF DF4D B2A3`) is the independent oracle.
Table 2-28 lists no MPYA-family instruction, so syntax 2 is repeatable
by omission on the same recorded basis as the S3d `ELIGIBLE`
classification. The four RED executable contracts are
`tests/unit/isa/catalog/test_mpya_dst_metadata.c`
(CTest `ti_c54x.mpya_dst_metadata`),
`tests/unit/isa/decode/test_mpya_dst_decode.c`
(CTest `ti_c54x.mpya_dst_decode`),
`tests/unit/isa/ops/multiply/test_mpya_dst_ops.c`
(CTest `ti_c54x.mpya_dst_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_mpya_dst.c`
(CTest `ti_c54x.cpu_mpya_dst`), plus the `SUBSET`
`FORM-T2-03-07` inventory binding. Projected closure is 197 catalog
rows, `ID_COUNT=197`, 13,799 lookup entries, 196 bindings, family
coverage 72/205 (35.122%), and 33/205 strict complete-form coverage;
363 evidence records, 575 requirements, and 459 tests (459 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.

### P3-S3k: SQUR-Smem square vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `SQUR Smem,dst` form from TI SPRU172C
§SQUR (printed pp. 4-160–4-161), dual-sourced against the TI SPRU179C
algebraic restatement, is implemented as `SQUR_SMEM` with value
`0x2600`, mask `0xFE00`, D bit 8, I bit 7, Smem field 6:0, 512
encodings, a new `DESTINATION_INDIRECT_ADDR7` layout (D plus
indirect/addr7 with no round or select bits — the shared
round/select/indirect/addr7 layout would mislabel fixed bit 9 as
rounding), resources `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and
repeatability `ELIGIBLE`. Word 0 is `0010_011D_IAAA_AAAA`: the Smem
word loads T in the read phase and feeds both multiplier inputs, so
pair completion runs `(Smem) x (Smem)` into dst through the existing
multiply core with STORE select and no rounding; C/TC are preserved
and `OVdst` follows the core. The TI Example 1 vector (`SQUR 30, B`
with `mem[031E]` = `000F`, `FRCT` = 0 yielding `B` = `00 0000 00E1`
and `T` = `000F`; 15 squared is 225) is the independent oracle. Table 2-28 lists no
SQUR-family instruction, so syntax 1 is repeatable by omission on the
recorded S3 omission basis. The five RED executable contracts are
`tests/unit/isa/catalog/test_squr_smem_metadata.c`
(CTest `ti_c54x.squr_smem_metadata`),
`tests/unit/isa/decode/test_squr_smem_decode.c`
(CTest `ti_c54x.squr_smem_decode`),
`tests/unit/isa/ops/multiply/test_squr_smem_ops.c`
(CTest `ti_c54x.squr_smem_ops`),
`tests/unit/cpu/isa/multiply/test_cpu_squr_smem.c`
(CTest `ti_c54x.cpu_squr_smem`), and the layout proof
`tests/unit/isa/decode/test_destination_indirect_addr7_layout.c`
(CTest `ti_c54x.destination_indirect_addr7_layout`), plus the `SUBSET`
`FORM-T2-03-09` inventory binding. Syntax 2 (`SQUR A,dst`,
`0xF48D`/`0xF58D`) stays `UNIMPLEMENTED` for its own register-only
slice, which will update the `test_mpya_dst_decode.c` UNKNOWN lane
that pins those words today. Projected closure is 198 catalog rows,
`ID_COUNT=198`, 14,311 lookup entries, 197 bindings, family coverage
73/205 (35.610%), and 33/205 strict complete-form coverage; 365
evidence records, 581 requirements, and 464 tests (464 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P3-S3l: SQUR-Adst register-only vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `SQUR A,dst` form from TI SPRU172C §SQUR
(printed p. 4-162), dual-sourced against TI SPRU179C §Square
(printed p. 4-164, algebraic `B = square(hi(A))`), is implemented as
`SQUR_ADST` with value `0xF48D`, mask `0xFEFF`, D bit 8, two
encodings, the reused `MAX_MIN_ACC` layout, resources
`ACCUMULATORS_STATUS_FRCT_OVM`, and repeatability `ELIGIBLE`. Word 0
is `1111_010D_1000_1101`: D=0 writes A, D=1 writes B. A single
`MASKED` row carries both encodings (two exact rows under mask
`0xFEFF` would collide on the unmasked D bit), following the S3j
`MPYA_DST` precedent — no new layout, resource, effect, address,
state, CPU, public API, or source-manifest seam is added. Execution
is `(A(32-16)) x (A(32-16))` into dst through the existing multiply
core with STORE select and no rounding; T is unaffected, C/TC are
preserved, and `OVdst` follows the core. The SPRU179C Example 2
vector (`B = square(hi(A))` with `A` = `00 000F 0000`, `FRCT` = 1
yielding `B` = `00 0000 01C2`, i.e. 15 squared FRCT-shifted) is the
independent oracle. Table 2-28 lists no SQUR-family instruction, so
syntax 2 is repeatable by omission on the recorded S3 omission basis.
The four RED executable contracts are
`tests/unit/isa/catalog/test_squr_adst_metadata.c`
(CTest `ti_c54x.squr_adst_metadata`),
`tests/unit/isa/decode/test_squr_adst_decode.c`
(CTest `ti_c54x.squr_adst_decode`),
`tests/unit/isa/ops/multiply/test_squr_adst_ops.c`
(CTest `ti_c54x.squr_adst_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_squr_adst.c`
(CTest `ti_c54x.cpu_squr_adst`), plus the `SUBSET`
`FORM-T2-03-10` inventory binding. The `test_mpya_dst_decode.c`
UNKNOWN lane that pins `0xF48D`/`0xF58D` today moves to the new row
as the recorded W57→W58 cross-slice update. Projected closure is 199
catalog rows, `ID_COUNT=199`, 14,313 lookup entries, 198 bindings,
family coverage 74/205 (36.098%), and 33/205 strict complete-form
coverage; 367 evidence records, 586 requirements, and 468 tests (468
`PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.

### P3-S3m: SQURA/SQURS accumulate-squares vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `SQURA Smem,src` and `SQURS Smem,src`
forms from TI SPRU172C §SQURA (printed p. 4-163) and §SQURS (printed
p. 4-164) are implemented as two catalog rows sharing one leaf file
with an ADD/SUBTRACT select: `SQURA_SMEM` with value `0x3800`, mask
`0xFE00` (bits 15:9 `0011100`, S bit 8, I bit 7, Smem field 6:0, 512
encodings) and `SQURS_SMEM` with value `0x3A00`, mask `0xFE00`
(bits 15:9 `0011101`, same field shape, 512 encodings). Both reuse
the S3k `DESTINATION_INDIRECT_ADDR7` layout (S select rides field 0),
resources `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and
repeatability `ELIGIBLE`. Word 0 is `0011_100S_IAAA_AAAA` /
`0011_101S_IAAA_AAAA`: the Smem word loads T in the read phase and
the pair completion runs `(src)` plus/minus `(Smem) x (Smem)` through
the existing multiply core with ADD/SUBTRACT select and no rounding;
C/TC are preserved and `OVdst` follows the core. The TI SQURA
Example 1 vector (`SQURA 30, B` with `mem[031E]` = `000F`, `B` =
`00 0320 0000`, `FRCT` = 0 yielding `B` = `00 0320 00E1` and `T` =
`000F`) and SQURS Example 1 vector (`SQURS 9, A` with `mem[0309]`
= `1234`, `A` = `00 014B 5DB0`, `FRCT` = 0 yielding `A` = `00 0000
0320` and `T` = `1234`; both vectors recomputed independently
before pinning) are the oracles. Table 2-28 lists no SQURA/SQURS
instruction, so both rows are repeatable by omission on the recorded
S3 omission basis. The four RED executable contracts are
`tests/unit/isa/catalog/test_squr_acc_metadata.c`
(CTest `ti_c54x.squr_acc_metadata`),
`tests/unit/isa/decode/test_squr_acc_decode.c`
(CTest `ti_c54x.squr_acc_decode`),
`tests/unit/isa/ops/multiply/test_squr_acc_ops.c`
(CTest `ti_c54x.squr_acc_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_squr_acc.c`
(CTest `ti_c54x.cpu_squr_acc`), plus the `SUBSET`
`FORM-T2-04-21`/`FORM-T2-04-22` inventory bindings. Projected closure
is 201 catalog rows, `ID_COUNT=201`, 15,337 lookup entries, 200
bindings, family coverage 76/205 (37.073%), and 33/205 strict
complete-form coverage; 370 evidence records, 592 requirements, and
472 tests (472 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P3-S3n: MPYU unsigned-multiply vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MPYU Smem,dst` form from TI SPRU172C
§MPYU (printed p. 4-106, opcode `0010 010D IAAA AAAA`,
dual-sourced against the TI SPRU307A functional-overview opcode
summary `0010 010D IAAA AAAA`) is implemented as one catalog row:
`MPYU_SMEM` with value `0x2400`, mask `0xFE00` (bits 15:9
`0010010`, D bit 8, I bit 7, Smem field 6:0, 512 encodings). The
row reuses the S3k `DESTINATION_INDIRECT_ADDR7` layout (D select
rides field 0, the same position as the S3m S select),
resources `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and
repeatability `ELIGIBLE`. Word 0 is `0010_010D_IAAA_AAAA`: T is
preset and read (never loaded — the TI example leaves T unchanged),
and the completion runs `uns(T)` times `uns(Smem)` into the
D-selected accumulator through the multiply core with a new
unsigned-operands select, STORE select, and no rounding; C/TC are
preserved and `OVdst` follows the core. The unsigned select lives
in the shared core (not the leaf) so the FRCT shift and the OVM
saturation path stay single-sourced; the product is non-negative,
so saturation keeps the existing positive direction. The TI MPYU
Example 1 vector (`MPYU *AR0-, A` with `A` = `FF 8000 0000`,
`T` = `4000`, `FRCT` = 0, `AR0` = `1000`, `mem[1000]` = `FE00`
yielding `A` = `00 3F80 0000` with `T` = `4000` and `AR0` =
`0FFF`; `0x4000` times `0xFE00` equals `0x3F800000`, recomputed
independently before pinning) is the oracle. Table 2-28 lists no
MPYU instruction, so the row is repeatable by omission on the
recorded S3 omission basis. The four RED executable contracts are
`tests/unit/isa/catalog/test_mpyu_smem_metadata.c`
(CTest `ti_c54x.mpyu_smem_metadata`),
`tests/unit/isa/decode/test_mpyu_smem_decode.c`
(CTest `ti_c54x.mpyu_smem_decode`),
`tests/unit/isa/ops/multiply/test_mpyu_smem_ops.c`
(CTest `ti_c54x.mpyu_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_mpyu_smem.c`
(CTest `ti_c54x.cpu_mpyu_smem`), plus unsigned-mode lanes in
`tests/unit/isa/ops/multiply/test_multiply_core.c` and the
`SUBSET` `FORM-T2-03-08` inventory binding. Projected closure
is 202 catalog rows, `ID_COUNT=202`, 15,849 lookup entries, 201
bindings, family coverage 77/205 (37.561%), and 33/205 strict
complete-form coverage; 372 evidence records, 597 requirements, and
476 tests (476 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.

### P3-S3o: MACA Smem accumulate-A slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MACA[R] Smem[, B]` syntax-1 form from TI
SPRU172C §MACA[R] (printed p. 4-85, opcode `0011 01R1 IAAA AAAA`)
is implemented as one catalog row: `MACA_SMEM` with value `0x3500`,
mask `0xFD00` (bits 15:10 `001101`, bit 8 fixed 1, R bit 9, I bit 7,
Smem field 6:0, 512 encodings). The R bit carries the MACAR rounding
variant on the shared row by the recorded S3c/S3f R-sharing
convention, so `FORM-T2-04-09` stays `UNIMPLEMENTED`. The row uses a
new `ROUND_INDIRECT_ADDR7` layout (R/I/addr7, mirroring the S3k
destination/indirect/addr7 set), resources
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and repeatability
`ELIGIBLE` (Class 3A/3B single-repeatable classes with no Table 2-28
entry). Word 0 is `0011_01R1_IAAA_AAAA`: the Smem word loads T in
the read phase and the completion runs `(B)` plus `(Smem)` times
`A(32-16)` into fixed B through the existing multiply core with ADD
select and R rounding; C/TC are preserved and `OVdst` follows the
core for B. A-high is the multiplicand, so input A must be known
(and B known for the accumulate); T arrives via the read-phase load
and needs no prior knownness. The TI MACA Example 1 vector
(`MACA *AR5+` with `A` = `00 1234 0000`, `B` = `00 0000 0000`,
`FRCT` = 0, `AR5` = `0100`, `mem[0100]` = `5678` yielding `B` =
`00 0626 0060` with `T` = `5678` and `AR5` = `0101`;
`0x1234` times `0x5678` equals `0x6260060`, recomputed
independently before pinning) and the MACAR Example 3 rounding lane
(R = 1 yields `B` = `00 0626 0000`) are the oracles. The five RED
executable contracts are
`tests/unit/isa/decode/test_round_indirect_addr7_layout.c`
(CTest `ti_c54x.round_indirect_addr7_layout`),
`tests/unit/isa/catalog/test_maca_smem_metadata.c`
(CTest `ti_c54x.maca_smem_metadata`),
`tests/unit/isa/decode/test_maca_smem_decode.c`
(CTest `ti_c54x.maca_smem_decode`),
`tests/unit/isa/ops/multiply/test_maca_smem_ops.c`
(CTest `ti_c54x.maca_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_maca_smem.c`
(CTest `ti_c54x.cpu_maca_smem`), plus the `SUBSET`
`FORM-T2-04-07` inventory binding. The MACA T,src syntax-2 row,
MACD, and MACP remain later slices (program-memory operands and
the delay seam). Projected closure is 203 catalog rows,
`ID_COUNT=203`, 16,361 lookup entries, 202 bindings, family
coverage 78/205 (38.049%), and 33/205 strict complete-form
coverage; 374 evidence records, 603 requirements, and 481 tests
(481 `PASSING`, 0 `PLANNED`). These numbers are projections until
the slice lands.

### P3-S3p: MACA T,src register-only slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MACA[R] T, src [, dst]` syntax-2 form
from TI SPRU172C §MACA[R] (printed p. 4-85, opcode
`1111 01SD 1000 100R`) is implemented as one catalog row:
`MACA_T` with value `0xF488`, mask `0xFCFE` (bits 15:10
`111101`, S bit 9, D bit 8, R bit 0, 8 encodings). The R bit
carries the MACAR T,src rounding variant on the shared row by the
recorded S3c/S3f R-sharing convention, so `FORM-T2-04-10` stays
`UNIMPLEMENTED`. The row uses a new `SOURCE_DESTINATION_ROUND`
layout (S/D/R selects), resources
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and repeatability
`ELIGIBLE` (Class 3A/3B single-repeatable classes with no Table 2-28
entry). Word 0 is `1111_01SD_1000_100R`: register-only, no memory
effect and no T load; the completion runs `(src)` plus `(T)` times
`A(32-16)` into the D-selected accumulator through the existing
multiply core with ADD select and R rounding; T, A, and the
unselected accumulator are preserved, C/TC ride through unchanged,
and `OVdst` follows the core for dst (which is src when dst is not
specified). T, A, and src must be known. The TI MACA Example 2
vector (`MACA T, B, B` with `A` = `00 1234 0000`, `B` =
`00 0002 0000`, `T` = `0444`, `FRCT` = 1 yielding `B` =
`00 009D 4BA0` with `T` unchanged; `0x0444` times `0x1234`
equals `0x4DA5D0`, FRCT-doubled to `0x9B4BA0`, plus `0x00020000`
equals `0x009D4BA0`, recomputed independently before pinning) and
the MACAR Example 4 rounding lane (R = 1 yields `B` =
`00 009D 0000`) are the oracles. The existing TI MACA §MACA[R]
record spans both syntaxes, so no new TI record is needed by the
recorded S3f convention — only the activation, six owner-split
requirements, and five test records. The five RED executable
contracts are
`tests/unit/isa/decode/test_source_destination_round_layout.c`
(CTest `ti_c54x.source_destination_round_layout`),
`tests/unit/isa/catalog/test_maca_t_metadata.c`
(CTest `ti_c54x.maca_t_metadata`),
`tests/unit/isa/decode/test_maca_t_decode.c`
(CTest `ti_c54x.maca_t_decode`),
`tests/unit/isa/ops/multiply/test_maca_t_ops.c`
(CTest `ti_c54x.maca_t_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_maca_t.c`
(CTest `ti_c54x.cpu_maca_t`), plus the `SUBSET`
`FORM-T2-04-08` inventory binding. MACD and MACP remain later
slices (program-memory operands plus the delay seam). Projected
closure is 204 catalog rows, `ID_COUNT=204`, 16,369 lookup
entries, 203 bindings, family coverage 79/205 (38.537%), and
33/205 strict complete-form coverage; 375 evidence records, 609
requirements, and 486 tests (486 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P3-S3q: MACP Smem,Pmad program-coefficient slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The two-word `MACP Smem, pmad [, dst]` syntax-1 form
from TI SPRU172C §MACP (printed p. 4-89, word 0
`0111_100S_IAAA_AAAA` with word 1 `pmad`) is implemented as one
catalog row: `MACP_SMEM` with value `0x7800`, mask `0xFE00`
(bits 15:9 `0111100`, S bit 8, I bit 7, Smem field 6:0, 512
encodings) and `program_word_count` 2. The row uses a new
`DESTINATION_INDIRECT_ADDR7_PMAD16` layout (S/I/addr7 from word 0
plus verbatim `pmad16` from word 1, mirroring the S3k
destination/indirect/addr7 set with the S3 branch `pmad16` second
word), resources `ACCUMULATORS_STATUS_FRCT_OVM` (the MPYA_SMEM
precedent: T is written, not read, and there is no R rounding
bit), and repeatability `UNRESOLVED` (two-word forms stay
unresolved by the STM precedent). Word 0 is
`0111_100S_IAAA_AAAA`: the Smem word is read from DATA memory,
the `pmad` word names a PROGRAM-memory coefficient read, T takes
the Smem word in the read phase, and the pair completion runs
`(dst)` plus `(Smem)` times `(pmad)` into the S-selected
accumulator through the existing multiply core with ADD select
and no rounding; C/TC are preserved and `OVdst` follows the
core. The S polarity follows MAC_SMEM (`0` selects A,
`1` selects B). Smem covers direct page-0 and standard-indirect
MOD0--MOD3 with exotic-mode deferral; the PROGRAM leg is an
absolute 16-bit program-word address with no post-update. The new
DATA+PROGRAM pair seam exists for this slice: address kind
`DATA_PROGRAM_PAIR_WORD16` stages the Smem leg through the
generic single-leg resolver and the PROGRAM leg as an absolute
read, the `data_program_word16_read_transaction` reducer issues
the DATA read then the PROGRAM read through one reused child
transaction with the shared `complete_dual` shape (`x` is Smem,
`y` is the coefficient), and the `memory_instruction_data_program`
root (with its commit and candidate-validate owners) publishes
the single Smem post-access commit with a zeroed PROGRAM commit
plus the read-phase X-side T load. The MACD delay sidecar
(Smem+1 write after pair completion) stays a later slice, so
`FORM-T2-04-11` stays `UNIMPLEMENTED`. The oracle vector is
recomputed independently before pinning: `MACP *AR5+, #0100h`
with `B` = `00 0000 0000`, `FRCT` = 0, `AR5` = `0100`,
`mem[0100]` = `4000`, `pmem[0100]` = `4000` yields `B` =
`00 1000 0000` with `T` = `4000` and `AR5` = `0101`
(`0x4000` times `0x4000` equals `0x10000000`); the FRCT lane
yields `B` = `00 2000 0000`. The seven RED executable contracts
are the five family proofs
`tests/unit/isa/decode/test_destination_indirect_addr7_pmad16_layout.c`
(CTest `ti_c54x.destination_indirect_addr7_pmad16_layout`),
`tests/unit/isa/catalog/test_macp_smem_metadata.c`
(CTest `ti_c54x.macp_smem_metadata`),
`tests/unit/isa/decode/test_macp_smem_decode.c`
(CTest `ti_c54x.macp_smem_decode`),
`tests/unit/isa/ops/multiply/test_macp_smem_ops.c`
(CTest `ti_c54x.macp_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_macp_smem.c`
(CTest `ti_c54x.cpu_macp_smem`), plus the two seam proofs
`tests/unit/sequencer/instruction/test_data_program_word16_read_transaction.c`
(CTest `ti_c54x.data_program_word16_read_transaction`) and
`tests/unit/sequencer/instruction/test_data_program_memory_instruction_transaction.c`
(CTest `ti_c54x.data_program_memory_instruction_transaction`),
plus the `SUBSET` `FORM-T2-04-12` inventory binding. Projected
closure is 205 catalog rows, `ID_COUNT=205`, 16,881 lookup
entries, 204 bindings, family coverage 80/205 (39.024%), and
33/205 strict complete-form coverage; 377 evidence records, 617
requirements, and 493 tests (493 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P3-S3r: MACD Smem,Pmad delay-write slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The two-word `MACD Smem, pmad [, dst]` syntax-1 form
from TI SPRU172C §MACD (printed p. 4-87, word 0
`0111_101S_IAAA_AAAA` with word 1 `pmad`) is implemented as one
catalog row: `MACD_SMEM` with value `0x7A00`, mask `0xFE00`
(bits 15:9 `0111101`, S bit 8, I bit 7, Smem field 6:0, 512
encodings) and `program_word_count` 2. The row reuses the S3q
`DESTINATION_INDIRECT_ADDR7_PMAD16` layout (no new layout),
resources `ACCUMULATORS_STATUS_FRCT_OVM`, and repeatability
`UNRESOLVED` (two-word STM precedent). Word 0 is
`0111_101S_IAAA_AAAA`: the Smem word is read from DATA memory,
the `pmad` word names a PROGRAM-memory coefficient read, T takes
the Smem word in the read phase, the pair completion runs
`(dst)` plus `(Smem)` times `(pmad)` into the S-selected
accumulator with ADD select and no rounding, and then the delay
sidecar writes the Smem word to DATA `Smem+1` (16-bit wraparound)
before the Smem post-access commit and PC publish. The sidecar
rides a new `write_smem_delay` flag on the MEMORY effect carrier
(a second boolean in the existing padding; the 48-byte ABI is
unchanged and only the DATA+PROGRAM pair kind may set it): the
pair root issues the delay WRITE through the endpoint with a
third transaction ID after both reads complete, reusing `PENDING`
with the pair reducer reset and the reused child driven directly,
so no new phase and no CPU resume-path change; WAIT resumes the
delay child, FAULT publishes nothing, and cancel/reset route by
pair-versus-delay child state. Pair traffic (reads and delay
write) always uses the endpoint by the dual precedent; there is
no MMR branch. The oracle vector is recomputed independently
before pinning: `MACD *AR5+, #0100h` with `B` = `00 0000 0000`,
`FRCT` = 0, `AR5` = `0100`, `mem[0100]` = `4000`,
`pmem[0100]` = `4000`, `mem[0101]` = `0000` yields `B` =
`00 1000 0000` with `T` = `4000`, `AR5` = `0101`, and
`mem[0101]` = `4000` (`0x4000` times `0x4000` equals
`0x10000000`); the FRCT lane yields `B` = `00 2000 0000`.
The five RED executable contracts are the four family proofs
`tests/unit/isa/catalog/test_macd_smem_metadata.c`
(CTest `ti_c54x.macd_smem_metadata`),
`tests/unit/isa/decode/test_macd_smem_decode.c`
(CTest `ti_c54x.macd_smem_decode`),
`tests/unit/isa/ops/multiply/test_macd_smem_ops.c`
(CTest `ti_c54x.macd_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_macd_smem.c`
(CTest `ti_c54x.cpu_macd_smem`), plus the seam proof
`tests/unit/sequencer/instruction/test_data_program_delay_write.c`
(CTest `ti_c54x.data_program_delay_write`), plus the
`SUBSET` `FORM-T2-04-11` inventory binding. Projected
closure is 206 catalog rows, `ID_COUNT=206`, 17,393 lookup
entries, 205 bindings, family coverage 81/205 (39.512%), and
33/205 strict complete-form coverage; 379 evidence records, 623
requirements, and 498 tests (498 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P3-S3s: MACSU Xmem,Ymem dual slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MACSU Xmem, Ymem, src` syntax-2 form
from TI SPRU172C §MACSU (printed p. 4-91, word 0
`1010_011S_XXXX_YYYY`) is implemented as nine catalog rows (X field 7:4, Y field 3:0):
`MACSU_DUAL_Mx_My` with values `0xA800|xmode<<6|ymode<<2`
(`0xA800`, `0xA804`, `0xA808`, `0xA840`, `0xA844`, `0xA848`,
`0xA880`, `0xA884`, `0xA888`) under mask `0xFECC` (bits 15:9
`1010011`, S bit 8, X/Y MOD bits 7:6/3:2 fixed per row, Xar/Yar
bits 5:4/1:0 free; 32 S/Xar/Yar encodings per row, 288 total).
The rows reuse the `DUAL_DESTINATION_XAR_YAR` layout (D[8]
positionally is the S select; no new layout, no trusted-capacity
pressure: 3 operands of widths 1/2/2) with `UNRESOLVED`
repeatability and
`ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM` resources.
Checked and trusted leaves stage dual-indirect MOD0-MOD2 reads
with read-phase X-side T load (`T` takes the `Xmem` word) and,
on pair completion, one accumulator write of `(src)` plus
`unsigned(Xmem)` times `signed(Ymem)` into the S-selected
accumulator (S selects the shared src/dst accumulator) through
the multiply core with ADD select, FRCT/OVM handling, and no
rounding. The mixed-sign product needs a core seam extension:
one additive `unsigned_t_only` select on the multiply operands
(`T` unsigned, multiplicand signed; the existing
`unsigned_operands` both-unsigned select keeps precedence for
`MPYU`, all-signed leaves set the new select false explicitly)
with the FRCT shift, accumulate, saturation, and overflow
disposition unchanged. The oracle vector is recomputed
independently before pinning: `MACSU *AR4+, *AR5+, A` with `A` =
`00 0000 0000`, `FRCT` = 0, `AR4` = `0100`, `AR5` = `0200`,
`mem[0100]` = `4000`, `mem[0200]` = `4000` yields `A` =
`00 1000 0000` with `T` = `4000`, `AR4` = `0101`, `AR5` =
`0201` (`16384` times `16384` equals `0x10000000`); the FRCT
lane yields `A` = `00 2000 0000`; the mixed-sign lane
(`X` = `8765`, `Y` = `C000`) pins the unsigned-X/signed-Y
product (`34661` times `-16384`) and fails under either
uniform-signedness reading. The four RED executable contracts
are the extended core proof
`tests/unit/isa/ops/multiply/test_multiply_core.c` (CTest
`ti_c54x.multiply_core`, mixed-sign product lanes), the family
proofs `tests/unit/isa/ops/multiply/test_macsu_dual_ops.c`
(CTest `ti_c54x.macsu_dual_ops`) and
`tests/unit/cpu/isa/multiply/test_cpu_macsu_dual.c` (CTest
`ti_c54x.cpu_macsu_dual`), plus the `SUBSET` `FORM-T2-04-13`
inventory binding. Projected closure is 215 catalog rows,
`ID_COUNT=215`, 17,681 lookup entries, 214 bindings, family
coverage 82/205 (40.000%), and 33/205 strict complete-form
coverage; 381 evidence records, 627 requirements, and 500
tests (500 `PASSING`, 0 `PLANNED`). These numbers are
projections until the slice lands.

### P3-S3t: MACR Xmem,Ymem dual lock-in slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MACR Xmem, Ymem, src [, dst]` (TI SPRU172C §MAC[R]
syntax 2, opcode `1011_0RSD_XXXX_YYYY`, pp. 4-82–4-84) shares the nine
`MAC_DUAL_Mx_My` rows with R=1: no new catalog row, layout, production
change, inventory binding, or neighbor pin moves. The slice locks the
R=1 rounding path end to end on the maintained suites — the
`0xB532` leaf completion (`(0x5678<<1)` x `0x1234` + 4 rounded to
`0x0C4C0000` into B) and the `0xB5BA` CPU retirement (same product
with MOD2 post-increments) already pin the checked path — with one
new executable proof: trusted R=1 equivalence through
`ti_c54x_trusted_op_mac_dual_m0_m0` on the `0xB532` word, pinning that
the trusted extractor stages the rounding select and completes the
rounded MACR value. `FORM-T2-04-05`/`FORM-T2-04-06` stay
`UNIMPLEMENTED` with empty symbol lists by the recorded S3f R-sharing
convention (shared rows cannot double-bind). Projected closure adds
one activation record, two requirements, and two test records on the
shared suites: 382 evidence records, 629 requirements, and 502 tests
(502 `PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.

### P3-S3u: MASR Smem and dual lock-in slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MASR Smem` (TI SPRU172C §MAS[R] Example 3, opcode
`0010_11RS_IAAAAAAA`) shares the `MAS_SMEM` row with
R=1, and `MASR Xmem, Ymem, src [, dst]` shares the nine
`MAS_DUAL_Mx_My` rows with R=1: no new catalog row, layout, production
change, inventory binding, or neighbor pin moves. The slice locks the
R=1 rounding path end to end on the maintained suites — the `0x2E95`
leaf completion (round(A - T*Smem) to `0xFFFFB70000` into A) and the
`0x2E34` CPU retirement already pin the checked Smem path, and the
`0xBD32` leaf completion (rounded `0xFFF3B40000` into B) and the
`0xBDBA` CPU retirement already pin the checked dual path — with two
new executable proofs: trusted R=1 completion through
`ti_c54x_trusted_op_mas_smem` on the `0x2E95` word, and trusted R=1
rounding equivalence through `ti_c54x_trusted_op_mas_dual_m0_m0` on
the `0xBD32` word, pinning that both trusted extractors stage the
rounding select and complete the rounded MASR values.
`FORM-T2-04-15`/`FORM-T2-04-17` stay `UNIMPLEMENTED` with empty symbol
lists by the recorded R-sharing convention (shared rows cannot
double-bind). Projected closure adds one activation record, two
requirements, and two test records on the shared suites: 383 evidence
records, 631 requirements, and 504 tests (504 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.

### P3-S3v: MACAR Smem lock-in slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MACAR Smem` (TI SPRU172C §MACA[R] Example 3, opcode
`0011_01RS_IAAAAAAA`) shares the `MACA_SMEM` row with R=1: no new
catalog row, layout, production change, inventory binding, or neighbor
pin moves. The slice locks the R=1 rounding path end to end on the
maintained suites — the `0x3734` leaf completion (the Example 1
product rounded to `0x6260000` into B) already pins the checked path
— with two new executable proofs: CPU retirement of the `0x3734`
direct word (same pre-state as the Example 1 direct lane, B rounded
to `0x6260000` with the read-phase T load), and trusted R=1
completion through `ti_c54x_trusted_op_maca_smem` on the `0x3734`
word, pinning that the trusted extractor stages the rounding select
and completes the rounded MACAR value. `FORM-T2-04-09`/`FORM-T2-04-10`
stay `UNIMPLEMENTED` with empty symbol lists by the recorded
R-sharing convention (shared rows cannot double-bind). Projected
closure adds one activation record, two requirements, and two test
records on the shared suites: 384 evidence records, 633
requirements, and 506 tests (506 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P3-S3w: MASA Smem and T slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MASA Smem` (TI SPRU179C §MASA syntax 1, opcode
`0011_0011_IAAAAAAA`, pp. 4-99–4-100) lands one `MASA_SMEM` row at
`0x3300` under mask `0xFF00` (256 encodings, one program word)
through the shared `INDIRECT_ADDR7` layout: B minus Smem times A-high
into fixed B with read-phase Smem T load. `MASA T` (syntax 2,
opcode `1111_01SD_1000_101R`) lands one `MASA_T` row at `0xF48A`
under mask `0xFCFE` (S/D/R ride the word, 8 encodings, one program
word, no memory traffic) through the shared
`SOURCE_DESTINATION_ROUND` layout: dst minus T times A-high into
dst, with R=1 carrying the future MASAR T variant by the recorded
R-sharing convention. Both handlers reuse the multiply core with the
SUBTRACT select and the A-high multiplicand, FRCT/OVM handling, and
C/TC preservation, mirroring the `MACA_SMEM`/`MACA_T` leaves. Both
rows land `UNRESOLVED` repeatability: no Table 2-28 MASA fact is on
record, and absence from the evidenced negative lists is not
`ELIGIBLE` authorization (P2-W48C precedent). The RED executable
contracts pin the TI vectors end to end — the Example 1 Smem lane
(`0x56 78` read, B `00 0002 0000` to `FF F9DB FFA0` with T load),
the Example 2 T lane (T `0x0444`, FRCT=1, B to `FF FF66 B460`),
and the Example 3 rounding lane (R=1, B to `FF FF67 0000`) — each at
the leaf, through the trusted entry point, and retired on the CPU.
`FORM-T2-04-18`/`FORM-T2-04-19` land `SUBSET` with the `MASA_SMEM`/
`MASA_T` symbols; `FORM-T2-04-20`/`FORM-T2-04-21` (MASAR) stay
`UNIMPLEMENTED` with empty symbol lists. Projected closure adds two
catalog rows, two inventory bindings, one TI source record, one
activation record, four requirements, and four test records: 386
evidence records, 637 requirements, and 510 tests (510 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.

### P3-S3x: MASAR T lock-in slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MASAR T,src` (TI SPRU179C §MASA syntax 2 with
`rnd()`, opcode `1111_01SD_1000_101R` with R=1) shares the `MASA_T`
row: no new catalog row, layout, production change, inventory
binding, or neighbor pin moves. The slice locks the R=1 rounding
path end to end on the maintained suites — the `0xF78B` leaf
completion (Example 2 product rounded to `0xFFFF670000` into B) and
the `0xF78B` CPU retirement already pin the checked path — with one
new executable proof: trusted R=1 completion through
`ti_c54x_trusted_op_masa_t` on the `0xF78B` word, pinning that the
trusted extractor stages the rounding select and completes the
rounded MASAR value. `FORM-T2-04-20`/`FORM-T2-04-21` stay
`UNIMPLEMENTED` with empty symbol lists by the recorded R-sharing
convention (shared rows cannot double-bind). Projected closure adds
one activation record, one requirement, and one test record on the
shared suite: 387 evidence records, 638 requirements, and 511 tests
(511 `PASSING`, 0 `PLANNED`). These numbers are projections until
the slice lands.

### P3-S3y: MPYR Smem lock-in slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MPYR Smem` (TI SPRU179C §MPY syntax 1 with `rnd()`,
opcode `0010_00RD_IAAAAAAA` with R=1) shares the `MPY_SMEM` row: no
new catalog row, layout, production change, inventory binding, or
neighbor pin moves. The slice locks the R=1 rounding path end to end
on the maintained suites — the `0x2200` leaf completion (TI MPY
Example 4: T `0x1234` times Smem `0x5678` rounded to `0x6260000`
into A) and the `0x2320` CPU retirement already pin the checked
path — with one new executable proof: trusted R=1 completion
through `ti_c54x_trusted_op_mpy_smem` on the `0x2200` word, pinning
that the trusted extractor stages the rounding select and completes
the rounded MPYR value. `FORM-T2-03-02` stays `UNIMPLEMENTED` with
an empty symbol list by the recorded R-sharing convention (shared
rows cannot double-bind). Projected closure adds one activation
record, one requirement, and one test record on the shared suite:
388 evidence records, 639 requirements, and 512 tests (512
`PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.

### P3-S3z: ADD Smem slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `ADD Smem` syntaxes 1–3 (TI SPRU179C §ADD,
opcodes `0000_000S_IAAAAAAA`,
`0000_010S_IAAAAAAA`, and
`0011_11SD_IAAAAAAA`, pp. 4-4–4-5) land
three rows: `ADD_SMEM` at `0x0000` under
mask `0xFE00` (`src += Smem`, S rides
bit 8), `ADD_SMEM_TS` at `0x0400` under
mask `0xFE00` (`src += Smem<<TS` with the
T-register shift), and `ADD_SMEM_SH16`
at `0x3C00` under mask `0xFC00` through
the shared arithmetic core with
SXM extension, OVM saturation with
sticky OVdst, and per-syntax C rules
(syntaxes 1–2 write C from the bit-32
carry like the established ADD leaves;
syntax 3 sets C on carry and otherwise
preserves it per the explicit TI rule).
Syntaxes 1–2 share a new
`SELECT_INDIRECT_ADDR7` layout (S@8,
I@7, addr@6:0); syntax 3 takes a new
`SOURCE_DESTINATION_INDIRECT_ADDR7`
layout (S@9, D@8, I@7, addr@6:0). All
three rows land `UNRESOLVED`
repeatability: Class 3A/3B membership is
not Table 2-28 authorization (P2-W48C
precedent). The RED executable contracts
pin derived vectors end to end (no TI
worked example exists for syntaxes 1–3;
each expectation is independently
recomputed): unshifted and TS-shifted
adds with SXM 0/1 extension and carry
cases at the leaf, through the trusted
entry points, and retired on the CPU
(direct plus MOD2 indirect, read-phase
behavior, T preservation where the
syntax takes no T shift). `FORM-T2-01-01`
/`02`/`03` land `SUBSET` with the three
symbols. Projected closure adds
three catalog rows, three inventory
bindings, one TI source record, one
activation record, two requirements, and
two test records: 390 evidence records,
641 requirements, and 514 tests (514
`PASSING`, 0 `PLANNED`). These numbers
are projections until the slice lands.

### P3-S4: XC conditional-execution slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `XC n,cond` (TI SPRU172C §XC, opcode
`1111_11N1_CCCCCCCC` with N at bit 9 selecting n = 1 (`0`) or n = 2
(`1`), pp. 4-198–4-200) lands one row: `XC` at `0xFD00` under mask
`0xFD00` (512 N/condition encodings under one masked row) through a
new `N_CONDITION8` layout (N@9, cond@7:0). Execution is taken/not-taken
over the 8-bit condition field: if the condition holds the next n
instruction words execute normally, otherwise the CPU retires n NOP
words (PC advances past them with no fetch, no effect, no bus
traffic). Single-condition evaluation reuses the shared
branch-condition core for its sixteen codes and adds `UNC`
(unconditional take); `BIO`/`NBIO`, `AOV`/`ANOV`/`BOV`/`BNOV`, and
all multi-condition combination fields have no input carrier yet
(no BIO pin, no OVA/OVB latch in the op snapshot) and defer as
`INVALID_OPERAND` at the op leaf, mirroring the deferred-code
precedent. The row lands `INELIGIBLE` repeatability on the explicit
TI `not repeatable` statement (BC precedent), so the repeat-body
gate blocks it with no new wiring. Status bits are none; inputs are
`ACCUMULATORS_STATUS_C_TC`. The TI three-word uninterruptible window
is recorded but not modeled: no IRQ service path exists to arbitrate,
and the single-threaded run loop cannot interleave inside the
retirement. Cycle counts and the sampled-two-cycles-early note are
not adopted. The RED executable contracts pin the TI Example vector
(`XC 1,ALEQ` with A −1 executing the follower) at the leaf, through
the trusted entry, and on the CPU (taken and not-taken for n = 1 and
2, T preservation, deferred-field transactional reject), plus the
layout suite. `FORM-T2-18-07` lands `SUBSET` with the `XC` symbol.
Projected closure adds one catalog row, one inventory binding, one TI
source record, one activation record, four requirements, and three
test records: 392 evidence records, 648 requirements, and 519 tests
(519 `PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.

### P3-S5: accumulator-addressed push seam (planned)

This immutable `(planned)` heading is the durable S5 FIT/ownership
anchor. S5 is a sealed architectural seam with no catalog rows, no
opcode identities, no handlers, and no CPU opcode branch, following
the W43A pattern. TI SPRU172C §4 `CALA[D]` (printed pp. 4-24–4-25)
is a one-word call to the 16-bit address in the low part of the
selected accumulator: nondelayed pushes `(PC) + 1` to TOS,
delayed pushes `(PC) + 3` to TOS, both over an SP − 1 TOS with a
PRE_ACCESS SP update, then `(src(15–0))` replaces PC; no
status-bit effects, and the description carries an explicit
not-repeatable note. The seam adds two MEMORY address kinds,
`STACK_PUSH_ACCUMULATOR_WORD16` and
`STACK_PUSH_ACCUMULATOR_DELAYED_WORD16`, with TOS planning
identical to the immediate pushes; the A/B accumulator select
rides `arf` (0 for A, 1 for B) while every other first-arm field
stays zero. The PC target is the low 16 bits of the selected
accumulator read from the frozen input snapshot gated on
accumulator knownness at prepare (unknown rejects before any bus
issue); commit re-derives the same target deterministically from
the frozen input, so no frozen field grows and root sizes stay
frozen. Return addresses reuse the existing bridge successor
(PC+1 for the one-word form, PC+3 delayed). No timing, IRQ/RPT,
wrap, machine-cycle, public-ABI, or new public CPU snapshot claim
is admitted. No knob is added.

Ownership: `REQ-PROJECT-P3-S5-PUSH-EFFECT`
(`src/isa/ops/memory/`),
`REQ-PROJECT-P3-S5-PUSH-RESOLVER` (`src/sequencer/memory/`),
and `REQ-PROJECT-P3-S5-PUSH-TRANSACTION`
(`src/sequencer/instruction/`) carry the project contract over TI
`CALA[D]` push evidence. The three RED lanes are
`tests/unit/isa/ops/memory/test_accumulator_push_memory_effect.c`
(CTest `ti_c54x.accumulator_push_memory_effect`),
`tests/unit/sequencer/memory/test_accumulator_push_resolution.c`
(CTest `ti_c54x.accumulator_push_resolution`), and
`tests/unit/sequencer/instruction/test_accumulator_push_root.c`
(CTest `ti_c54x.accumulator_push_root`). Exit: sealed seam with
all three lanes green and the CALA/CALAD family unblocked.
Projected closure adds one TI source record, one plan-anchor
record, three requirements, and three test records: 394 evidence
records, 651 requirements, and 522 tests (522 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.

### P3-S8: BACC accumulator-addressed branch family slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `BACC[D]` (TI SPRU172C §BACC[D], printed p. 4-15)
lands two rows on the S5 accumulator-addressed push seam:
`BACC` at `0xF4E2` and `BACCD` at `0xF6E2`, both under the shared
`0xFEFF` mask (one S field at bit 8, one nondelayed and one delayed
row) through the shared `ACCUMULATOR_SOURCE` layout (S = 0 selects
accumulator A, S = 1 selects B; the layout is shared with the future
`CALA`/`CALAD` rows). Execution is unconditional over the selected
accumulator: PC is replaced by the 16 low bits of
`Acc(DMD)` with no status, accumulator, T-register, or MEMORY
effect. Both rows land `INELIGIBLE` repeatability on the explicit TI
`not repeatable` statement, so the repeat-body gate blocks them with
no new wiring. CPU retirement retires `BACC` followers normally and
`BACCD` followers through the two-word delay window with PC advance,
T preservation, and no endpoint traffic. The RED executable contracts
pin A/B selection on both rows at the leaf, through the trusted
entry, and on the CPU (delay-window consumption, unknown-accumulator
transactional reject), plus the shared layout suite.
`FORM-T2-12-02` lands `SUBSET` with the `BACC` symbol under
`ACCUMULATOR_NONDELAYED` and the `BACCD` symbol under
`ACCUMULATOR_DELAYED`. No knob is added.
Projected closure adds five TI source records, one activation record,
four requirements, and five test records: 400 evidence records, 655
requirements, and 527 tests (527 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P3-S9: CALA accumulator-addressed call family slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `CALA[D]` (TI SPRU172C §CALA[D], printed pp. 4-24–
4-25, opcode `1111_01ZS_1110_0011` with Z at bit 9 selecting
nondelayed `CALA` (`0`) or delayed `CALAD` (`1`) and S at bit 8
selecting accumulator A (`0`) or B (`1`)) lands two rows on the S5
accumulator-addressed push seam: `CALA` at `0xF4E3` and `CALAD` at
`0xF6E3`, both under the shared `0xFEFF` mask through the shared
`ACCUMULATOR_SOURCE` layout (no new layout requirement: the S8
layout requirement already names the future `CALA`/`CALAD` rows).
Checked and trusted leaves stage a MEMORY
`STACK_PUSH_ACCUMULATOR(_DELAYED)` TOS WRITE with the select in
`arf` and a zero return-address placeholder, gated on SP knownness
(`INVALID_OPERAND`) and selected-accumulator knownness
(`UNKNOWN_INPUT`); the S5 seam root derives the PC target from the
frozen input snapshot and stages PC+1 (`CALA`) or PC+3 (`CALAD`)
return addresses. Both rows land `INELIGIBLE` repeatability on the
explicit TI `not repeatable` statement, so the repeat-body gate
blocks them with no new wiring. A new input-resource token,
`ACCUMULATORS_STACK_POINTER`, carries exactly the two consumed
inputs (no status claim); the collector and `KNOWN_MASK` already
cover both bits, so no sequencing change is admitted. CPU
retirement reuses the generic MEMORY route with no `run.c` change:
`CALA` retires with the TOS WRITE carrying PC+1 and PC replaced by
the accumulator target, `CALAD` through the two-word delay window
with PC+3 before the frozen deferred target takes effect. The RED
executable contracts pin the TI worked examples (`CALA A` with A
00_0000_3000 at PC 0025/SP 1111 to TOS[1110] 0026; `CALAD B` with B
00_0000_2000 to TOS[1110] 0028) at the leaf, through the trusted
entry, and on the CPU, plus delay-window, MMR-route, WAIT, fault,
and unknown-input transactionality. `FORM-T2-13-01` lands `SUBSET`
with the `CALA` symbol under `ACCUMULATOR_NONDELAYED` and the
`CALAD` symbol under `ACCUMULATOR_DELAYED`. No knob is added.
Projected closure adds five TI source records, one activation record,
three requirements, and four test records: 406 evidence records, 658
requirements, and 531 tests (531 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

### P3-S10: STH syntax-1 Smem store slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `STH` syntax 1 (`STH src, Smem`, TI SPRU172C §STH,
printed pp. 4-168–4-171, opcode `1000_001S_IAAAAAAA` with S at bit 8
selecting accumulator A (`0`) or B (`1`)) lands seven rows mirroring
the `STL_SMEM` family with bit 9 set: `STH_SMEM` at `0x8280` under
mask `0xFEE0` (MOD0–MOD3) and `STH_SMEM_MOD4`/`MOD5`/`MOD6`/`MOD7`/
`MOD8`/`MOD10` at `0x82A0`/`0x82A8`/`0x82B0`/`0x82B8`/`0x82C0`/
`0x82D0` under mask `0xFEF8`, through the shared
`ACCUMULATOR_ARF_MOD`/`ACCUMULATOR_ARF_MOD4` layouts. Checked and
trusted leaves stage single-indirect, indexed, circular, and
reverse-carry MEMORY WRITE effects carrying bits 31–16 of the
selected accumulator — the unshifted high part — with no status,
accumulator, or T-register effect. All seven rows land
`UNRESOLVED` repeatability on the absent TI repeatability note
(`STL_SMEM` precedent). CPU retirement reuses the generic MEMORY
route with no `run.c` change. The RED executable contracts pin the
TI worked example (`STH A,10` with A FF_8765_4321 at DP 004
storing 8765) at the leaf and high-word publication with
post-increment, WAIT resume, fault, and boundary transactionality
on the CPU. `FORM-T2-20-05` lands `SUBSET` with all seven symbols
under `STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`
(the `STL_SMEM` token, reused: identical mode coverage). Direct-DMA7
and shifted syntaxes 2–4 stay for later slices. No knob is added.
Projected closure adds two TI source records, one activation record,
three requirements, and two test records: 409 evidence records, 661
requirements, and 533 tests (533 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.

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
