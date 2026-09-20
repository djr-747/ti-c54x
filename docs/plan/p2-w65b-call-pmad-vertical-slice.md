### P2-W65B: CALL pmad vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W65B FIT/ownership
anchor. W65B activates one Table 2-13 row under `FORM-T2-13-02`, TI
SPRU172C §4 `CALL[D]` (printed pp. 4-27–4-28, cross-checked against
the SPRU179C algebraic volume `call pmad`, class 29B, §4-27, which
agrees on `SP-1 → SP`, `PC+2 → TOS`, `pmad → PC`, and the worked
example): one row `CALL_PMAD` at first-word value `0xF074` under mask
`0xFFFF` (bits `1111_00Z0_0111_0100` with Z=0; Z=1 word `0xF274` is
the deferred delayed `CALLD` variant and decodes `UNKNOWN_WORD`),
two program words, one encoding. (An early research misread of the
diagram as `0xF034` was corrected against the `re/dsp` disassembly,
which renders `f074` as `call`; `0xF074` is unclaimed by every
existing row.) The row reuses the `BRANCH_PMAD16` layout, declares
`STACK_POINTER` input resources (consuming the W64 seam; `collect.c`
arms it generically), and is `INELIGIBLE` on the manual's explicit
"not repeatable" note plus the Table 2-28 absence boundary. The
handler demands known SP (`INVALID_OPERAND` otherwise) and stages a
MEMORY `STACK_PUSH` WRITE with a zero value placeholder; the W65A
root derives the `PC+2` return address at prepare and commits SP,
TOS, and `PC = pmad` atomically. The worked example pins `CALL
3333h` with `PC=0025,SP=1111` to `TOS[1110]=0027,SP=1110,PC=3333`.
The form lands `SUBSET` (delayed `CALLD` deferred to the delay
seam).

Ownership: `REQ-COMMON-P2-W65B-CALL-PMAD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W65B-CALL-PMAD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W65B-CALL-PMAD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W65B-CALL-PMAD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W65B-CALL-PMAD-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W65B-CALL-PMAD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_call_pmad_metadata.c`
(CTest `ti_c54x.call_pmad_metadata`),
`tests/unit/isa/decode/test_call_pmad_decode.c`
(CTest `ti_c54x.call_pmad_decode`),
`tests/unit/isa/ops/branch/test_call_pmad.c`
(CTest `ti_c54x.call_pmad`), and
`tests/unit/cpu/isa/branch/test_cpu_call_pmad.c`
(CTest `ti_c54x.cpu_call_pmad`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule (the future CALLD
slice updates this test). Projected closure is 147 catalog rows,
`ID_COUNT=148`, 5,567 lookup entries, 147 trusted bindings, family
coverage 55/205 (26.829%), and 33/205 strict complete-form coverage
(`FORM-T2-13-02` stays `SUBSET`); 279 evidence records, 431
requirements (400 `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and
352 tests (352 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
