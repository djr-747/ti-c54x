### P3-WAVE5: ADD/SUB leaf fan-out (implemented; local verification green)

This immutable anchor heads the P3-WAVE5 evidence freeze. Six
catalog-free handler leaves land through the second managed
fan-out (three isolated-worktree workers, maintainer-sourced TI
briefs, maintainer merge): `ti_c54x_op_add_lk` /
`ti_c54x_op_sub_lk` (ADD/SUB syntaxes 7-8, `FORM-T2-01-07/08` and
`FORM-T2-02-07/08`), `ti_c54x_op_add_xmem` /
`ti_c54x_op_sub_xmem` (ADD/SUB syntax 5, `FORM-T2-01-05` and
`FORM-T2-02-05`), and `ti_c54x_op_add_src_asm` /
`ti_c54x_op_sub_src_asm` (ADD/SUB syntax 10, `FORM-T2-01-10` and
`FORM-T2-02-10`), each with a plain layout-gated entry plus a
trusted three-operand entry. TI basis dual-pinned per family from
vendor/ti/spru172c.txt (§ADD opcode diagrams, pp. 4-3–4-4; §SUB
opcode diagrams, pp. 4-186–4-187): syn7 `lk<<SHFT`,
syn8 `lk<<16`, syn5 `Xmem<<SHFT`, syn10 `src<<ASM`, with
SXM-widened shifts, OVM saturation plus sticky overflow, C
written (carry for ADD, no-borrow for SUB), and TC preserved.
No catalog rows, dispatch, decode, or CPU change; all eight forms
stay `UNIMPLEMENTED` for the inventory-owning rows slices.
Projected closure adds four evidence records (three TI section
records plus this activation), six ARCHITECTURAL requirements,
and six COMMON test records on the six new suites: 447 evidence
records, 693 requirements, and 554 tests (554 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice
lands.
