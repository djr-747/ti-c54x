### P3-S89: interrupt-wake diagnosis slice (planned)

The `0xF6A` IDLE1 parks boot honest with
`STOP_IDLE`: the library owns no interrupt source,
so nothing can ever wake it. Before modeling
arrival (doorbell stages 4-5), S89 diagnoses the
wait it parks in: which mask/flag state the DSP
holds at the stop, and what the runtime-populated
vectors point at. The state view (`ti_c54x_state`)
"intentionally grows as the instruction set is
implemented" but carries no interrupt fields, and
the runner prints none of it; the program overlay
(`prog_writes=1`) may populate the zero-in-image
vector table, which only a runtime dump can show.

Scope: extend `ti_c54x_state` with `ifr_value` /
`ifr_known` plus `intm_value` / `intm_known` and
populate them in `ti_c54x_cpu_inspect` (IFR/INTM
accessors exist; S90's take consumes the same
fields). Runner: print one `irq=` line
(INTM/IFR knownness and values) at the stop and
add `--dump-prog <base> <count>` for runtime
program words (vector area, ISR). No MMR, knob,
timing, or semantic changes: IFR stays unknown
(no arrival source exists yet).

RED contracts:
`tests/unit/cpu/isa/status/test_cpu_interrupt_snapshot.c`
(reset reports IFR known-zero with INTM known-set;
`SSBX INTM` holds known-set; `RSBX INTM` reports
known-clear; IFR stays latched-clear).
Acceptance is the `0xF6A` stop reporting its wait
state plus dumped vectors/ISR decoded with the
repo's own decoder, recorded in the ledger with
the S90 seam defined.

Out of scope: interrupt arrival, take, and wake
(S90); faithful power modeling; timing.

Findings (GREEN): the `0xF6A` stop reports
`irq intm=1 ifr=0x0000` — globally masked with
nothing latched, so only NMI/reset can wake it.
Runtime vectors `0x80-0x9F` are all zero (the
single overlay write is the `0xF67` MVDP copying
boot state to `0xFF87`, invisible since the image
already holds it), so a take would fetch zeros.
Decoding the fall-through shows the wait shape is
an infinite idle-poll: `IDLE` at `0xF6A`, `B_PMAD
0x0F6A` at `0xF6B` (`0xF073` is `B`, not RPTB) —
`0xF6D` is unreachable without wake plus
redirect. Deliverables: `ti_c54x_state` grows
`ifr_value/ifr_known` + `intm_value/intm_known`
(view contract: "intentionally grows");
`ti_c54x_cpu_inspect` populates them; runner
prints `irq=` at every stop and offers
`--dump-prog <base> <count>` (overlay-aware) plus
a `prog_overlay last=` journal line.

Validation evidence: `make test` exit 0 (new
`test_cpu_interrupt_snapshot` RED-first green:
reset reports IFR known-zero with INTM known-set,
`SSBX INTM` holds set, `RSBX INTM` clears;
selftest PASS; 286 Python quality tests OK);
fresh CMake Debug `ctest` 756/756. S90 seam:
MCU-side wake (doorbell/NMI/warm-reset, deferred
stages 4-5) — take machinery only if evidence
demands, since the runtime vectors are zero.
