### P2-W58: LD #lk[,SHFT],dst vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W58 FIT/ownership anchor.
W58 activates Table 2-19 form `FORM-T2-19-08`, TI SPRU172C §4 LD syntax
7 `LD #lk[,SHFT],dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67, which agrees on all ten LD opcode
diagrams): word0 `1111_000D_0010_SHFT` (value `0xF020`, mask `0xFEF0`,
destination in bit 8, unsigned 4-bit SHFT in bits 3:0 spanning 0..15,
all other first-word bits fixed, 32 encodings), word1 is the copied
raw 16-bit `lk` in range -32768..32767, two program words, a new
`ACCUMULATOR_IMMEDIATE_SHIFT4_LK16` layout carrying destination,
shift, plus `lk16`, `UNRESOLVED` repeatability (Table 2-28 lists only
`LD ARP`/`LD DP`), and `STATUS_SXM_OVM` input resources. Omitted
SHFT assembles as 0; the assembler note redirecting SHFT=0,
0≤lk≤255 to syntax 6 is assembler behavior and does not narrow
decode. Semantics is `dst = SXM-widen(lk) << SHFT` in canonical
40-bit space: SXM=0 zero-extends `lk`, SXM=1 sign-extends bit 15,
followed by logical left shift by the unsigned SHFT. Guard-bit
overflow engages OVM saturation with sticky destination overflow
following the variable-shift W53/W54 precedent; C and TC stay
preserved. Unknown SXM/OVM rejects transactionally before any
effect. No memory is touched: no AR dependency, one extension fetch
at `PC+1` with retirement at `PC+2`, no new effect kind, address
kind, CPU branch, public ABI, timing, or device/MMR claim is
admitted. The prerequisite `operand_layout` table split (sets file
vs plans/resolver file, both below 500 lines) lands first as a
behavior-preserving refactor.

Ownership: `REQ-COMMON-P2-W58-LD-LK-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W58-LD-LK-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W58-LD-LK-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W58-LD-LK-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W58-LD-LK-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W58-LD-LK-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_lk_shift_metadata.c` with CTest name
`ti_c54x.ld_lk_shift_metadata`,
`tests/unit/isa/decode/test_ld_lk_shift_decode.c` with CTest name
`ti_c54x.ld_lk_shift_decode`,
`tests/unit/isa/ops/load/test_ld_lk_shift.c` with CTest name
`ti_c54x.ld_lk_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_lk_shift.c` with CTest name
`ti_c54x.cpu_ld_lk_shift`; all four are green. The W58 closure
registry is 249 immutable evidence records, 390 requirements (359
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 324 tests (324
`PASSING`, 0 `PLANNED`). Coverage is 140 catalog rows, `ID_COUNT=141`,
4,701 lookup entries, 140 trusted bindings, family coverage 50/205
(24.390%), and 32/205 strict complete-form coverage (`FORM-T2-19-08`
is `COMPLETE`). The row is reset-runnable (SXM/OVM are reset-known,
no memory or accumulator input required).
