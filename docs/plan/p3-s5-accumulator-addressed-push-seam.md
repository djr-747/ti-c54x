### P3-S5: accumulator-addressed push seam (planned)

This immutable `(planned)` heading is the durable S5 FIT/ownership
anchor. S5 is a sealed architectural seam with no catalog rows, no
opcode identities, no handlers, and no CPU opcode branch, following
the W43A pattern. TI SPRU172C §4 `CALA[D]` (printed pp. 4-24–4-25)
is a one-word call to the 16-bit address in the low part of the
selected accumulator: nondelayed pushes `(PC) + 1` to TOS,
delayed pushes `(PC) + 3` to TOS, both over an SP − 1 TOS with a
PRE_ACCESS SP update, then `(src(15–0))` replaces PC; no
status-bit effects, and the description carries an explicit
not-repeatable note. The seam adds two MEMORY address kinds,
`STACK_PUSH_ACCUMULATOR_WORD16` and
`STACK_PUSH_ACCUMULATOR_DELAYED_WORD16`, with TOS planning
identical to the immediate pushes; the A/B accumulator select
rides `arf` (0 for A, 1 for B) while every other first-arm field
stays zero. The PC target is the low 16 bits of the selected
accumulator read from the frozen input snapshot gated on
accumulator knownness at prepare (unknown rejects before any bus
issue); commit re-derives the same target deterministically from
the frozen input, so no frozen field grows and root sizes stay
frozen. Return addresses reuse the existing bridge successor
(PC+1 for the one-word form, PC+3 delayed). No timing, IRQ/RPT,
wrap, machine-cycle, public-ABI, or new public CPU snapshot claim
is admitted. No knob is added.

Ownership: `REQ-PROJECT-P3-S5-PUSH-EFFECT`
(`src/isa/ops/memory/`),
`REQ-PROJECT-P3-S5-PUSH-RESOLVER` (`src/sequencer/memory/`),
and `REQ-PROJECT-P3-S5-PUSH-TRANSACTION`
(`src/sequencer/instruction/`) carry the project contract over TI
`CALA[D]` push evidence. The three RED lanes are
`tests/unit/isa/ops/memory/test_accumulator_push_memory_effect.c`
(CTest `ti_c54x.accumulator_push_memory_effect`),
`tests/unit/sequencer/memory/test_accumulator_push_resolution.c`
(CTest `ti_c54x.accumulator_push_resolution`), and
`tests/unit/sequencer/instruction/test_accumulator_push_root.c`
(CTest `ti_c54x.accumulator_push_root`). Exit: sealed seam with
all three lanes green and the CALA/CALAD family unblocked.
Projected closure adds one TI source record, one plan-anchor
record, three requirements, and three test records: 394 evidence
records, 651 requirements, and 522 tests (522 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
