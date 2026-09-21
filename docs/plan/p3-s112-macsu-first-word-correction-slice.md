### P3-S112: MACSU dual first-word correction 0xA8xx to 0xA6xx (implemented; local verification green)

This immutable anchor heads the P3-S112 evidence freeze. P3-S3s
landed nine `MACSU_DUAL_Mx_My` rows at `0xA800` under mask
`0xFECC`; the TI SPRU172C section MACSU diagram (p. 4-91,
`1010_011S_XXXX_YYYY`) shows the dual-MACSU first word is
`0xA600`, and the landed LD||MAC section diagram
(`1010_10RD_XXXX_YYYY`) independently owns `0xA800`. The
pinned E-TI record already carries bits `1010011`; only the
rows, the shared leaf hardcodes, and the tests move. The
bit-agnostic semantic leaves are untouched: the nine rows
move to `0xA600` plus X/Y mode bits under unchanged mask
`0xFECC` (288 relocated lookup words, all counts hold), the
op/CPU/decode tests move their word constructors to
`0xA6xx`/`0xA7xx` (S-bit), and the freed `0xA800` decodes
UNKNOWN, matching TI's map which assigns LD||MAC[R] there.
The S3s freeze doc keeps its body untouched per the
immutability rule (this doc and the ledger carry the
correction); the stale project activation record retires
with a reason naming the transcription error and its
replacement, and the S3s inventory requirement relinks to
the correction record. Standing lesson extended:
Table-13-level opcode confirmation precedes landing, and
same-nibble neighbors disambiguate same-prefix dual rows
(MACSU `1010_011` vs LD||MAC `1010_10x`).
Closure adds one PROJECT correction record, retires the
stale activation record, and updates two evidence-index doc
rows plus one tombstone row; the registry counts land at
510 evidence records, 847 requirements, and 701 tests
(701 `PASSING`, 0 `PLANNED`).
All other pins hold: 383 bindings, 188/205 forms (91.707%),
55 strict-complete (26.829%), 359 rows, 360 IDs, 44953
lookup slots.
