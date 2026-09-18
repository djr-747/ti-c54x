### P3-S31: ADD/SUB Smem SHIFT rows (implemented; local verification green)

This immutable anchor heads the P3-S31 evidence freeze. It
lands Seam 1 end to end: a new shared two-word operand layout
`INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5` (Smem cookie in
word 0, S/D plus signed 5-bit SHIFT in word 1), two
catalog-free pure leaves (`op_add_smem_shift`,
`op_sub_smem_shift` with plain plus trusted entries), and two
`MASKED2` catalog rows `ADD_SMEM_SHIFT` (`0x6F00/0xFF00` +
`0x0C00/0xFCE0`) and `SUB_SMEM_SHIFT` (`0x6F00/0xFF00` +
`0x0C20/0xFCE0`), all `UNRESOLVED` repeatability with
`ACCUMULATORS_STATUS_SXM_OVM` inputs, wired through the
catalog X-macro into dispatch. The seven overlapping STH
shift rows (`STH_SHIFT_DIRECT`, `STH_SHIFT_SMEM_MOD4/5/6/7/8/10`)
convert `MASKED` to `MASKED2` with their `0x0C60/0xFEE0`
second-word pattern (TI-pinned, disjoint from both syn4
patterns), and the syn4 rows sit after every `0x6Fxx` row so
first-match keeps the established first-word identities;
completion re-resolves through the disjoint second-word
patterns. `FORM-T2-01-04` and `FORM-T2-02-04` bind at `SUBSET`
under `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3` and flip
`UNRESOLVED` to `COMMON_CANDIDATE` with the rows. The
generated first-word lookup grows by 48 entries with zero
neighbor moves. Projected closure adds two evidence records,
three requirements, and two test records on the new semantics
suites plus the binding suites: 456 evidence records, 702
requirements, and 558 tests (558 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
