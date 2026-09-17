### P2-W41: fixed-MOD2 LD Smem,TS,dst vertical slice (planned)

This heading is the FIT/ownership anchor for the W41 activation. TI SPRU172C
§4.66 (printed pp. 4-65–4-69) defines the one-word `LD Smem,TS,dst` form with
opcode diagram `0001_010D_IAAAAAAA`. The project composes that form with the
registered fixed-MOD2 `*ARx+` rule, producing a row with value `0x1490`, mask
`0xFEF8`, D bit 8, ARF bits 2:0, and sixteen encodings. The existing
`ACCUMULATOR_ARF` layout and a combined
`STATUS_SXM+T_REGISTER` input mask are reused. `TS` is the signed six-bit
`T[5:0]` shift count with documented encodings 00..1F = 0..31 and 30..3F =
-16..-1; raw 20..2F is outside the documented range and must reject
`INVALID_OPERAND` transactionally.

The pure effect reads one WORD16 from the old selected-AR address before
postincrement, widens according to SXM, then applies the signed TS transform to
the canonical 40-bit accumulator result. Pinned boundary vectors include
`FEDC,T=8,SXM=1 -> 0xfffffedc00`; `8000,T=0x30 -> 0` (SXM=0) or
`0xffffffffff` (SXM=1); `T=0x3f -> 0x4000`/`0xffffffc000`; and
`T=0x1f,7fff -> 0xff80000000`. The transform is modulo-40 for positive left
shifts and arithmetic-right for negative shifts of the widened signed value.
TS is distinct from `SHIFT`/`ASM`; OVM and OVdst do not apply, and C, TC, OVM,
OVA/OVB, and unrelated state other than the selected destination accumulator
remain unchanged. Repeatability remains
`UNRESOLVED`; no timing, IRQ/RPT, device/MMR, public-ABI, or opcode-specific
CPU branch claim is admitted.

Four RED requirements/tests cover catalog, decode, pure effect, and generic CPU
retirement; the combined input mask is exercised within effect/CPU coverage.
Expected post-implementation closure is 35 catalog rows, 1503 populated
first-word encodings, ID_COUNT 36, five aggregate fixed-MOD2 memory rows, six
reset-runnable rows, inventory touched 35/205 (17.073%), and strict complete
25/205 (12.195%).

Implementation is complete. Four RED contracts first failed on the absent
opcode/resource/handler and unknown CPU instruction, then pass through one
catalog-derived handler and the generic fixed-MOD2 memory transaction. T[5:0]
valid counts cover 0..31 and -16..-1 with high T bits ignored; the project-only
0x20..0x2f zone rejects before a data-bus request and also rejects completion
transactionally. The source remains cohesive at 491 lines after explicit
review, below the 500-line split trigger; further growth requires a split.
Focused strict and ASan/UBSan tests, the 207-test quality gate, independent
semantic/integration reviews, and the authoritative standalone `make test`
gate are green.
