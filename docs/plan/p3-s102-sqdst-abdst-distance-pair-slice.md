### P3-S102: SQDST/ABDST dual-read distance pair (planned)

This immutable anchor heads the P3-S102 evidence freeze. Two
dual-data-memory single-word rows land on a new dual-XY seam:
`SQDST Xmem,Ymem` (`0xE200`/`0xFF00`, TI SPRU172C section SQDST)
and `ABDST Xmem,Ymem` (`0xE300`/`0xFF00`, TI SPRU172C section
ABDST) with a new dual-XMOD/XAR/YMOD/YAR layout (XMOD bits 7:6,
raw XAR bits 5:4, YMOD bits 3:2, raw YAR bits 1:0; both sides
map to AR2..AR5 as in the MAC-dual precedent),
`ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM` input,
and ELIGIBLE repeatability (Class 7 repeat-used pair; Table 6-18
omits both). Semantics (TI-verified examples: X=0x0055 Y=0x00AA
AR3=0x0100 AR4=0x0200 A=FF ABCD 0000 B=0 FRCT=0): A_new holds the
SXM-widened (X-Y) shifted left 16 with OVA on guard mismatch and
OVM saturation; B_new adds the distance term to 40-bit B with
OVB on 32-bit signed overflow and OVM saturation (|A_hi|,
FRCT-doubled, for ABDST with the ABS -32768 edge rule;
A_hi squared, FRCT-doubled, for SQDST); C follows the B-channel
addition carry as in the ADD-dual precedent, TC preserved. Both
writes publish through a new accumulator-write-pair effect
(S98 effect-extension precedent: one dual completion stages
both writes atomically). All four XMOD/YMOD codes stage with
MOD3 indexed/circular commits owned by the shared dual
resolver. No dispatch, sequencing, or CPU change beyond the
automatic X-macro arms and the pair-effect application. The
T2-06-15 (SQDST) and T2-06-01 (ABDST) forms flip to
`COMMON_CANDIDATE`/`COMPLETE` with one binding each under ALL.
Projected closure adds two TI section records, four
ARCHITECTURAL requirements, and four COMMON test records on the
op and CPU suites: 503 evidence records, 827 requirements, and
681 tests (681 `PASSING`, 0 `PLANNED`); 376 bindings, 181/205
forms (88.293%), 352 rows, 353 IDs, 43864 lookup slots. These
numbers are projections until the slice lands.
