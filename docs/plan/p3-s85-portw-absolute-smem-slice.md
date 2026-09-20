### P3-S85: PORTW absolute Smem slice (implemented; local verification green)

`PORTW *(abs),PA` is TI SPRU172C §4 pp. 4-129–4-130
(PORTW with absolute Smem, 3 words): first `0x75F8`
with I=1 and the `0x78` absolute selector, word 2
carrying the 16-bit DATA address, word 3 carrying the
16-bit PA; execution writes the word at abs to the
port with no status-bit change (P3-S6 seam evidence:
"one word added for long-offset indirect or absolute
Smem"). The widened-window loader1 run stops
`INVALID_STATE` on it at `0xF57` (words
`75F8 0008 0000`, so DATA `*(0x0008)` to port 0):
the `0x75F8` first word classifies into the 2-word
PORTW_SMEM masked row, whose leaf rejects mode 15.
Only PORTW_SMEM covers `0x75F8`; the exact row sits
before it (its `0xFF80` mask spans the absolute
cookie, S81/S82 precedent) and wins by generation,
so no shadow carve is needed. Absolute operands
cannot repeat (section 2 rule, same evidence as
S70/S72-S84), so the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `PORTW_SMEM_ABSOLUTE`
(`0x75F8`, 3 words) with a new 4-operand absolute
layout set (indirect I[7:7], selector A[6:0],
absolute word, PA word — no reusable set exists).
The leaf gates nothing (PORTW_SMEM precedent: the
family consumes AUXILIARY_REGISTERS only through the
resolver; absolute needs no AR) and stages the
existing PORT WORD16 WRITE arm with absolute DATA
address plus PA. Only the evidenced absolute
selector composes. New catalog ID moves `ID_COUNT`
340 to 341 with pins; `FORM-T2-26-10` binds the row
`SUBSET` with `ABSOLUTE_DATA`; no MMR, knob, or
timing changes. Like S81/S82, the exact word reclaims
its lookup pin from the masked row, so populated first
words stay 40274.

Open design points: the PORT WORD16 arm and the
sequencer PORT resolver need the absolute-DATA
variant (new arm fields or extended address kind);
the hosted runner owns no port peer (`io_calls=0`
in every run), so acceptance is the first IO
request stop with the staged effect proven —
decided for the stop against the runner evidence
in-slice (retirement is proven by the CPU test).

RED contracts: new decode/ops/metadata/CPU tests
(`test_portw_smem_absolute_decode`,
`test_portw_smem_absolute`,
`test_portw_smem_absolute_metadata`,
`test_cpu_portw_smem_absolute`) plus the
catalog-contract row pin. Acceptance is the hosted
repro advancing past `0xF57` (or stopping on the
first IO request with the staged effect proven).

Out of scope: PORTW direct/indirect/long-offset
family (own slices as the boot reaches them), a
hosted port peer device model (own seam unless the
repro demands it), timing.

Implementation record (GREEN): the absolute-DATA
variant rides the PORT WORD16 arm with the marker
`indirect=1, smem_mode=15, smem_arf=0,
smem_dma7=0` carrying the DATA address in
`reserved_half` (PA stays in `port_address`). The
arm-shape mirror in `memory_effect.c` needed the
same carve-out as the IO-leg planner, or prepare
rejects the effect before pair routing
(INVALID_CANDIDATE). The PORT shape validator
routes the synthesized ABSOLUTE_DATA leg through
the absolute-data shape (Smem-absolute precedent);
PORT liveness dispatches the marker to a new
absolute live helper (frozen echo plus sequential
PC, no CPL/DP observed). The CPU-test endpoint
demultiplexes both pair legs (MVDP precedent):
DATA READ served, IO WRITE recorded. The two
PORT layout sets moved to
`operand_layout_sets_port.c` (size split:
`operand_layout_sets.c` 829 to 781 lines).

Validation evidence: `make test` exit 0 (four new
tests RED-first green under the ASan/UBSan build;
all C binaries pass; 286 Python quality tests OK;
fresh CMake Debug `ctest` 746/746). Targeted
`ENTRY=0xF57` run stops BUS_FAULT on the first IO
request (`attempted=1 retired=0 pc=0xF57 opcode=
0x75F8`, full 3-word fetch, DATA leg served, IO to
port 0 faulted, `io_calls=0`). BUDGET=1000000
widened-window run with the 64-block MCU peer
retires 394676 with zero data faults to the same
stop (`first_data_fault=0x0000`, fills 32/32/64):
the pre-slice INVALID_STATE is now the staged
first hardware-port access, with no port peer in
the runner. Next slice is the follower past the
port access.
