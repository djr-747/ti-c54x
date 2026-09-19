### P3-S77: MAR Smem slice (planned)

`MAR Smem` is TI SPRU172C section MAR (`0110 1101
IAAA AAAA`, modify auxiliary register, one word, no
memory access, no status effects): the Smem byte
selects one indirect address modification that
applies to its auxiliary register only. The staged
loader1 run stops `UNIMPLEMENTED_OPCODE` on it at
`0x8023` (word `6D8A`: I=1, modifier MOD1
postdecrement, AR2, so `MAR *AR2-`). The word at
`0x8024` (`0x56F8`) matches no row, confirming the
single-word length. The `BANZ` mask (`0x6C00 /
0xFF00`) never matches `0x6D8A`, so no shadow carve
is needed.

Leaf design: new `MASKED` row `MAR_SMEM`
(`0x6D00 / 0xFF00`, 1 word) with a new
`MAR_SMEM8` layout (one 8-bit Smem operand from
word 0 bits 7:0, reusing the `FRAME_K8` set
shape). The leaf gates the evidenced indirect
cookie (I=1, MOD1 postdecrement; direct and other
modifiers reject) and any AR index 0-7 as data,
requires the addressed AR known (auxiliary-register
input precedent), and stages a new `AR_WRITE` arm
(kind 13: addressed index plus frozen value-after,
mirroring the `SP_WRITE` shape) retired by a new
`ar_write` sequencer owner through a `run.c` arm
mirroring the `FRAME` route (bridge first, checked
AR writer marking known, sequential PC, no bus
traffic). No repeat evidence exists, so the row
marks `UNRESOLVED` (`FRAME` precedent, blocking
repeat admission). New catalog ID moves
`ID_COUNT` 332 to 333 with pins; `FORM-T2-18-02`
binds the row `SUBSET` with
`STANDARD_INDIRECT_MOD1`; populated first words
40013 to 40014 (first mapped word of the `0x6Dxx`
gap); no MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_mar_smem_decode`,
`test_mar_smem`, `test_mar_smem_metadata`,
`test_cpu_mar_smem`) plus the catalog-contract row
pin. Acceptance is the hosted repro advancing past
`0x8023` to the next explicit stop.

Out of scope: MAR direct and MOD0/MOD2+ modifiers
(own slices as the boot reaches them), other
`0x6Dxx` words, timing.
