### P2-W62: B pmad unconditional-branch vertical slice (planned)

This immutable `(planned)` heading is the durable W62 FIT/ownership anchor.
W62 activates one Table 2-12 row under `FORM-T2-12-01`, TI SPRU172C §4
`B[D]` (printed p. 4-14, cross-checked against the SPRU179C algebraic
volume `goto pmad`, class 29A, §4-14, which agrees on opcode diagram,
`PC = pmad(15–0)` execution, and the worked example): one row `B_PMAD`
at first-word value `0xF073` under mask `0xFFFF` (bits
`1111_00Z0_0011_0011` with Z=0; Z=1 is the deferred delayed `BD`
variant and decodes `UNKNOWN_WORD`), two program words, one encoding.
The new `BRANCH_PMAD16` layout carries the copied 16-bit pmad from
the second bundle word and no first-word operands. Repeatability is
`INELIGIBLE` on the manual's explicit "not repeatable" note (printed
p. 4-14) plus the Table 2-28 absence boundary; input resources are
`NONE` (status-bits note says none: no input, no saturation, no
status effect). Semantics is `PC = pmad` with all other state
preserved; the worked example pins `B 2000h` with `PC=1F45` to
`PC=2000`.

The slice adds the first PC-target seam: a new
`TI_C54X_OP_EFFECT_PC_WRITE` kind carrying one copied `uint16_t`
pmad, a `src/isa/ops/branch/op_b_pmad.[ch]` pure checked/trusted
leaf (per-row identity validation, transactional output discipline),
a `src/sequencer/program_pc_target.[ch]` planner that validates and
stages the target in the bridge's transactional style, and a `run.c`
route that commits the target atomically with the staged effect
transaction instead of the sequential bridge plan. No MEMORY
endpoint, no new address kind, no status/accumulator/T effect, no
public ABI, timing, device/MMR, or delay-slot claim is admitted. The
form lands `SUBSET` (delayed `BD` deferred to the delay seam).

Ownership: `REQ-COMMON-P2-W62-B-PMAD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W62-B-PMAD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W62-B-PMAD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W62-B-PMAD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W62-B-PMAD-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W62-B-PMAD-INVENTORY` (`tools/quality/`) carry the
project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_b_pmad_metadata.c`
(CTest `ti_c54x.b_pmad_metadata`),
`tests/unit/isa/decode/test_b_pmad_decode.c`
(CTest `ti_c54x.b_pmad_decode`),
`tests/unit/isa/ops/branch/test_b_pmad.c`
(CTest `ti_c54x.b_pmad`), and
`tests/unit/cpu/isa/branch/test_cpu_b_pmad.c`
(CTest `ti_c54x.cpu_b_pmad`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule (the future BD
slice updates this test). Projected closure is 145 catalog rows,
`ID_COUNT=146`, 5,310 lookup entries, 145 trusted bindings, family
coverage 53/205 (25.854%), and 33/205 strict complete-form coverage
(`FORM-T2-12-01` stays `SUBSET`); 268 evidence records, 414
requirements (383 `IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and
340 tests (340 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
