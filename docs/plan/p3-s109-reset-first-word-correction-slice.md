### P3-S109: RESET first-word correction 0xF7C0 to 0xF7E0 (planned)

This immutable anchor heads the P3-S109 evidence freeze. One
exact one-word row moves on established TI evidence: `RESET`
at 0xF7C0 becomes 0xF7E0. Two independent TI
instruction-set summary tables (SPRU307A Table 13 and SPRS039C
Table 13, both listing INTR K at 1110_0111_110K_KKKK beside
RESET at 1110_0111_1110_0000) show the S103 transcription of
the SPRU172C section RESET diagram was off by the low nibble;
the landed IDLE row (0xF7E1) and the INTR row landing next
(0xF7C0 under mask 0xFFE0) cross-confirm that table. The
bit-agnostic semantic leaf is untouched: the exact row moves
to 0xF7E0, the generated lookup sheds the 0xF7C0 population
for 0xF7E0 (counts hold at 357 rows, 358 IDs, 44665 words,
381 bindings, 186/205, 53 strict-complete), op/CPU/metadata
tests move their word constructors to 0xF7E0 with 0xF7DF
UNKNOWN beside 0xF7E1 IDLE, and the freed 0xF7C0 decodes
UNKNOWN, matching TI's map which assigns INTR there.
Evidence follows the tombstone precedent (R1 carries 0xF7E0;
S103 requirements relink; S103 freeze doc untouched).
Projected closure holds 509 evidence records, 841
requirements, and 695 tests (695 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
