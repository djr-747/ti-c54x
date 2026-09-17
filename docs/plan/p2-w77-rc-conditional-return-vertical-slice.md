### P2-W77: RC conditional-return vertical slice (planned)

This immutable `(planned)` heading is the durable W77 FIT/ownership
anchor. W77 activates one Table 2-15 row under `FORM-T2-15-03`,
TI SPRU172C §4 `RC[D]` (printed pp. 4-133–4-134): one row `RC`
at first-word value `0xFC00` under mask `0xFF00` (bits
`1111_11Z0` fixed with Z=0 plus the 8-bit condition field),
one program word, 256 encodings. The `0xFC00` word keeps its
`RET` identity under the W77A single-word shadow seam (the
unconditional word behaves identically either way); the
other 255 encodings decode as `RC`. The row adds the 1-word
`BRANCH_COND8` layout, declares the
`ACCUMULATORS_STATUS_C_TC_STACK_POINTER` input combination,
and is `INELIGIBLE` on the manual's explicit "not
repeatable" note. Semantics per the manual: the shared
condition seam evaluates the sixteen image-evidenced single
conditions (BIO/NBIO, UNC, overflow, and multi-condition
combinations reject as deferred, exactly like `BC`/`CC`);
taken stages a `MEMORY` `STACK_POP` READ through the
`RET`-proven pop seam gated on known SP, while not-taken
emits `NONE` for sequential advance with no endpoint
traffic; no status-bit effects either way. No `run.c`
change, no new effect kind, and no new address kind are
admitted. The form lands `SUBSET` with the `RC` symbol under
the shared `STACK_POP_NONDELAYED` binding (delayed `RCD`
waits for W78 reusing this seam unchanged). This slice needs
three new TI records (opcode diagram, TOS/SP semantics with
worked example, nonrepeatable note).

Ownership: `REQ-COMMON-P2-W77-RC-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W77-RC-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W77-RC-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W77-RC-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W77-RC-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-pop/not-taken-sequential
retirement proof), and `REQ-PROJECT-P2-W77-RC-INVENTORY`
(`tools/quality/`) carry the project contract. The four RED
executable contracts are
`tests/unit/isa/catalog/test_rc_metadata.c`
(CTest `ti_c54x.rc_metadata`),
`tests/unit/isa/decode/test_rc_decode.c`
(CTest `ti_c54x.rc_decode`),
`tests/unit/isa/ops/branch/test_rc.c`
(CTest `ti_c54x.rc`), and
`tests/unit/cpu/isa/branch/test_cpu_rc.c`
(CTest `ti_c54x.cpu_rc`). Projected closure is 160 catalog
rows, `ID_COUNT=161`, 7,109 lookup entries, 160 trusted
bindings, family coverage 61/205 (29.756%), and 33/205
strict complete-form coverage (`FORM-T2-15-03` lands
`SUBSET`); 329 evidence records, 511 requirements (480
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 406 tests
(406 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
