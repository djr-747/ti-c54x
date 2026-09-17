### P3-S8: BACC accumulator-addressed branch family slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `BACC[D]` (TI SPRU172C §BACC[D], printed p. 4-15)
lands two rows on the S5 accumulator-addressed push seam:
`BACC` at `0xF4E2` and `BACCD` at `0xF6E2`, both under the shared
`0xFEFF` mask (one S field at bit 8, one nondelayed and one delayed
row) through the shared `ACCUMULATOR_SOURCE` layout (S = 0 selects
accumulator A, S = 1 selects B; the layout is shared with the future
`CALA`/`CALAD` rows). Execution is unconditional over the selected
accumulator: PC is replaced by the 16 low bits of
`Acc(DMD)` with no status, accumulator, T-register, or MEMORY
effect. Both rows land `INELIGIBLE` repeatability on the explicit TI
`not repeatable` statement, so the repeat-body gate blocks them with
no new wiring. CPU retirement retires `BACC` followers normally and
`BACCD` followers through the two-word delay window with PC advance,
T preservation, and no endpoint traffic. The RED executable contracts
pin A/B selection on both rows at the leaf, through the trusted
entry, and on the CPU (delay-window consumption, unknown-accumulator
transactional reject), plus the shared layout suite.
`FORM-T2-12-02` lands `SUBSET` with the `BACC` symbol under
`ACCUMULATOR_NONDELAYED` and the `BACCD` symbol under
`ACCUMULATOR_DELAYED`. No knob is added.
Projected closure adds five TI source records, one activation record,
four requirements, and five test records: 400 evidence records, 655
requirements, and 527 tests (527 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
