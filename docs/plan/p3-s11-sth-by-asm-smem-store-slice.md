### P3-S11: STH by-ASM Smem store slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `STH` syntax 2 (`STH src, ASM, Smem`, TI SPRU172C §STH,
printed pp. 4-168–4-171, opcode `1000_011S_IAAAAAAA` with S at bit 8
selecting accumulator A (`0`) or B (`1`)) lands seven rows mirroring
the `STH_SMEM` family with bit 9 set: `STH_ASM_SMEM` at `0x8680` under
mask `0xFEE0` (MOD0–MOD3) and `STH_ASM_SMEM_MOD4`/`MOD5`/`MOD6`/`MOD7`/
`MOD8`/`MOD10` at `0x86A0`/`0x86A8`/`0x86B0`/`0x86B8`/`0x86C0`/
`0x86D0` under mask `0xFEF8`, through the shared
`ACCUMULATOR_ARF_MOD`/`ACCUMULATOR_ARF_MOD4` layouts. Checked and
trusted leaves shift the S-selected accumulator by ASM (0–15 left,
16–31 arithmetic right with sign fill) and stage single-indirect,
indexed, circular, and reverse-carry MEMORY WRITE effects carrying
bits 31–16 of the shifted value — with SXM proven out of the value
path (both SXM rules coincide with arithmetic fill over the whole ASM
domain, so the new `ACCUMULATORS_STATUS_ASM` token claims ASM alone)
and no accumulator or T-register effect. All seven rows land
`UNRESOLVED` repeatability on the absent TI repeatability note
(`STH_SMEM` precedent). CPU retirement reuses the generic MEMORY
route with no `run.c` change. The RED executable contracts pin the
TI worked examples (`STH B,-8` with B FF_8421_1234 storing FF84 and
`STH A,-4` with A FF_8421_1234 under SXM=1 storing F842) at the leaf
and shifted high-word publication with post-increment, WAIT resume,
fault, and boundary transactionality on the CPU.
`FORM-T2-20-06` lands `SUBSET` with all seven symbols under
`STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`
(the `STH_SMEM` token, reused: identical mode coverage). Syntaxes 3–4
and `STLM` stay for later slices. No knob is added. Projected closure
adds two TI source records, one activation record, three requirements,
and two test records: 412 evidence records, 664 requirements, and 535
tests (535 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
