### P3-S37: W7 pilot ABS/NEG/NORM accumulator rows (implemented; local verification green)

First managed fan-out landing: three single-word accumulator rows
on the existing `ACCUMULATOR_SOURCE_DESTINATION` layout, no new
seam — `FORM-T2-06-02` (ABS src[,dst], `0xF485/0xFCFF`, TI
SPRU172C section ABS p. 4-3), `FORM-T2-06-10` (NEG src[,dst],
`0xF484/0xFCFF`, section NEG pp. 4-119--4-120), and
`FORM-T2-06-11` (NORM src[,dst], `0xF48F/0xFCFF`, section NORM
pp. 4-121--4-122). S/D ride bits 9:8; all three rows stay
`UNRESOLVED` (neither Table 2-28 nor Table 6-18 lists them, and
omission never implies eligibility — the SAT precedent). ABS
stages `dst = |src|` with OVM-gated saturation of guard
overflow and C set iff the result is zero; NEG stages the
40-bit 2s complement with C set iff src is zero and full
signed-32 saturation (both the `0x80_0000_0000` wrap and the
`0xFF_8000_0000`-to-`0x00_8000_0000` prose edge); NORM stages
`dst = src << TS` with T(5:0) signed count, SXM-ruled right
shifts, OVM-saturated left-shift overflow, and C/TC preserved.
Leaves were built row-less by isolated workers against
maintainer TI briefs (RND sibling structure, UNKNOWN id) and
merged with id flips plus the NEG saturation generalization
(the worker covered only the minimum edge). Five older tests
needed sibling treatment for the new words (`0xF48E` EXP,
`0xF490` ROR, `0xF486` MAX, `0xF487` MIN, MACA-T probe to
`0xF47F`). Projected closure from the S36 freeze (283 rows,
289 bindings, 30,333 lookup entries, 131/205 forms at 63.902%,
730 requirements, 586 tests): 286 rows, 292 bindings, 30,345
lookup entries (+12 pure), 134/205 forms at 65.366%
SUBSET-or-better, 739 requirements, 595 tests. These numbers
are projections until the slice lands.
