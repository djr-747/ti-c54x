### P2-W50B: dual-memory ADD/SUB MOD0 activation (planned)

This immutable `(planned)` anchor records the bounded opcode activation over
W50A. TI SPRU172C defines syntax 6 `ADD Xmem,Ymem,dst` and
`SUB Xmem,Ymem,dst` as one-word fixed-left-16 dual-memory operations. Their
full-form values/masks are `0xA000/0xFE00` and `0xA200/0xFE00`; W50B fixes
both encoded mode fields to MOD0 and therefore registers only
`ADD_DUAL_MOD0` at `0xA000/0xFECC` and `SUB_DUAL_MOD0` at
`0xA200/0xFECC`. The decode-owned
`DUAL_DESTINATION_XAR_YAR` layout exposes D bit 8, Xar bits 5:4, and Yar bits
1:0. Raw X/Y selectors `0..3` map to AR2..AR5 through the W50A address seam.
All non-MOD0 variants remain outside this slice.

SPRU131G section 4.2.1 establishes that the DB and CB WORD16 operands are
independently zero-extended when SXM is zero or sign-extended when SXM is one
into the 40-bit ALU input. Each extended value is shifted left 16 before the
shared W49A modulo-40 ADD/SUB operation. Carry and borrow are evaluated at bit
32. The fixed-16 exception in Table 4-1 makes C directional: ADD may set but
not clear C, while SUB may clear but not set C. Thus ADD stages C=1 only on a
carry and otherwise preserves C; SUB stages C=0 only on a borrow and otherwise
preserves C. OVM zero retains the modulo-40 result; OVM one saturates signed-32
overflow to `00 7FFF FFFFh` or `FF 8000 0000h`, and only the selected
destination OVA/OVB flag is preserved or set sticky.

The catalog input contract is exactly auxiliary registers plus C, SXM, and
OVM; no accumulator value or carry-in participates in the arithmetic. Checked
and trusted leaves emit one generic dual READ effect with a static completion
callback. The existing W50A resolver, X-then-Y reducer, root transaction,
ACCUMULATOR_WRITE effect, status transaction, and sequential PC retirement own
the rest of execution. No opcode-specific CPU branch, carrier widening,
allocator, environment access, I/O, formatting, observer work, public ABI,
device behavior, physical parallelism, or machine-cycle claim is added.

Executable acceptance covers all 64 restricted encodings, fixed-bit near
misses, operand extraction, checked/trusted binding, X/Y selector mapping,
SXM extension, modulo-40 arithmetic, directional C, OVM saturation, sticky
destination overflow, READY/WAIT/FAULT pair lifecycle, no-update AR behavior,
atomic PC retirement, inventory projection, and observer-off public-CPU
performance. The hosted benchmark uses four ADD/SUB by destination scenarios,
two deterministic DATA reads per retirement, fixed semantic hashes, and
same-cohort advisory comparison without an absolute nanosecond or TI-cycle
threshold. W50A carrier sizes and generic no-refetch/no-replan structure remain
frozen.

Both inventory forms remain `SUBSET` because W50B covers only MOD0/MOD0:
`FORM-T2-01-06` and `FORM-T2-02-06` bind through
`DUAL_INDIRECT_MOD0_MOD0`. The expected realized catalog is 104 rows,
`ID_COUNT=105`, 3,031 populated first words, and 104 bindings; family coverage
rises to 44/205 while strict complete-form coverage stays 27/205.
Repeatability remains `UNRESOLVED`: omission from the negative repeat tables
is not positive eligibility evidence.

W50B is implementation-complete. The catalog, decode, semantics, CPU lifecycle,
inventory, and performance contracts are all GREEN: focused CTest is 4/4, the
observer-off benchmark smoke and Python quality checks pass, and the authoritative
full local gate is recorded by the closeout ledger. The benchmark retires
320,000 instructions per scenario with exactly two DATA reads per retirement;
one hosted characterization produced medians of approximately
1,749.8–1,811.9 ns/instruction across the four ADD/SUB destination scenarios.
These values are volatile characterization only, not a release threshold or TI
timing claim, and no benchmark history artifact is created. The dual benchmark
remains a visible optimization target because its
generic two-read transaction path is materially slower than register-only
arithmetic.

The 499-line benchmark remains a cohesive fixed-roster owner below the 500-line
review trigger; any additional scenarios must be split into a separate capture
module before growth. `operand_layout.c` remains a cohesive descriptor owner
below the 800-line hard limit; future layout growth requires an explicit split.
The next forcing seam is W51 coordinated non-MOD0 dual addressing/update modes,
including same-AR Xmod precedence and atomic two-AR commit, while preserving the
trusted-path and dual-read performance constraints.
