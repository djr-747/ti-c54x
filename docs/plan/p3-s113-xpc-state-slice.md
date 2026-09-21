### P3-S113: XPC program-counter extension state (implemented; local verification green)

This immutable anchor heads the P3-S113 evidence freeze. The
far control seam needs a home for the page register before
any far row lands: XPC becomes a private 16-bit
architectural register with a dedicated scalar knownness bit
(bit 6), checked read/write accessors mirroring the BK
register seam, and reset_common initialization to 0 known
(TI SPRU131G Memory §3.2.5 p. 3-20: XPC initializes to 0 at
hardware reset; §3.3.4.11 p. 3-28: XPC carries the upper 7
bits of the extended program address, is memory-mapped at
data 001Eh, and is modified by exactly six control
instructions). XPC stays outside the MMR file (the MMR route
contract already excludes XPC 0x1E as reserved), and no ISA,
sequencer, or CPU behavior changes in this slice: the reset
knownness mask moves from 0x07 to 0x47 with the XPC value
pinned at 0. Landing adds one PROJECT correction-style record, one
PROJECT_CONTRACT requirement (project-owned storage), and
one PROJECT test record on the accessor contract suite: 511
evidence records, 848 requirements, and 702 tests
(702 `PASSING`, 0 `PLANNED`); all ISA pins hold (383
bindings, 188/205 forms, 55 strict-complete, 359 rows, 360
IDs, 44953 lookup slots). XPC and extended-page semantics
stay excluded from the common target (LEAKAGE gate), so the
far-control ISA will arrive as profile work, not common
rows.
