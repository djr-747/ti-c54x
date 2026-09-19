### P3-S60: MVDK Smem-to-absolute move slice (planned)

`MVDK Smem, dmad` (FORM-T2-26-02) copies a single data-memory
word to a 16-bit-immediate DATA address. It is the fourth 5110
census MMR-adjacent move (207 occurrences) and the first
Smem-to-DATA copy: both legs are DATA memory, so neither the
MMR sidecar nor the stack TOS computation applies.

Encoding (TI SPRU172C p. 4-107, vendored text): two words pin
`0x7100` with the standard 7-bit Smem cookie at bits 6:0
(`I` at bit 7); word 1 is the 16-bit absolute `dmad`. Two
rows: `MVDK_DIRECT` (`0x7100` under `0xFF80`, DP-relative
direct, `STATUS_DP`) and `MVDK_SMEM` (`0x7180` under `0xFF80`,
indirect `MOD0--MOD3`, `AUXILIARY_REGISTERS`). The
direction-neutral `INDIRECT_ADDR7_EXTENSION16` layout is
shared with `ST_LK_SMEM`. Absolute and long-offset Smem
(three-word forms) stay undecoded: extension-requiring cookies
reject in the leaf. Repeatability stays UNRESOLVED per the
W52A rule (MVDK is repeatable but Table 2-28 does not list
it).

Root design: a new `SMEM_ABSOLUTE_PAIR_WORD16` carrier stages
the Smem selectors plus `dmad` in a fresh 20-byte arm (same
footprint as the pair arms). The effect `access` is WRITE
(the completing-leg direction, PSHD convention). A new pair
root mirrors the PORT root: it synthesizes the Smem DATA-leg
spec (direct-DP or single-indirect) through the existing
single-leg resolver and plans the `dmad` leg through the
absolute resolver, then issues read-before-write with the
generic MMR-route-or-endpoint decision per leg. No SP update,
no delay, no T load. `MVKD` reuses the carrier next slice.

Out of scope: MVKD/MVPD/MVDP/READA/WRITA (own slices),
three-word absolute/long-offset Smem, RPT stepping (EAR),
timing.
