### P3-S20: AND Smem rows (implemented; local verification green)

This immutable anchor heads the P3-S20 evidence freeze. `AND Smem`
syntax 1 (TI SPRU172C §AND, p. 4-11: `0001_100S_IAAAAAAA`, class
3A/3B, no status effects) lands one catalog row over the shared
`SELECT_INDIRECT_ADDR7` layout: `AND_SMEM` (`0x1800/0xFE00`,
`UNRESOLVED` repeatability, `ACCUMULATORS` inputs) with a new plain
`ti_c54x_op_and_smem` entry that extracts the 1/1/7 field triple
from the decoded instruction and delegates to the landed P3-S15
`op_and_smem_values` core, plus the landed trusted entry, both
wired through the catalog X-macro into dispatch. No layout, decode,
or CPU change; the generated first-word lookup grows by 512 entries
with zero neighbor moves. `FORM-T2-07-01` binds at `SUBSET` under
`DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`; immediate (syntaxes
2-3, already bound), accumulator-shift (syntax 4, already bound),
and `ANDM` (already bound) are untouched. Projected closure adds
two evidence records, one requirement, and no new test record on
the extended atomic-logical plus binding suites: 433 evidence
records, 681 requirements, and 546 tests (546 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
