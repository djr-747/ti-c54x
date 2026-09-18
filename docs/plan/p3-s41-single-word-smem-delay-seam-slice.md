### P3-S41: single-word Smem delay write-back seam (implemented; local verification green)

Single-word sibling of the S3r MACD pair delay sidecar, unblocking
LTD Smem execution (S40 staged the leaf with `write_smem_delay`
set, which the WORD16 root rejected) and the future DELAY leaf. TI
SPRU172C §LTD (p. 4-81) pins the semantics: Smem is copied into T
and simultaneously into the address following Smem, leaving Smem
itself unchanged; the worked example (`LTD *AR3` with data word
`0100h` holding `6CACh`) loads `6CACh` into T and stores `6CACh`
into `0101h` while `0100h` keeps `6CACh`. No status bits are
affected.

The seam generalizes the `write_smem_delay` MEMORY carrier flag to
the single-word DIRECT and SINGLE_INDIRECT DATA READ shapes (48-byte
ABI unchanged): READ-only with no RMW and a zero staged value, with
an optional completion (a T-register-write completion is the LTD
shape; a null completion is the DELAY shape whose delay WRITE is the
only publication). WRITE and RMW access with delay stay
carrier-invalid, as do all other address kinds; MOD3 single reads
stay invalid by the established single-leg write-only rule.

The new `memory_instruction_single_delay` owner derives the DATA
delay target from the frozen candidate resolution (Smem address plus
one with 16-bit wraparound) and issues or resumes the delay WRITE
through the reused WORD16 child under a fresh transaction ID, by the
MACD pair precedent (DATA-space endpoint traffic, no MMR branch).
The READ observation is stashed in a root union slot the carrier
keeps disjoint from RMW staging; a delay-write resume pins the
in-flight child WRITE against the frozen target and the stashed
word. The WORD16 root runs the delay stage after the READ leg
completes and before the shared READY tail (which moves to the
commit owner for root and delay reuse): WAIT resumes only the delay
WRITE, a delay FAULT or cancel publishes nothing, and reset routes
by the ordinary PENDING/COMPLETE/FAULT pairings with no new phase.
Delay on an MMR-routed Smem read rejects transactionally before any
routing (MMR+1 write semantics are unevidenced; the S40 freeze
behavior for those addresses is preserved as INVALID_CANDIDATE).

The RED executable contract is the seam proof
`tests/unit/sequencer/instruction/test_single_word_delay_write.c`
(CTest `ti_c54x.single_word_delay_write`): the LTD shape (indirect
MOD0, T-write completion) issues a DATA WRITE of the TI-example
word `6CACh` from `0100h` to `0101h` and loads T; the DELAY shape
(direct, null completion) issues the write with no T or accumulator
change; read-then-write WAIT resumes mid-write; a delay-write FAULT
retires nothing; WRITE/RMW delay and MMR-routed delay reject with
no bus traffic. No catalog row, binding, or lookup change; the LTD
leaf needs no edit. Projected closure from the S40 freeze (291
rows, 297 bindings, 32,265 lookup entries, 139/205 forms at
67.805%, 754 requirements, 610 tests): 291 rows, 297 bindings,
32,265 lookup entries, 139/205 forms at 67.805%, 755 requirements,
611 tests (611 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
