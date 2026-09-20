### P2-W31P: trusted execution hot path and release-performance contract (implemented; local verification green)

State: implementation and structural proofs are green; hosted-performance
acceptance remains pending/deferred. The optimized cohorts are below the 60 ns
median target, but the exact three-capture stability rule remains open. This
follow-up no longer blocks W31B2.

The current implementation has clean ownership and transactional semantics. The
hosted audit showed that the common retired-instruction path performed too much
defensive work, so this shared CPU/decode optimization remains the performance
architecture freeze while W31B2 proceeds as the next ISA slice.
It must improve the common path without weakening the checked contracts or making
the library dependent on a particular compiler, linker, host, or emulator.

The frozen shape is:

- `src/isa/catalog/` continues to own the compact generated first-word-to-ID
  lookup and immutable scalar metadata. `src/isa/ops/dispatch/` owns the
  catalog-derived per-ID binding that combines those immutable facts with the
  semantic handler. A CPU iteration performs one first-word ID lookup and one
  per-ID binding selection. It does not repeatedly look up metadata, revalidate
  a layout, or re-extract fields after the packet has been prepared.
- `src/isa/decode/` owns the checked decode boundary and creates one private,
  value-owned trusted execution packet. Packet preparation consumes the selected
  binding and one prevalidated generated/static operand plan, then copies the
  decoded instruction facts, handler, and extracted operand values into the
  packet. The packet retains no data pointer or caller-owned storage; a stored
  function pointer refers only to process-lifetime code. It is prepared once per
  instruction and is not a public mutable object. Checked decode and dispatch
  entry points remain available for forged-input tests; the fast path consumes
  only a packet produced by the private constructor.
- A packet lives only through ordinary semantic dispatch. If it produces a
  MEMORY effect, the existing root transaction copies the decoded instruction,
  input, effect, address, and PC candidates before the packet expires. The root
  retains no packet pointer. Its endpoint context remains the separately
  borrowed caller-owned object copied into the CPU bus descriptor for the CPU
  lifetime; that endpoint lifetime is not packet ownership and is unchanged by
  this work.
- `src/cpu/input/` continues to own generic resource collection. The CPU does not
  add an opcode switch, family-specific resource lookup, or handler-specific field
  extraction. New resources extend the generic collector and its packet contract.
- `src/cpu/execution/` owns the trusted packet loop and one precomputed PC plan.
  PC validation and span arithmetic happen while preparing the plan; retirement
  performs one trusted commit after the semantic effect is proven. The loop keeps
  the existing attempt/retire/stop behavior and never turns opcode identity into
  CPU orchestration logic.
- `src/sequencer/instruction/`, the existing effect transaction, and its owned
  status/accumulator/temporary leaves retain effect atomicity, candidate
  validation, commit ordering, observer boundaries, and resource/effect
  ownership. The memory WAIT/RESUME path is unchanged: a pending transaction is
  resumed before fetch and is never refetched, redecoded, or reissued by the
  optimization.
- `bench/` owns fixed-scenario construction and observer-off captures; `tests/`
  mirrors the owning source subtree for packet, binding, PC-plan, atomicity, and
  regression proofs. No new monolithic `cpu.c`, decoder, or opcode-family file is
  permitted; a source owner approaching the project size review threshold must
  split before adding another concern.

The portable default deliberately does not use a 65,536-entry pointer table or
GNU computed goto. A full pointer table imposes a large, permanently resident
data structure and avoidable relocation/cache pressure for a vendored library;
computed goto would make the default dispatch contract compiler-specific. The
catalog-derived binding and direct generated lookup retain predictable portable C
while allowing a consumer to opt into ordinary compiler IPO/LTO. The standard
consumer build remains free to choose IPO; the core must not require it. The
benchmark release profile may use portable `-O3` plus LTO and must record the
exact flags in every capture.

TDD and validation proceed in explicit phases:

1. RED: capture the current O2/no-LTO and O3/LTO audit, add packet and binding
   contract tests, and add deterministic structural checks for one lookup/one
   binding, one packet-preparation extraction pass, one PC plan, and no
   opcode-specific CPU switch.
2. GREEN: add the generated/static binding and value-owned trusted packet behind
   the existing checked decode/dispatch seams. Prove forged IDs, layouts,
   operands, PC spans, and packets are rejected before the trusted path; prove
   valid packets execute with the same effect, observer, stop, and retirement
   behavior as the checked path.
3. GREEN: route the CPU fast path through the packet and one PC plan while keeping
   memory WAIT/READY/FAULT and cancellation behavior byte-for-byte equivalent at
   the contract level. Add strict, sanitizer, observer-off, and consumer-build
   evidence and retain those checks while W31B2 proceeds.
4. PERF: repeat each fixed scenario in three stable same-host captures for both
   the portable O2/no-LTO diagnostic profile and the advisory portable
   O3/LTO release profile, record compiler/linker/platform metadata, and compare
   against the frozen audit. A set is exactly three consecutive captures with
   identical binary and scenario hashes. Every target scenario in every capture
   must remain below 60 ns and have `(max - min) / median <= 0.15`. A failing
   set is retained as `INCONCLUSIVE`, never silently discarded or selectively
   averaged; a replacement uses a new set ID and records the changed external
   condition.
5. W31B2 execution slice: add immediate-READY `LD *ARx+,dst` and `STL src,*ARx+`
   plus dedicated observer-off benchmark scenarios. Their fixed-scenario medians
   must meet the same target as a follow-up performance requirement before later
   memory fanout. WAIT-capable memory remains a separate contract and is not
   hidden in the benchmark's immediate-READY path.

Acceptance is an implementation-throughput target, not a TI timing claim: on one
same host, both observer-off profiles must record a median below 60 ns per retired
instruction for every fixed scenario in three stable captures. The O2/no-LTO
requirement prevents IPO from hiding a structurally expensive execution path;
O3/LTO proves the intended release profile. This is advisory evidence and is not
a flaky CI threshold. Structural tests must also prove the binding/packet/PC-plan
invariants deterministically, and no result may trade away effect atomicity,
observer isolation, or memory WAIT semantics. There is no runtime knob,
environment read, diagnostic path, A/B switch, or test instrumentation in the
production core for this work; source checks and measurement live in their test
and benchmark owners.

The triggering observations and the exact commands, hashes, host/toolchain
metadata, raw medians, ranges, and acceptance rule are committed in
`bench/audits/p2-w31p-baseline.json`. Those observations freeze the problem;
they do not constitute the later three-capture acceptance set.
