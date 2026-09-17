### P2-W57: LD #lk,16,dst vertical slice (planned)

This immutable `(planned)` heading is the durable W57 FIT/ownership anchor.
W57 activates Table 2-19 form `FORM-T2-19-09`, TI SPRU172C §4 LD syntax
8 `LD #lk, 16, dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67, which agrees on all ten LD opcode
diagrams): word0 `1111_000D_0110_0010` (value `0xF062`, mask `0xFEFF`,
destination in bit 8, all other first-word bits fixed, two encodings),
word1 is the copied raw 16-bit `lk` in range -32768..32767, two program
words, a new `ACCUMULATOR_IMMEDIATE_LK16` layout carrying destination
plus `lk16`, `UNRESOLVED` repeatability (Table 2-28 lists only `LD ARP`/
`LD DP`), and `STATUS_SXM` input resources. Semantics is
`dst = SXM-widen(lk) << 16` in canonical 40-bit space: SXM=0
zero-extends `lk`, SXM=1 sign-extends bit 15, followed by logical left
shift 16. Pinned vectors mirror the fixed-16 W38 precedent: `0000` is
zero in both modes, `7fff` is `0x007fff0000`, `8000` is
`0x0080000000`/`0xff80000000`, and `ffff` is
`0x00ffff0000`/`0xffffff0000`. C, TC, and OV stay preserved with no OVM
input: the fixed-16 shift follows W38, not the variable-SHIFT
saturation path. Unknown SXM rejects transactionally before any effect.
No memory is touched: no AR dependency, one extension fetch at `PC+1`
with retirement at `PC+2` (the `PC=0xfffe` span edge rejects with no
mutation), no new effect kind, address kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W57-LD-LK16-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W57-LD-LK16-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W57-LD-LK16-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W57-LD-LK16-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W57-LD-LK16-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W57-LD-LK16-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_lk16_metadata.c` with CTest name
`ti_c54x.ld_lk16_metadata`,
`tests/unit/isa/decode/test_ld_lk16_decode.c` with CTest name
`ti_c54x.ld_lk16_decode`,
`tests/unit/isa/ops/load/test_ld_lk16.c` with CTest name
`ti_c54x.ld_lk16`, and
`tests/unit/cpu/isa/load/test_cpu_ld_lk16.c` with CTest name
`ti_c54x.cpu_ld_lk16`; all four are green. The W57 closure
registry is 244 immutable evidence records, 384 requirements (353
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 320 tests (320
`PASSING`, 0 `PLANNED`). Coverage is 139 catalog rows, `ID_COUNT=140`,
4,669 lookup entries, 139 trusted bindings, family coverage 49/205
(23.902%), and 31/205 strict complete-form coverage (`FORM-T2-19-09`
is `COMPLETE`). The row is reset-runnable (SXM is reset-known, no
memory or accumulator input required).
