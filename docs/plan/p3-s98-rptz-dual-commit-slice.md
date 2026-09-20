### P3-S98: RPTZ dual-commit slice (planned)

`FORM-T2-16-05` (`RPTZ dst,#lk`: 0 ← dst, lk ← RC).
Split from P3-S97: zeroing an accumulator in the same
retirement as the repeat start is the first dual-commit
instruction in the tree — a leaf stages one effect, so
this needs a `REPEAT_START` effect-struct extension
carrying the optional dst-zero plus the
`repeat_progress_start` apply path, not a leaf alone.

Evidence basis (one new E-TI row). TI SPRU172C §RPTZ
(p. 4-150): `1111 000D 0111 0001` plus the 16-bit lk
(two words, 0 ← dst with D selecting A/B, lk ← RC,
status bits none).

Scope: `RPTZ_DST_LK` (`MASKED 0xF071/0xFEFF`,
2 words, `ACCUMULATOR_IMMEDIATE_LK16` reusing the
`LD_LK_16` shape, `INELIGIBLE`); the shared
`REPEAT_START` dst-zero extension with all existing
producers defaulting it off; one
checked-plus-trusted leaf in `src/isa/ops/repeat/`;
dispatch wiring; inventory flip of `FORM-T2-16-05`
to `SUBSET`; generated lookup regeneration.

RED contracts:
`tests/unit/isa/ops/repeat/test_rptz.c` (D
selection, lk extraction, staged `REPEAT_START`
carrying the dst-zero, parity, rejects); sequencer
`repeat_progress_start` clear-path tests;
`tests/unit/cpu/isa/repeat/` extension (dst zeroes,
RC loads, body repeats lk+1 times). Acceptance is
the focused tests plus the full gate, with the park
run byte-identical absent the new opcode.

Out of scope: parallel dual-memory forms, `RPTB[D]`
extensions, block repeat, timing.
