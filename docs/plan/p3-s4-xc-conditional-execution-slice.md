### P3-S4: XC conditional-execution slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `XC n,cond` (TI SPRU172C §XC, opcode
`1111_11N1_CCCCCCCC` with N at bit 9 selecting n = 1 (`0`) or n = 2
(`1`), pp. 4-198–4-200) lands one row: `XC` at `0xFD00` under mask
`0xFD00` (512 N/condition encodings under one masked row) through a
new `N_CONDITION8` layout (N@9, cond@7:0). Execution is taken/not-taken
over the 8-bit condition field: if the condition holds the next n
instruction words execute normally, otherwise the CPU retires n NOP
words (PC advances past them with no fetch, no effect, no bus
traffic). Single-condition evaluation reuses the shared
branch-condition core for its sixteen codes and adds `UNC`
(unconditional take); `BIO`/`NBIO`, `AOV`/`ANOV`/`BOV`/`BNOV`, and
all multi-condition combination fields have no input carrier yet
(no BIO pin, no OVA/OVB latch in the op snapshot) and defer as
`INVALID_OPERAND` at the op leaf, mirroring the deferred-code
precedent. The row lands `INELIGIBLE` repeatability on the explicit
TI `not repeatable` statement (BC precedent), so the repeat-body
gate blocks it with no new wiring. Status bits are none; inputs are
`ACCUMULATORS_STATUS_C_TC`. The TI three-word uninterruptible window
is recorded but not modeled: no IRQ service path exists to arbitrate,
and the single-threaded run loop cannot interleave inside the
retirement. Cycle counts and the sampled-two-cycles-early note are
not adopted. The RED executable contracts pin the TI Example vector
(`XC 1,ALEQ` with A −1 executing the follower) at the leaf, through
the trusted entry, and on the CPU (taken and not-taken for n = 1 and
2, T preservation, deferred-field transactional reject), plus the
layout suite. `FORM-T2-18-07` lands `SUBSET` with the `XC` symbol.
Projected closure adds one catalog row, one inventory binding, one TI
source record, one activation record, four requirements, and three
test records: 392 evidence records, 648 requirements, and 519 tests
(519 `PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.
