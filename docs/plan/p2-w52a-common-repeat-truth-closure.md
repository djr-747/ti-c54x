### P2-W52A: COMMON repeat-truth closure (planned)

This immutable `(planned)` heading is the durable W52A FIT/ownership anchor.
W52A closes the five `PLANNED` COMMON repeat requirements with executable
TI-evidence-bound contracts over the existing W20D/W20E/W48A/W48B leaves; it
adds no production RPT catalog row, no CPU repeat branch, no public API, and
no timing claim. The W20F deferral stays in force: exact internal RC
decrement ordering, repeated-body PC flow/final PC, and predecode request
arbitration remain `UNRESOLVED`, and `decode_first_word(0xEC00)` and
`decode_first_word(0xF070)` stay explicitly unknown. Full first-word
value/mask metadata pins for `RPT #K` (`0xEC00`/`0xFF00`) and `RPT #lk`
(exact `0xF070`) are deferred to the later RPT activation slice that
registers production rows.

The five contracts prove the TI claims through production paths. Decode
pins the exhaustive `0xEC00`–`0xECFF` K neighborhood (bits 7:0, width 8,
one word), the exact-`0xF070` plus full-range 16-bit extension word (width
16, two words), width-mismatch rejection, and the explicit unknown-word
pins. Operation chains the op leaf into `repeat_progress_start` to prove
the exact decoded operand reaches architectural RC for both widths with no
status payload, plus transactional malformed rejection and borrowed-field
preservation. Cardinality pins the evidenced examples (`#2` to three
executions, `#0` to one, `#0xffff` to 65,536) with widen-before-add and
null rejection. The IRQ gate pins maskable-plus-NMI deferral at remaining
counts one and 65,536, literal RS/reset admission at both endpoints, full
admission at zero, and transactional invalid-kind/count rejection.
Eligibility pins the admission tri-state, sweeps all 120 registered rows to
a valid admission, spot-pins documented `INELIGIBLE` rejections and the
DLD/DST asymmetry, and audits the twenty W49B/W50B/W51C rows: none matches
a positively listed Table 2-28 nonrepeatable form, so all twenty stay
`UNRESOLVED` (blocked, never inferred eligible or ineligible).

Ownership mirrors the requirement owners: `REQ-COMMON-RPT-IMMEDIATE-DECODE`
(`src/isa/decode/`), `REQ-COMMON-RPT-IMMEDIATE-OPERATION`
(`src/isa/ops/repeat/`), `REQ-COMMON-SINGLE-REPEAT-CARDINALITY`,
`REQ-COMMON-SINGLE-REPEAT-IRQ-GATE`, and
`REQ-COMMON-SINGLE-REPEAT-BODY-ELIGIBILITY` (`src/sequencer/repeat/`) carry
the TI architectural evidence. The RED executable contracts are
`tests/unit/isa/decode/test_rpt_immediate_decode.c` with CTest name
`ti_c54x.rpt_immediate_decode_common`,
`tests/unit/isa/ops/repeat/test_rpt_immediate.c` with
CTest name `ti_c54x.rpt_immediate_operation_common`,
`tests/unit/sequencer/repeat/test_single_repeat_cardinality.c` with CTest
name `ti_c54x.repeat_cardinality_common`,
`tests/unit/sequencer/repeat/test_single_repeat_irq_gate.c` with CTest
name `ti_c54x.repeat_request_gate_common`, and
`tests/unit/sequencer/repeat/test_single_repeat_body_eligibility.c` with
CTest name `ti_c54x.repeat_body_eligibility_common`; all five are green.
The W52A closure registry is 211 immutable evidence records, 344
requirements (311 `IMPLEMENTED`, 2 `PLANNED`, 31 `UNRESOLVED`), and 292
tests (290 `PASSING`, 2 `PLANNED`). Catalog, lookup, binding, family, and
strict-complete coverage are unchanged at 120, 3,543, 120, 44/205, and
27/205. The next documented work is AR0/BK indexed/circular addressing
including MOD3 dual, then reset/MMR edges.
