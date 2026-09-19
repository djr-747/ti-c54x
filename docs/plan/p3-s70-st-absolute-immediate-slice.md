### P3-S70: ST absolute-immediate slice (implemented; local verification green)

`ST #lk, *(abs)` is TI SPRU172C section ST syntax 3 with
absolute addressing (3 words: first, 16-bit absolute
address, 16-bit lk; "add 1 word for absolute
addressing", execution `lk -> Smem`, no status bits,
all vendored in-repo). The faithful loader1 cold run
stops `UNIMPLEMENTED_OPCODE` on `ST #4,*(0x803)` at
`0xF0C` (words `76F8 0803 0004`). The first word pins
I=1 with the absolute selector cookie `0x78` (the same
cookie the following `MVDP`/`MVPD` absolute words
carry); the established `ST_LK_SMEM` row owns only the
I=0 direct half, and long-offset indirect stays
deferred. Absolute operands cannot repeat (section 2
repeat rules), so the row marks `INELIGIBLE` with
evidence.

Leaf design: new `EXACT` row `ST_LK_SMEM_ABSOLUTE`
(`0x76F8`, 3 words) with a new first+abs16+lk16 layout
set (`INDIRECT_ADDR7_ABSOLUTE16_LK`), beside the
direct leaf's `op_st_lk_smem` pair (same fire-and-forget
WORD16 write effect of the copied lk, same
no-T/no-completion shape), staged through the absolute
address resolution the MVDM/MVMD rows established. New
catalog ID moves `ID_COUNT` 326 to 327 with pins; no
inventory binding (FORM evidence belongs to a census
follow-up), no MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_st_lk_smem_absolute_decode`,
`test_st_lk_smem_absolute`,
`test_st_lk_smem_absolute_metadata`,
`test_cpu_st_lk_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF0C` to the next explicit stop (`MVDP` absolute at
`0xF0F` is the predicted follower).

Out of scope: long-offset indirect, MVDP/MVPD
absolute (own slices), LD/STL/STH/ADDM absolute (own
slices as the boot reaches them), timing.
