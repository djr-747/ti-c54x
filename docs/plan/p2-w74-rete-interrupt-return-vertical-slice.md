### P2-W74: RETE interrupt-return vertical slice (planned)

This immutable `(planned)` heading is the durable W74 FIT/ownership
anchor. W74 activates one Table 2-15 row under `FORM-T2-15-05`,
TI SPRU172C §4 `RETE[D]` (printed p. 4-139): one row `RETE`
at first-word value `0xF4EB` under mask `0xFFFF` (bit 9 clear
selects the non-delayed variant; the delayed `RETED` at
`0xF6EB` stays deferred), one program word, one encoding, no
operands. The row reuses the `OPERAND_LAYOUT_NONE` layout,
declares `STACK_POINTER` inputs (the INTM write consumes no
input), and is `INELIGIBLE` on the manual's explicit "not
repeatable" note. Semantics per the manual: `PC=TOS`,
`SP+1`, `0→INTM` (ST1 bit 11), affecting INTM only; the
worked example pins `PC 01C3/SP 2001/TOS 0110` to `PC
0110/SP 2002/ST1 xCxx→x4xx`. The pure leaf stages a `MEMORY`
`STACK_POP` READ with a zero value and an INTM-clearing
completion; the root commits the completion through the
generic pop-completion seam (new: pops honor a non-null
completion exactly like loads) before the pop architecture
commit, so no `run.c` change and no new effect kind are
admitted. The form lands `SUBSET` with a lone `RETE` symbol
under a `STACK_POP_NONDELAYED` binding shared with `RET`.
This slice needs three new TI records (opcode diagram,
TOS/SP/INTM semantics with worked example, nonrepeatable
note).

Ownership: `REQ-COMMON-P2-W74-RETE-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W74-RETE-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W74-RETE-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W74-RETE-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W74-RETE-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the INTM-clear retirement proof), and
`REQ-PROJECT-P2-W74-RETE-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_rete_metadata.c`
(CTest `ti_c54x.rete_metadata`),
`tests/unit/isa/decode/test_rete_decode.c`
(CTest `ti_c54x.rete_decode`),
`tests/unit/isa/ops/branch/test_rete.c`
(CTest `ti_c54x.rete`), and
`tests/unit/cpu/isa/branch/test_cpu_rete.c`
(CTest `ti_c54x.cpu_rete`). Projected closure is 157 catalog
rows, `ID_COUNT=158`, 6,852 lookup entries, 157 trusted
bindings, family coverage 59/205 (28.780%), and 33/205
strict complete-form coverage (`FORM-T2-15-05` lands
`SUBSET`); 322 evidence records, 499 requirements (468
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 398 tests
(398 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
