### P2-W75: RETED delayed-interrupt-return vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W75 FIT/ownership
anchor. W75 activates one Table 2-15 row under `FORM-T2-15-05`,
TI SPRU172C §4 `RETE[D]` (printed p. 4-139): one row `RETED`
at first-word value `0xF6EB` under mask `0xFFFF` (bit 9 set
selects the delayed variant; the non-delayed `RETE` shipped
in W74), one program word, one encoding, no operands. The row
reuses the `OPERAND_LAYOUT_NONE` layout, declares
`STACK_POINTER` inputs, and is `INELIGIBLE` on the manual's
explicit "not repeatable" note. Semantics per the manual with
delayed-branch sequencing: `PC=TOS`, `SP+1`, `0→INTM` (ST1
bit 11); SP and the INTM clear commit at retire while the TOS
redirect alone defers past the two delay-slot words (the same
eager-commit/deferred-redirect split the `RETD` slice proves
for SP). The pure leaf stages a `MEMORY` `STACK_POP_DELAYED`
READ with the shared INTM-clearing completion owned once in
`op_rete_common`; the root commits it through the generic
pop-completion seam, so no `run.c` change, no new effect
kind, and no new address kind are admitted. The form stays
`SUBSET` with `RETE` and `RETED` symbols under a
`STACK_POP_DELAYED` binding shared with `RETD`. This slice
needs no new TI records: opcode (with the Z delayed bit),
semantics, and nonrepeatable note are the W74 records
`E-TI-C54X-SPRU172C-S4-RETE-OPCODE-P4-139`,
`E-TI-C54X-SPRU172C-S4-RETE-SEMANTICS-P4-139`, and
`E-TI-C54X-SPRU172C-S4-RETE-NONREPEATABLE-P4-139`.

Ownership: `REQ-COMMON-P2-W75-RETED-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W75-RETED-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W75-RETED-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W75-RETED-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W75-RETED-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the delayed INTM-clear retirement proof),
and `REQ-PROJECT-P2-W75-RETED-INVENTORY` (`tools/quality/`)
carry the project contract. The four RED executable
contracts are `tests/unit/isa/catalog/test_reted_metadata.c`
(CTest `ti_c54x.reted_metadata`),
`tests/unit/isa/decode/test_reted_decode.c`
(CTest `ti_c54x.reted_decode`),
`tests/unit/isa/ops/branch/test_reted.c`
(CTest `ti_c54x.reted`), and
`tests/unit/cpu/isa/branch/test_cpu_reted.c`
(CTest `ti_c54x.cpu_reted`). Projected closure is 158 catalog
rows, `ID_COUNT=159`, 6,853 lookup entries, 158 trusted
bindings, family coverage unchanged at 59/205 (28.780%), and
33/205 strict complete-form coverage (`FORM-T2-15-05` stays
`SUBSET`); 324 evidence records, 505 requirements (474
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 402 tests
(402 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
