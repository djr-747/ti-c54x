### P3-S65: WRITA Smem-to-program move slice (planned)

`WRITA Smem` (FORM-T2-26-12) copies the Smem word into the
program word addressed by accumulator A. It is the eighth
5110 census move (46 occurrences) and the mirror of the S64
READA slice on the same `SMEM_PROGRAM_A_PAIR_WORD16`
carrier with access READ: the Smem leg reads first, the
PROGRAM leg writes second.

Encoding (TI SPRU172C WRITA p. 4-196, vendored text): one
word pins `0x7F00` with the standard 7-bit Smem cookie at
bits 6:0 (`I` at bit 7). Two rows: `WRITA_DIRECT` (`0x7F00`
under `0xFF80`, DP-relative direct, `ACCUMULATORS_STATUS_DP`)
and `WRITA_SMEM` (`0x7F80` under `0xFF80`, indirect
`MOD0--MOD3`, `ACCUMULATORS_AUXILIARY_REGISTERS`). The
one-word `INDIRECT_ADDR7` layout is shared (no `pmad`
extension word). The `0x7C`/`0x7D`/`0x7E` neighbors stay
MVPD/MVDP/READA. Absolute and long-offset Smem (three-word
forms) stay undecoded. Status Bits are None. Repeatability
stays UNRESOLVED per the W52A rule (PAR/RC repeat stepping
stays with the RPT machinery; only the single transfer is
modeled).

Root design: no carrier or root changes — the S64 root
already implements both leg orders keyed on the effect
access with the PROGRAM address derived from input A. The
Smem leg resolves through the single-leg resolver (READ
access), the PROGRAM WRITE leg builds from the A-derived
address, and issue runs Smem-read-before-program-write with
per-leg MMR-route-or-endpoint service (PROGRAM legs bypass
the DATA/IO-only adapter). The Smem AR update publishes
through the existing commit block. MOD3 reads reject in the
address planner (established `UNSUPPORTED_ACCESS` rule for
pre-access updates on reads), pinned by a CPU reject test.

Out of scope: three-word absolute/long-offset Smem, PAR/RC
repeat stepping, RPT stepping (PAR/DAR), timing.
