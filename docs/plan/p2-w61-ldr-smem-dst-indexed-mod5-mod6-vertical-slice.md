### P2-W61: LDR Smem,dst indexed MOD5/MOD6 vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W61 FIT/ownership anchor.
W61 activates two Table 2-19 rows under `FORM-T2-19-19`, TI SPRU172C §4
LDR with AR0-indexed Smem per SPRU131G Table 5-4 (MOD5 `*ARx-AR0`
with `ARx = ARx - AR0`, MOD6 `*ARx+AR0` with `ARx = ARx + AR0`,
borrow/carry boundary rejected): `LDR_MOD5` at first-word value
`0x16A8` and `LDR_MOD6` at `0x16B0`, both under mask `0xFEF8`
with the `ACCUMULATOR_ARF_MOD4` layout (destination, ARF, 4-bit
mode), one program word, `UNRESOLVED` repeatability, and
`STATUS_SXM` resources, 16 encodings each. Semantics reuses the
W60 rounding completion (`dst = (SXM-widen(Smem) << 16) +
0x8000`, C/TC/OV preserved, no OVM input) over the existing
`INDEXED_AR0_WORD16` transaction seam with selected-AR
post-update; AR0 must be known and unknown SXM rejects
transactionally. One program word retires at `PC+1`; no new
layout, address kind, effect kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted. The form stays
`SUBSET` (MOD4/MOD7 reverse-carry and MOD8--MOD15 remain
deferred).

Ownership: `REQ-COMMON-P2-W61-LDR-INDEXED-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W61-LDR-INDEXED-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W61-LDR-INDEXED-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W61-LDR-INDEXED-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W61-LDR-INDEXED-CPU-RETIREMENT` (`src/cpu/`),
and `REQ-PROJECT-P2-W61-LDR-INDEXED-INVENTORY`
(`tools/quality/`) carry the project contract. Following the
W43B shared-family convention, the RED executable contracts
extend the shared indexed tables instead of adding new files:
`tests/unit/isa/catalog/test_indexed_standard_indirect_metadata.c`
(CTest `ti_c54x.indexed_standard_indirect_metadata`, rows 20→22),
`tests/unit/isa/decode/test_indexed_standard_indirect_decode.c`
(CTest `ti_c54x.indexed_standard_indirect_decode`),
`tests/unit/isa/ops/load/test_indexed_standard_indirect_load_effects.c`
(CTest `ti_c54x.indexed_standard_indirect_load_effects`), and
`tests/unit/cpu/memory/test_cpu_indexed_standard_indirect.c`
(CTest `ti_c54x.cpu_indexed_standard_indirect`); all four are
green. The W61 closure
registry is 263 immutable evidence records, 408 requirements (377
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 336 tests (336
`PASSING`, 0 `PLANNED`). Coverage is 144 catalog rows,
`ID_COUNT=145`, 5,309 lookup entries, 144 trusted bindings,
family coverage 52/205 (25.366%), and 33/205 strict
complete-form coverage (`FORM-T2-19-19` stays `SUBSET`; W61 adds
rows to an already-covered family, so family coverage is unchanged
from W60). The rows
are reset-runnable (SXM is reset-known, no memory or accumulator
input required).
