### P3-S9: CALA accumulator-addressed call family slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `CALA[D]` (TI SPRU172C §CALA[D], printed pp. 4-24–
4-25, opcode `1111_01ZS_1110_0011` with Z at bit 9 selecting
nondelayed `CALA` (`0`) or delayed `CALAD` (`1`) and S at bit 8
selecting accumulator A (`0`) or B (`1`)) lands two rows on the S5
accumulator-addressed push seam: `CALA` at `0xF4E3` and `CALAD` at
`0xF6E3`, both under the shared `0xFEFF` mask through the shared
`ACCUMULATOR_SOURCE` layout (no new layout requirement: the S8
layout requirement already names the future `CALA`/`CALAD` rows).
Checked and trusted leaves stage a MEMORY
`STACK_PUSH_ACCUMULATOR(_DELAYED)` TOS WRITE with the select in
`arf` and a zero return-address placeholder, gated on SP knownness
(`INVALID_OPERAND`) and selected-accumulator knownness
(`UNKNOWN_INPUT`); the S5 seam root derives the PC target from the
frozen input snapshot and stages PC+1 (`CALA`) or PC+3 (`CALAD`)
return addresses. Both rows land `INELIGIBLE` repeatability on the
explicit TI `not repeatable` statement, so the repeat-body gate
blocks them with no new wiring. A new input-resource token,
`ACCUMULATORS_STACK_POINTER`, carries exactly the two consumed
inputs (no status claim); the collector and `KNOWN_MASK` already
cover both bits, so no sequencing change is admitted. CPU
retirement reuses the generic MEMORY route with no `run.c` change:
`CALA` retires with the TOS WRITE carrying PC+1 and PC replaced by
the accumulator target, `CALAD` through the two-word delay window
with PC+3 before the frozen deferred target takes effect. The RED
executable contracts pin the TI worked examples (`CALA A` with A
00_0000_3000 at PC 0025/SP 1111 to TOS[1110] 0026; `CALAD B` with B
00_0000_2000 to TOS[1110] 0028) at the leaf, through the trusted
entry, and on the CPU, plus delay-window, MMR-route, WAIT, fault,
and unknown-input transactionality. `FORM-T2-13-01` lands `SUBSET`
with the `CALA` symbol under `ACCUMULATOR_NONDELAYED` and the
`CALAD` symbol under `ACCUMULATOR_DELAYED`. No knob is added.
Projected closure adds five TI source records, one activation record,
three requirements, and four test records: 406 evidence records, 658
requirements, and 531 tests (531 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
