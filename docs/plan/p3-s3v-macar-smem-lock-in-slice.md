### P3-S3v: MACAR Smem lock-in slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `MACAR Smem` (TI SPRU172C §MACA[R] Example 3, opcode
`0011_01RS_IAAAAAAA`) shares the `MACA_SMEM` row with R=1: no new
catalog row, layout, production change, inventory binding, or neighbor
pin moves. The slice locks the R=1 rounding path end to end on the
maintained suites — the `0x3734` leaf completion (the Example 1
product rounded to `0x6260000` into B) already pins the checked path
— with two new executable proofs: CPU retirement of the `0x3734`
direct word (same pre-state as the Example 1 direct lane, B rounded
to `0x6260000` with the read-phase T load), and trusted R=1
completion through `ti_c54x_trusted_op_maca_smem` on the `0x3734`
word, pinning that the trusted extractor stages the rounding select
and completes the rounded MACAR value. `FORM-T2-04-09`/`FORM-T2-04-10`
stay `UNIMPLEMENTED` with empty symbol lists by the recorded
R-sharing convention (shared rows cannot double-bind). Projected
closure adds one activation record, two requirements, and two test
records on the shared suites: 384 evidence records, 633
requirements, and 506 tests (506 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
