### P2-W32: SSBX/RSBX status-bit vertical slice (implemented; local verification green)

State: implementation, focused tests, independent no-P0/P1 review, quality
gate, focused ASan/UBSan validation, and the full standalone `make test` gate
are green. The completed non-memory ISA slice is limited to the registered TI
`SPRU172C` SSBX entry (printed p. 4-166) and RSBX entry (printed p. 4-151); no
emulator, device profile, timing table, or inferred status policy is an
authority. The rows are one-word forms with values `0xF5B0`
(SSBX) and `0xF4B0` (RSBX), shared mask `0xFDF0`, `N` at bit 9, and `SBIT` at
bits 3:0. They have no input resources and are marked repeatability
`INELIGIBLE` from the registered instruction boundary.

The slice adds five independently tested ownership pieces:

1. `src/isa/catalog/` registers the two rows and their 32 encodings each;
   `src/isa/decode/` owns the value-owned `N` and `SBIT` extraction.
2. `src/sequencer/status/` generalizes the private status-bit write/application
   route. A candidate changes and marks known only the selected raw ST0 or ST1
   bit, preserving every unrelated raw and known bit and committing atomically.
3. `src/isa/ops/status/` owns pure SSBX/RSBX handlers that produce set/clear
   effects only. `N=1, SBIT=10` is accepted as the encoded ST1 reserved bit;
   this slice does not invent a side effect for its reserved meaning.
4. `src/cpu/` retires both forms through generic decode, input, status-effect,
   and sequential-PC paths. No opcode-specific CPU branch, memory request,
   RPT/IRQ behavior, timing API, knob, environment read, diagnostic callback,
   or public ABI change is permitted.

TDD starts with RED catalog/decode, status-application, pure-handler, and CPU
retirement tests under mirrored owner paths. GREEN may proceed only after the
existing status-field transaction is shown to fit the generalized raw-bit
candidate; a new status route must remain private and transactional. Any need
for status-register policy, reserved-bit behavior, memory, interrupts,
repeat, parallel issue, timing, or public state is a GAP report, not a family
workaround. The expected post-slice catalog is 26 rows and 1383 populated
first words, with reset-runnable coverage likely 6/26; these are lookup
population counts, not a distinct-instruction or timing claim. W31P hosted
stability and W31B2 immediate-READY benchmark follow-ups remain open.
