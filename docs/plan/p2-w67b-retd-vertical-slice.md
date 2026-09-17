### P2-W67B: RETD vertical slice (planned)

This immutable `(planned)` heading is the durable W67B FIT/ownership
anchor. W67B activates one Table 2-15 row under `FORM-T2-15-04`, TI
SPRU172C §4 `RET[D]` (printed p. 4-139): one row `RETD` at
first-word value `0xFE00` under mask `0xFFFF` (bits
`1111_11Z0_0000_0000` with Z=1), one program word, one encoding,
no operands. The row reuses the `OPERAND_LAYOUT_NONE` layout,
declares `STACK_POINTER` input resources, and is `INELIGIBLE` on
the manual's explicit "not repeatable" note. The handler demands
known SP (`INVALID_OPERAND` otherwise) and stages a MEMORY
`STACK_POP_DELAYED` READ with a zero value and null completion;
the W67A root reads TOS at old SP, commits `SP+1` atomically,
advances PC to the one-word successor, and reports TOS deferred.
The W67B `run.c` wiring adopts the report into the CPU-owned
countdown (replacing any pending branch) and, after every later
retirement that arms nothing new, either disarms on an inner
PC-discontinuity (a non-delayed stack branch or `PC_WRITE`
effect wins; TI forbids these in delay slots per SPRU131G
§6.3.1 printed p. 6-6) or consumes the retired word count,
taking the deferred branch (PC=TOS) at zero with saturating
consume for malformed fills. WAIT/FAULT/budget-exhaustion leave
the countdown untouched in the instance; reset clears it. The
form lands `SUBSET` (`RETE`/`RETF` stay `UNRESOLVED`; `CALLD`
waits for W67C). This slice updates the W66B `test_ret_decode.c`
deferred-word pin (0xFE00 now decodes `RETD`) under the
cross-slice hazard rule, including the bit-9 near-miss entry.

Ownership: `REQ-COMMON-P2-W67B-RETD-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W67B-RETD-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W67B-RETD-EFFECTS`
(`src/isa/ops/branch/`) carry the TI architectural evidence;
`REQ-PROJECT-P2-W67B-RETD-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W67B-RETD-CPU-RETIREMENT` (`src/cpu/`, owning
the run-loop adopt/advance wiring),
and `REQ-PROJECT-P2-W67B-RETD-INVENTORY` (`tools/quality/`)
carry the project contract. The four RED executable contracts
are `tests/unit/isa/catalog/test_retd_metadata.c`
(CTest `ti_c54x.retd_metadata`),
`tests/unit/isa/decode/test_retd_decode.c`
(CTest `ti_c54x.retd_decode`),
`tests/unit/isa/ops/branch/test_retd.c`
(CTest `ti_c54x.retd`), and
`tests/unit/cpu/isa/branch/test_cpu_retd.c`
(CTest `ti_c54x.cpu_retd`). Projected closure is 149 catalog
rows, `ID_COUNT=150`, 5,569 lookup entries, 149 trusted
bindings, family coverage 56/205 (27.317%), and 33/205 strict
complete-form coverage (`FORM-T2-15-04` stays `SUBSET`); 297
evidence records, 451 requirements (420 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 366 tests (366 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
