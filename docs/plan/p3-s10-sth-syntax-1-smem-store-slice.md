### P3-S10: STH syntax-1 Smem store slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
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
