### P2-W21: shared-memory transaction foundation

State: P2-W21A, P2-W21B, and P2-W21C implemented and green. External adapter
visibility and arbitration remain planned but are introduced only when a concrete
memory opcode requires them.
This work builds the independent gate required before any instruction may mutate
memory. The core remains word-native and host-endian-neutral; sharing, storage,
device mapping, and arbitration stay outside the CPU instance.

The owner hierarchy is phased so no early value accumulates unrelated policy:

1. P2-W21A creates `src/bus/request/` and the mirrored
   `tests/unit/bus/request/` subtree for one pure WORD16 request value.
2. P2-W21B creates `src/bus/byte_order/` for explicit BE and LE conversion of one
   word to two byte lanes. Program-image encoding, physical shared-memory lanes,
   and snapshot/wire formats remain separate policies rather than one global endian
   flag.
3. P2-W21C creates `src/bus/transaction/` for a copied request, caller-owned
   transaction object/pending token, lifecycle-only state, and a begin/poll/complete
   reducer with protocol-only exactly-once issue and completion across READY, WAIT,
   and FAULT outcomes. External visibility and arbitration remain a separate planned
   adapter owner.
4. Only after that lifecycle is proven may `src/sequencer/memory/` own declarative
   memory effects and ordered visibility. External adapters arbitrate DSP, ARM, DMA,
   and device masters; the core never owns a host mutex or master identity.

The existing synchronous public callbacks and `ti_c54x_bus_status` remain unchanged
during the private foundation. A later one-shot adapter may wrap them, but WAIT is
not appended to the current ABI and no new behavior is routed through `cpu.c` yet.

#### P2-W21A: private WORD16 bus-request contract (active kickoff)

The first slice owns only the canonical command for one architectural 16-bit word.
`src/bus/request/word16_request.[ch]` defines zero-sentinel space and access enums,
an exact `uint32_t address_word`, and a copied `uint16_t write_value_word`. Separate
read and write constructors return a by-value status plus request: PROGRAM preserves
the full 32-bit carrier; DATA and IO reject values above `UINT16_MAX` before any
narrowing; READ always publishes a canonical zero write value; WRITE preserves the
exact input value. Construction reports exactly `OK`, `INVALID_SPACE`, or
`INVALID_ADDRESS`, validating the space discriminant before its address domain.
Invalid construction zeroes every named request field, which leaves both
discriminants invalid; no padding-byte value is promised or tested. A borrowed
request validator retains no pointer and rejects null, zero, forged, range-invalid,
or noncanonical values.

WORD16 is fixed by the type, so W21A adds no redundant transfer-width enum.
Multiword operands will compose separately ordered word requests rather than claim
an accidentally atomic host `uint32_t` transfer. W21A also carries no phase: W21C
binds a copied request only to transaction identity and lifecycle state. A later
`src/sequencer/memory/` owner binds instruction and memory-effect phases, keeping
pure request shape separate from both lifecycle and semantic phase policy.

The planned TDD proof covers all six space/access combinations; zero, maximum, and
full-width program carriers; DATA/IO zero and `0xffff`; rejection of `0x10000` and
larger DATA/IO inputs without truncation; copied write boundaries; canonical read
payload; zero/forged discriminants; invalid-result shape; borrowed-validator nulls;
and named-field zeroing on each failure. Source-boundary checks prove that the
callback-free request owner imports no host I/O surface. Source and test manifests
mirror `bus/request/`; root Make/CMake files only aggregate the new bus owner.

This is a project contract, not TI behavior. It adds no public API, callback, read
completion, byte-order conversion, host memory, raw pointer, transaction token,
phase, wait state, issued/completed state, timing, arbitration, memory map, opcode
effect, CPU/state mutation, observer, diagnostic, knob, environment read, allocator,
mutable global, firmware, or emulator dependency. Exit requires the registered
project test to pass through both supported build systems with current CPU behavior
unchanged.

The completed private leaf is 102 lines of implementation, 77 lines of interface,
and 247 lines of focused proof. Its constructors and validator preserve the frozen
status precedence and address domains, and validation is non-mutating for both valid
and invalid borrowed values. Nested Make/CMake manifests mirror the bus/request
owner; root files add aggregation lines only. The full standalone Make gate passes.
No public callback or CPU execution path consumes the request yet.

#### P2-W21B: explicit WORD16 byte-order codec (implemented)

This slice creates the separate `src/bus/byte_order/` and mirrored
`tests/unit/bus/byte_order/` owners. `word16_codec.[ch]` converts one copied
`uint16_t` word to exactly two value-owned `uint8_t` lanes, or two borrowed readable
lanes to one copied word, under an explicit zero-sentinel `INVALID`, `BIG`, or
`LITTLE` byte order. BIG maps `0x1234` to `{0x12, 0x34}` and LITTLE maps it to
`{0x34, 0x12}`.

Encode and decode return separate by-value result types. Their only statuses are
`OK`, `INVALID_ARGUMENT`, and `INVALID_ORDER`. Encode accepts no pointer; decode
borrows exactly two bytes until return and retains none. Decode validates a null
input before the order, so null plus a forged order reports `INVALID_ARGUMENT`.
Otherwise a zero or forged order reports `INVALID_ORDER`. Every failed result zeroes
its named byte or word payload fields without making a C-padding claim.

The implementation uses unsigned shifts, masks, and explicit fixed-width scalar
conversions. Pointer/type-punning casts, unions, packed structs, native-layout
`memcpy`, and host-order inference are forbidden. There is no mutable or immutable
global "current endian" setting: program-image encoding, physical shared-memory
lanes, and snapshot/wire formats remain three separately owned adapter/profile
policies that select this codec explicitly.

The TDD proof exhausts every `uint16_t` value through encode-then-decode for both
orders, checks exact lane vectors for `0x0000`, `0x1234`, and `0xffff`, covers invalid
and forged orders, proves the decode null-before-order precedence, checks failed
named payload fields, and snapshots borrowed input lanes before and after decoding.
Parent bus manifests aggregate the new child; the already present root bus
aggregation is unchanged.

This is a `PROJECT_CONTRACT` conversion policy, not evidence that C54x, Nokia,
program images, shared RAM, or any device use one physical byte order. It adds no
public API, request change, callback, host memory, transaction, phase, wait,
arbitration, memory effect, CPU/state/ISA behavior, observer, diagnostic, knob,
environment read, allocator, mutable global, firmware, or emulator dependency.
The completed private leaf is 93 lines of implementation, 58 lines of interface,
and 170 lines of focused proof. The proof exhausts both orders and independently
pins external encode and decode vectors, closes null-before-order precedence and
named error-payload behavior, and verifies borrowed-input nonmutation through both
branches. Owner-local Make and CTest registrations mirror the production tree.
Focused normal and sanitizer tests and the full standalone Make gate pass while
W21A, the public ABI, and current CPU behavior remain unchanged.

#### P2-W21C: resumable WORD16 transaction protocol (implemented)

This implementation owns the private `src/bus/transaction/` leaf, mirrored by
`tests/unit/bus/transaction/`. Production leaves are `word16_transaction.[ch]`
and `word16_transaction_validate.[ch]`; `test_word16_transaction.c` proves
lifecycle behavior and `test_word16_transaction_contract.c` proves adversarial
validation. Ownership is explicit in nested Make/CMake manifests and parent
aggregation. The implementation changes no public ABI, CPU path, request type,
codec, or adapter behavior.

The reducer is pure, callback-free, allocation-free, and host-storage-free. The
caller declares transaction storage as canonical `{0}`. Its lifecycle vocabulary
is deliberately not an address-effect phase vocabulary:
`IDLE=0`, `ISSUE_READY`, `IN_FLIGHT`, `COMPLETION_READY`, `COMPLETE`, and `FAULT`.
`PRE_ACCESS` and `POST_ACCESS` remain owned by address effects; future
`src/sequencer/memory/` work binds instruction/memory-effect phases separately.

The transaction object is the caller-owned pending token and copies a fresh,
caller-generated nonzero `uint64_t transaction_id`, a valid W21A request, a staged
read word, and a staged `ti_c54x_bus_status`. The caller must provide a fresh ID
for each reuse while stale observations may exist; generation, wrap, and identity
allocation are external policies. Active transaction storage must not be copied or
used as a second authoritative value while active. Ordinary pointers to the same
storage do not create a second authority. An ID cannot detect a copied state bearing
the same ID or deliberate ID reuse, so those are explicit caller preconditions
rather than implementation claims.

The planned header-level API is:

- `prepare(tx *, borrowed valid request *, id)` copies the request and ID only from
  canonical `IDLE` and enters `ISSUE_READY`.
- `begin(tx *)` returns a by-value issue result with exactly the named fields
  `status`, `transaction_id`, and `request`. Only success enters `IN_FLIGHT` and
  authorizes one external issue; failed issue results zero `transaction_id` and
  every named request field.
- `poll(tx *, borrowed observation *)` accepts the copied ID and zero-sentinel
  `INVALID`, `WAIT`, `READY`, or `FAULT` observation. Repeated canonical `WAIT`
  returns transaction status `WAIT` and remains `IN_FLIGHT` without another begin
  or issue authorization. The first canonical `READY` returns `OK`, or the first
  canonical `FAULT` returns transaction status `FAULT`; either latches one
  observation and enters `COMPLETION_READY`. Later polls are rejected.
- `complete(tx *, id)` returns a by-value result with exactly the named fields
  `status`, `bus_status`, and `read_value_word`. While `IN_FLIGHT`, a matching
  nonzero ID returns transaction status `WAIT` and leaves the transaction unchanged;
  zero and nonmatching IDs still return `INVALID_ARGUMENT` and
  `STALE_TRANSACTION`, respectively. With a matching ID in `COMPLETION_READY`, it
  returns transaction status `OK`, bus status `TI_C54X_BUS_OK`, and a read word only
  for a READ/READY transaction, or transaction status `FAULT`, the exact staged
  non-OK bus status, and a zero read word. It then enters `COMPLETE` or `FAULT`;
  duplicate completion is rejected. Every failed completion result zeroes its
  `bus_status` and `read_value_word` fields.
- `reset(tx *)` clears only canonical `IDLE`, `COMPLETE`, or `FAULT` to zeroed
  `IDLE`; it rejects `ISSUE_READY`, `IN_FLIGHT`, and `COMPLETION_READY`, so it
  cannot imply cancellation. There is no abort operation in this contract.

Statuses are exactly `OK`, `WAIT`, `FAULT`, `INVALID_ARGUMENT`, `INVALID_STATE`,
and `STALE_TRANSACTION`. Null pointers are checked first, then canonical state
and lifecycle, then zero IDs as invalid arguments, then nonmatching nonzero IDs as
stale, and finally observation tag/payload shape. `WAIT` and `READY` observations
carry `TI_C54X_BUS_OK`; `FAULT` accepts only the existing known non-OK
`UNMAPPED`, `FAULT`, or `NO_CALLBACK` public bus statuses. WAIT/FAULT read payloads
are zero, READY writes publish no read word, and READY reads may publish any
`uint16_t`. Every rejected transition preserves all named transaction fields; no
C-padding-byte claim is made.

Exactly-once is a protocol claim only: one issue authorization and one terminal
completion per authoritative token under the caller ownership and unique-ID
preconditions. A separate planned `src/bus/adapter/` owner and
`tests/unit/bus/adapter/test_word16_exactly_once.c` must prove actual callback,
MMIO, or RAM issue and completion counts and physical visibility across wait-state
suspension. Deterministic multi-master arbitration remains another separately owned
later proof. W21C fabricates no adapter count and claims no host memory, device,
timing, observer, diagnostic, environment, global, CPU, request, or codec behavior.

The TDD matrix covers all six space/access combinations, copied request and source
mutation, one begin, repeated WAIT, READY reads `0`, `0x1234`, and `0xffff`, READY
write zero payload, all three public fault statuses, one completion, reset/reuse
with a fresh ID, and stale observations. The adversarial contract proof covers
canonical and forged states/requests, nulls, zero or forged observation kinds and
bus statuses, noncanonical payloads, active-reset rejection, precedence, named
field nonmutation, and zero failed issue/completion payloads. Focused normal and
ASan/UBSan tests pass with runners `ti_c54x.bus_word16_transaction` and
`ti_c54x.bus_word16_transaction_contract`; the full standalone gate is green.

The completion review recorded TDD RED before the production leaves existed,
independent lifecycle/adversarial ownership, no major defect, and closed proof
gaps. Production files are 227/113 lines, headers 115/40, lifecycle test 453,
and adversarial test 499; future growth splits tests rather than exceeding the
review threshold. The implementation commit is `a3edba970cb2143bfb9d3eb437b9882084352a0d`.
No knob or integration behavior was added. Actual callback/MMIO/RAM exactly-once
visibility, arbitration, timing, memory effects, CPU/ISA integration, and public
ABI behavior remain outside this leaf. Follow-up publish/remote verification is
required before the next ISA slice; external adapter work stays deferred until a
concrete memory opcode requires it.
