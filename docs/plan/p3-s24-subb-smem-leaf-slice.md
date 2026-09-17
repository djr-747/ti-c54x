### P3-S24: SUBB Smem leaf (planned)

This immutable anchor heads the P3-S24 evidence freeze. `SUBB Smem`
(TI SPRU172C §SUBB, p. 4-191: `0000_111D_IAAAAAAA`, class 3A/3B)
lands as a catalog-free handler leaf via managed fan-out:
`ti_c54x_op_subb_smem` (plain layout-gated entry) plus
`ti_c54x_trusted_op_subb_smem` (trusted three-operand entry) over
the shared `SELECT_INDIRECT_ADDR7` layout, executing
`dst = src40 − smem16 − ~C` with Smem never sign-extended and SXM
never consumed, C written 1 iff no borrow against the full 40-bit
lane, OVM saturation with sticky destination overflow, TC
preserved, and exotic-mode deferral. C and OVM knownness gate the
op alongside the source accumulator. Direct page-0 and
standard-indirect MOD0-MOD3 reads stage through the shared MEMORY
effect. No catalog rows, dispatch, decode, or CPU change;
`FORM-T2-02-11` stays `UNIMPLEMENTED` for the inventory-owning
rows slice. Projected closure adds two evidence records, one
requirement, and one test record on the new SUBB suite: 441
evidence records, 685 requirements, and 548 tests (548 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
