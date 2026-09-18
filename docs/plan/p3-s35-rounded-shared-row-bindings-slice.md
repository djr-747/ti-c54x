### P3-S35: rounded-variant shared-row bindings (implemented; local verification green)

This immutable anchor heads the P3-S35 evidence freeze. It
binds the six single-memory/register R-suffixed forms whose
encodings already ride established rows through a mode bit,
with no catalog, decode, or leaf change: `FORM-T2-03-02`
(MPYR Smem) to `MPY_SMEM` (R at bit 9 in `0x2000/0xFC00`, TI
SPRU172C section MPY[R] p. 4-101), `FORM-T2-04-05` (MACR
Smem) to `MAC_SMEM` (R at bit 9 in `0x2800/0xFC00`, section
MAC[R] p. 4-82), `FORM-T2-04-09` (MACAR Smem) to `MACA_SMEM`
(R at bit 9 in `0x3500/0xFD00`, section MACA[R] pp. 4-85),
`FORM-T2-04-10` (MACAR T) to `MACA_T` (R at bit 0 in
`0xF488/0xFCFE`, same section), `FORM-T2-04-15` (MASR Smem)
to `MAS_SMEM` (R at bit 9 in `0x2C00/0xFC00`, section MAS[R]
p. 4-94), and `FORM-T2-04-20` (MASAR T) to `MASA_T` (R at
bit 0 in `0xF48A/0xFCFE`, section MASA[R] p. 4-97). All six
leaves carry the R bit into the multiply core and all six
suites pin checked-path R=1 rounded vectors (MPY Example 4,
MACAR Examples 3/4, MAS Example 3 shapes). Each form binds
at `SUBSET` under its row's addressing token and flips
`UNRESOLVED`/`UNIMPLEMENTED` to `COMMON_CANDIDATE`/`SUBSET`.
The shared-row pattern needs a minimal schema evolution:
schema v1 demanded one binding per catalog symbol, which
forbids two forms sharing a row, so the symbol-uniqueness
rule is retired in favor of exact-pair uniqueness (COMPLETE
still demands exactly one binding; coverage still counts
distinct bound forms, so multi-form rows cannot inflate the
denominator). The dual-memory MACR/MASR Xmem,Ymem forms stay
deferred to the dual seam (Class 7, separate rows). No new
evidence, requirement, or test records: the row CATALOG,
DECODE, and EFFECTS records plus the rounding suites already
cover the behavior. Projected closure: 285 bindings, 127/205
forms (61.951%), requirements and tests unchanged at 718 and
574. These numbers are projections until the slice lands.
