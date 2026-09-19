### P3-S69: NOP repeat-eligibility slice (implemented; local verification green)

`NOP` (`0xF495`) is absent from the Table 2-28
nonrepeatable list (TI SPRU172C §2, vendored in-repo),
so it is repeatable: the faithful loader1 cold run's
`RPT #0x140` + `NOP` delay loop at `0xF09` is legal,
but the core stops `INVALID_STATE` on the first body
because the catalog row is `REPEATABILITY_UNRESOLVED`
and the eligibility gate blocks unresolved bodies. The
W52A rule resolves rows from Table 2-28 evidence, and
`BITF`/`CMPM`/`BITT` precedent shows the `ELIGIBLE`
marking in the row.

Leaf design: one-field catalog change (`NOP` row
`UNRESOLVED` → `ELIGIBLE`); no sequencer, eligibility,
planner, knob, or timing changes. Sibling updates:
`test_opcode_metadata` NOP pin, and the two
`test_cpu_rpt` cases that use NOP as the UNRESOLVED
example body (`check_unresolved_body_blocks`,
`check_latched_request_persists`) move to the still
`UNRESOLVED` `STM` body with identical block semantics.

RED contracts: `tests/unit/isa/catalog/test_opcode_metadata.c`
(NOP pin `ELIGIBLE`) and `tests/unit/cpu/isa/repeat/test_cpu_rpt.c`
(`check_eligible_nop_body_retires`: `RPT #2` + NOP over
a four-instruction budget retires 4 with RC 0 and phase
COMPLETE). Acceptance is the hosted repro retiring the
`0xF09` delay loop and advancing past `0xF0B`.

Out of scope: other UNRESOLVED rows (resolved only with
their own Table 2-28 evidence), RPTB/RPTBD block
repeat, repeat timing.
