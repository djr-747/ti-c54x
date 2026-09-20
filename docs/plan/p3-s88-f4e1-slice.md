### P3-S88: IDLE slice (implemented; local verification green)

`IDLE K` is TI SPRU172C power-down (functional
overview §1.2.17: IDLE1 shuts the CPU, IDLE2 adds
peripherals, IDLE3 adds the PLL; dormant until
interrupt), 1 word: `1111 01NN 1110 0001` with
NN+1 = K. The peer run stops `UNIMPLEMENTED_OPCODE`
on it at `0xF6A` (`0xF4E1`, K=1). Image census
shows only K=1,2,3 (never NN=3); the reserved
selector rejects in the leaf. No existing row
covers it (nearest misses: ADD/SUB/LD/ST src,ASM
`0xF480/0xFCFF`, CMPR `0xF4A8/0xFCF8`, RSBX
`0xF4B0/0xFDF0`).

Leaf design: new `MASKED` row `IDLE`
(`0xF4E1/0xFCFF`, 1 word) with a new 1-operand
`IDLE_K` layout (K at bits 9-8), `INELIGIBLE`,
resources NONE. The leaf gates K=1..3 and stages
a new `IDLE` effect carrying K. The run loop
takes a new `IDLE` arm to the reserved
`STOP_IDLE`: with no interrupt source in the
library, retiring through an interrupt-wait would
invent control flow the hardware never takes, so
the wait surfaces as an honest stop (attempted,
unretired, pc held). New catalog ID moves
`ID_COUNT` 342 to 343 with pins;
`FORM-T2-18-01` binds the row `SUBSET` with
`IMMEDIATE_K` (RPT_K precedent); populated first
words move 40274 to 40278 (four new NN pins); no
MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_idle_decode`, `test_idle_ops`,
`test_idle_metadata`, `test_cpu_idle`) plus the
catalog-contract row pin. Acceptance is the peer
repro stopping `STOP_IDLE` at `0xF6A`
(`attempted=394725 retired=394724`), targeted and
full run.

Out of scope: interrupt arrival and wake (own
seam — doorbell stages 4-5), K=4 reserved
behavior beyond rejection, faithful power
modeling, timing.

Implementation record (GREEN): masked row plus
`IDLE_K` layout/set plus `op_idle` checked/trusted
leaf plus a new `IDLE` effect kind with a run-loop
arm to the reserved `STOP_IDLE`. All effect-kind
switches carry defaults, so the new enumerator
compiles clean. Build note (S87 repeat): the
hosted runner exe needs a fresh `opcode_catalog.o`
after lookup regeneration.

Validation evidence: `make test` exit 0 (four new
tests RED-first green under the ASan/UBSan build;
286 Python quality tests OK); fresh CMake Debug
`ctest` 755/755. Targeted `ENTRY=0xF6A` peer run
stops `IDLE` (`attempted=1 retired=0 pc=0xF6A`,
one program read, zero bus traffic). BUDGET=
1000000 widened-window run with the 64-block MCU
peer plus `--port-peer` stops `IDLE` at
`attempted=394725 retired=394724` (byte-identical
rerun) — the pre-slice `UNIMPLEMENTED_OPCODE` is
now the honest interrupt wait.
