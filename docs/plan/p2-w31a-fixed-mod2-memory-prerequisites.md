### P2-W31A: fixed-MOD2 memory prerequisites (implemented; local verification green)

This immutable `(planned)` heading remains the FIT/ownership anchor for the
memory-oriented slice. P2-W31A is implemented: focused strict and ASan/UBSan
tests, two independent reviews, the full standalone local gate, and remote CI
(`34069533137`) are green. The clean same-catalog-22 performance record is
captured at `66ef384`. This work piece closes only three prerequisites; it makes
no opcode, catalog, or memory ABI claim.

The first prerequisite is canonical CPU state composition. `src/cpu/` must own
one embedded `ti_c54x_arch_state`, which is the sole owner of PC, status, AR0–AR7,
SP, DP, CPL, and CMPT. Duplicate CPU PC/status storage is removed. The private
`arch_state.pc_word` is widened to `ti_c54x_program_word_address` so the existing
32-bit public program-word carrier remains representable; the existing program-PC
bridge continues to enforce the common target's 16-bit validity and does not infer
XPC or extended pages. Initialization/reset seeds the documented common state,
then applies the configured reset carrier. Public inspection, execution, and
observer behavior must remain byte- and semantically equivalent. Accumulators, T,
execution counters, bus descriptors, and observers remain separate owners.

The second prerequisite is a private checked SXM read under
`src/state/status/`. It derives SXM from the existing ST1 representation and
returns explicit value/knownness or a checked failure. This slice intentionally
adds no write accessor: status writes remain separate until a later evidenced
instruction requires them.

The third prerequisite is generic input plumbing. `src/cpu/input/` adds a
`STATUS_SXM` input resource and copies `status_sxm` plus
`status_sxm_known` into the operation-input value. Collection is generic and
opcode-agnostic; no handler consumes SXM yet, and no opcode-specific CPU branch
is part of this work piece.

P2-W31A explicitly leaves opcode and catalog changes, the WAIT adapter and
public ABI, memory effects, shared-memory visibility, byte order, fault
visibility, AR `0xffff` wrap, repeatability, timing, and later instruction
forms to later P2-W31B+ evidence freezes. A lane that needs any of those seams
must report a GAP with its evidence, owner, tests, and hot-path consequence
rather than broaden this prerequisite.
