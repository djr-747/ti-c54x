### P3-S2: RPT activation seam (planned)

This immutable `(planned)` heading is the durable S2 FIT/ownership
anchor. S2 activates single-repeat on the W48A/W48B seam: TI
SPRU172C §4 `RPT` (printed pp. 4-146–4-147) loads `RC` with
the iteration count and repeats the following instruction
`n + 1` times with no status effects; `RPT` itself is
nonrepeatable and `RC` is inaccessible while decrementing.
Two rows land: `RPT_K` at `0xEC00` under mask `0xFF00`
(`K` in bits 7:0, one program word, `FORM-T2-16-02`) and
`RPT_LK` exact at `0xF070` (16-bit extension, two program
words, `FORM-T2-16-03`), both `INELIGIBLE` with no inputs,
reusing the `RPT_IMMEDIATE_K`/`RPT_IMMEDIATE_LK` layouts and
the `op_rpt_immediate` core behind checked and trusted
leaves. `RPT Smem` (`FORM-T2-16-01`) stays `UNIMPLEMENTED`:
its memory-source operand needs address-resolution and
read integration that this slice does not admit.

The CPU owns `ti_c54x_repeat_progress` plus the W48B
pending latch and a body-PC word (delayed-branch pattern).
The `REPEAT_START` route starts progress (`RC` load plus
cardinality), rejects `RPT` inside delay slots and nested
`RPT` while `ACTIVE`, and retires sequentially onto the
body. The predecode point, taken only while `ACTIVE` and
before extension fetch, affirms request deferral through
the gate (latched maskable requests persist; take stays
with the future vector slice and `RS`/reset preempts by
construction) and admits only `ELIGIBLE` bodies —
`UNRESOLVED` blocks and `INELIGIBLE` rejects, both as
transactional `INVALID_STATE` stops with the body
unretired and `PC` frozen. A new `repeat_single` driver
under `src/sequencer/repeat/` owns start, predecode
(arbitration plus admission), and body-retired (`HOLD`
versus `ADVANCE`) decisions; `run.c` wires start plus
predecode now and owns no repeat policy. No catalog row is
`ELIGIBLE` today, so end-to-end replay unblocks with the
first positively-evidenced `ELIGIBLE` classification plus
its retirement-tail `HOLD` call sites; the `HOLD`/`ADVANCE`
flow itself is proven at the driver seam. No vector,
take-path, NMI-source, timing, or `RPTB`/`RPTZ`/`RPTBD`
claim is admitted.
The RED executable contracts are
`tests/unit/isa/catalog/test_rpt_k_metadata.c` and
`test_rpt_lk_metadata.c`
(CTests `ti_c54x.rpt_k_metadata`, `ti_c54x.rpt_lk_metadata`),
`tests/unit/isa/decode/test_rpt_k_decode.c` and
`test_rpt_lk_decode.c`
(CTests `ti_c54x.rpt_k_decode`, `ti_c54x.rpt_lk_decode`),
`tests/unit/isa/ops/repeat/test_rpt_k.c` and
`test_rpt_lk.c`
(CTests `ti_c54x.rpt_k_ops`, `ti_c54x.rpt_lk_ops`),
`tests/unit/sequencer/repeat/test_repeat_single_flow.c`
(CTest `ti_c54x.repeat_single_flow`), and
`tests/unit/cpu/isa/repeat/test_cpu_rpt.c`
(CTest `ti_c54x.cpu_rpt`), plus the `SUBSET`
`FORM-T2-16-02`/`FORM-T2-16-03` inventory bindings.
Realized closure is 165 catalog rows, `ID_COUNT=165`,
7,877 lookup entries, family coverage 64/205 (31.220%), and
33/205 strict complete-form coverage; 341 evidence records,
537 requirements, and 426 tests (426 `PASSING`, 0
`PLANNED`). Landing also split the run.c repeat wiring into
`src/cpu/execution/repeat_activate.c` (run.c 795 lines) and
re-pinned five neighbor near-miss flips plus the W20F
no-row pins that now resolve into the `RPT` rows.
