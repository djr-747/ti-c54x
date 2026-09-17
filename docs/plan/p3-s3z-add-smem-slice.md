### P3-S3z: ADD Smem slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `ADD Smem` syntaxes 1–3 (TI SPRU179C §ADD,
opcodes `0000_000S_IAAAAAAA`,
`0000_010S_IAAAAAAA`, and
`0011_11SD_IAAAAAAA`, pp. 4-4–4-5) land
three rows: `ADD_SMEM` at `0x0000` under
mask `0xFE00` (`src += Smem`, S rides
bit 8), `ADD_SMEM_TS` at `0x0400` under
mask `0xFE00` (`src += Smem<<TS` with the
T-register shift), and `ADD_SMEM_SH16`
at `0x3C00` under mask `0xFC00` through
the shared arithmetic core with
SXM extension, OVM saturation with
sticky OVdst, and per-syntax C rules
(syntaxes 1–2 write C from the bit-32
carry like the established ADD leaves;
syntax 3 sets C on carry and otherwise
preserves it per the explicit TI rule).
Syntaxes 1–2 share a new
`SELECT_INDIRECT_ADDR7` layout (S@8,
I@7, addr@6:0); syntax 3 takes a new
`SOURCE_DESTINATION_INDIRECT_ADDR7`
layout (S@9, D@8, I@7, addr@6:0). All
three rows land `UNRESOLVED`
repeatability: Class 3A/3B membership is
not Table 2-28 authorization (P2-W48C
precedent). The RED executable contracts
pin derived vectors end to end (no TI
worked example exists for syntaxes 1–3;
each expectation is independently
recomputed): unshifted and TS-shifted
adds with SXM 0/1 extension and carry
cases at the leaf, through the trusted
entry points, and retired on the CPU
(direct plus MOD2 indirect, read-phase
behavior, T preservation where the
syntax takes no T shift). `FORM-T2-01-01`
/`02`/`03` land `SUBSET` with the three
symbols. Projected closure adds
three catalog rows, three inventory
bindings, one TI source record, one
activation record, two requirements, and
two test records: 390 evidence records,
641 requirements, and 514 tests (514
`PASSING`, 0 `PLANNED`). These numbers
are projections until the slice lands.
