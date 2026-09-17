### P2-W46B: ANDM/ORM/XORM neutral MOD0–3 composition (planned)

This immutable `(planned)` heading remains the FIT/ownership anchor for W46B.
The primary TI SPRU172C instruction pages are ANDM printed p4-13, ORM p4-125,
and XORM p4-203. The implemented catalog rows are
`ANDM_SMEM_MOD0_3` base `0x6880`, `ORM_SMEM_MOD0_3` base `0x6980`, and
`XORM_SMEM_MOD0_3` base `0x6A80`, all mask `0xFFE0`, two program words,
existing `ARF_EXTENSION16_MOD` layout, no input resources or status effects,
and ineligible for repeatability.

MOD0/1/2 are admitted under the current safe boundary policy. MOD3 decodes for
metadata but the handlers reject it before endpoint issue because these forms
require a read and MOD3 is write-only. The rows reuse W46A and generic decode,
memory effects, CPU retirement, and audit-only inventory binding; all five
implementation requirements and their focused runners are green. Direct,
absolute, long-offset, CMPT1, indexed, circular, reverse, MMR/device, wrap,
and timing expansion remain deferred. The projection is three additional
rows/96 populated first-word entries: ID_COUNT=99, lookup=2,679, bindings=98;
inventory family progress is 38/205 (18.537%) while strict complete-form
coverage remains 25/205 (12.195%). The dedicated observer-off benchmark cohort
has an executable, capture, comparator, quality contract, clean baseline artifact,
and baseline-relative comparison. The baseline is
`bench/audits/p2-w46b-rmw-baseline.json` (semantic hash `3b39187c40a4fd43`);
the benchmark remains advisory and makes no TI timing claim.
