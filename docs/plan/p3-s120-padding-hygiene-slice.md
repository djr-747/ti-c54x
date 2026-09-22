# P3-S120: Release -O3 padding-hygiene slice

### P3-S120: padding-safe architectural comparisons in CPU retirement tests (implemented; local verification green)

Scope: the 22 cmake-Release (`-O3`) ctest failures that compare whole
`ti_c54x_arch_state` structs with `memcmp` (21 branch/take CPU
retirement proofs plus `repeat_pending_request`). Under `-O3` the
optimizer leaves struct padding stale across the commit path, so
byte comparison fails while every architectural field matches;
Debug and sanitizer lanes are green. TI vectors: none — this slice
changes no behavior and stages no opcode, operand, or sequencing
semantics; the oracle is the already-green Debug/sanitizer result
for the same binaries.

Approach: add one shared padding-safe comparator in
`tests/support/` that checks every `arch_state` field
individually (program word, IFR/RC/BK/SP/RTN/XPC, AR0–AR7,
status words and masks, MMR file, SWWSR, CLKMD, knownness
masks) and swap the 22 failing `memcmp` sites to it. No
production source changes; fault-path `memcmp` sites that compare
byte-identical state stay as-is.

Gates: `make test` exit 0, sanitizer-lane ctest 847/847, Release
ctest 847/847, hosted `--selftest` PASS, evidence registry PASS.
No evidence registry changes (no new tests, requirements, or
behavior); the ledger entry records the Release count.
