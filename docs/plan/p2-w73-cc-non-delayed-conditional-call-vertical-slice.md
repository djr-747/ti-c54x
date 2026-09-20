### P2-W73: CC non-delayed-conditional-call vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W73 FIT/ownership
anchor. W73 activates one Table 2-13 row under `FORM-T2-13-03`,
TI SPRU172C §4 `CC[D]` (printed pp. 4-29–4-31): one row `CC`
at first-word value `0xF900` under mask `0xFF00` (bit 9 clear
selects the non-delayed variant; the delayed `CCD` shipped in
W72), two program words, 256 encodings. The row reuses the
`BRANCH_COND_PMAD16` layout (copied 8-bit condition plus
16-bit pmad), declares the W72 `ACCUMULATORS_STATUS_C_TC`
plus `STACK_POINTER` input combination, and is `INELIGIBLE` on
the manual's explicit "not repeatable" note. Semantics per the
manual: the sixteen image-evidenced single conditions evaluate
through the shared branch-condition helper (BIO/NBIO, UNC,
overflow, and multi-condition combinations reject
`INVALID_OPERAND` as deferred); taken branches stage the
`CALL`-shaped `MEMORY` stack-push effect (the root transaction
pushes PC+2, plans the absolute pmad target, and retires
PC=pmad at once with no countdown) while not-taken branches
emit `NONE` and retire sequentially at once. Taken branches
additionally gate known SP (`INVALID_OPERAND` per the `CALL`
leaf convention); not-taken branches consume condition inputs
only. The `CALL` `MEMORY` route and the sequential bridge
retire both outcomes, so no `run.c` change is admitted. No new
layout, effect, address kind, status/accumulator/T effect,
public ABI, timing, or device claim is admitted. The form
stays `SUBSET` with `CCD` and `CC` symbols, the new row under
an `ABSOLUTE_PMAD_COND` binding (shared with `BC_PMAD`:
conditional non-delayed). This slice needs no new TI records:
opcode diagram, semantics, and nonrepeatable note are the W72
records `E-TI-C54X-SPRU172C-S4-CC-OPCODE-P4-29`,
`E-TI-C54X-SPRU172C-S4-CC-SEMANTICS-P4-29-4-31`, and
`E-TI-C54X-SPRU172C-S4-CC-NONREPEATABLE-P4-31`.

Ownership: `REQ-COMMON-P2-W73-CC-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W73-CC-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W73-CC-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W73-CC-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W73-CC-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-immediate retirement proof), and
`REQ-PROJECT-P2-W73-CC-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_cc_metadata.c`
(CTest `ti_c54x.cc_metadata`),
`tests/unit/isa/decode/test_cc_decode.c`
(CTest `ti_c54x.cc_decode`),
`tests/unit/isa/ops/branch/test_cc.c`
(CTest `ti_c54x.cc`), and
`tests/unit/cpu/isa/branch/test_cpu_cc.c`
(CTest `ti_c54x.cpu_cc`). Projected closure is 156 catalog
rows, `ID_COUNT=157`, 6,851 lookup entries, 156 trusted
bindings, family coverage unchanged at 58/205 (28.293%), and
33/205 strict complete-form coverage (`FORM-T2-13-03` stays
`SUBSET`); 317 evidence records, 493 requirements (462
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 394 tests
(394 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
