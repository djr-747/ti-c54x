### P2-W55: LD #K,dst vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W55 FIT/ownership anchor.
W55 activates Table 2-19 form `FORM-T2-19-07`, TI SPRU172C §4 LD syntax
6 `LD #K, dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume pp. 4-67–4-69): word
`1110_100D_KKKK_KKKK` (value `0xE800`, mask `0xFE00`, destination in
bit 8, unsigned 8-bit K in bits 7-0, `0 v K v 255`), one program word,
a new `ACCUMULATOR_IMMEDIATE_K8` layout, `UNRESOLVED` repeatability
(Table 2-28 lists only `LD ARP`/`LD DP`), and `NONE` input resources.
Semantics is `dst = K` zero-extended to 40 bits: TI Example 5
(`LD #248, B` with SXM=1 yields `00 0000 00F8`) proves the constant
ignores SXM, and with no SHIFT or ASM shift there is no OVM/OV effect,
so no status inputs are collected and C/TC/OVA/OVB stay preserved. No
memory is touched: no AR dependency, no fetch beyond the single word,
no new effect kind, address kind, CPU branch, public ABI, timing, or
device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W55-LD-K-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W55-LD-K-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W55-LD-K-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W55-LD-K-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W55-LD-K-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W55-LD-K-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_k_metadata.c` with CTest name
`ti_c54x.ld_k_metadata`,
`tests/unit/isa/decode/test_ld_k_decode.c` with CTest name
`ti_c54x.ld_k_decode`,
`tests/unit/isa/ops/load/test_ld_k.c` with CTest name
`ti_c54x.ld_k`, and
`tests/unit/cpu/isa/load/test_cpu_ld_k.c` with CTest name
`ti_c54x.cpu_ld_k`; all four are green. The W55 closure
registry is 234 immutable evidence records, 371 requirements (340
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 311 tests (311
`PASSING`, 0 `PLANNED`). Coverage is 137 catalog rows, `ID_COUNT=138`,
4,663 lookup entries, 137 trusted bindings, family coverage 47/205
(22.927%), and 29/205 strict complete-form coverage (`FORM-T2-19-07`
is `COMPLETE`). The row is reset-runnable (no inputs required).
