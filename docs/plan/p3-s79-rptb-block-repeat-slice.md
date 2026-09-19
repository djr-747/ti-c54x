### P3-S79: RPTB block-repeat slice (planned)

`RPTB pmad` is TI SPRU172C section RPTB[D] (vendored
`vendor/ti/spru172c.txt` pp. 4-147–4-148): syntax
`RPTB[D] pmad` with `0 v pmad v 65535`, opcode
`1111 000Z 0111 0010` plus the 16-bit pmad word;
Z=0 is the plain form (`0xF072`, RSA=PC+2),
Z=1 (`0xF272`) is the delayed form (own slice,
same `FORM-T2-16-04` `_DELAYED` composition as the
denominator audit). Execution sets BRAF, loads
RSA with PC+2 and REA with pmad; the block
RSA..REA then executes BRC+1 times (BRC preloaded,
`ST #99,BRC` runs a block 100 times). Status bits:
affects BRAF (ST1 bit 15). Nonrepeatable, 2 words,
interruptible; single-repeat loops may nest inside;
far branches/calls are forbidden in the block;
nested block repeat needs software save/restore.
The staged loader1 run stops `UNIMPLEMENTED_OPCODE`
on it at `0x802F` (words `F072 803A`, block
`0x8031..0x803A`, straight-line bodies, no nested
repeat or branch): no `0xF07x` row covers Z=0
(`0xF070` is `RPT_LK`, `0xF073`/`0xF074` are
`B`/`CALL`). The boot's BRC comes from the
earlier `STM` chain through the MMR file.

Leaf design: new `EXACT` row `RPTB` (`0xF072`, 2
words) reusing the `BRANCH_PMAD16` layout set
(single pmad16 operand, `B`/`CALL` precedent),
`INELIGIBLE` ("not repeatable"), no input
resources (BRC reads at activation from MMR state
with knownness gating). The leaf stages a new
`REPEAT_BLOCK_START` effect (kind 14, pmad
payload); the CPU route (in `repeat_activate`,
no new run.c route site) rejects on bridge
failure, delay-slot arming, and unknown BRC, then
writes RSA/REA through the MMR file, sets BRAF,
and starts the new `repeat_block` driver. Loop
control rides the existing retirement tails with
zero new tail sites: `run.c` records the fetch
address plus decoded word count (2 fields, 2
lines), and `ti_c54x_cpu_repeat_body_retired`
reuses them after the single handling — REA
fall-through with BRC>0 decrements BRC and loops
to RSA, BRC==0 ends the block (BRAF clear,
sequential advance). Branches out, delay-slot
countdowns (armed guard), and interrupt takes
(take preserves CPU+MMR state; block is
interruptible, no deferral) compose by
construction. Nested block start overwrites
(TI-faithful reload). New catalog ID moves
`ID_COUNT` 334 to 335 with pins;
`FORM-T2-16-04` binds the row `SUBSET` (scope
flip `UNRESOLVED`→`COMMON_CANDIDATE`, coverage
`UNIMPLEMENTED`→`SUBSET`); no MMR, knob, or
timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_rptb_decode`, `test_rptb`,
`test_rptb_metadata`, `test_cpu_rptb`) plus the
driver flow test (`test_repeat_block_flow`) and
the catalog-contract row pin. Acceptance is the
hosted repro advancing past `0x802F` to the next
explicit stop (`0xFF0C` inside the block is the
predicted follower).

Out of scope: RPTBD `0xF272` (own slice with
delay-slot activation), far-branch/call block
validation, BRC/RSA/REA migration out of the MMR
file, timing.
