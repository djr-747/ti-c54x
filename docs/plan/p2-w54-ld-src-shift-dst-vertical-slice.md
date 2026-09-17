### P2-W54: LD src[,SHIFT],dst vertical slice (planned)

This immutable `(planned)` heading is the durable W54 FIT/ownership anchor.
W54 activates Table 2-19 form `FORM-T2-19-11`, TI SPRU172C §4 LD syntax
10 `LD src[,SHIFT],dst` (printed pp. 4-66–4-68): word
`1111_01SD_010S_HIFT` (value `0xF440`, mask `0xFCE0`, source in bit 9,
destination in bit 8, five-bit signed SHIFT in bits 4-0), reusing the
existing `ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout,
`UNRESOLVED` repeatability, and `ACCUMULATORS_STATUS_SXM_OVM` resources. Semantics
is `dst = src40 << SHIFT` in canonical 40-bit space: no SXM widening
applies to the 40-bit source, but right shifts follow the general C54x
shift rule shared with the ADD core (SXM=0 logical, SXM=1 arithmetic);
OVM saturation applies on guard-vs-bit31 overflow with sticky
destination overflow, and C/TC stay preserved. The D bit is
authoritative for the writeback and OV target, so D==S encodings
naturally produce the documented OVsrc case. Table 2-28 lists only
`LD ARP`/`LD DP`, so this form stays `UNRESOLVED` repeatability. No
memory is touched: no AR dependency, no fetch beyond the single word,
no new effect kind, address kind, CPU branch, public ABI, timing, or
device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W54-LD-SRC-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W54-LD-SRC-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W54-LD-SRC-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W54-LD-SRC-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W54-LD-SRC-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W54-LD-SRC-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_src_shift_metadata.c` with CTest name
`ti_c54x.ld_src_shift_metadata`,
`tests/unit/isa/decode/test_ld_src_shift_decode.c` with CTest name
`ti_c54x.ld_src_shift_decode`,
`tests/unit/isa/ops/load/test_ld_src_shift.c` with CTest name
`ti_c54x.ld_src_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_src_shift.c` with CTest name
`ti_c54x.cpu_ld_src_shift`; all four are green. The W54 closure
registry is 229 immutable evidence records, 365 requirements (334
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 307 tests (307
`PASSING`, 0 `PLANNED`). Coverage is 136 catalog rows, `ID_COUNT=137`,
4,151 lookup entries, 136 trusted bindings, family coverage 46/205
(22.439%), and 28/205 strict complete-form coverage (`FORM-T2-19-11`
is `COMPLETE`). The row is not reset-runnable (accumulators start
unknown). W54 also corrects the resource token to
`ACCUMULATORS_STATUS_SXM_OVM`: accumulator-only inputs must be
collected or every execution rejects `UNKNOWN_INPUT`.
