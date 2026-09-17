### P2-W63: BC pmad,cond conditional-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W63 FIT/ownership anchor.
W63 activates one Table 2-12 row under `FORM-T2-12-04`, TI SPRU172C §4
`BC[D]` (printed pp. 4-18–4-20, cross-checked against the SPRU179C
algebraic volume `if (cond(s)) goto pmad`, class 31A, §4-18, which
agrees on the opcode diagram, conditional `PC = pmad(15–0)` execution,
and the worked examples): one row `BC_PMAD` at first-word value
`0xF800` under mask `0xFF00` (bits `1111_10Z0` fixed with Z=0 plus
the 8-bit condition code in bits 7:0; Z=1 is the deferred delayed
`BCD` variant and decodes `UNKNOWN_WORD`), two program words, 256
encodings. The new `BRANCH_COND_PMAD16` layout carries the copied
condition code and the second-word pmad. Repeatability is
`INELIGIBLE` on the manual's explicit "not repeatable" note (printed
p. 4-19); input resources are the new
`ACCUMULATORS_STATUS_C_TC` combination. Semantics: if the condition
holds, `PC = pmad`; else `PC = PC+2`; no status effect for the
implemented conditions. Worked examples pin `BC 2000h,AGT` with
`A=00 0000 0053,PC=1000` to `PC=2000` and with `A=FF FFFF FFFF` to
`PC=1002`.

The slice implements the 16 image-evidenced single conditions over
1,752 of the 1,754 static uses: TC (`0011_0000`), NTC (`0010_0000`),
C (`0000_1100`), NC (`0000_1000`), and the twelve accumulator
compares AEQ/ANEQ/AGT/AGEQ/ALT/ALEQ (`0100_0101/0100/0110/0010/0011/
0111`) and BEQ/BNEQ/BGT/BGEQ/BLT/BLEQ (`0100_1101/1100/1110/1010/
1011/1111`). Accumulator compares are full-40-bit two's-complement
signed project-interpretation pins (guard bits included; TI shows no
guard-nonzero vector): EQ/NEQ/GT/GEQ/LT/LEQ against zero. BIO/NBIO
(external-pin seam), UNC, AOV/BOV/ANOV/BNOV (OVA/OVB effect seam),
multi-condition combinations, and delayed `BCD` stay deferred; their
codes reject `INVALID_OPERAND` transactionally. Knownness is demanded
only for consumed inputs (TC cond ignores unknown accumulators).
The taken path reuses the W62 `PC_WRITE` route; the not-taken path
emits effect `NONE` through the proven sequential bridge (no `run.c`
change). The form lands `SUBSET`.

Ownership: `REQ-COMMON-P2-W63-BC-PMAD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W63-BC-PMAD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W63-BC-PMAD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W63-BC-PMAD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W63-BC-PMAD-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W63-BC-PMAD-INVENTORY`
(`tools/quality/`) carry the project contract. The four RED
executable contracts are
`tests/unit/isa/catalog/test_bc_pmad_metadata.c`
(CTest `ti_c54x.bc_pmad_metadata`),
`tests/unit/isa/decode/test_bc_pmad_decode.c`
(CTest `ti_c54x.bc_pmad_decode`),
`tests/unit/isa/ops/branch/test_bc_pmad.c`
(CTest `ti_c54x.bc_pmad`), and
`tests/unit/cpu/isa/branch/test_cpu_bc_pmad.c`
(CTest `ti_c54x.cpu_bc_pmad`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule. Projected closure
is 146 catalog rows, `ID_COUNT=147`, 5,566 lookup entries, 146
trusted bindings, family coverage 54/205 (26.341%), and 33/205
strict complete-form coverage (`FORM-T2-12-04` stays `SUBSET`); 273
evidence records, 420 requirements (389 `IMPLEMENTED`, 0 `PLANNED`,
31 `UNRESOLVED`), and 344 tests (344 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
