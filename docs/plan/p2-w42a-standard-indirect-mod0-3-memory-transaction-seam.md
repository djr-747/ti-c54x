### P2-W42A: standard-indirect MOD0–3 memory transaction seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W42A
activation. W42A is a generic sequencer/memory seam only: the catalog and all
opcode handlers remain unchanged. The existing
`SINGLE_INDIRECT_WORD16` memory effect accepts MOD0 through MOD3, with MOD3
read effects structurally rejected before address resolution or endpoint
issue. The W42B load-handler slice will select the `INVALID_OPERAND` status
before an endpoint request. The
resolver uses the existing planner and records address order as MOD0 `NONE`
(no AR commit), MOD1/MOD2 `POST`, and MOD3 `PRE` (writes only). `PRE` is
address-order metadata; it never exposes an auxiliary-register mutation before
the root transaction reaches `READY`.

The TI address-mode boundaries used to scope the seam are the existing
`E-TI-C54X-SPRU131G-S5.5.3-P5-13` and
`E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` records; they are not evidence for bus,
fault, or transaction behavior.

The root memory-instruction transaction remains atomic: `WAIT`, `FAULT`, and
cancel preserve architectural state, and `READY` commits the selected AR only
when the target AR commit and the program-counter/effect commits are all
accepted. No public ABI, machine-cycle/timing, device/MMR, CPU opcode branch,
new effect kind, new address kind, or new architectural state kind is admitted.
The three RED tests are owned by `src/isa/ops/memory`, `src/sequencer/memory`,
and `src/sequencer/instruction` respectively. Implementation is complete: each
RED contract now runs GREEN through the corresponding generic seam, covering
MOD0–MOD3 validation, address-order planning, and atomic WAIT/FAULT/cancel/
READY behavior. Focused strict and ASan/UBSan CTest execution, the 207-test
quality gate, independent semantic/integration reviews, the authoritative
standalone `make test`, the evidence checker, and repository diff validation
are green; no catalog row, opcode handler, or public ABI changed. A reviewed
retain decision keeps
`memory_instruction_transaction.c` cohesive at 523 lines as the root
transaction owner; candidate validation will be split if growth approaches
approximately 700 lines and must be split before 800 lines.

W42A is staged before W42B's opcode fan-out. W42B must make an explicit,
neutral symbol and MOD-layout decision before catalog rows or handlers are
added; its corrected candidate masks are `0xFEE0` and `0xFFE0`, not the earlier
incorrect values. The combined W42A/W42B lookup projection is 1503 → 1863
populated first words (+360 matched words), consisting of +272 legal encodings
and +88 known-but-rejected MOD3 reads. These lookup counts are planning
projections only: inventory percentages remain 35/205 (17.073%) catalog-family
progress and 25/205 (12.195%) strict complete-form coverage until complete
forms are actually bound and implemented.
