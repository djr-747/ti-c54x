### P3-S21: OR Smem rows (planned)

This immutable anchor heads the P3-S21 evidence freeze. `OR Smem`
syntax 1 (TI SPRU172C §OR, p. 4-123: `0001_101S_IAAAAAAA`, class
3A/3B, no status effects) lands one catalog row over the shared
`SELECT_INDIRECT_ADDR7` layout: `OR_SMEM` (`0x1A00/0xFE00`,
`UNRESOLVED` repeatability, `ACCUMULATORS` inputs) with a new plain
`ti_c54x_op_or_smem` entry that extracts the 1/1/7 field triple
from the decoded instruction and delegates to the landed P3-S16
`op_or_smem_values` core, plus the landed trusted entry, both
wired through the catalog X-macro into dispatch. No layout, decode,
or CPU change; the generated first-word lookup grows by 512 entries
with zero neighbor moves. `FORM-T2-08-01` binds at `SUBSET` under
`DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`; immediate (syntaxes
2-3, already bound), accumulator-shift (syntax 4, already bound),
and `ORM` (already bound) are untouched. Projected closure adds
two evidence records, one requirement, and no new test record on
the extended atomic-logical plus binding suites: 435 evidence
records, 682 requirements, and 546 tests (546 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
