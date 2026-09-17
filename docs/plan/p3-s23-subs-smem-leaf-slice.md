### P3-S23: SUBS Smem leaf (planned)

This immutable anchor heads the P3-S23 evidence freeze. `SUBS Smem`
(TI SPRU172C §SUBS, p. 4-194: `0000_101S_IAAAAAAA`, class 3A/3B)
lands as a catalog-free handler leaf via managed fan-out:
`ti_c54x_op_subs_smem` (plain layout-gated entry) plus
`ti_c54x_trusted_op_subs_smem` (trusted three-operand entry) over
the shared `SELECT_INDIRECT_ADDR7` layout, executing
`dst = src40 − unsigned(smem16)` with SXM never consumed, C
written 1 iff no borrow, OVM saturation with sticky destination
overflow, TC preserved, and exotic-mode deferral. Direct page-0
and standard-indirect MOD0-MOD3 reads stage through the shared
MEMORY effect. No catalog rows, dispatch, decode, or CPU change;
`FORM-T2-02-13` stays `UNIMPLEMENTED` for the inventory-owning
rows slice. Projected closure adds two evidence records, one
requirement, and one test record on the new SUBS suite: 439
evidence records, 684 requirements, and 547 tests (547 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
