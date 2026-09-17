### P2-W68: BD delayed-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W68 FIT/ownership
anchor. W68 activates one Table 2-12 row under `FORM-T2-12-01`, TI
SPRU172C §4 `B[D]` (printed p. 4-14, already pinned by the W62 TI
records): one row `BD` at first-word value `0xF273` under mask
`0xFFFF` (bits `1111_00Z0_0011_0011` with Z=1), two program
words, one encoding. The row reuses the `BRANCH_PMAD16` layout,
declares no input resources, and is `INELIGIBLE` on the manual's
explicit "not repeatable" note. The slice adds the first
non-MEMORY deferred-branch seam, mirroring the W62 combined
seam-plus-row shape: a new `TI_C54X_OP_EFFECT_DELAYED_PC_WRITE`
kind carrying one copied 16-bit pmad, a pure handler emitting it
with all other state preserved, and a `run.c` route that
validates the absolute target through the existing target
planner, commits the sequential successor, retires, and arms the
CPU-owned countdown with pmad (replacing any pending countdown;
the arming instruction consumes no slot words itself). No MEMORY
endpoint, no new address kind, no status/accumulator/T effect,
no public ABI, timing, or device claim is admitted. The form
stays `SUBSET` (`B_PMAD` keeps `ABSOLUTE_PMAD_NONDELAYED`
while the new row takes `ABSOLUTE_PMAD_DELAYED`; the delayed
conditional `BCD` waits for a later slice reusing this route
unchanged). This slice updates the W62 `test_b_pmad_decode.c`
deferred-word pin (0xF273 now decodes `BD`) under the
cross-slice hazard rule, including the bit-9 near-miss entry.

Ownership: `REQ-COMMON-P2-W68-BD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W68-BD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W68-BD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence
(reusing the W62 `B[D]` opcode, semantics, and nonrepeatable
records; no new TI record is needed);
`REQ-PROJECT-P2-W68-BD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W68-BD-CPU-RETIREMENT` (`src/cpu/`, owning
the new delayed-PC route), and
`REQ-PROJECT-P2-W68-BD-INVENTORY` (`tools/quality/`) carry the
project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_bd_metadata.c`
(CTest `ti_c54x.bd_metadata`),
`tests/unit/isa/decode/test_bd_decode.c`
(CTest `ti_c54x.bd_decode`),
`tests/unit/isa/ops/branch/test_bd.c`
(CTest `ti_c54x.bd`), and
`tests/unit/cpu/isa/branch/test_cpu_bd.c`
(CTest `ti_c54x.cpu_bd`). Projected closure is 151 catalog
rows, `ID_COUNT=152`, 5,571 lookup entries, 151 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-12-01` stays `SUBSET`); 301
evidence records, 463 requirements (432 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 374 tests (374 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
