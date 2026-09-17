### P2-W53: LD Smem[,SHIFT],dst vertical slice (planned)

This immutable `(planned)` heading is the durable W53 FIT/ownership anchor.
W53 activates Table 2-19 form `FORM-T2-19-05`, TI SPRU172C §4 LD syntax 4
`LD Smem[,SHIFT],dst` (printed pp. 4-66–4-67): word0
`0110_1111_IAAAAAAA` (value `0x6F80`, mask `0xFFE0`, mode bits [4:3], ARF
bits [2:0]) with word1 `0000_110D_010S_HIFT` (D bit 8, five-bit signed
SHIFT in bits [4:0], `-16 v SHIFT v 15`). Semantics is
`dst = SXM-widened Smem << SHIFT` in canonical 40-bit space: SXM selects
sign/zero extension, OVM saturation applies on overflow, and OVdst is
affected; C, TC, and unrelated state stay unchanged. Loads admit
MOD0/MOD1/MOD2 through the W42A seam and reject MOD3 `INVALID_OPERAND`
before any MEMORY effect, following the W42B precedent; Table 2-28 lists
only `LD ARP`/`LD DP`, so this form stays `UNRESOLVED` repeatability. A
dedicated `D/ARF/MOD/SHIFT5` operand layout carries the decoded fields;
handlers never parse raw instruction words. No new effect kind, address
kind, CPU branch, public ABI, timing, or device/MMR claim is admitted.

Ownership: `REQ-COMMON-P2-W53-LD-SMEM-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W53-LD-SMEM-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W53-LD-SMEM-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W53-LD-SMEM-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W53-LD-SMEM-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W53-LD-SMEM-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_smem_shift_metadata.c` with CTest name
`ti_c54x.ld_smem_shift_metadata`,
`tests/unit/isa/decode/test_ld_smem_shift_decode.c` with CTest name
`ti_c54x.ld_smem_shift_decode`,
`tests/unit/isa/ops/load/test_ld_smem_shift.c` with CTest name
`ti_c54x.ld_smem_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_smem_shift.c` with CTest name
`ti_c54x.cpu_ld_smem_shift`; all four are green. The W53 closure
registry is 224 immutable evidence records, 359 requirements (328
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 303 tests (303
`PASSING`, 0 `PLANNED`). Coverage is 135 catalog rows, `ID_COUNT=136`,
4,023 lookup entries, 135 trusted bindings, family coverage 45/205
(21.951%), and unchanged 27/205 strict complete-form coverage. The row
is not reset-runnable (Smem reads need a known AR).
