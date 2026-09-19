### P3-S64: READA program-to-Smem move slice (implemented; local verification green)

`READA Smem` (FORM-T2-26-11) copies the program word
addressed by accumulator A into Smem. It is the seventh 5110
census move (47 occurrences) and the first
accumulator-addressed PROGRAM leg: neither the immediate
`pmad` carrier nor the DATA-only pair carrier applies.

Encoding (TI SPRU172C READA p. 4-136, vendored text): one
word pins `0x7E00` with the standard 7-bit Smem cookie at
bits 6:0 (`I` at bit 7). Two rows: `READA_DIRECT` (`0x7E00`
under `0xFF80`, DP-relative direct, `ACCUMULATORS_STATUS_DP`)
and `READA_SMEM` (`0x7E80` under `0xFF80`, indirect
`MOD0--MOD3`, `ACCUMULATORS_AUXILIARY_REGISTERS`). The
one-word `INDIRECT_ADDR7` layout is shared (no `pmad`
extension word). The `0x7C`/`0x7D` neighbors stay MVPD/MVDP;
`0x7F` stays undecoded for WRITA. Absolute and long-offset
Smem (three-word forms) stay undecoded. Repeatability stays
UNRESOLVED per the W52A rule.

Root design: a new `SMEM_PROGRAM_A_PAIR_WORD16` arm stages
the Smem selectors (same 20-byte footprint, no `pmad`
field); the PROGRAM address resolves from input A (low 16
bits, `PAR <- A`) at prepare time with an `UNKNOWN_INPUT`
gate on A-unknown. The effect `access` names the Smem-leg
direction (PORT convention): READA completes WRITE. The S62
pair root is extended to the new kind: the Smem leg resolves
through the single-leg resolver (WRITE access), the PROGRAM
leg builds from the A-derived address, and issue runs
program-read-before-Smem-write with per-leg
MMR-route-or-endpoint service (PROGRAM legs bypass the
DATA/IO-only adapter). The Smem AR update publishes through
the existing commit block. PAR/RC repeat stepping is out of
scope (RPT machinery owns it); only the single transfer is
modeled. `WRITA` mirrors on the carrier next slice with
access READ.

Out of scope: WRITA (own slice), three-word
absolute/long-offset Smem, PAR/RC repeat stepping, RPT
stepping (PAR/DAR), timing.

Decisions recorded during implementation. The new kind routes
through the existing SMEM_PROGRAM pair route: the child-route
classifier maps `SMEM_PROGRAM_A_PAIR_WORD16` to
`ROUTE_SMEM_PROGRAM_PAIR_WORD16` (missing this maps to
ROUTE_NONE and the CPU traps INVALID_STATE before issue with
no endpoint traffic). Trusted leaves keep the sibling
`(void)instruction` contract with no NULL-instruction guard;
the null test matches test_mvdp.c (NULL operands/input/effect
only). `memory_effect.h` stands at 517 lines with the new arm:
the one-arm-per-kind ABI union cannot split across headers
without breaking the static-assert-pinned 24-byte spec, so the
reviewed no-split decision tracks the repository <800 rule
(the dual hot-path bound carries the exemption).
