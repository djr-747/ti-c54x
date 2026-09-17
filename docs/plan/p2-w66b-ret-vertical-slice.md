### P2-W66B: RET vertical slice (planned)

This immutable `(planned)` heading is the durable W66B FIT/ownership
anchor. W66B activates one Table 2-15 row under `FORM-T2-15-04`, TI
SPRU172C §4 `RET[D]` (printed p. 4-139, cross-checked against the
SPRU179C instruction list `[d]return`, "Unconditional return",
which agrees on the mnemonic family): one row `RET` at first-word
value `0xFC00` under mask `0xFFFF` (bits `1111_11Z0_0000_0000`
with Z=0; Z=1 word `0xFE00` is the deferred delayed `RETD`
variant and decodes `UNKNOWN_WORD`), one program word, one
encoding, no operands. (Both words are unclaimed by every
existing row.) The row reuses the `OPERAND_LAYOUT_NONE` layout,
declares `STACK_POINTER` input resources (consuming the W64 seam;
`collect.c` arms it generically), and is `INELIGIBLE` on the
manual's explicit "not repeatable" note plus the Table 2-28
absence boundary. The handler demands known SP
(`INVALID_OPERAND` otherwise) and stages a MEMORY `STACK_POP`
READ with a zero value and null completion; the W66A root reads
TOS at old SP and commits `SP+1` and `PC = TOS` atomically. The
worked example pins `RET` with `PC=2112,SP=0300,TOS[0300]=1000`
to `PC=1000,SP=0301`. The form lands `SUBSET` (delayed `RETD`
deferred to the delay seam; `RETE`/`RETF` stay `UNRESOLVED`).

Ownership: `REQ-COMMON-P2-W66B-RET-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W66B-RET-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W66B-RET-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W66B-RET-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W66B-RET-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W66B-RET-INVENTORY` (`tools/quality/`) carry the
project contract. The four RED executable contracts are
`tests/unit/isa/catalog/test_ret_metadata.c`
(CTest `ti_c54x.ret_metadata`),
`tests/unit/isa/decode/test_ret_decode.c`
(CTest `ti_c54x.ret_decode`),
`tests/unit/isa/ops/branch/test_ret.c`
(CTest `ti_c54x.ret`), and
`tests/unit/cpu/isa/branch/test_cpu_ret.c`
(CTest `ti_c54x.cpu_ret`). The decode contract pins Z=1 to
`UNKNOWN_WORD` under the cross-slice hazard rule (the future
RETD slice updates this test). Projected closure is 148 catalog
rows, `ID_COUNT=149`, 5,568 lookup entries, 148 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-15-04` stays `SUBSET`); 289
evidence records, 441 requirements (410 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 359 tests (359 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
