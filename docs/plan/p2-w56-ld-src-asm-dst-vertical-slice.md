### P2-W56: LD src,ASM[,dst] vertical slice (planned)

This immutable `(planned)` heading is the durable W56 FIT/ownership anchor.
W56 activates Table 2-19 form `FORM-T2-19-10`, TI SPRU172C §4 LD syntax
9 `LD src, ASM[, dst]` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67): word
`1111_01SD_1000_0010` (value `0xF482`, mask `0xFCFF`, source in bit 9,
destination in bit 8; omitted dst defaults to src),
reusing the `ACCUMULATOR_SOURCE_DESTINATION` layout, `UNRESOLVED`
repeatability (Table 2-28 lists only `LD ARP`/`LD DP`), and a new
`ACCUMULATORS_STATUS_SXM_OVM_ASM` resource combination built on a new
`STATUS_ASM` input bit. Semantics is `dst = src << ASM` where the
5-bit ASM field is read signed (-16..15): SXM selects logical versus
arithmetic right shift exactly as in W54, OVM saturates on guard-bit
overflow with sticky destination overflow, and C/TC stay preserved.
No memory is touched: no AR dependency, no fetch beyond the single
word, no new effect kind, address kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W56-LD-SRC-ASM-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W56-LD-SRC-ASM-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W56-LD-SRC-ASM-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W56-LD-SRC-ASM-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W56-LD-SRC-ASM-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W56-LD-SRC-ASM-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_src_asm_metadata.c` with CTest name
`ti_c54x.ld_src_asm_metadata`,
`tests/unit/isa/decode/test_ld_src_asm_decode.c` with CTest name
`ti_c54x.ld_src_asm_decode`,
`tests/unit/isa/ops/load/test_ld_src_asm.c` with CTest name
`ti_c54x.ld_src_asm`, and
`tests/unit/cpu/isa/load/test_cpu_ld_src_asm.c` with CTest name
`ti_c54x.cpu_ld_src_asm`; all five are green (four RED contracts plus
the ASM carrier proof). The W56 closure
registry is 239 immutable evidence records, 378 requirements (347
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 316 tests (316
`PASSING`, 0 `PLANNED`). Coverage is 138 catalog rows, `ID_COUNT=139`,
4,667 lookup entries, 138 trusted bindings, family coverage 48/205
(23.415%), and 30/205 strict complete-form coverage (`FORM-T2-19-10`
is `COMPLETE`). The row is not reset-runnable (accumulators start
unknown). W56 adds the shared `STATUS_ASM` input bit (snapshot
48→56 bytes, transaction 336→344 bytes, twelve 512-byte CPU test
storages to 1024) with an independent carrier proof.
