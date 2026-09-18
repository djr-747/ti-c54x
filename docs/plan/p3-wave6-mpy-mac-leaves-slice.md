### P3-WAVE6: MPY/MAC #lk leaf fan-out (implemented; local verification green)

This immutable anchor heads the P3-WAVE6 evidence freeze. Two
catalog-free handler leaves land through the third managed
fan-out (two isolated-worktree workers, maintainer-sourced TI
briefs, maintainer merge): `ti_c54x_op_mpy_lk` (MPY syntax 4,
`FORM-T2-03-05`) and `ti_c54x_op_mac_lk` (MAC syntax 3,
`FORM-T2-04-02`), each with a plain layout-gated entry plus a
trusted three-operand entry through the shared P3-S3 multiply
datapath. TI basis dual-pinned from vendor/ti/spru172c.txt
(§MPY[R] opcode/syntax, pp. 4-100–4-101; §MAC[R]
opcode/syntax, pp. 4-81–4-82): syn4 `(T) x lk -> dst` over
`0xF066|D<<8` plus lk, syn3 `(T) x lk + src -> dst` over
`0xF067|S<<9|D<<8` plus lk, unrounded forms only, T read-only,
FRCT shift with OVM saturation, OVdst (OVsrc when dst==src for
MAC), C and TC preserved. No catalog rows, dispatch, decode, or
CPU change; both forms stay `UNIMPLEMENTED` for the
inventory-owning rows slices. Projected closure adds three
evidence records (two TI section records plus this activation),
two ARCHITECTURAL requirements, and two COMMON test records on
the two new suites: 451 evidence records, 696 requirements, and
556 tests (556 `PASSING`, 0 `PLANNED`). These numbers are
projections until the slice lands.
