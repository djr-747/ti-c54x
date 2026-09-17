### P3-S3t: MACR Xmem,Ymem dual lock-in slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `MACR Xmem, Ymem, src [, dst]` (TI SPRU172C §MAC[R]
syntax 2, opcode `1011_0RSD_XXXX_YYYY`, pp. 4-82–4-84) shares the nine
`MAC_DUAL_Mx_My` rows with R=1: no new catalog row, layout, production
change, inventory binding, or neighbor pin moves. The slice locks the
R=1 rounding path end to end on the maintained suites — the
`0xB532` leaf completion (`(0x5678<<1)` x `0x1234` + 4 rounded to
`0x0C4C0000` into B) and the `0xB5BA` CPU retirement (same product
with MOD2 post-increments) already pin the checked path — with one
new executable proof: trusted R=1 equivalence through
`ti_c54x_trusted_op_mac_dual_m0_m0` on the `0xB532` word, pinning that
the trusted extractor stages the rounding select and completes the
rounded MACR value. `FORM-T2-04-05`/`FORM-T2-04-06` stay
`UNIMPLEMENTED` with empty symbol lists by the recorded S3f R-sharing
convention (shared rows cannot double-bind). Projected closure adds
one activation record, two requirements, and two test records on the
shared suites: 382 evidence records, 629 requirements, and 502 tests
(502 `PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.
