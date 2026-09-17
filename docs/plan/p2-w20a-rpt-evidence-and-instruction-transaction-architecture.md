### P2-W20A: RPT evidence and instruction-transaction architecture (active kickoff)

State: evidence and architecture contract complete. P2-W20B has implemented the
canonical instruction-bundle seam without adding an RPT opcode. Direct RPT handler
or CPU integration work remains gated by the later owner-specific proof slices.

The immediate forms are frozen from TI primary evidence:

- `RPT #K` is one program word with first-word value `0xEC00`, mask `0xFF00`,
  and unsigned `K` in bits 7:0 (`0..255`).
- `RPT #lk` is two program words with exact first word `0xF070` and the complete
  unsigned 16-bit operand in extension word one (`0..65535`).
- The RPT operation loads `RC` with the operand. The following instruction executes
  operand+1 times, giving a range of 1 through 65536 body executions. RPT changes
  no status bits and is itself nonrepeatable.
- Once RPT is decoded, all interrupts including NMI are deferred until every body
  execution completes; literal RS/reset remains admissible. A request still sets
  its IFR flag without immediate acknowledgement, and that flag clears when the
  interrupt is eventually taken. No device source, vector, priority, or wiring is
  selected by this common-core contract.
- Repeat-body eligibility comes from the documented repeatability restrictions;
  handlers and the CPU may not infer eligibility from opcode identity or width.

Implementation is deliberately split along the ownership hierarchy:

1. P2-W20B adds a canonical copied, value-owned instruction-word bundle under
   `src/isa/decode/`. CPU fetch owns only population of that value; neither owner
   retains caller storage.
2. P2-W20C binds immutable operand-layout and repeatability tokens through the sole
   catalog. Layout owners reuse the checked field extractor, and opcode handlers
   never hand-mask instruction words.
3. P2-W20D adds a focused RPT operation and proves both immediate layouts without
   making a partially integrated RPT row executable by the production CPU. It emits
   a discriminated, value-owned `REPEAT_START` effect carrying the exact operand and
   never mutates state, PC, interrupt state, memory, buses, observers, or diagnostics.
   Production catalog activation must be atomic with a generic effect-consumption
   path or an explicit unsupported-effect contract; a transient decode-success then
   `INVALID_STATE` CPU behavior is forbidden.
4. P2-W20E adds `src/sequencer/repeat/` as the pure single-repeat owner. Its internal
   cardinality is at least 17 bits (implemented with `uint32_t`) so `0xffff + 1`
   represents 65536 without wrap. It owns body eligibility and the generic
   interrupt-admission decision, including the reset exception, but not request
   sampling or device wiring.
5. P2-W20F may integrate the generic effect handoff into the CPU only after the
   remaining architectural rules below are resolved. `cpu.c` must never branch on
   the RPT opcode identity; it orchestrates decoded values and sequencer effects.

The directory tree and tests mirror those owners. Each phase is a separate atomic
TDD work piece, no handwritten file becomes a cross-owner monolith, and generated
metadata stays immutable and initialization-free. Machine-cycle timing, shared
memory, profiles, and emulator integration remain outside P2-W20.

Three behaviors remain explicitly unresolved and must not be encoded as convenience
policy: the phase and final value of RC decrement, the mechanism that holds or
reloads the repeated-body PC and its final PC, and arbitration when an interrupt is
already pending before RPT decode. Detailed pipeline and machine-cycle behavior
also remains outside the active timing contract. These gaps do not weaken the
proven cardinality or post-decode interrupt gate; they prevent premature CPU
integration.

#### P2-W20B: canonical complete-instruction bundle (complete)

Implementation commit:
`00700d5dd8bd72b711a5cb8ad1147efd5198a92c`.

This work piece moves the current two-word helper bundle type from CPU ownership to
`src/isa/decode/` and adds a checked complete-instruction binding without changing
the production catalog. The fixed capacity remains explicitly local to the current
helper and is not advertised as the ISA maximum. The bundle contains copied 16-bit
program words plus an explicit count and retains no pointers.

The existing first-word decode remains the only catalog lookup needed for an
attempt. A new completion step validates the first-word result, immutable metadata,
bundle count/capacity, first-word identity/pattern, and full copied bundle before
publishing a value-owned decoded instruction transactionally. Dispatch and NOP
consume that complete value. CPU orchestration calls the existing extension-fetch
owner to populate it, but NOP still performs exactly one bus callback, has identical
stop/counter/PC behavior, and remains the only registered production opcode.

Ownership is split: `src/isa/decode/` owns the type and completion validation;
`src/cpu/` owns fetching/population and execution orchestration; `src/isa/ops/`
owns the complete-instruction handler signature and NOP validation. Tests and build
manifests mirror each owner. The work piece adds no operand layout, RPT row/effect,
repeat state, architectural state integration, IRQ behavior, machine cycles,
memory mutation, observer event, knob, public API, or emulator dependency.

The completed implementation preserves the first-word classifier, asks the CPU
fetch owner to populate extension words, transactionally binds the copied bundle,
and passes that value through generic dispatch. NOP still performs one callback and
one-word retirement; `cpu.c` contains no opcode-identity branch. P2-W20C is next and
must define catalog-bound operand-layout and repeatability metadata without
prematurely activating an instruction whose effect the CPU cannot yet consume.

#### P2-W20C: catalog operand-layout and repeatability metadata (complete)

State: complete in two atomic owner-specific proofs. Production continues to
register only NOP; no RPT first word, handler, effect, or CPU behavior is activated.

P2-W20C1 is complete in implementation commit
`d56a9cc28f183c8e348837e393458660e37091d3`. It extends the sole X-macro
catalog row with two fully namespaced scalar
tokens: a catalog-owned operand-layout identity and a tri-state repeatability class.
The valid repeatability states are `UNRESOLVED`, `ELIGIBLE`, and `INELIGIBLE`;
unresolved is a safe explicit state and never implies admission. Layout identity
reserves `UNKNOWN` as invalid for production rows and provides `NONE` for NOP.
NOP binds `NONE` plus `UNRESOLVED`, because this slice does not infer its body
eligibility from absence in an exclusion summary. The Python schema, every C
X-macro pass, immutable metadata, complete decoded-instruction value, dispatch,
and NOP validation agree on the tokens. The generated 65,536-entry lookup remains
byte-for-byte unchanged and normal builds remain compiler-only. The schema rejects
old arity, unknown/unqualified/non-string tokens, and the reserved layout sentinels;
completion and operations reject forged scalar metadata transactionally.

P2-W20C2 is complete in implementation commit
`4746a8c0a1074bdf3b9419721901a0f8f0808486`. It adds decode-owned immutable
operand-layout-set descriptors and resolution.
A layout set, rather than a single field, is required so future multi-operand
instructions do not force a carrier redesign. The catalog stores only compact IDs,
never decoder-owned pointers. Decode resolves an ID to process-lifetime static data,
validates every field against the instruction program-word width, and routes copied
operand extraction through `operand_field`; handlers never hand-mask instruction
words. Outputs are transactional and no caller pointer is retained.

The directory hierarchy stays explicit: `src/isa/catalog/` owns row syntax, token
identity, immutable metadata, and catalog tests; `src/isa/decode/` owns descriptors,
resolution, extraction, and mirrored decode tests; `src/isa/ops/` only consumes the
completed values. CPU, sequencer, state, timing, buses, observers, diagnostics,
knobs, allocators, environment, and public API are unchanged.

RPT layout identities may exist only as inert decoder metadata during W20C2. They
must remain unreferenced by the production opcode catalog until a typed
`REPEAT_START` effect and generic consumption path can land coherently. RPT
encoding/operation, RC and repeat-PC progression, body admission, interrupt gates,
machine cycles, shared memory, and emulator integration remain outside W20C.

The W20C2 resolver owns three initial identities: `NONE`, inert one-word RPT
immediate-K, and inert two-word RPT immediate-lk. The RPT descriptors are grounded
in the already pinned TI layouts but remain unreachable from production first-word
decode. `operand_field` gains a pure descriptor-validation entry point reused by
both resolution and extraction; W20C2 may not duplicate its coverage, overlap, or
source-index rules. A value-owned layout-set view may borrow process-lifetime static
descriptor storage, but no catalog metadata contains a pointer and no caller storage
is retained. The executable layout proof covers minimum and maximum immediate
values, invalid identities and widths, forged views, multi-operand descriptor shape,
static lifetime, and transactional failures. The generated direct lookup remains
byte-for-byte unchanged. P2-W20D is next and may build the typed value-owned
`REPEAT_START` operation against these layouts, but it may not activate a production
RPT row until generic effect consumption is coherent.

#### P2-W20D: typed immediate-RPT effect (complete)

State: implemented and green in commit
`f4a445ccd8d6bb54d791c86f12066409fc0c6cef`. This project-contract TDD proof does
not implement or close the common TI RPT operation requirement; it prepares the
value boundary that later state/sequencer integration can consume.

`src/isa/ops/repeat/` owns a pure immediate-RPT effect builder and its mirrored
test subtree. Its input is a borrowed, copied `ti_c54x_operand_field_value` already
extracted through one of W20C2's inert K/lk layouts. This avoids inventing a valid
`ti_c54x_decoded_instruction` identity before RPT exists in the catalog and keeps
first-word masks entirely under decode/catalog ownership. The builder accepts only
the proven unsigned 8-bit K and 16-bit lk field shapes, retains no pointer, and
publishes only after full validation. Null pointers use `INVALID_ARGUMENT`; a
nonnull field with an unsupported width or a raw value outside its declared width
uses the new private `INVALID_OPERAND` status without renumbering existing values.

`src/isa/ops/op_effect.h` adds a discriminated `REPEAT_START` payload containing
the exact `uint16_t` operand value. The payload deliberately does not contain
operand+1 cardinality, a repeated-body PC, cycle cost, interrupt state, or a state
pointer; those belong to later sequencer/state owners. Existing NOP and dispatch
paths must initialize the expanded effect deterministically, while CPU continues to
accept only `NONE` and therefore remains unchanged.

The TDD proof covers K and lk composition through the immutable layout extractor,
minimum/interior/maximum values, copied ownership, invalid widths and out-of-range
raw values, nulls, and byte-transactional failure output. The implementation creates
no production RPT catalog row, first-word lookup, generated handler-table entry,
dispatch path, or CPU behavior. The existing common RPT decode and operation
requirements remain `PLANNED`; RC mutation/decrement, operand+1 cardinality,
repeat-body eligibility, IRQ/NMI/IFR/reset admission, repeated-body PC flow, timing,
memory, profiles, and emulator integration remain outside W20D.

The completed module accepts only copied field values, appends an explicit
`INVALID_OPERAND` status without renumbering existing statuses, and commits a named
value-owned effect payload only after validating the 8- or 16-bit shape and raw
range. The operation retains no pointer and imports no catalog, decoded-instruction,
state, bus, observer, or host surface. Source and tests live in mirrored repeat-family
subtrees with owner-local manifests. P2-W20E is next and owns pure single-repeat
cardinality, body admission, and request-admission policy; W20D does not precompute
those decisions.

#### P2-W20E: pure single-repeat policy foundation (active kickoff)

State: implemented and green in commit
`d876f94dfb6ddbd08257f477457ca20ae4f34e68` as three project-contract TDD
proofs. The common TI cardinality,
body-eligibility, and IRQ-gate requirements remain planned until production
integration proves actual body retirement, concrete catalog classifications, and
the active repeat window. This avoids treating a useful pure calculation as broader
execution conformance.

`src/sequencer/repeat/` becomes a dedicated subowner with three leaf modules and a
mirrored test subtree:

- `repeat_cardinality` accepts a copied `uint16_t` operand, widens before adding one,
  and transactionally publishes a `uint32_t` body-execution count. The complete
  input range is valid, including zero to one and `0xffff` to 65536. It owns no
  execution loop, RC, PC, state, effect consumption, or timing.
- `repeat_body_eligibility` consumes only the existing catalog-owned
  `ti_c54x_repeatability_class` scalar. `ELIGIBLE` admits, `INELIGIBLE` rejects, and
  `UNRESOLVED` remains explicitly non-admitting. Forged values fail; the policy may
  not inspect opcode identity, width, operands, addressing forms, or catalog tables.
- `repeat_request_gate` consumes a caller-supplied `uint32_t` count remaining and a
  small gate-local request class: maskable, NMI, or literal RS/reset. Zero means the
  repeat gate is inactive and blocks nothing; counts one through 65536 defer
  maskable and NMI requests while never blocking literal reset. Larger counts and
  forged request kinds fail transactionally. “Not blocked” is deliberately not a
  claim that another interrupt policy must service the request.

Each leaf owns its status/decision types and value contract. Parent sequencer
Make/CMake manifests aggregate `repeat/`; they do not flatten the leaf sources into
an unrelated owner. The first TDD registration creates three planned project tests
under `tests/unit/sequencer/repeat/`, then implementation makes only those contracts
passing. The existing common tests remain planned and separate.

W20E must not import the W20D opcode effect, decode a catalog row, mutate CPU or
architectural state, sample or write IFR, choose sources/vectors/priorities, advance
or reload PC, expose architectural RC phase/final value, add timing, touch memory or
buses, add observers/diagnostics/knobs, read the environment, allocate, or activate
production RPT decode. Pending-request persistence stays with the later
`src/sequencer/interrupt/` owner. RC progression, repeated-body PC flow, and
predecode interrupt arbitration remain explicitly unresolved.

Exit condition: the three project requirements and mirrored tests are passing in
Make and CTest; the full `uint16_t` cardinality domain is proven without wrap;
unresolved repeatability never admits; request decisions cover inactive, first,
interior, and 65536-remaining windows plus invalid inputs; source and test manifests
mirror the new owner; production lookup remains unchanged; and the common TI rows
remain planned without overclaim.

The completed implementation keeps every policy value-only and allocation-free.
Cardinality widens before addition and exhaustively maps the entire `uint16_t`
domain to one through 65536. Body admission preserves `UNRESOLVED` as a distinct
non-admitting result and rejects forged repeatability values transactionally.
Request gating validates counts through 65536, reports only `NOT_BLOCKED` or
`DEFER`, and names the literal RS/reset exception without owning a reset opcode or
device path. Make and CMake aggregate matching repeat-owner manifests, while CPU,
state, opcode effects, production lookup, and every unresolved integration policy
remain unchanged.

#### P2-W20F: CPU integration evidence closure (deferred)

State: evidence review remains open and CPU integration is intentionally deferred.
The original bounded audit of official TI `SPRU172C` Rev C and `SPRU131G` Rev G
material confirmed RPT load, cardinality, repeatability, and the post-decode
interrupt gate. P2-W48A's later primary-source correction closes the observable
RC value progression, but not the exact internal decrement ordering, repeated-body
PC behavior, or already-pending request arbitration required by the generic CPU
transaction.

The additional narrow evidence is useful but insufficient for retirement:

- The RPT entry says RC is inaccessible to software while single-repeat execution
  decrements it. READA and WRITA operation pseudocode later closes the observable
  sequence and final zero, while leaving the exact internal decrement ordering
  unstated.
- Repeated MVDK/MVKD behavior tests the current RC value for a conditional address
  increment. W48A therefore keeps architectural RC authoritative and validates it
  against the remaining-body invariant rather than substituting an unverified
  abstract counter.
- Generic sequential-PC, block-repeat, request-receipt, acknowledgement, latency,
  and pipeline examples do not specify the single-repeat PC hold/reload mechanism,
  final PC, or arbitration of a request already pending before RPT reaches decode.
- GNU binutils 2.42 remains a secondary encoding and operand-shape oracle only. Its
  opcode metadata supplies no RC, PC, IFR, or interrupt-arbitration semantics.

Therefore production RPT rows remain inactive and `cpu.c` gains no repeat branch.
The next admissible normative W20F input is stronger TI evidence or a versioned
original-hardware observation for repeated-body PC flow and predecode request
arbitration. An optional black-box differential vector may expose a hypothesis or
disagreement but cannot close the common requirement by itself. No such test input
may introduce an emulator build dependency.

#### P2-W48A: observable RC progression lifecycle (implemented)

The earlier W20F ledger conclusion is corrected additively by stronger passages in
TI SPRU172C Rev C. READA (printed pp. 4-136–4-137) and WRITA (printed
pp. 4-196–4-197) explicitly perform their transfer, increment PAR, and write
`RC-1` while RC is nonzero, then perform a terminal transfer at RC zero without
another RC update. FIRS (printed p. 4-59) independently corroborates the nonzero
RC decrement loop. Paired with RPT's operand load and operand-plus-one cardinality,
this proves the observable sequence and final zero without inventing a clock edge.

`src/state/repeat_counter_register.[ch]` owns checked typed RC reads and writes
plus the existing dedicated knownness bit. `src/sequencer/repeat/repeat_progress.[ch]`
consumes the existing value-owned `REPEAT_START` effect into a fixed-size,
caller-owned progress value. Start loads the exact operand into RC and widens before
forming the one-through-65,536 body count. Each caller-reported successful body
retirement first proves active phase, count bounds, RC knownness, and
`RC == remaining - 1`; it then decrements nonzero RC once or preserves terminal
zero, and reaches COMPLETE only when the remaining count becomes zero. Every
rejected start or retirement preserves both architectural state and progress.

Mirrored state and repeat tests prove checked RC access, zero/one/two/`0xffff`
progressions, the full 65,536-body completion, unrelated-state preservation, and
transactional null, unsupported-effect, active-restart, forged, unknown, stale,
and extra-completion rejection. The reducer is O(1), allocation-free, value-owned,
and performs no I/O, callback, observation, diagnostics, environment access, or
mutable-global work. It is not yet called by `run.c`, so this slice adds no branch
or carrier growth to the current trusted CPU path.

The existing exact-phase `REQ-COMMON-SINGLE-REPEAT-RC-PROGRESSION` remains
`UNRESOLVED`; the new narrower observable-progression requirement is implemented.
Repeated-body PC flow and predecode interrupt arbitration remain unresolved.
Production RPT catalog rows, body replay, request service, timing, and public API
changes remain outside W48A.

#### P2-W48B: repeat-gated pending IFR latch (implemented)

TI SPRU131G separates request receipt from acknowledgement: receipt sets the
corresponding IFR flag even when the request cannot yet be taken. Its single-repeat
rule defers maskable requests received after RPT decode until every body execution
completes. Section 6.10.1 also identifies reset, explicit write-one-to-IFR, and
`INTR` as clear paths in addition to normal interrupt take. W48B therefore does
not retain the earlier overbroad statement that only normal take can clear IFR.

`src/state/interrupt_flag_register.[ch]` owns knownness-aware reads, full writes,
and checked one-bit set and clear operations over the existing 16-bit IFR storage.
It preserves unrelated flags and every other architectural field. The fixed-size,
caller-owned `src/sequencer/interrupt/repeat_pending_request.[ch]` reducer admits
exactly one caller-selected maskable IFR bit. Receipt latches the bit before consulting the
existing repeat gate; an active gate reports deferral without clearing either the
bit or pending latch. An explicit take is rejected while the gate is active and,
after the remaining-body count reaches zero, clears the matching bit and latch.
Separate reset, IFR-write-one, and `INTR` operations clear matching pending state.

Focused common and project tests prove receipt-before-deferral, preservation at
remaining counts one and 65,536, eligibility and normal take at zero, unrelated
IFR-bit preservation, alternate clears, authoritative reset from unknown IFR, and
transactional null, validation-dependent unknown, forged, stale, duplicate,
invalid-mask, and invalid-count rejection. The reducer calls the existing repeat
request gate rather than duplicating it. It allocates nothing, retains no pointer,
and performs no I/O, observer, diagnostic, environment, or mutable-global work. No
CPU, `run.c`, catalog, trusted packet, public ABI, or benchmark binary changes, so
current CPU execution gains no branch or carrier.

NMI representation, concrete bit assignment, request-priority selection, vector
entry, PC behavior, and requests already pending before RPT reaches decode remain
outside W48B. Production RPT activation therefore remains gated by repeated-body
PC flow and predecode arbitration evidence. Closure accounting is 187 immutable
evidence records; 309 requirements (273 `IMPLEMENTED`, 7 `PLANNED`, 29
`UNRESOLVED`); and 263 tests (256 `PASSING`, 7 `PLANNED`). Catalog and ISA
coverage counts remain unchanged.

#### P2-W48C: DLD repeatability correction (implemented)

A complete audit of the current 100 catalog rows against TI SPRU172C §2.5/Table
2-28 and SPRU131G §6.7/Table 6-18 finds 16 directly supported `INELIGIBLE`
classifications, 83 unresolved classifications, and no positively authorized
`ELIGIBLE` row. The tables enumerate nonrepeatable instructions but do not state
that every omitted form is repeatable. Chapter 3 repeat-execution timing classes
cannot fill that gap because they include instructions separately documented as
nonrepeatable.

`DLD_LONG32_MOD13` was the one unsupported negative: DLD is absent from both
negative lists, and its SPRU172C §4.42 entry does not state a repeat rule. W48C
therefore changes DLD from `INELIGIBLE` to `UNRESOLVED`, not `ELIGIBLE`.
`DST_LONG32_MOD13` remains `INELIGIBLE`, as the negative tables explicitly list
the long-word store. The DLD semantic validator and exact metadata/admission tests
preserve this asymmetry. Both values remain non-admitting through the existing
tri-state repeat-body policy.

The correction changes no opcode identity, encoding, program-word count, layout,
input resource, semantic result, generated lookup population, inventory binding,
CPU routing, carrier, or benchmark binary. It therefore adds no normal or memory
instruction hot-path work. Closure accounting is 188 immutable evidence records;
310 requirements (274 `IMPLEMENTED`, 7 `PLANNED`, 29 `UNRESOLVED`); and 264
tests (257 `PASSING`, 7 `PLANNED`). Catalog and ISA coverage remain 100 rows,
`ID_COUNT=101`, 2,711 lookup entries, 100 bindings, 40/205 family coverage, and
25/205 strict coverage.
