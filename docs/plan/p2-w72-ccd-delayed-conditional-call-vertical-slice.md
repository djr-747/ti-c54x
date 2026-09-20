### P2-W72: CCD delayed-conditional-call vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W72 FIT/ownership
anchor. W72 activates one Table 2-13 row under `FORM-T2-13-03`,
TI SPRU172C §4 `CC[D]` (printed pp. 4-29–4-31): one row `CCD`
at first-word value `0xFB00` under mask `0xFF00` (bit 9 set
selects the delayed variant; the non-delayed `CC` stays
deferred), two program words, 256 encodings. The row reuses the
`BRANCH_COND_PMAD16` layout (copied 8-bit condition plus
16-bit pmad), declares the new `ACCUMULATORS_STATUS_C_TC`
plus `STACK_POINTER` input combination, and is `INELIGIBLE` on
the manual's explicit "not repeatable" note. Semantics per the
manual: the sixteen image-evidenced single conditions evaluate
through the shared branch-condition helper (BIO/NBIO, UNC,
overflow, and multi-condition combinations reject
`INVALID_OPERAND` as deferred); taken branches stage the
`CALLD`-shaped `MEMORY` stack-push-delayed effect (the root
transaction pushes PC+4 past the slots, plans the absolute
pmad target, and arms the CPU-owned countdown) while not-taken
branches emit `NONE` and retire sequentially at once. Taken
branches additionally gate known SP (`INVALID_OPERAND` per the
`CALLD` leaf convention); not-taken branches consume
condition inputs only. The `CALLD` `MEMORY` route and the
sequential bridge retire both outcomes, so no `run.c` change
is admitted. No new layout, effect, address kind,
status/accumulator/T effect, public ABI, timing, or device
claim is admitted. The form lands `SUBSET` with a lone `CCD`
symbol under an `ABSOLUTE_PMAD_COND_DELAYED` binding (shared
with `BCD`: conditional plus delayed). This slice needs three
new TI records (opcode diagram, delayed PC+4 push with worked
examples, nonrepeatable note).

Ownership: `REQ-COMMON-P2-W72-CCD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W72-CCD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W72-CCD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W72-CCD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W72-CCD-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-deferral retirement proof), and
`REQ-PROJECT-P2-W72-CCD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_ccd_metadata.c`
(CTest `ti_c54x.ccd_metadata`),
`tests/unit/isa/decode/test_ccd_decode.c`
(CTest `ti_c54x.ccd_decode`),
`tests/unit/isa/ops/branch/test_ccd.c`
(CTest `ti_c54x.ccd`), and
`tests/unit/cpu/isa/branch/test_cpu_ccd.c`
(CTest `ti_c54x.cpu_ccd`). Projected closure is 155 catalog
rows, `ID_COUNT=156`, 6,595 lookup entries, 155 trusted
bindings, family coverage 58/205 (28.293%), and 33/205 strict
complete-form coverage (`FORM-T2-13-03` lands `SUBSET`); 312
evidence records, 487 requirements (456 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 390 tests (390 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
