### P3-S3y: MPYR Smem lock-in slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `MPYR Smem` (TI SPRU179C §MPY syntax 1 with `rnd()`,
opcode `0010_00RD_IAAAAAAA` with R=1) shares the `MPY_SMEM` row: no
new catalog row, layout, production change, inventory binding, or
neighbor pin moves. The slice locks the R=1 rounding path end to end
on the maintained suites — the `0x2200` leaf completion (TI MPY
Example 4: T `0x1234` times Smem `0x5678` rounded to `0x6260000`
into A) and the `0x2320` CPU retirement already pin the checked
path — with one new executable proof: trusted R=1 completion
through `ti_c54x_trusted_op_mpy_smem` on the `0x2200` word, pinning
that the trusted extractor stages the rounding select and completes
the rounded MPYR value. `FORM-T2-03-02` stays `UNIMPLEMENTED` with
an empty symbol list by the recorded R-sharing convention (shared
rows cannot double-bind). Projected closure adds one activation
record, one requirement, and one test record on the shared suite:
388 evidence records, 639 requirements, and 512 tests (512
`PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.
