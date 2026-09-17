### P2-W70: BANZ auxiliary-register-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W70 FIT/ownership
anchor. W70 activates one Table 2-12 row under `FORM-T2-12-03`, TI
SPRU172C §4 `BANZ[D]` (printed pp. 4-16–4-17): one row `BANZ`
at first-word value `0x6C00` under mask `0xFD00` (bits
`0110_11Z0` fixed with Z=0 plus the 8-bit Sind field), two
program words, 256 encodings. The row reuses the `BRANCH_PMAD16`
pmad half with a new 8-bit Sind operand, declares the
`AUXILIARY_REGISTERS` input combination, and is `INELIGIBLE`
on the manual's explicit "not repeatable" note. Semantics per
the manual: the Sind addressing side effect always applies
(`*ARx-` post-decrements even when the branch is not taken),
then taken `ARx != 0` selects pmad else sequential advance;
under the repo CMPT=0 operating condition with explicit ARX
selection (no ARP read). The slice adds the indirect-branch
seam, mirroring the W65A shape: a new
`TI_C54X_OP_EFFECT_BRANCH_AUXILIARY` kind carrying the AR
index, value-after, update flag, taken flag, delay flag, and
copied pmad; a pure handler evaluating taken/not-taken over the
image-evidenced `*ARx`/`*ARx-` Sind subset (I=1, MOD0/MOD1;
all other Sind combinations reject `INVALID_OPERAND` as
deferred) gated on known ARx; and a `run.c` route that
validates the absolute target, gates CMPT=0, commits the AR
write when flagged, retires PC immediate or deferred per the
taken/delay flags, and arms the CPU-owned countdown for
taken-delayed. No MEMORY endpoint, no new address kind, no
status/accumulator/T effect, no public ABI, timing, or device
claim is admitted. The form lands `SUBSET` (delayed `BANZD`
waits for W71 reusing this seam unchanged). This slice needs
three new TI records (opcode diagram, always-applied Sind side
effect with worked examples, nonrepeatable note).

Ownership: `REQ-COMMON-P2-W70-BANZ-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W70-BANZ-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W70-BANZ-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W70-BANZ-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W70-BANZ-CPU-RETIREMENT` (`src/cpu/`, owning
the new auxiliary-branch route), and
`REQ-PROJECT-P2-W70-BANZ-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_banz_metadata.c`
(CTest `ti_c54x.banz_metadata`),
`tests/unit/isa/decode/test_banz_decode.c`
(CTest `ti_c54x.banz_decode`),
`tests/unit/isa/ops/branch/test_banz.c`
(CTest `ti_c54x.banz`), and
`tests/unit/cpu/isa/branch/test_cpu_banz.c`
(CTest `ti_c54x.cpu_banz`). Projected closure is 153 catalog
rows, `ID_COUNT=154`, 6,083 lookup entries, 153 trusted
bindings, family coverage 57/205 (27.805%), and 33/205 strict
complete-form coverage (`FORM-T2-12-03` lands `SUBSET`); 318
evidence records, 475 requirements (444 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 382 tests (382 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
