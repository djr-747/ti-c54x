### P3-S36: BITF/CMPM/BITT/ST single-memory rows (implemented; local verification green)

Four single-memory rows land on the `INDIRECT_ADDR7_EXTENSION16`
layout seam added in the S36 layout commit (`ca97e35`; the
single-word BITT row rides the extension-less `INDIRECT_ADDR7`
variant):
`FORM-T2-11-02` (BITF Smem,#lk — `0x6100/0xFF00`,
TI SPRU172C section BITF p. 4-22), `FORM-T2-11-04` (CMPM
Smem,#lk — `0x6000/0xFF00`, section CMPM p. 4-33),
`FORM-T2-11-03` (BITT Smem — `0x3400/0xFF00`,
section BITT p. 4-23), and `FORM-T2-20-03` (ST #lk,Smem —
`0x7600/0xFF80` direct-only, section ST p. 4-166). The
`0xFF00` masks admit both direct (I=0) and indirect (I=1)
`Smem` syntaxes; each leaf extracts `(indirect, addr7, lk)`
through the shared layout set and serves direct page-0 plus
standard-indirect MOD0--MOD3, deferring exotic modes as
`INVALID_OPERAND`. BITF stages `TC = ((Smem & lk) != 0)` with
the copied `#lk` extension word and no T traffic; CMPM stages
`TC = (Smem == lk)` the same way; single-word BITT stages
`TC = Smem(15 - T(3:0))` gated on a known T register with no
T load. All three are `ELIGIBLE` (Table 6-18 omits every BIT
form and CMPM) and write only ST0-TC. ST `#lk`,Smem is the
direct-only carve-out under the S1 `ST_IMMEDIATE_SMEM` row
(`0x7680/0xFFE0`): the new row takes the 128 direct-dmad
encodings (`0x7600`--`0x767F`, I fixed at 0) staging a
fire-and-forget `lk -> Smem` word write with no status
effects, while I=1 words keep composing through the
established `ST_IMMEDIATE_SMEM` row. Projected closure from the S35
freeze (279 rows, 285 bindings, 29,437 lookup entries,
127/205 forms at 61.951%, 718 requirements, 574 tests):
283 rows, 289 bindings, 30,333 lookup entries (+896 pure),
131/205 forms at 63.902% SUBSET-or-better, 730 requirements,
586 tests. These numbers are projections until the slice
lands.
