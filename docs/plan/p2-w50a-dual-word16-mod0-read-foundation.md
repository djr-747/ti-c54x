### P2-W50A: dual-WORD16 MOD0 read foundation (planned)

This immutable `(planned)` anchor records the architecture-forcing seam now
implemented and green. Six W50A requirements and six executable tests are
closed; the anchor title remains `(planned)` so its historical locator is
stable.
SPRU131G §5.5.4 (printed pp. 5-19–5-22) defines dual data-memory operations as
one-word, indirect-only forms: X memory is read on the D bus and Y memory on the
C bus. The encoded fields are `Xmod[7:6]`, `Xar[5:4]`, `Ymod[3:2]`, and
`Yar[1:0]`; selectors `00..11` choose `AR2..AR5`. MOD0 reads the old selected
AR without updating it. When both sides select one AR with differing modes,
`Xmod` controls; this foundation admits only the evidence-fit MOD0/MOD0 case.
MMR/device behavior, physical dual-bus arbitration, and machine-cycle timing
remain excluded. W50A admits only MOD0/MOD0 and makes no opcode, catalog, or
physical timing claim.

The implementation is intentionally generic and fixed-size. `src/address/`
owns the checked dual-MOD0 address plan with two copied old-AR WORD16 addresses,
selector validation, and no post-access AR candidate. The address foundation
uses `STATE_POLICY_NONE`: only selected AR2..AR5 snapshots are live dependencies,
so unknown or nonzero CMPT/ARP does not block resolution. Full dual resolution
is prepare-local; the persistent root snapshot retains only two canonical
requests. The memory effect owns a compact dual-WORD16 read arm that copies
encoded X/Y modes and AR selectors plus a static pair-completion callback;
transaction IDs and pair state stay in the generic reducer. The pair reducer
deterministically serializes X then Y over one endpoint, retains child identity
across WAIT, cancels on FAULT or failed terminal transitions, and publishes both
words only after both READY observations. This one-endpoint serializer is a
hosted deterministic serialization choice, not a TI machine-cycle or dual-bus
timing claim. The foundation is CMPT/ARP-independent, performs no AR update,
and uses generic cold pair dispatch; no CPU opcode branch was added.

The supported carrier sizes are frozen at address spec 24 bytes, memory effect
40/36 bytes (LP64/ILP32), outer op effect 48/40 bytes, dual reducer 80/72
bytes, and persistent root transaction 336/308 bytes. The common
`memory_instruction_transaction.c` remains the 521-line owner because common
WORD16 orchestration and local hot checks stay together; pair lifecycles are
split into `memory_instruction_dual.c`, capped at 477 lines. The 769-line root
integration test remains one cohesive lifecycle matrix below the 800-line
review threshold; further cases should split into focused files.

No opcode row, catalog binding, decode layout, or CPU opcode branch is activated
in W50A. The six TDD records are
`tests/unit/address/test_indirect_dual_mod0.c`,
`tests/unit/sequencer/memory/test_dual_address_resolution.c`,
`tests/unit/isa/ops/memory/test_dual_word16_memory_effect.c`,
`tests/unit/sequencer/instruction/test_dual_word16_read_transaction.c`,
`tests/unit/sequencer/instruction/test_dual_memory_instruction_transaction.c`,
and `tests/contract/quality/perf/test_dual_word16_hot_path.py`. They are green
under focused CTest 5/5 and Python hot-path 7/7; the legacy DLD/DST structural
suite is also 7/7. The performance contract shows fixed-size, allocation-free
state, no replan/refetch or host-side I/O, and observer-off structural checks
without making a TI cycle claim. The project record and six implemented
requirements/tests are
`E-PROJECT-P2-W50A-DUAL-WORD16-MOD0-FOUNDATION-FIT-OWNERSHIP-CONTRACT`,
`REQ-COMMON-P2-W50A-DUAL-WORD16-MOD0-ADDRESS-PLANNING`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-ADDRESS-RESOLUTION`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-EFFECT-CARRIER`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-PAIR-TRANSACTION`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-ROOT-INTEGRATION`,
`REQ-PROJECT-P2-W50A-DUAL-WORD16-MOD0-HOT-PATH-PERFORMANCE`, and their
corresponding `T-*` records in `config/evidence.json`.
