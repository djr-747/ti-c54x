### P2-W66A: stack-pop MEMORY transaction seam (planned)

This immutable `(planned)` heading is the durable W66A FIT/ownership
anchor. W66A is a generic project-contract seam only, mirroring the
W65A precedent: it adds no catalog row, opcode handler, layout, CPU
opcode branch, inventory form, or TI claim. It gives the W66B `RET`
row a stack transaction that pops one WORD16 through the existing
MEMORY root with commit-at-READY atomicity.

The seam adds one address kind
`TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16` with an all-zero spec
(arf zero; every mode field zero), one pure planner leaf
`src/address/stack_pop.[ch]` mapping old SP to the TOS DATA WORD16
at old SP with a `POST_ACCESS` SP update to `SP+1` mod 65536 (the
access address is the pre-update register, mirroring the MOD0
post-increment shape), a `POST_ACCESS` shape through the existing
`ADDRESS_UPDATE_SP` target (the source kind stays `STACK_PUSH`:
the snapshot is identical and only the phase differs), a
`STACK_POP` resolution arm issuing the DATA READ request with no
mode read, and root prepare/commit support: prepare proves any
16-bit TOS value is plannable against the live PC carrier before
any bus issue, and commit plans the absolute target from the
transaction's own READ observation and commits SP plus the TOS
target atomically at READY. `SP=0xFFFF` pops to `SP=0x0000`; like
the push wrap this is an explicit project-interpretation pin, not
a TI claim. The READ value is never routed through the ordinary
accumulator/status completion: the handler stages `complete ==
NULL` and the pop commit consumes the observation directly. No
`run.c` change is admitted; the existing MEMORY route retires the
pop.

Ownership: `REQ-PROJECT-P2-W66A-STACK-POP-PLANNER`
(`src/address/`), `REQ-PROJECT-P2-W66A-STACK-POP-VALIDATE`
(`src/sequencer/` intrinsic POST shape),
`REQ-PROJECT-P2-W66A-STACK-POP-MEMORY`
(`src/isa/ops/memory/`, `src/sequencer/memory/`), and
`REQ-PROJECT-P2-W66A-STACK-POP-ROOT`
(`src/sequencer/instruction/`) carry the project contract. The
three RED executable contracts are
`tests/unit/address/test_stack_pop_planner.c`
(CTest `ti_c54x.stack_pop_planner`),
`tests/unit/sequencer/memory/test_stack_pop_resolution.c`
(CTest `ti_c54x.stack_pop_resolution`), and
`tests/unit/sequencer/instruction/test_stack_pop_root.c`
(CTest `ti_c54x.stack_pop_root`), driven with hand-built effects
and bundles; no catalog identity is consumed. Projected closure
is 284 evidence records, 435 requirements (404 `IMPLEMENTED`, 0
`PLANNED`, 31 `UNRESOLVED`), and 355 tests (355 `PASSING`, 0
`PLANNED`) with catalog, lookup, bindings, and family coverage
unchanged. These numbers are projections until the slice lands.
