### P3-S97: RPT Smem row slice (implemented; local verification green)

One of the 36 inventory-UNIMPLEMENTED forms, on the
existing single-repeat machinery with no new seam:
`RPT Smem` loads RC from a data-memory word. `RPTZ
dst,#lk` splits to P3-S98: zeroing an accumulator in
the same retirement as the repeat start is the first
dual-commit instruction and needs an effect-struct
extension, not a leaf alone. The WAVE7 fan-out plan's
COVERED class is stale for the single-memory rounded
cousins (already `SUBSET` through the multiply
leaves); the remaining `MACR`/`MASR` Xmem,Ymem forms
are parallel dual-memory and stay in the
deferred-seam class.

Evidence basis (one new E-TI row). TI SPRU172C §RPT
(p. 4-146): syntax 1 `RPT Smem` is `0100 0111 I
AAAAAAA` (one word plus long/absolute extension
words, RC ← Smem, 3 cycles) over Class 5A/5B;
syntaxes 2–3 (`0xEC00|K`, `0xF070` + lk) are the
landed rows.

Scope: one catalog row, `RPT_SMEM` (`MASKED 0x4700/
0xFF00`, 1 word, `INDIRECT_ADDR7` reusing the
`MPYA_SMEM` shape, `INELIGIBLE`, no input resources
— address planning owns DP/AR downstream); one
checked-plus-trusted leaf in `src/isa/ops/repeat/`
(`op_rpt_smem` staging Smem address planning with a
read-value `REPEAT_START` completion through the
`MEMORY` chain); dispatch wiring; one small shared
seam — the memory commit reports a completion-staged
`REPEAT_START` (pending RC operand on the
transaction, published by the READY tail into the
result like the deferred-branch report) and both
run-loop MEMORY retire sites route it through the
existing single-repeat start wiring (the ordinary
follow-on stage keeps rejecting `REPEAT_START`);
inventory flip of `FORM-T2-16-01` to `SUBSET` with
the new symbol; generated lookup regeneration.

RED contracts:
`tests/unit/isa/ops/repeat/test_rpt_smem.c`
(direct/indirect cookie extraction over the
`INDIRECT_ADDR7` layout; staged DATA READ with the
direct/indirect address spec; read-value completion
staging `REPEAT_START` with RC = Smem; exotic
indirect modes defer as `INVALID_OPERAND`;
checked/trusted/dispatch parity; null rejects);
decode binding extension for the row;
`tests/unit/cpu/isa/repeat/` extension (RPT Smem
end-to-end with scripted Smem repeating an eligible
body Smem+1 times; nested-RPT and repeatable-body
blocks preserved). Acceptance is the focused tests
plus the full gate, with the park run byte-identical
absent the new opcode.

Out of scope: `RPTZ` (P3-S98 dual-commit
extension), `MACR`/`MASR` Xmem,Ymem (parallel
dual-memory seam), `RPTB[D]` extensions, block
repeat, repeatable-RPT, timing.

Findings (GREEN): the row, leaf, and dispatch work
first try, but retiring through the MEMORY root
exposed a missing seam — the read-value completion
stages `REPEAT_START`, which the ordinary follow-on
stage rejects, so the commit now records the pending
RC operand on the transaction (cleared at prepare)
and the READY tail publishes it into the result
exactly like the deferred-branch report; both
run-loop MEMORY retire sites (first-issue and
resume-pending — the first draft wired only the
resumed one) route it through the existing
single-repeat start wiring via a `run_support`
helper (run.c was breaching the 800-line split
gate). The transaction root grows 456→464
(420→428 ILP32, both measured) following the pinned
growth protocol. Two genuine first-word collisions
surfaced: 0x4690/0x4680 ^ 0x0100 and the 0x47FF
DAT127 shape now decode `RPT_SMEM` (MASA_SMEM
precedent arms added). No new knobs; no E-PROJECT
row (the seam is covered by the COMMON retirement
requirement, not a pure contract test).

Validation evidence: `make test` exit 0 (new
RED-first op/CPU tests green; evidence registry
PASS at 494 evidence / 809 requirements / 663
tests; inventory PASS at 170/205 bound with 349
bindings and 40534 lookup slots; source
boundaries, knobs, manifests, observer-off
artifact); fresh CMake Debug `ctest` 765/765
including `ti_c54x.rpt_smem_ops` and
`ti_c54x.cpu_rpt_smem`; hosted runner `--selftest`
PASS. The S90 canonical park is not re-runnable
here (probe `.fls` absent) but parity holds: the
new opcode is unreachable absent its first word and
the report fields default off.
