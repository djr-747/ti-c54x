### P3-S52: PSHD/POPD/PSHM/POPM stack pair-leg slice (planned)

Closes table 2-17 (`FORM-T2-17-02` through `-05`) behind the
landed S51 FRAME row. TI semantics (SPRU172C pp. 4-127/4-128,
4-131/4-132): `PSHD Smem` reads SP at decode, decrements it, and
stores `(Smem)` at `mem[SP]`; `POPD Smem` stores `(mem[SP])` at
`Smem` and increments SP; `PSHM`/`POPM` do the same with the MMR
operand. Encodings: `PSHD` `0x4B00`, `PSHM` `0x4A00`, `POPD`
`0x8B00`, `POPM` `0x8A00`, each under mask `0xFF00` with the
I-bit plus 7-bit cookie in bits 6:0 (Class 16A/16B and 17A/17B:
direct plus single-indirect Smem; MMR forms ride the S1 routing
seam). No neighbor row claims `0x4A00`/`0x4B00`/`0x8A00`/`0x8B00`;
each projects 256 first-word encodings.

Design (pinned): the Smem operand value must be read before the
stack write commits, so each leaf stages a DATA read-plus-stack-
write pair effect rather than a single-leg MEMORY effect. The
S6 PORT pair reducer owns the read-then-write pair shape
(`PORTW` = Smem read + IO write); S52 reuses that carrier with
both legs in DATA space (Smem read + stack write) instead of
forking a parallel implementation. The stack leg commits through
the landed W65A/W66A push/pop roots (which already own SP
decrement/increment with the mod-65536 wrap pin and unknown-SP
transactional rejection); the Smem leg resolves direct-only in
this slice (I=0 page-0, mirroring the S50 direct-only landing).
MMR legs (`PSHM`/`POPM`) route through the S1 MMR seam the way
`STM`/`LDM` rows do.

Scope: direct-only `PSHD`/`POPD`/`PSHM`/`POPM` (four rows, four
inventory flips to SUBSET). Indirect Smem (I=1, 16B/17B classes)
defers to a follow-on over the indirect address machinery; it
shares the pair carrier, so no leaf rewrite is admitted later.
No status-bit effects (Status Bits None on all four pages), no
timing, no RPT interaction (repeatability UNRESOLVED unless
Table 2-28 positively lists a form, per the W52A rule).

Census weight: `PSHM` 430 + `POPM` 447 + `PSHD` 24 + `POPD` 18
static words, the heaviest remaining block after `MAR` and the
`MVxx` moves. Landing this slice plus S51 completes table 2-17
and unblocks the first honest 5110 smoke runs past stack traffic.

Projected closure: 307 rows, 313 bindings, 36,361 lookup
entries, 155/205 forms SUBSET-or-better with strict 43/205.
These numbers are projections until the slice lands.
