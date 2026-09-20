### P2-W33: CMPR CC,ARx vertical slice (implemented; local verification green)

This implementation-complete bounded non-memory ISA slice has green focused,
review, sanitizer, quality, and final full-gate validation; full integration
caught and corrected two stale UNKNOWN near-miss fixtures. Its evidence boundary is the
registered TI `SPRU172C` CMPR entry (printed p. 4-34) plus the registered
repeatability boundary in §2.5/Table 2-28 (printed pp. 2-19–2-21). CMPR is a
one-word `CC,ARx` form with value `0xF4A8`, mask `0xFCF8`, `CC` in bits 9:8,
and `ARx` in bits 2:0, yielding 32 encodings. `CC` selects unsigned EQ, LT,
GT, or NEQ comparison of selected ARx against AR0. `TC` is set to one iff the
condition is true and otherwise cleared; every other raw and known status bit
is preserved. CMPR is explicitly nonrepeatable (`INELIGIBLE`).

The project fit is intentionally narrow. `src/cpu/input/` copies a generic
`AUXILIARY_REGISTERS` snapshot containing all eight `uint16_t` AR values and
per-register knownness, collected only through existing checked architectural
state reads. The pure handler returns `UNKNOWN_INPUT` unless AR0 and selected
ARx are known. It emits the existing `STATUS_BIT_WRITE` effect targeting ST0
bit 12, and the generic CPU path retires it without an opcode-specific branch.
All other state is preserved transactionally. No public API, memory, timing,
IRQ/RPT, device, or environment surface is introduced.

TDD ownership is split across catalog, decode, CPU-input, compare-handler, and
CPU retirement tests under mirrored paths. The catalog/decode tests prove the
32 masked encodings and CC/ARx fields; the input test proves all-eight AR
snapshot copying and unknownness; the pure-effect test proves unsigned
comparison and TC-only mutation; the CPU test proves generic retirement and
PC progression. Any need for a new status policy, memory, interrupt/repeat,
parallel issue, timing, public state, or opcode-specific CPU branch is a GAP
report rather than a CMPR workaround. Expected post-slice coverage is 27 rows,
1415 populated first words, and 6/27 reset-runnable; those counts are now
realized. W31P hosted stability and W31B2 immediate-READY benchmark follow-ups
remain open.
