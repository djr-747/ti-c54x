### P3-S99: dual-memory rounded-variant shared-row bindings (implemented; local verification green)

This immutable anchor heads the P3-S99 evidence freeze. It
binds the two dual-memory R-suffixed forms whose encodings
already ride established rows through a mode bit, with no
catalog, decode, or leaf change: `FORM-T2-04-06` (MACR
Xmem,Ymem) to the nine `MAC_DUAL_*` rows (R rides each
`0xB0xx/0xF8CC` word through the shared dual round/source/
destination layout, TI SPRU172C section MAC[R] p. 4-82), and
`FORM-T2-04-17` (MASR Xmem,Ymem) to the nine `MAS_DUAL_*`
rows (same R select, section MAS[R] p. 4-94). Both leaves
carry the R bit into the shared dual datapath and both
suites pin checked-path R=1 rounded vectors (MAC Example 6
shape at `0xB532` through the trusted MAC dual entry, MAS
Example 3 shape at `0xBD32` through the trusted MAS dual
entry, plus CPU retirements with MOD2 post-increments).
Each form binds at `SUBSET` under its row's addressing token
(`DUAL_INDIRECT_MODx_MODy`) and flips
`UNRESOLVED`/`UNIMPLEMENTED` to `COMMON_CANDIDATE`/`SUBSET`.
This completes the S35 deferral: the dual seam has since
landed, so the Class-7 reason to wait is gone. No new
evidence, requirement, or test records: the row CATALOG,
DECODE, and EFFECTS records plus the S3t/S3u rounding suites
already cover the behavior. Projected closure: 368 bindings,
173/205 forms (84.390%), requirements and tests unchanged at
811 and 665. These numbers are projections until the slice
lands.
