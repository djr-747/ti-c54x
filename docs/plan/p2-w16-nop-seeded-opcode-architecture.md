### P2-W16: NOP-seeded opcode architecture

This completed work piece establishes the smallest evidence-backed ISA architecture slice
without integrating it into the public CPU shell. The ownership boundary is
deliberate:

- `src/isa/catalog/` owns one declarative X-macro catalog, and that catalog is
  the only instruction inventory. No second handwritten opcode list or copied
  legacy table is permitted.
- Macro passes generate opcode identity and metadata, a direct 16-bit first-word
  lookup, and a function-pointer semantic dispatch table. The generated views
  remain derived from the one catalog.
- The first registered row is NOP `0xF495` with width one program word. Words
  absent from the catalog classify explicitly as unknown; they do not alias NOP
  or silently select a default handler.
- NOP emits an explicit no-opcode-owned-effect result. Opcode semantics do not
  mutate the program counter, status, memory, buses, devices, or observers.
- `src/sequencer/` alone plans checked private 16-bit sequential PC `+1`. A
  source PC of `0xffff` returns an explicit unresolved-boundary result and
  leaves the input and output state unchanged.
- The public CPU shell is not integrated in this slice: its current carrier is
  32-bit, while high-carrier and XPC policy remain unresolved. The private
  16-bit seam is therefore tested independently of `include/ti_c54x/cpu.h`.
- This slice makes no machine-cycle or pipeline timing claim and adds no memory,
  bus, device, or observer behavior to opcode operations. It does not copy code,
  tables, comments, or tests from legacy emulators or other external projects.

The TDD order is catalog contract and decoder classification first, explicit NOP
semantic no-effect second, and checked sequential-PC planning third. Each owner
has its own test path; cross-owner integration is deferred until the central
sequencer and public CPU-shell contracts are ready.
