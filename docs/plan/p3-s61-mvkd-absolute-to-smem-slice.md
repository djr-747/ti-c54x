### P3-S61: MVKD absolute-to-Smem move slice (planned)

`MVKD dmad, Smem` (FORM-T2-26-05) copies the DATA word at a
16-bit-immediate address into Smem. It is the MVDK mirror
(204 occurrences) and reuses the S60 `SMEM_ABSOLUTE_PAIR`
carrier with access READ (POPD convention): the dmad leg
reads first, the Smem leg writes second.

Encoding (TI SPRU172C p. 4-113, vendored text): two words pin
`0x7000` with the standard 7-bit Smem cookie at bits 6:0
(`I` at bit 7); word 1 is the 16-bit absolute `dmad`. Two
rows: `MVKD_DIRECT` (`0x7000` under `0xFF80`, DP-relative
direct, `STATUS_DP`) and `MVKD_SMEM` (`0x7080` under `0xFF80`,
indirect `MOD0--MOD3`, `AUXILIARY_REGISTERS`). The
direction-neutral `INDIRECT_ADDR7_EXTENSION16` layout is
shared. Absolute and long-offset Smem (three-word forms) stay
undecoded. Repeatability stays UNRESOLVED per the W52A rule.

Root work: the S60 pair root already implements both leg
orders; this slice opens the READ direction in the effect
shape gate, derives the Smem-leg access from the effect
access in the pair shape validator, and adds the READ template
arm to the shape tail. No resolver, issue, or commit changes.

Out of scope: MVPD/MVDP/READA/WRITA (own slices), three-word
absolute/long-offset Smem, RPT stepping (DAR), timing.
