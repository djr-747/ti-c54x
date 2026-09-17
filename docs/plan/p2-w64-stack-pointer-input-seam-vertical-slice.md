### P2-W64: stack-pointer input seam vertical slice (planned)

This immutable `(planned)` heading is the durable W64 FIT/ownership anchor.
W64 adds the stack-pointer input seam that the W65 `CALL` row will
consume, mirroring the SST seam precedent (`d21b884`): one new input
resource bit `TI_C54X_INPUT_RESOURCES_STACK_POINTER`, snapshot fields
`stack_pointer_raw16` plus `stack_pointer_known`, a `collect.c` arm
reading the architectural SP through
`ti_c54x_arch_state_read_sp`, and KNOWN_MASK inclusion. The
op-input carrier ABI grows accordingly (LP64 and ILP32 static
asserts plus their stale messages are updated to the verified
sizes). No catalog row, layout, handler, effect, inventory form, or
CPU retirement change is admitted; SP commit and the stack
transaction belong to W65.

Ownership: `REQ-PROJECT-P2-W64-SP-INPUT-SEAM` (`src/cpu/input/`,
`src/isa/ops/`) carries the project contract. The RED executable
contracts are `tests/unit/cpu/input/test_collect_stack_pointer.c`
(CTest `ti_c54x.collect_stack_pointer`) plus the updated carrier
ABI asserts in
`tests/unit/cpu/input/test_collect_ovm_status.c`. Projected closure
is 273→275 evidence records, 421 requirements (390 `IMPLEMENTED`,
0 `PLANNED`, 31 `UNRESOLVED`), and 345 tests (345 `PASSING`, 0
`PLANNED`) with catalog, lookup, bindings, and family coverage
unchanged. These numbers are projections until the slice lands.
