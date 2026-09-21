### P3-S107: LMS first-word correction 0xC100 to 0xE100 (implemented; local verification green)

This immutable anchor heads the P3-S107 evidence correction.
P3-S106 landed `LMS Xmem,Ymem` at first word `0xC100`; three
independent TI instruction-set summary sources (SPRU307A
functional overview Table 13, the 5402-functions opcode
table, and the SPRS039C datasheet Table 13) list FIRS at
`1110_0000_XXXX_YYYY` and LMS at `1110_0001_XXXX_YYYY`, and
the landed EXP row (`1111_010S_1000_1110`) cross-confirms
that summary table against this repo. The in-repo S102
metadata test anticipated LMS at `0xE1FF`. The semantic leaf
is bit-agnostic, so the correction moves the catalog row to
`0xE100` under mask `0xFF00` with no leaf change: the
generated lookup sheds the `0xC1xx` population and gains the
`0xE1xx` population (counts unchanged at 356 rows, 357 IDs,
44409 words), the op/CPU/metadata tests move their word
constructors to `0xE19A`/`0xE1xx`, the LMS metadata test pins
`0xE0FF` UNKNOWN (FIRS not yet landed) beside `0xE200`
SQDST, and the SQDST boundary test absorbs `0xE1FF` as LMS.
Evidence follows the tombstone precedent: the `1100_0001`
record retires with a reason naming the transcription error
and its replacement, one R1 record carries diagram
`1110_0001_XXXX_YYYY`, and the three S106 requirements
relink to R1 with the `0xE100` address. The S106 freeze doc
keeps its body untouched; this doc and the ledger carry the
correction. Lesson for FIRS/POLY and all remaining rows:
Table-13-level opcode confirmation precedes landing.
Projected closure: evidence, requirements, and tests hold at
507/838/692; bindings hold at 380; forms hold at 185/205
with 52 strict-complete.
