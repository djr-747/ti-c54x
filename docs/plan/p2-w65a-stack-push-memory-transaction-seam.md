### P2-W65A: stack-push MEMORY transaction seam (implemented; local verification green)

This immutable `(planned)` heading is the durable W65A FIT/ownership
anchor. W65A is a generic project-contract seam only, mirroring the
W42A precedent: it adds no catalog row, opcode handler, layout, CPU
opcode branch, inventory form, or TI claim. It gives the W65B `CALL`
row a stack transaction that pushes one WORD16 through the existing
MEMORY root with commit-at-READY atomicity.

The seam adds one address kind
`TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16` with an all-zero spec
(arf zero; every mode field zero), one pure planner leaf
`src/address/stack_push.[ch]` mapping old SP to the TOS DATA WORD16
at `SP-1` with a `PRE_ACCESS` SP update to `SP-1` (the access address
is the post-update register, mirroring the MOD3 precedent), one
update target `TI_C54X_ADDRESS_UPDATE_SP` through the
intrinsic/dependency/validate/apply chain (mode-independent: no
CMPT/ARP/CPL read; unknown live SP rejects; live SP must match the
frozen snapshot), a `STACK_PUSH` resolution arm issuing the DATA
WRITE request, and root prepare/commit support: prepare stages the
live return address (bridge successor) into the frozen effect copy
(the pure handler cannot know PC; the shape check requires request
value == frozen effect value), commit writes SP and the frozen pmad
target atomically at READY with the existing WAIT/FAULT/resume
machinery unchanged. 16-bit SP arithmetic wraps mod 65536
(`SP=0` pushes at `0xFFFF` with SP after `0xFFFF`); the manual states
no stack-overflow status, so wrap is an explicit
project-interpretation pin, not a TI claim. No `run.c` change is
admitted; the existing MEMORY route retires the push.

Ownership: `REQ-PROJECT-P2-W65A-STACK-PUSH-PLANNER`
(`src/address/`), `REQ-PROJECT-P2-W65A-STACK-PUSH-VALIDATE`
(`src/sequencer/` intrinsic/dependency/validate/apply),
`REQ-PROJECT-P2-W65A-STACK-PUSH-MEMORY`
(`src/isa/ops/memory/`, `src/sequencer/memory/`), and
`REQ-PROJECT-P2-W65A-STACK-PUSH-ROOT`
(`src/sequencer/instruction/`) carry the project contract. The three
RED executable contracts are
`tests/unit/address/test_stack_push_planner.c`
(CTest `ti_c54x.stack_push_planner`),
`tests/unit/sequencer/memory/test_stack_push_resolution.c`
(CTest `ti_c54x.stack_push_resolution`), and
`tests/unit/sequencer/instruction/test_stack_push_root.c`
(CTest `ti_c54x.stack_push_root`), driven with hand-built effects and
bundles; no catalog identity is consumed. Projected closure is 277
evidence records, 425 requirements (394 `IMPLEMENTED`, 0 `PLANNED`,
31 `UNRESOLVED`), and 348 tests (348 `PASSING`, 0 `PLANNED`) with
catalog, lookup, bindings, and family coverage unchanged. These
numbers are projections until the slice lands.
