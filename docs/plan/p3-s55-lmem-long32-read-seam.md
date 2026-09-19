### P3-S55: single-word Lmem LONG32 read seam (planned)

This immutable `(planned)` heading is the durable S55 FIT/ownership
anchor. S55 is a sealed seam with no catalog rows: the T2-05
double-precision family (`DADD`/`DSUB`/`DRSUB`/`DADST`/`DSADT`/
`DSUBT`, TI SPRU172C Table 2-5) reads a 32-bit `Lmem` through a
single-word `I` + 7-bit cookie encoding, but the only landed
LONG32 leg is the two-word `MOD13` long-offset arm
(`DLD_LONG32_MOD13`/`DST_LONG32_MOD13`). The seam adds address
kind 21 (`LONG32_SINGLE`, reusing the pair-arm shape: direct-DP
`dma7` plus standard-indirect MOD0-MOD3 `mode`/`arf` with the
frozen DP page in the word slot; no ABI growth) and the pure
`ti_c54x_sequencer_plan_long32_single_access` planner composing
the landed `direct_dp` concatenation and
`indirect_single_long32` resolvers into a canonical LONG32
`data_operand` (even MSW pairs up, odd MSW pairs down) with
MOD3-read rejection carried by the single-leg write-only rule.
No handlers, no CPU branches, no timing, no knob.

Scope: kind plus arm plus planner plus validation plus the lane-2
hook (the pair-transaction commit root consumes the planned
operand). Lane 1 lands no catalog rows and no registry deltas
(S6-lane-1 precedent); the DADD pilot row plus C16 ALU core
follow as lane 2 with FORM-T2-05-01 binding.

RED lanes: `ti_c54x.long32_single_resolver` (direct DP+DMA
vectors, indirect MOD0-MOD3 AR vectors with two-word deltas,
MOD3-read decline, nine malformed arms plus wrong-kind/argument
rejects); `ti_c54x.long32_single_transaction` (planned operand
feeds the landed pair transaction MSW/LSW issue shape).
Non-goals: exotic MOD4-MOD10 Lmem, long-offset/absolute Lmem,
the C16 ALU, any catalog row, far/XPC, timing.
