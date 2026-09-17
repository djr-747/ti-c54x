### P2-W60: LDR Smem,dst vertical slice (planned)

This immutable `(planned)` heading is the durable W60 FIT/ownership anchor.
W60 activates Table 2-19 form `FORM-T2-19-19`, TI SPRU172C §4 LDR
syntax `LDR Smem, dst` (printed p. 4-78; cross-checked against the
SPRU179C algebraic volume p. 4-78, which agrees on opcode,
execution, and worked example): word0 `0001_011D_IAAAAAAA`
(value `0x1680`, mask `0xFEE0`, destination in bit 8, MOD0--MOD2
single-indirect Smem byte mirroring the `LD_SMEM` row, one program
word, 64 encodings with MOD3 patterns admitted at decode and
rejected transactionally at effect validation as write-only).
The existing `ACCUMULATOR_ARF_MOD` layout carries destination,
ARF, and MOD; repeatability is `UNRESOLVED` (Table 2-28 lists
only `LD ARP`/`LD DP`); input resources are `STATUS_SXM` (status
bits say SXM only: no OVM input, no saturation, no OV effect).
Semantics is `dst = (SXM-widen(Smem) << 16) + 0x8000` with bits
14:0 cleared, i.e. guard is the SXM sign extension, bits 31:16
are Smem verbatim, and bits 15:0 are always `0x8000`; C and TC
stay preserved. The checked/trusted leaves reuse the Smem common
identity/mode validation and single-indirect staging with an
LDR-specific rounding completion. Unknown SXM rejects
transactionally. One program word retires at `PC+1`; no new
layout, address kind, effect kind, CPU branch, public ABI,
timing, or device/MMR claim is admitted. MOD4--MOD15 Smem
variants stay deferred to later slices following the
W43B/W44B pattern, so `FORM-T2-19-19` lands as `SUBSET`.

Ownership: `REQ-COMMON-P2-W60-LDR-SMEM-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W60-LDR-SMEM-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W60-LDR-SMEM-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W60-LDR-SMEM-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W60-LDR-SMEM-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W60-LDR-SMEM-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ldr_smem_metadata.c` with CTest name
`ti_c54x.ldr_smem_metadata`,
`tests/unit/isa/decode/test_ldr_smem_decode.c` with CTest name
`ti_c54x.ldr_smem_decode`,
`tests/unit/isa/ops/load/test_ldr_smem.c` with CTest name
`ti_c54x.ldr_smem`, and
`tests/unit/cpu/isa/load/test_cpu_ldr_smem.c` with CTest name
`ti_c54x.cpu_ldr_smem`; all four are green. The W60 closure
registry is 260 immutable evidence records, 402 requirements (371
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 332 tests (332
`PASSING`, 0 `PLANNED`). Coverage is 142 catalog rows, `ID_COUNT=143`,
5,277 lookup entries, 142 trusted bindings, family coverage 52/205
(25.366%), and 33/205 strict complete-form coverage (`FORM-T2-19-19`
is `SUBSET`). The row is reset-runnable (SXM is reset-known,
no memory or accumulator input required).
