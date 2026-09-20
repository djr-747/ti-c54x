### P2-W36: fixed-MOD2 immediate-store vertical slice (implemented; local verification green)

State: implementation-complete. Focused catalog/decode/effects/CPU contracts,
sanitizer checks, quality, independent review, and the full standalone gate are
green.

This activated evidence-fit slice is bounded by the TI `SPRU172C` Store
T/TRN/immediate entry (printed pp. 4-166–4-167), the registered
standard-indirect MOD2 rule in `SPRU131G` §5.5.3 (printed p. 5-13), and the
registered Table 2-28 repeatability boundary (printed pp. 2-19–2-21). Syntax 3
defines two-word `ST #lk,Smem` with opcode diagram
`0111_0110_IAAAAAAA`; fixed-MOD2 composition yields value `0x7690`, mask
`0xFFF8`, fixed bit 8, ARF bits 2:0, and exactly eight first words. A planned
private `TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16` layout extracts that ARF and
copies word 1 as the raw 16-bit `lk`. Repeatability remains `UNRESOLVED`:
absence from Table 2-28 is not positive eligibility evidence, and its separate
long-offset or absolute prohibition does not decide this `*ARx+` form.

The project fit reuses generic extension fetch, width-aware sequential PC,
fixed-MOD2 address resolution, and the W31B MEMORY transaction. The pure store
leaf stages a WORD16 WRITE with `write_value_word=lk` and `complete=NULL`.
Under CMPT=0 and ARP=0 the write targets the old selected AR address; READY
then commits postincrement and PC+2, WAIT resumes the same transaction without
refetch, and FAULT publishes neither architectural change. No public ABI,
opcode-specific CPU branch, new input resource, effect kind, address kind,
timing, IRQ/RPT, device/MMR, environment, diagnostic, or allocation behavior
is authorized.

TDD ownership is split across catalog, decode, pure immediate-store effect,
and generic CPU retirement tests under mirrored paths; all four executable
contracts are passing. The current `operand_layout.c` is 535 lines and remains
cohesive; the two-field descriptor has a recorded no-split review and remains
well below the 800-line split threshold. Realized coverage is 30 rows, 1,447
populated first words, and 6/30 reset-runnable. `LD Smem,DP/ASM` remains a later strict-fit
candidate. `ST T`, `STH`, and the audited ALU/control forms remain blocked by
their T-input, saturation/OVM/OV, or PC/SP ownership gaps. P2-W31P hosted
stability and the W31B immediate-READY benchmark follow-ups remain open and
nonblocking.
