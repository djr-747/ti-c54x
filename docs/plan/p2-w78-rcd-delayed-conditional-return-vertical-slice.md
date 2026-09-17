### P2-W78: RCD delayed-conditional-return vertical slice (planned)

This immutable `(planned)` heading is the durable W78 FIT/ownership
anchor. W78 activates the delayed companion of W77 under the same
`FORM-T2-15-03`, TI SPRU172C §4 `RC[D]` (printed pp. 4-133–4-134):
one row `RCD` at first-word value `0xFE00` under mask `0xFF00`
(bits `1111_11Z0` with Z=1 plus the 8-bit condition field),
one program word, 256 encodings. The `0xFE00` word keeps its
`RETD` identity under the W77A single-word shadow seam; the
other 255 encodings decode as `RCD`. The row reuses the
`BRANCH_COND8` layout, declares the
`ACCUMULATORS_STATUS_C_TC_STACK_POINTER` input combination,
and is `INELIGIBLE` on the same explicit note. Semantics per
the manual: the shared condition seam evaluates the sixteen
image-evidenced single conditions (deferred codes reject as
in W77); taken stages a `MEMORY` `STACK_POP_DELAYED` READ
through the `RETD`-proven delayed-pop seam gated on known
SP, while not-taken emits `NONE`; the two delay-slot words
execute through the unchanged delayed-branch countdown and
the tested conditions are unaffected by the slots, with no
status-bit effects either way. No `run.c` change, no new
effect kind, and no new address kind are admitted. The form
stays `SUBSET` with the `RCD` symbol added under the shared
`STACK_POP_DELAYED` binding. This slice needs three new TI
records (delayed opcode diagram with Z=1, slot semantics,
nonrepeatable note).

Ownership: `REQ-COMMON-P2-W78-RCD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W78-RCD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W78-RCD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W78-RCD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W78-RCD-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the delayed taken-pop/not-taken
retirement proof), and `REQ-PROJECT-P2-W78-RCD-INVENTORY`
(`tools/quality/`) carry the project contract. The four RED
executable contracts are
`tests/unit/isa/catalog/test_rcd_metadata.c`
(CTest `ti_c54x.rcd_metadata`),
`tests/unit/isa/decode/test_rcd_decode.c`
(CTest `ti_c54x.rcd_decode`),
`tests/unit/isa/ops/branch/test_rcd.c`
(CTest `ti_c54x.rcd`), and
`tests/unit/cpu/isa/branch/test_cpu_rcd.c`
(CTest `ti_c54x.cpu_rcd`). Projected closure is 161 catalog
rows, `ID_COUNT=162`, 7,364 lookup entries, 161 trusted
bindings, family coverage unchanged at 61/205 (29.756%),
and 33/205 strict complete-form coverage
(`FORM-T2-15-03` stays `SUBSET`); 334 evidence records, 517
requirements (486 `IMPLEMENTED`, 0 `PLANNED`, 31
`UNRESOLVED`), and 410 tests (410 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
