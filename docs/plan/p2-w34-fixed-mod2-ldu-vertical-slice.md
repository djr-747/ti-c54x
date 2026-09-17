### P2-W34: fixed-MOD2 LDU vertical slice (planned)

This implemented bounded memory ISA slice has an evidence boundary of the
registered TI `SPRU172C` LDU entry (printed p. 4-79), the registered standard
indirect MOD2 rule in `SPRU131G` §5.5.3 (printed p. 5-13), and the registered
Table 2-28 repeatability boundary (printed pp. 2-19–2-21). LDU is a one-word
`*ARx+,dst` form derived from opcode diagram `0001_001D_IAAAAAAA`, with value
`0x1290`, mask `0xFEF8`, `D` at bit 8, `ARF` at bits 2:0, 16 encodings, and
the `ACCUMULATOR_ARF` layout. It has no input resources. Repeatability remains
`UNRESOLVED`: absence from the forbidden list does not establish eligibility.

The project fit reuses the W31B fixed-MOD2 MEMORY effect/completion and generic
CPU retirement contract. CMPT=0 and ARP=0 are frozen; the old selected AR is
used for transfer before its postincrement, and the checked planner continues
to reject AR=0xffff. LDU loads Smem into destination bits 15:0, clears bits
39:16, ignores SXM, and reports no status-bit effects. No new resource, effect,
or address kind is introduced, and no public ABI, opcode-specific CPU branch,
timing, IRQ/RPT, device/MMR, environment, diagnostics, or allocation behavior
is claimed. The open W31P hosted stability and W31B immediate-READY benchmark
captures remain nonblocking follow-ups.

TDD ownership was split across catalog, decode, pure LDU MEMORY
effect/completion, and generic CPU retirement tests under mirrored paths; all
four executable contracts are now passing. Any
need for an SXM read, a new memory/address seam, or a repeatability decision is
a GAP report rather than an LDU workaround. Catalog coverage is now 28 rows,
1431 populated first words, and 6/28 reset-runnable. Repeatability remains
unresolved.
