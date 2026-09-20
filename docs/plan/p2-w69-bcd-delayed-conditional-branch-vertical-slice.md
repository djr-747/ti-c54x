### P2-W69: BCD delayed-conditional-branch vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the durable W69 FIT/ownership
anchor. W69 activates one Table 2-12 row under `FORM-T2-12-04`, TI
SPRU172C §4 `BC[D]` (printed pp. 4-18–4-20, already pinned by the
W63 TI records): one row `BCD` at first-word value `0xFA00`
under mask `0xFF00` (bits `1111_10Z0` fixed with Z=1 plus the
8-bit condition code in bits 7:0), two program words, 256
encodings. The row reuses the `BRANCH_COND_PMAD16` layout,
declares the `ACCUMULATORS_STATUS_C_TC` input combination, and is
`INELIGIBLE` on the manual's explicit "not repeatable" note. The
pure handler evaluates the sixteen image-evidenced single
conditions exactly as the W63 leaf (shared branch-condition
helper, no semantic fork), emitting taken
`DELAYED_PC_WRITE` versus not-taken `NONE`; deferred codes
reject `INVALID_OPERAND` and consumed-input-only knownness is
preserved. The W68 `run.c` delayed-PC route retires taken
branches and the proven sequential bridge retires not-taken
ones, so no `run.c` change is admitted by this slice. No MEMORY
endpoint, no new effect kind, no status/accumulator/T effect, no
public ABI, timing, or device claim is admitted. The form stays
`SUBSET` (`BC_PMAD` keeps `ABSOLUTE_PMAD_COND` while the new row
takes `ABSOLUTE_PMAD_COND_DELAYED`; multi-condition combinations
stay deferred). This slice updates the W63
`test_bc_pmad_decode.c` deferred-word pin (0xFA00 now decodes
`BCD`) under the cross-slice hazard rule, including the bit-9
near-miss entry.

Ownership: `REQ-COMMON-P2-W69-BCD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W69-BCD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W69-BCD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence
(reusing the W63 `BC[D]` opcode, semantics, and nonrepeatable
records; no new TI record is needed);
`REQ-PROJECT-P2-W69-BCD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W69-BCD-CPU-RETIREMENT` (`src/cpu/`, owning
the no-change adoption of the W68 delayed-PC route), and
`REQ-PROJECT-P2-W69-BCD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_bcd_metadata.c`
(CTest `ti_c54x.bcd_metadata`),
`tests/unit/isa/decode/test_bcd_decode.c`
(CTest `ti_c54x.bcd_decode`),
`tests/unit/isa/ops/branch/test_bcd.c`
(CTest `ti_c54x.bcd`), and
`tests/unit/cpu/isa/branch/test_cpu_bcd.c`
(CTest `ti_c54x.cpu_bcd`). Projected closure is 152 catalog
rows, `ID_COUNT=153`, 5,827 lookup entries, 152 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-12-04` stays `SUBSET`); 303
evidence records, 469 requirements (438 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 378 tests (378 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
