### P2-W37: fixed-MOD2 memory-to-status vertical slice (planned)

This activated evidence-fit slice is bounded by the TI `SPRU172C` §4.70 LD
entry (printed pp. 4-69–4-71), the registered standard-indirect MOD2 rule in
`SPRU131G` §5.5.3 (printed p. 5-13), and the registered Table 2-28
repeatability boundary (printed pp. 2-19–2-21). The one-word `LD Smem,DP` and
`LD Smem,ASM` forms use the existing fixed-MOD2 `ARF` layout: values `0x4690`
and `0x3290`, mask `0xFFF8`, fixed bit 8, ARF bits 2:0, and exactly eight
encodings each. No `LD Smem,ARP` syntax is registered. Both forms have no input
resources. Table 2-28 makes DP repeatability `INELIGIBLE`; ASM repeatability is
`UNRESOLVED` because absence is not positive eligibility evidence.

The project fit reuses the W31B MEMORY WORD16 read/completion, existing
status-field effect/writer, old-AR-address read before postincrement, and generic
CPU WAIT/READY/FAULT and sequential-PC paths under CMPT=0 and ARP=0. Completion
masks the copied read to `0x01ff` before `STATUS_FIELD_WRITE` DP or to `0x001f`
before `STATUS_FIELD_WRITE` ASM. The checked planner's AR=0xffff boundary
remains in force. No new layout, resource, effect, address kind, CPU branch,
public API, timing, IRQ/RPT, device/MMR, environment, diagnostics, or allocation
behavior is claimed. The current `operand_layout.c` is 535 lines and remains
unchanged; its reviewed no-split decision remains below the 800-line threshold.

TDD ownership is split across catalog, decode, pure load/status-field completion,
and generic CPU retirement tests under mirrored paths; all four executable
contracts pass through strict and sanitizer runners, with quality and clean dual
review green. The implementation preserves unique catalog handler tokens through
thin DP/ASM wrappers over one shared internal core. Realized coverage is 32 rows,
1463 populated first words, and 6/32 reset-runnable; the pure leaf is 283 lines,
while `operand_layout.c` remains 535 lines and unchanged under its reviewed
no-split decision. W31P hosted stability and W31B immediate-READY benchmark
follow-ups remain open and nonblocking; the final clean full gate is green.
