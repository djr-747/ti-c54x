### P3-S12: STH by-SHFT Xmem store slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. `STH` syntax 3 (`STH src, SHFT, Xmem`, TI SPRU172C §STH,
printed pp. 4-168–4-171, opcode `1001_101S_XXXXSHFT` with S at bit 8
selecting accumulator A (`0`) or B (`1`)) lands one row:
`STH_SHFT_XMEM` at `0x9A00` under mask `0xFE00`, reusing the shared
`ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4` layout (Xmod in bits 7–6, raw Xar
in bits 5–4 mapping to AR2–AR5, unsigned SHFT in bits 3–0).
Checked and trusted leaves shift the S-selected accumulator
arithmetically right by `(16-SHFT)` and stage dual-indirect X-only
MEMORY WRITE effects carrying bits 31–16 of the shifted value — every
encoded shift is a right shift of at most 16, so the stored field is
pure source bits, SXM rides hostile and unconsumed, and the new
`ACCUMULATORS_AUXILIARY_REGISTERS` token claims accumulators plus
auxiliary registers alone. The row lands `UNRESOLVED` repeatability
on the absent TI repeatability note (`LD_XMEM_SHIFT` precedent). The
X-only address seam gains its first WRITE arm (direction-independent
dual planning; the full-dual resolver stays read-only). CPU
retirement reuses the generic MEMORY route with no `run.c` change.
The RED executable contracts pin the TI worked example (`STH B,-8`,
encoded SHFT 8, with B FF_8421_1234 storing FF84) at the leaf and
shifted high-word publication with Xmod AR updates, WAIT resume,
fault, and boundary transactionality on the CPU. `FORM-T2-20-07`
lands `COMPLETE` with its single symbol under `ALL` (the
`LD_XMEM_SHIFT` token, reused: one row covers the whole syntax).
Syntax 4 and `STLM` stay for later slices. No knob is added.
Projected closure adds two TI source records, one activation record,
three requirements, and two test records: 415 evidence records, 667
requirements, and 537 tests (537 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
