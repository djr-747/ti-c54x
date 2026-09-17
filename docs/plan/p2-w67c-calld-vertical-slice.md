### P2-W67C: CALLD vertical slice (planned)

This immutable `(planned)` heading is the durable W67C FIT/ownership
anchor. W67C activates one Table 2-13 row under `FORM-T2-13-02`, TI
SPRU172C §4 `CALL[D]` (printed pp. 4-27–4-28, already pinned by the
W65B TI records): one row `CALLD` at first-word value `0xF274`
under mask `0xFFFF` (bits `1111_00Z0_0111_0100` with Z=1), two
program words, one encoding. The row reuses the `BRANCH_PMAD16`
layout, declares `STACK_POINTER` input resources, and is
`INELIGIBLE` on the manual's explicit "not repeatable" note. The
handler demands known SP (`INVALID_OPERAND` otherwise) and stages a
MEMORY `STACK_PUSH_DELAYED` WRITE with a zero value placeholder;
the W67A root stages `PC+4` into TOS (the CALLD worked example
pushes `0029` for `CALLD` at `0025`), commits SP and TOS, advances
PC to the sequential successor, and reports pmad deferred. The
W67B `run.c` countdown adopts the report unchanged, so no `run.c`
change is admitted by this slice. The form stays `SUBSET` (a
`COMPLETE` form remains one production catalog row per the
inventory schema; `CALL_PMAD` keeps `ABSOLUTE_PMAD_NONDELAYED`
and `CALLD` takes `ABSOLUTE_PMAD_DELAYED`). This slice updates the
W65B `test_call_pmad_decode.c` deferred-word pin (0xF274 now
decodes `CALLD`) under the cross-slice hazard rule, including the
bit-9 near-miss entry.

Ownership: `REQ-COMMON-P2-W67C-CALLD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W67C-CALLD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W67C-CALLD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence
(reusing the W65B `CALL[D]` opcode, semantics, and nonrepeatable
records; no new TI record is needed);
`REQ-PROJECT-P2-W67C-CALLD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W67C-CALLD-CPU-RETIREMENT` (`src/cpu/`, owning
the no-change adoption of the W67B countdown wiring), and
`REQ-PROJECT-P2-W67C-CALLD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_calld_metadata.c`
(CTest `ti_c54x.calld_metadata`),
`tests/unit/isa/decode/test_calld_decode.c`
(CTest `ti_c54x.calld_decode`),
`tests/unit/isa/ops/branch/test_calld.c`
(CTest `ti_c54x.calld`), and
`tests/unit/cpu/isa/branch/test_cpu_calld.c`
(CTest `ti_c54x.cpu_calld`). Projected closure is 150 catalog
rows, `ID_COUNT=151`, 5,570 lookup entries, 150 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-13-02` stays `SUBSET`); 299
evidence records, 457 requirements (426 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 370 tests (370 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
