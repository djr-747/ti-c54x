### P3-S62: MVPD program-to-Smem move slice (planned)

`MVPD pmad, Smem` (FORM-T2-26-08) copies the program word at
a 16-bit-immediate address into Smem. It is the fifth 5110
census move (19 occurrences) and the first program-memory
leg: neither the DATA sidecar nor the DATA-only pair carrier
applies.

Encoding (TI SPRU172C p. 4-117, vendored text): two words pin
`0x7D00` with the standard 7-bit Smem cookie at bits 6:0
(`I` at bit 7); word 1 is the 16-bit absolute `pmad`. Two
rows: `MVPD_DIRECT` (`0x7D00` under `0xFF80`, DP-relative
direct, `STATUS_DP`) and `MVPD_SMEM` (`0x7D80` under `0xFF80`,
indirect `MOD0--MOD3`, `AUXILIARY_REGISTERS`). The
direction-neutral `INDIRECT_ADDR7_EXTENSION16` layout is
shared. The `0x78`/`0x79` words stay MACP (`S` at bit 8,
behaviorally pinned by the MACP CPU tests); `MVDP` will take
the `0x7C` neighbor next slice. Absolute and long-offset Smem
(three-word forms) stay undecoded. Repeatability stays
UNRESOLVED per the W52A rule.

Root design: a new `SMEM_PROGRAM_PAIR_WORD16` carrier stages
the Smem selectors plus `pmad` in a fresh 20-byte arm (same
footprint as the pair arms). The effect `access` names the
Smem-leg direction (PORT convention): MVPD completes WRITE.
A new pair root mirrors the S60 root: the Smem leg resolves
through the single-leg resolver (WRITE access), the PROGRAM
leg builds directly (absolute, stateless, infallible), and
issue runs program-read-before-Smem-write with per-leg
MMR-route-or-endpoint service. The Smem AR update publishes
through a commit hook beside the stack pair's. `MVDP` reuses
the carrier next slice with access READ.

Out of scope: MVDP/READA/WRITA (own slices), three-word
absolute/long-offset Smem, RPT stepping (PAR/DAR), timing.
