### P2-W49A: bit-32 ADD/SUB overflow and status foundation (planned)

TI SPRU131G describes a 40-bit ALU and accumulator lane while defining carry
and borrow at bit 32. Arithmetic overflow for the common C54x mode is measured
against the signed 32-bit result range: with OVM clear, the full modulo-40 ALU
result is retained; with OVM set, a positive or negative overflow is replaced
by `00 7FFF FFFFh` or `FF 8000 0000h`. The destination accumulator's OVA or
OVB flag is set on overflow and otherwise remains sticky. W49A establishes
these shared rules before any ADD or SUB catalog form is activated.

`src/foundation/arithmetic40_addsub.[ch]` owns checked, allocation-free ADD and
SUB primitives over canonical 40-bit operands. Each produces one modulo-40
result, the exact low-32 carry or no-borrow result, and an explicit signed-32
overflow direction. The foundation does not choose OVM storage policy or mutate
status. It uses defined unsigned arithmetic for the lane and sufficiently wide
signed arithmetic for overflow classification, including the signed-40 extrema.

The private status owner adds typed knownness-aware OVM, OVA, and OVB access.
The generic input-resource mask widens from eight to sixteen bits before OVM
occupies bit `0x0080`, retaining representable invalid and future resource bits
without widening the copied 48-byte opcode-input snapshot or trusted packet.
The accumulator effect's status delta gains a destination-overflow action that
may preserve or set the selected OVA/OVB flag but may not implicitly clear it.
The existing accumulator-plus-status candidate remains the atomic commit unit;
no new effect kind or opcode-specific CPU branch is introduced.

Mirrored tests cover modulo-40 wrap, bit-32 carry/no-borrow independent of guard
bits, signed-32 positive and negative overflow including signed-40 extremes,
typed field knownness and transactional writes, OVM collection, sticky A/B
overflow effects, malformed rejection, and LP64/ILP32 carrier-size stability.
W49A adds no catalog row, handler, timing claim, runtime repeat behavior, bus
work, public ABI, allocator, I/O, environment access, observer, diagnostic, or
mutable process state. W49B composes the separately evidenced register-only
ADD/SUB syntax-9 forms through this seam and benchmarks the resulting public CPU
path.

W49A is implementation-complete. Its supported ILP32 probe freezes metadata,
decoded instruction, input snapshot, trusted packet, and outer effect sizes at
24/20/48/76/40 bytes; native LP64 remains 24/20/48/104/48. Six interleaved
same-host observer-off benchmark pairs against the frozen W48C binary found no
material regression: median deltas were -1.34% NOP, -2.96% LD-DP, -1.21%
LD-ASM, +1.23% LD-ARP, and -3.62% mixed-current, with semantic hashes unchanged.
Closure accounting is 192 immutable evidence records; 314 requirements (278
`IMPLEMENTED`, 7 `PLANNED`, 29 `UNRESOLVED`); and 269 tests (262 `PASSING`, 7
`PLANNED`). Catalog and inventory counts remain unchanged.
