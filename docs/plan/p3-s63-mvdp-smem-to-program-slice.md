### P3-S63: MVDP Smem-to-program move slice (implemented; local verification green)

`MVDP Smem, pmad` (FORM-T2-26-04) copies the Smem word into
the program word at a 16-bit-immediate address. It is the
sixth 5110 census move (5 occurrences) and the mirror of the
S62 MVPD slice on the same `SMEM_PROGRAM_PAIR_WORD16`
carrier with access READ: the Smem leg reads first, the
PROGRAM leg writes second.

Encoding (TI SPRU172C MVDP, vendored text): two words pin
`0x7D00` with the standard 7-bit Smem cookie at bits 6:0
(`I` at bit 7); word 1 is the 16-bit absolute `pmad`. Two
rows: `MVDP_DIRECT` (`0x7D00` under `0xFF80`, DP-relative
direct, `STATUS_DP`) and `MVDP_SMEM` (`0x7D80` under `0xFF80`,
indirect `MOD0--MOD3`, `AUXILIARY_REGISTERS`). The
direction-neutral `INDIRECT_ADDR7_EXTENSION16` layout is
shared. The leaf admits `MOD0--MOD3`; `MOD3` reads reject in
the address planner (established `UNSUPPORTED_ACCESS` rule
for pre-access updates on reads), pinned by a CPU reject
test. The `0x7C` neighbor stays MVPD (S62 pins both
directions). Absolute and long-offset Smem (three-word
forms) stay undecoded. Repeatability stays UNRESOLVED per
the W52A rule.

Root design: no carrier or root changes — the S62 root
already implements both leg orders keyed on the effect
access. The Smem leg resolves through the single-leg
resolver (READ access), the PROGRAM WRITE leg builds
directly, and issue runs Smem-read-before-program-write
with per-leg MMR-route-or-endpoint service (PROGRAM legs
bypass the DATA/IO-only adapter). The Smem AR update
publishes through the existing commit block; a direct Smem
read of an MMR address routes synchronously with no bus ID.

Out of scope: READA/WRITA (own slices), three-word
absolute/long-offset Smem, RPT stepping (PAR/DAR), timing.
