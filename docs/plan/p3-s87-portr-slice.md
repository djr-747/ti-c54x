### P3-S87: PORTR absolute Smem slice (planned)

`PORTR *(abs),PA` is TI SPRU172C §4 p. 4-129
(PORTR with absolute Smem, 3 words): first `0x74F8`
with I=1 and the `0x78` absolute selector, word 2
carrying the 16-bit DATA address, word 3 carrying the
16-bit PA; execution reads the port into the word at
abs with no status-bit change. The peer run stops
`INVALID_STATE` on it at `0xF97` (words
`74F8 0008 002D`, so port `0x2D` into `*(0x0008)`):
the `0x74F8` first word classifies into the 2-word
PORTR_SMEM masked row, whose leaf rejects mode 15.
Only PORTR_SMEM covers `0x74F8`; the exact row sits
before it (its `0xFF80` mask spans the absolute
cookie, S81/S82/S85 precedent) and wins by
generation, so no shadow carve is needed. Absolute
operands cannot repeat (section 2 rule, same
evidence as S70/S72-S85), so the row marks
`INELIGIBLE`.

Leaf design: new `EXACT` row `PORTR_SMEM_ABSOLUTE`
(`0x74F8`, 3 words) reusing the S85 4-operand
absolute layout set (no new layout, no new set
file). The leaf gates nothing (PORTR_SMEM
precedent; absolute needs no AR) and stages the
existing PORT WORD16 arm with the mode-15 marker
(arf 0, dma7 0), absolute DATA address in
`reserved_half`, PA in `port_address`, DATA-WRITE
direction, zero staged value, complete NULL. Only
the evidenced absolute selector composes. The
sequencer needs no change (S85 marker handling is
direction-agnostic: plan, synthesize, effect gate,
shape, and liveness). New catalog ID moves
`ID_COUNT` 341 to 342 with pins;
`FORM-T2-26-09` binds the row `SUBSET` with
`ABSOLUTE_DATA`; no MMR, knob, or timing changes.
The exact word reclaims its lookup pin from the
masked row, so populated first words stay 40274.

RED contracts: new decode/ops/metadata/CPU tests
(`test_portr_smem_absolute_decode`,
`test_portr_smem_absolute`,
`test_portr_smem_absolute_metadata`,
`test_cpu_portr_smem_absolute`) plus the
catalog-contract row pin. Acceptance is the peer
repro retiring the `0xF97` PORTR (sticky port read
serves `0x0000`) and recording the new frontier.

Out of scope: PORTR direct/indirect/long-offset
family (own slices as the boot reaches them),
faithful hardware port truth, timing.

Implementation record (GREEN): exact row plus
`portr_smem_absolute` checked/trusted leaf only —
layout, plan, synthesize, effect gate, shape, and
liveness reuse the S85 marker handling untouched
(all four new tests green on the first full run).
Build note: the hosted runner exe needs a fresh
`opcode_catalog.o` after regenerating the lookup
(the hosted object rule does not track the `.inc`
derivative); symptom is the stale 2-word masked
decode (`prog_reads=2`, no bus traffic).

Validation evidence: `make test` exit 0 (four new
tests RED-first green under the ASan/UBSan build;
286 Python quality tests OK); fresh CMake Debug
`ctest` 748/748. Targeted `ENTRY=0xF97` peer run
retires the PORTR (`reads=1`, sticky port read
serves `0x0000`, DATA `0x0008` written) and stops
`INVALID_STATE` at pc=`0xF9A`, opcode `0xF130`.
BUDGET=1000000 widened-window run with the
64-block MCU peer plus `--port-peer` retires
394724 with zero data faults (`io_calls=4`) to
the new frontier `UNIMPLEMENTED_OPCODE` at
pc=`0xF6A`, opcode `0xF4E1` (byte-identical
rerun; `prog_covered` 133 to 166) — the next
library slice.
