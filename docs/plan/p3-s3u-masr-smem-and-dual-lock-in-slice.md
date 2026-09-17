### P3-S3u: MASR Smem and dual lock-in slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `MASR Smem` (TI SPRU172C §MAS[R] Example 3, opcode
`0010_11RS_IAAAAAAA`) shares the `MAS_SMEM` row with
R=1, and `MASR Xmem, Ymem, src [, dst]` shares the nine
`MAS_DUAL_Mx_My` rows with R=1: no new catalog row, layout, production
change, inventory binding, or neighbor pin moves. The slice locks the
R=1 rounding path end to end on the maintained suites — the `0x2E95`
leaf completion (round(A - T*Smem) to `0xFFFFB70000` into A) and the
`0x2E34` CPU retirement already pin the checked Smem path, and the
`0xBD32` leaf completion (rounded `0xFFF3B40000` into B) and the
`0xBDBA` CPU retirement already pin the checked dual path — with two
new executable proofs: trusted R=1 completion through
`ti_c54x_trusted_op_mas_smem` on the `0x2E95` word, and trusted R=1
rounding equivalence through `ti_c54x_trusted_op_mas_dual_m0_m0` on
the `0xBD32` word, pinning that both trusted extractors stage the
rounding select and complete the rounded MASR values.
`FORM-T2-04-15`/`FORM-T2-04-17` stay `UNIMPLEMENTED` with empty symbol
lists by the recorded R-sharing convention (shared rows cannot
double-bind). Projected closure adds one activation record, two
requirements, and two test records on the shared suites: 383 evidence
records, 631 requirements, and 504 tests (504 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
