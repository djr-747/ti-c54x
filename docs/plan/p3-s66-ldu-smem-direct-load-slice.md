### P3-S66: LDU Smem-direct load slice (implemented; local verification green)

`LDU Smem, dst` direct (FORM-T2-19-20) zero-extends the
DP-relative Smem word into accumulator A or B. It extends
the S34 fixed-MOD2 LDU row, which covers only the I=1
indirect form: the I=0 direct half (`0x1200-0x127F`,
`0x1300-0x137F`) is a catalog gap sitting directly below
the LDU MOD4-MOD10 fan-out.

Encoding (TI SPRU172C LDU p. 4-78, vendored text): one word
`0x1200` under `0xFE80` with the D destination bit at bit 8
(A=0/B=1), `I` pinned 0 at bit 7, and the 7-bit direct
cookie at bits 6:0. One row: `LDU_DIRECT` (256 encodings,
`SELECT_INDIRECT_ADDR7` layout, `STATUS_DP` input resources).
Execution is `(Smem)` into `dst(15:0)` with `0` into
`dst(39:16)`; SXM is neither observed nor interpreted and no
status bits are affected (S34-established). Repeatability
stays UNRESOLVED per the W52A rule.

Leaf design: new `op_ldu_direct` checked/trusted pair
mirroring `op_ldm_smem_direct` (same layout, same
`DIRECT_WORD16` read effect, same zero-extending
completion), plus the DP-knownness gate (`UNKNOWN_INPUT`)
the MMR-addressed LDM leaf does not need. The I=1 indirect
half stays decodable in the shared extractor for
transactional rejection (no new indirect rows here).

Out of scope: indirect MOD forms (covered), absolute and
long-offset Smem, RPT stepping, timing.
