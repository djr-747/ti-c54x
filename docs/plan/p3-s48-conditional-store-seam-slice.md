### P3-S48: conditional-store seam (implemented; local verification green)

Maintainer arch seam unblocking the class-15 conditional
stores (`SACCD`/`SRCCD`/`STRCD`, `FORM-T2-21-02/03/04`):
all three share a 4-bit accumulator condition, a dual-X
Xmem operand, and always-read/conditional-value RMW with
one AR update. TI ground truth (SPRU172C pp. 4-152, 4-165,
4-186): cond codes AGEQ=0010, ALT=0011, ANEQ=0100,
AEQ=0101, AGT=0110, ALEQ=0111 over accumulator A and
BGEQ=1010, BLT=1011, BNEQ=1100, BEQ=1101, BGT=1110,
BLEQ=1111 over accumulator B (A-codes `0xxx`, B-codes
`1xxx`; nibbles 0000, 0001, 1000, 1001 unassigned). The
execution pages state the seam design verbatim: "If the
condition is false, the instruction reads Xmem and writes
the value in Xmem back to the same address ... Regardless
of the condition, Xmem is always read and updated."
Opcodes: SACCD `1001_111S_XXXX_COND` (S selects the A/B
source), SRCCD `1001_1101_XXXX_COND` (BRC source), STRCD
`1001_1100_XXXX_COND` (T source); one word, one cycle,
class 15. SACCD stores `src << (ASM-16)` and is affected
by ASM and SXM, so the S49 SACCD leaf must shift before
staging. TI worked examples pin SRCCD/STRCD `*AR5-,AGT`
with A `00_70FF_FFFF` (AGT true), AR5 `0202` to `0201`,
source `4321`, data `0202h` `1234` to `4321`.

The seam adds: (1) a `BRC_REGISTER` input resource (bit
16384, fifteenth resource) with snapshot fields and a
`collect.c` arm reading the MMR file BRC cell (S44 TRN
mirror; the `mmr_file` source is already wired); the
eleven `0x4000` invalid-resource probes move to `0x8000`
(the snapshot carrier stays 72 bytes on LP64 with the
embedded instruction transaction root staying 392 on
LP64 and moving 356 to 360 on ILP32).
(2) a pure shared accumulator-condition evaluator (new
small store-owned module `op_store_cond_common`,
mirroring `branch_cond_evaluate` in a dependency-free
shape because no store-to-branch include exists): the
twelve TI codes as full-40-bit two's-complement signed
compares against zero with guard bits included, explicit
consumed-input-only knownness, unknown selected
accumulator rejecting, unassigned nibbles rejecting.
(3) the `OVERWRITE`/`WRITEBACK` RMW operations selected by
the leaf from the evaluated condition: cond-true stages
the store value and ignores the read word, cond-false
rewrites the read word and ignores the staged value,
exactly the TI read-always/update-always execution;
neither stages flags and `NONE` still rejects.
(4) the `SELECT_DUAL_XMOD_XAR_COND4` operand layout (S@8,
XMOD@7:6, XAR@5:4, COND@3:0) for the `1001_11xx_XXXX_COND`
family shape. No catalog row, handler, inventory form, or
CPU retirement change is admitted; the three leaves belong
to S49. Closure from the S47 freeze (298 rows, 304
bindings, 33,801 lookup entries, 146/205 forms at 71.220%,
777 requirements, 633 tests): catalog, lookup, bindings,
and family coverage unchanged, 779 requirements, 635
tests.
