### P2-W59: LD Xmem,SHFT,dst vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W59 FIT/ownership anchor.
W59 activates Table 2-19 form `FORM-T2-19-06`, TI SPRU172C §4 LD syntax
5 `LD Xmem, SHFT, dst` (printed pp. 4-66–4-68; cross-checked against the
SPRU179C algebraic volume p. 4-67, which agrees on all ten LD opcode
diagrams): word0 `1001_010D_XXXX_SHFT` (value `0x9400`, mask `0xFE00`,
destination in bit 8, unsigned 4-bit SHFT in bits 3:0 spanning 0..15,
one program word, 512 encodings). Per SPRU131G Figure 5-11/Table 5-6
(printed pp. 5-19–5-20) the XXXX field is Xmod in bits 7:6 plus Xar
in bits 5:4; Xar selects AR2–AR5 (Table 5-7) and Xmod selects
`*ARx`/`*ARx-`/`*ARx+`/`*ARx+0%` circular (Table 5-8). §5.5.4.4
confirms single-operand dual-format instructions expose only Xmem
with Xmod/Xar defining the mode; the Xmem read travels the D bus.
A new `ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4` layout carries destination,
Xmod, Xar, plus SHFT; repeatability is `UNRESOLVED` (Table 2-28
lists only `LD ARP`/`LD DP`); input resources are a new
`AUXILIARY_REGISTERS_STATUS_SXM_OVM` combination (no carry is read).
Semantics is `dst = SXM-widen(Xmem) << SHFT` in canonical 40-bit
space with OVM saturation and sticky destination overflow on
guard-bit overflow following the variable-shift precedent; C and TC
stay preserved. The AR update (post-access decrement/increment for
MOD1/MOD2, circular AR0/BK commit for MOD3) needs an X-only dual
resolution: the pair machinery always issues two reads and two
commits, and single-indirect translation is rejected (spurious
CMPT0/ARP0 policy, wrong MOD3). W59A therefore adds one address
kind reusing the pair planner with Y self-aliased, packaged as a
single WORD16 read through the existing single-read child — one
D-bus DATA read, X commit only. Unknown AR/SXM/OVM rejects
transactionally. One program word retires at `PC+1`; no new effect
kind, CPU branch, public ABI, timing, or device/MMR claim is
admitted beyond the single X-only address kind.

Ownership: `REQ-COMMON-P2-W59-LD-XMEM-SHIFT-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W59-LD-XMEM-SHIFT-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W59-LD-XMEM-SHIFT-EFFECTS`
(`src/isa/ops/load/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W59-LD-XMEM-SHIFT-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W59-LD-XMEM-SHIFT-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W59-LD-XMEM-SHIFT-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_ld_xmem_shift_metadata.c` with CTest name
`ti_c54x.ld_xmem_shift_metadata`,
`tests/unit/isa/decode/test_ld_xmem_shift_decode.c` with CTest name
`ti_c54x.ld_xmem_shift_decode`,
`tests/unit/isa/ops/load/test_ld_xmem_shift.c` with CTest name
`ti_c54x.ld_xmem_shift`, and
`tests/unit/cpu/isa/load/test_cpu_ld_xmem_shift.c` with CTest name
`ti_c54x.cpu_ld_xmem_shift`; all four are green. The W59 closure
registry is 255 immutable evidence records, 396 requirements (365
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 328 tests (328
`PASSING`, 0 `PLANNED`). Coverage is 141 catalog rows, `ID_COUNT=142`,
5,213 lookup entries, 141 trusted bindings, family coverage 51/205
(24.878%), and 33/205 strict complete-form coverage (`FORM-T2-19-06`
is `COMPLETE`). The row is not reset-runnable (AR contents start
unknown).
