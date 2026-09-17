### P2-W71: BANZD delayed auxiliary-register-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W71 FIT/ownership
anchor. W71 activates one Table 2-12 row under `FORM-T2-12-03`,
TI SPRU172C §4 `BANZ[D]` (printed pp. 4-16–4-17, TI records
reused from W70 so no new TI record is needed): one row `BANZD`
at first-word value `0x6E00` under mask `0xFF00` (bit 9 set
selects the delayed variant), two program words, 256 encodings.
The row reuses the W70 `BRANCH_SIND_PMAD16` layout with
`AUXILIARY_REGISTERS` inputs and `INELIGIBLE` repeatability.
Semantics per the manual: the Sind addressing side effect always
applies (`*ARx-` post-decrements even when the branch is not
taken), then taken `ARx != 0` defers pmad through the two-word
delay slots while not-taken advances sequentially at once; under
the repo CMPT=0 operating condition with explicit ARX selection
(no ARP read). The slice adds a `BANZD` leaf emitting
`BRANCH_AUXILIARY` with the delay flag set over the same
image-evidenced `*ARx`/`*ARx-` Sind subset (I=1, MOD0/MOD1;
all other Sind combinations reject `INVALID_OPERAND` as
deferred) gated on known ARx; shared Sind evaluation hoists
into a branch-Sind helper with the `BANZ` leaf refactored onto
it at zero behavior change. The W70 `run.c` auxiliary-branch
route already retires taken-delayed (sequential advance plus
countdown arm) and not-taken (sequential advance), so no
`run.c` change is admitted; W71 covers the taken-delayed arm
path the W70 slice exercised only by construction. No MEMORY
endpoint, no new layout, effect, address kind, status/
accumulator/T effect, public ABI, timing, or device claim is
admitted. The form stays `SUBSET` with `BANZ` and `BANZD`
symbols. This slice needs no new TI records.

Ownership: `REQ-COMMON-P2-W71-BANZD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W71-BANZD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W71-BANZD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W71-BANZD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W71-BANZD-CPU-RETIREMENT` (`src/cpu/`, owning
no route change but the taken-delayed retirement proof), and
`REQ-PROJECT-P2-W71-BANZD-INVENTORY` (`tools/quality/`) carry
the project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_banzd_metadata.c`
(CTest `ti_c54x.banzd_metadata`),
`tests/unit/isa/decode/test_banzd_decode.c`
(CTest `ti_c54x.banzd_decode`),
`tests/unit/isa/ops/branch/test_banzd.c`
(CTest `ti_c54x.banzd`), and
`tests/unit/cpu/isa/branch/test_cpu_banzd.c`
(CTest `ti_c54x.cpu_banzd`). Projected closure is 154 catalog
rows, `ID_COUNT=155`, 6,339 lookup entries, 154 trusted
bindings, family coverage 57/205 (27.805%), and 33/205 strict
complete-form coverage (`FORM-T2-12-03` stays `SUBSET`); 310
evidence records, 481 requirements (450 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 386 tests (386 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
