# C54x common-core architecture baseline

Status: active baseline. Primary target: `C54X_CORE_COMMON_V1`.

The machine-readable source of truth is
[`config/evidence.json`](../config/evidence.json). This page states the boundary
that implementation work must preserve; the index gives an ID-by-ID review view.
The current registry contains 2 targets, 7 sources, 78 immutable evidence
records, 115 requirements (91 `IMPLEMENTED`, 9 `PLANNED`, 15 `UNRESOLVED`),
and 82 tests (73 `PASSING`, 9 `PLANNED`).

## Targets and address boundary

`C54X_CORE_COMMON_V1` (`COMMON`, `ACTIVE`) is a device-neutral 16-bit C54x CPU
target. `C54X_VC5402_PROFILE_V1` (`PROFILE`, `PLANNED`) is a later executable
target whose parent is the common target. Only real executable architecture
targets appear in the registry; project governance records use the common target
with `claim_kind: PROJECT_CONTRACT`. The common target allows exactly
`ARCHITECTURAL` and `PROJECT_CONTRACT` claims; the profile target allows exactly
`PROFILE` claims. `PROJECT_CONTRACT` binds implementation and governance records
to the real common execution target, while mandatory `PROJECT_CONTRACT_ONLY`
evidence and test scopes prevent those records from being treated as TI
conformance. No fake project machine target is created.

The public API currently carries a program address in a 32-bit
`ti_c54x_program_word_address`. This avoids imposing a 16-bit storage limit while
the common address model is being established. It is only a carrier and ownership
decision: it does not implement or promise XPC, extended program pages, or a
device memory map. The scalar program/data/I/O word-address typedefs are owned by
`include/ti_c54x/address.h`; this module owns carriers only, not mapping, memory,
XPC semantics, timing, or CPU state.

## Common target scope

The common target includes CPU instruction/register foundations, program-word and
data-word concepts, documented reset effects, and instruction sequencing only when
each behavior has pinned evidence and a test. It excludes XPC and extended pages,
device ROM/RAM maps and wait states, exact IFR/IMR sources and vectors,
peripherals, PLL/clock and electrical behavior, silicon errata, and machine-cycle
or pipeline timing promises. The VC5402 and enhanced-peripheral sources are
profile-only and cannot widen this target implicitly.

### Exact machine exclusions

These project-owned strings intentionally duplicate `config/evidence.json` so the
scope checker can verify documentation parity:

- XPC and any extended program-page or extended-address semantics
- Device-specific ROM, RAM, memory maps, wait states, and boot maps
- Exact IFR/IMR interrupt sources, vector tables, and device interrupt wiring
- Peripherals, PLL and clock setup, electrical behavior, and silicon errata
- Machine-cycle or pipeline timing guarantees until timing evidence and tests are added
- VC5402-specific behavior, which belongs to the separate profile target
- No VC5402 behavior is implied by the common target before this profile is implemented and tested

The common target's program-address carrier has `semantic_scope: CARRIER_ONLY`,
32 bits, and unit `program words`. Its registry claim is exact: “The current
public API uses a 32-bit program-word carrier so the common interface does not
impose a 16-bit storage limit. The carrier is an implementation boundary only; it
does not implement or promise XPC, extended pages, or a device address map.”

## Initial architectural facts

The `SPRU131G` §6.9 reset evidence gives this common subset:

`PC=FF80h`, `IPTR=1FFh`, `IFR=0`, `RC=0`, `ARP=0`, `ASM=0`, `AVIS=0`,
`BRAF=0`, `C16=0`, `C=1`, `CLKOFF=0`, `CMPT=0`, `CPL=0`, `DP=0`, `DROM=0`,
`FRCT=0`, `HM=0`, `INTM=1`, `OVA=0`, `OVB=0`, `OVM=0`, `OVLY=0`, `SXM=1`,
`TC=1`, and `XF=1`. XPC is mentioned only where applicable and remains outside
the common target. SP and remaining status bits are not initialized by the reset
source, so no value is asserted for them (`UNRESOLVED`). Pin, bus, and peripheral
reset behavior is also outside this requirement.

The `SPRU172C` NOP entry identifies word `0xF495`, a one-word operation with no
status-bit effects. Ownership is split explicitly: `src/isa/decode/` recognizes
the word and width, `src/isa/ops/` produces no opcode-owned architectural/status
effects, and `src/sequencer/` owns the sole PC increment. `SPRU131G` §§6.1–6.2
provide the sequential-flow context: the PC is placed on the program address bus
and incremented for the next fetch. Cycle information is deliberately not a
common-core timing promise. The test tree mirrors that ownership split with
separate passing tests for decode, opcode semantics, and sequential flow; no
cross-owner unit test is used as a substitute for those seams.

### NOP-seeded opcode architecture (P2-W16 implemented)

The implemented first ISA architecture slice adds a project-owned `src/isa/catalog/` seam
with one declarative X-macro catalog as the only instruction inventory. Macro
passes derive opcode identity and metadata, a direct 16-bit first-word lookup,
and a function-pointer semantic dispatch table; unknown words classify
explicitly. The catalog contains only the evidenced NOP row (`0xF495`, one
program word). NOP returns an explicit no-opcode-owned-effect result, while the
private sequencer owns checked 16-bit PC `+1` planning and reports an unresolved
boundary without mutation at `0xffff`.

This slice is intentionally independent of the public CPU shell because that
API carries a 32-bit program address and high-carrier/XPC policy is unresolved.
It makes no machine-cycle or pipeline timing, memory/bus, device, or observer
claim and does not copy legacy or external implementation material. It makes no
claim about masked encodings or CPU-shell integration. The project catalog
contract is recorded separately from TI conformance evidence.

### Masked opcode catalog foundation (P2-W17 implemented)

P2-W17 is the implemented infrastructure-only extension of the exact-row
catalog. The NOP `0xF495` row, behavior, and API remain unchanged; no masked or
operand-bearing production instruction is registered. The shared schema under
`tools/isa/catalog/` validates the sole `src/isa/catalog/` inventory and accepts
`EXACT` and `MASKED` rows over canonical 16-bit `value`/`mask` pairs: masked
rows must have a nonzero, non-full mask, values may not set bits outside their
masks, widths are one through 255 program words, IDs and handlers are unique,
and every pairwise overlap is rejected deterministically with no priority rule.

The catalog subsystem owns a deterministic generator at
`tools/isa/catalog/generate_opcode_lookup.py` and a checked-in immutable
65,536-entry `uint16_t` direct lookup derivative at
`generated/isa/catalog/opcode_first_word.inc`. Normal Make, CMake, and consumer
builds consume only checked-in C sources and do not run Python; quality checks
regenerate and reject stale output. Runtime performs no table initialization,
allocation, or mutable global update. The schema and generator are proven by
the passing quality tests under `tests/contract/quality/isa/catalog/`, run
through `make test` and CTest `ti_c54x.quality.python`.

Operand-field layouts/extraction and extension-word fetch are separate future
owners before any real masked instruction. CPU fetch/decode/dispatch/retire,
timing, memory, bus, device, observer, XPC, and external-code/table reuse remain
outside this work piece. Existing TI field evidence is architectural context only;
`E-PROJECT-MASKED-OPCODE-CATALOG-CONTRACT` is explicitly
`PROJECT_CONTRACT_ONLY` and makes no TI conformance claim.

### Private program-PC bridge and CPU NOP transaction (P2-W18 active kickoff)

P2-W18 is the first completed cross-owner execution slice. It connects the existing
public CPU shell to the exact NOP decoder and generated dispatch seam while
keeping the private 16-bit common-PC policy inside `src/sequencer/`. The public
`ti_c54x_program_word_address` remains an exact 32-bit carrier for bus callbacks;
high-carrier words are not rejected or truncated before fetch, and no XPC/page
meaning is inferred. If a fetched and decoded word is a known NOP but its carrier
does not fit the private 16-bit bridge, the bridge stops explicitly without
truncation or retirement. Unsupported carriers, unresolved program boundaries,
and cumulative-counter exhaustion have distinct public stop reasons; malformed
internal transactions continue to fail as invalid state.

The implemented transaction begins an attempt immediately before the program-word bus
read, fetches one 16-bit word, and then classifies it. Consequently, bus faults
and unknown words count as one attempted instruction but do not retire; zero
budget and cumulative-counter preflight failure count zero. A known NOP contributes
no opcode-owned effects, stages sequential PC through the private bridge, and
commits the PC only after semantic and boundary validation. `0xfffe -> 0xffff` is
admitted; a one-word successor staged from `0xffff` is an explicit unresolved
boundary with no retirement or committed-PC mutation. `run(N)` stops after `N`
retirements or the first explicit stop. Counter overflow is an explicit no-wrap
stop covered by the execution-budget test.

Ownership and evidence remain separate: `src/cpu/` owns public orchestration,
fetch, dispatch sequencing, stop/counter reporting, and retirement accounting;
`src/sequencer/` owns the private bridge and transactional PC stage. Existing TI
NOP and sequential-flow records support only the one-word NOP/no-status/sequential
context; the carrier, stop, counter, and transaction rules are project-contract
requirements anchored in the implementation plan. Timing, shared-memory mutation,
interrupts/repeat, operand extension fetch, full ISA coverage, profiles, and
emulator integration remain outside this slice. Operand-field extraction and
extension-word fetch are now proven in the completed P2-W19A/P2-W19B seams.

### Value-owned operand-field extraction (P2-W19A implemented)

P2-W19A is the implemented first half of a deliberately split pre-repeat
foundation. It owns only a pure, allocation-free extractor in
`src/isa/decode/operand_field.[ch]`; completed P2-W19B separately owns
extension-word fetch and width-aware PC span/commit.
The split keeps bus I/O in the `src/cpu/` instruction-fetch owner while
`src/sequencer/` owns sequencing and commit, and keeps both concerns out of the
decode value assembly seam. No production opcode inventory, catalog macro arity,
layout binding, handler, public API, or fetch path changes in 19A.

The extractor accepts a borrowed `ti_c54x_program_words_view`: its
`program_words` pointer names an array of at least `program_word_count`
`uint16_t` elements that remains alive and unchanged for the call. A borrowed
immutable layout similarly borrows an array of at least `piece_count` pieces, while
the output is caller-owned. No pointer is retained, and a caller may safely
discard or mutate its input only after the call returns. Each conceptual piece
names a program-word index, source LSB, width 1..16, and destination/value LSB
0..63. A field contains 1..N pieces and declares a raw width 1..64. Pieces stay
within their source word and declared destination width; destination pieces do
not overlap and cover the declared value exactly; source slices for one field do
not overlap within a source word. First-word, extension-word,
split/non-contiguous, and 64-bit mechanics are represented without sign
extension or semantic interpretation.

Null or invalid views/layouts/outputs, out-of-range word indexes, invalid widths,
overlap, or gaps leave the output byte-for-byte unchanged. The passing test
exhausts every valid one-word source position and width, then covers extension-
word, multiword/non-contiguous, 64-bit, borrowed-input, and complete invalid/
transactional matrices under `tests/unit/isa/decode/test_operand_field.c`. The
seam owns no bus, CPU, architectural state, PC/effect, timing, memory,
interrupt/repeat, observer, diagnostic, file/environment, heap, or mutable-global
behavior. The registry evidence is `PROJECT_CONTRACT_ONLY` mechanics evidence and
makes no TI encoding or instruction-conformance claim.

Performance boundary: this checked general extractor proves descriptor mechanics
without allocation, but production hot paths need not revalidate an immutable
descriptor on every instruction. A future catalog/layout binding may validate
layouts offline and emit or directly use specialized extraction, keeping runtime
descriptor revalidation out of the required hot path. No generator or
specialization is claimed or present here.

The corrected direct-addressing evidence in `SPRU131G` §5.4 records a 7-bit
`dma`. For `CPL=0`, the implemented common requirement is the complete
valid-domain result formed by concatenating the 9-bit `DP` field and `dma` into a
16-bit data-memory word address. For `CPL=1`, the implemented common requirement
covers the complete safe domain where `SP` plus zero-extended `dma` remains in
the 16-bit data-word range. Overflow and wrap behavior remain explicitly
unresolved at the SP boundary. A separate implemented requirement and test own
the source statement that direct addressing leaves `DP` and `SP` unchanged;
that register-effect proof is deliberately not part of either address-formation
claim. The source does not specify AR effects. The sequencer test also proves
that a direct effect selects the tagged DP source only under `CPL=0`, or the
tagged SP source only under `CPL=1`, while preserving the planned data-word
address; that source-selection contract is kept narrow and does not claim
direct long32 behavior.

### Transactional extension fetch and width-aware program PC (P2-W19B active kickoff)

P2-W19B freezes the two private seams needed before any multiword opcode or repeat
instruction can be integrated. It adds no production opcode, handler, catalog row,
or CPU retire behavior. The evidence record is project-contract-only and makes no
TI conformance, machine-cycle, memory, interrupt, repeat, or emulator claim.

Status: implemented and green at commit
`3e57e316bdac5cbb469cba2cc5d5060cc45d1dea`. Focused normal and ASan/UBSan tests
cover both seams, and the full standalone `make -B test` gate passed. The next
slice is evidence-first RPT/IRQ semantics; count-two CPU retirement, timing,
memory mutation, XPC, and emulator integration remain out of scope.

The implemented `src/cpu/instruction_fetch.[ch]` owner collects only extension words
after a first word has already been fetched and decoded. Its allocation-free
by-value result has helper capacity exactly two `uint16_t` program words (not an
ISA-wide maximum), explicit `program_word_count`, status, and exact bus status.
Inputs copy the first word and exact 32-bit `first_program_word_address` and borrow
a bus descriptor/context only for the call; no pointer is retained. Width zero and
every width above two are rejected before any callback. Width one publishes the
first word with zero callbacks and `TI_C54X_BUS_OK` because no bus transaction was
attempted. Validation and overflow paths that make zero callbacks—including null
buses, unsupported widths, and address overflow—report `TI_C54X_BUS_OK` in the
bus-status field for the same reason.
Width two checks `first_program_word_address + 1` before one exact callback, admits
high carriers without truncation, rejects `UINT32_MAX` overflow before I/O, maps
missing callbacks to a bus fault plus `TI_C54X_BUS_NO_CALLBACK`, preserves every
callback failure's exact non-OK bus status, and maps fetch status to bus fault.
Every non-OK path publishes no partial result (zero count and zero words), all
named result fields are initialized deterministically, and no retry occurs. Null
buses, unsupported widths, overflow, callback failure, and callback scribbles are
deterministic. The helper owns no first-word fetch, counters, CPU/PC/state mutation,
common 16-bit/XPC policy, timing/waits, memory, observers, or diagnostics.

The implemented `src/sequencer/sequential_pc.[ch]` and
`program_pc_bridge.[ch]` owners
generalize staged PC effects to explicit `uint8_t` widths 1..255. Both operands
are converted to `uint32_t` before addition for hosts where `int` is 16 bits.
Checked arithmetic rejects count zero and sums above `UINT16_MAX` as unresolved,
with no pre-validation wrap or narrowing. Count one preserves the existing
`0xfffe -> 0xffff` admission and `0xffff` unresolved boundary. Valid, stale, and
malformed effects remain transactional. The public bridge rejects carriers above
`0xffff` without inferring XPC and delegates admitted width arithmetic; it performs
no bus I/O.

P2-W18 CPU orchestration and NOP ordering remain unchanged; no count-two opcode is
registered, so the CPU does not yet consume this helper end-to-end. Later RPT work
will choose the ordering of bridge span validation, extension reads, operand
extraction, and semantics. `last_fetched_word` remains unchanged. Passing proofs
are owned by `tests/unit/cpu/test_instruction_extension_fetch.c` and
`tests/unit/sequencer/test_program_pc_bridge_multiword.c`, with exhaustive width,
boundary, failure, callback, no-retention, and no-mutation cases. Existing P2-W18
CPU tests remain regression gates, and both new tests pass under their owner-local
runners. The boundary matrix explicitly admits `0xfffd + 2 = 0xffff` and rejects
`0xfffe + 2` as unresolved, alongside count zero and all widths through 255.

### RPT transaction architecture (P2-W20A complete; P2-W20B implemented)

P2-W20A pins the first difficult ISA slice before implementation. TI
`SPRU172C` establishes the two immediate forms: one-word `RPT #K` matches
`0xEC00/0xFF00` and carries unsigned K in bits 7:0; two-word `RPT #lk` begins
with exact `0xF070` and carries the complete unsigned 16-bit operand in its
extension word. RPT loads RC with that operand, changes no status bits, and
causes the following instruction to execute operand+1 times, including 65536
executions for `0xffff`. The RPT instruction itself is nonrepeatable.

The execution shape stays owner-driven. `src/isa/decode/` will own one copied
instruction-word bundle plus catalog-bound immutable operand-layout metadata;
CPU fetch only populates the bundle. `src/isa/ops/` will turn the extracted
operand into a discriminated, value-owned `REPEAT_START` effect without touching
state or sequencing. `src/sequencer/repeat/` will own body cardinality,
repeatability admission, and the device-neutral request-admission gate. CPU
orchestration consumes those generic values and must not branch on an RPT opcode
identity. This is intentionally phased as P2-W20B through P2-W20F so no owner is
expanded into a monolith simply to land one instruction.

TI `SPRU131G` establishes the critical gate: after RPT decode, maskable requests
and NMI cannot be serviced until every body execution completes, while literal
RS/reset remains admissible. Requests are still represented in IFR while
acknowledgement is deferred and clear only when taken; exact source bits, vectors,
and priorities remain outside the common target. Repeatability metadata is also
first-class because the TI restrictions exclude documented instruction and
operand/addressing forms from being a repeated body.

The registry leaves three architectural questions unresolved: exact RC decrement
phase and final RC, repeated-body PC hold/reload and final PC, and arbitration of
a request already pending before RPT decode. CPU repeat retirement cannot land
until these are resolved. Machine-cycle cost and detailed pipeline behavior stay
outside this functional slice.

P2-W20B now implements the first seam as a decode-owned, copied instruction bundle
with an explicit helper capacity rather than an ISA-wide size claim. CPU fetch
populates it, completion validates it transactionally against the sole catalog, and
generic dispatch consumes it. This preserves one-callback NOP execution and adds no
opcode-identity branch to CPU orchestration. Catalog activation for RPT remains
deferred until its typed effect has a generic consumption contract, avoiding an
intermediate decode-success/CPU-`INVALID_STATE` architecture.

P2-W20C preserves the dependency direction by storing only scalar layout and
repeatability identities in catalog metadata. Decode owns the immutable layout-set
descriptors and the token resolver; catalog headers never include decoder field
types or retain their pointers. Repeatability is tri-state rather than boolean, so
unresolved evidence cannot silently become an eligible repeated body. Layout sets
support zero, one, or multiple named fields and route every copied value through the
shared field extractor.

P2-W20C1 now implements the scalar half of that contract. The sole X-macro row,
immutable catalog metadata, copied decoded-instruction value, and generic dispatch
all carry the same typed layout/repeatability values. NOP is explicitly `NONE` plus
`UNRESOLVED`; no eligibility inference or RPT classification is made. The direct
first-word lookup remains unchanged.

P2-W20C2 now implements the decoder-owned half. Immutable layout sets support zero,
one, or multiple operand fields and resolve from catalog scalar IDs with exact
program-word-width checks. The shared operand-field validator remains the sole owner
of bit coverage, overlap, width, and source-index rules; layout extraction publishes
copied values transactionally and authenticates its process-lifetime static view.
`NONE`, immediate RPT K, and immediate RPT lk descriptors exist, but both RPT
identities are inert: `0xEC00` and `0xF070` still decode as unknown production words,
and the generated direct lookup is unchanged. The next owner is the pure RPT
operation/effect slice, not CPU opcode-specific control flow.

P2-W20D now implements that effect boundary as a project contract under the
repeat-operation family. A pure builder accepts only an already extracted copied
8-bit K or 16-bit lk field and emits a named, discriminated `REPEAT_START` payload
containing the exact `uint16_t` operand. It does not accept an opcode identity,
decoded-instruction carrier, encoded word, state pointer, or bus, so it cannot own
decode masks or mutate architectural state. It also does not add operand+1;
cardinality belongs to the single-repeat sequencer. Existing production dispatch
and CPU execution remain NOP-only, and the common TI RPT operation requirement
remains planned until the effect has a proven generic consumer.

P2-W20E now implements a similarly narrow project-contract foundation under the
new `src/sequencer/repeat/` owner. Three leaf policies remain separate: checked
`uint16_t` operand to `uint32_t` body-cardinality derivation, admission from the
catalog's existing tri-state repeatability scalar, and a repeat-only request gate
over an explicit remaining-body count plus maskable, NMI, or literal RS/reset
request class. The gate reports only whether repeat blocks a request; it does not
decide general interrupt eligibility.

This pure slice intentionally does not close the common TI cardinality,
body-eligibility, or IRQ-gate requirements. A derived count is not proof that CPU
execution performed that many body retirements, and generic admission over a
preclassified scalar is not proof that specific production rows carry the right
classification. Those common requirements stay planned for integration-level
proof. IFR sampling and mutation stay under the later `src/sequencer/interrupt/`
owner, while RC progression, PC flow, and predecode request arbitration remain
unresolved.

The cardinality proof exhausts all 65,536 input operands and preserves 65536 in
the widened output. Body admission maps all three catalog classes and rejects
forged values without changing caller output. The request gate covers inactive,
first, interior, and maximum remaining counts for maskable, NMI, and literal
RS/reset classes, with explicit range/kind failures. These tests execute through
owner-local Make and CTest registrations; the parent sequencer manifests only
aggregate the repeat subtree.

The P2-W20F evidence-closure review does not authorize CPU integration. A second
pass over official TI `SPRU172C` and `SPRU131G` material adds two narrower facts:
software cannot access RC while single-repeat execution decrements it, and the
repeated MVDK/MVKD forms use a current-`RC != 0` predicate for their address
increment behavior. Neither fact identifies the decrement phase or final RC.
The generic PC, block-repeat, interrupt-receipt, acknowledgement, and pipeline
sections likewise do not define a single-repeat PC hold/reload rule, final PC, or
the arbitration edge for a request already pending before RPT reaches decode.
Those three requirements therefore remain unresolved, production RPT decode stays
inactive, and W20F is deferred until a separately recorded original-hardware vector
or stronger TI evidence resolves the externally visible behavior. A black-box
differential vector may identify a hypothesis or disagreement, but cannot itself
close a normative common-core requirement.

### WORD16 bus-request boundary (P2-W21A implemented)

The shared-memory foundation starts below CPU and opcode execution with a private
`src/bus/request/` value. One request represents exactly one architectural 16-bit
word in PROGRAM, DATA, or IO space and one READ or WRITE access. PROGRAM keeps the
current exact 32-bit program-word carrier; DATA and IO admit only values through
`0xffff`, rejecting wider inputs before narrowing. A read request has a canonical
zero write value, while a write request owns the exact copied 16-bit value.

Width is expressed by the WORD16 request type rather than a one-value enum, and a
request carries no sequencer phase. Multiword composition stays explicit; the later
transaction owner binds a copied request only to transaction identity and lifecycle
state. A separate later `src/sequencer/memory/` owner binds instruction and
memory-effect phases. This boundary owns no callbacks, read result, byte order,
backing memory, waits, issue/completion, arbitration, CPU state, or device policy.
In particular, shared memory is an adapter resource, not a fourth architectural
address-space tag.

The completed leaf returns only `OK`, `INVALID_SPACE`, or `INVALID_ADDRESS`, with
space validation preceding address-domain validation. Invalid construction zeroes
every named request field, while the validator proves both valid and invalid
borrowed requests remain unchanged without making assertions about C padding. The
focused normal and sanitizer tests pass, the full standalone Make gate is green,
and the current public callback ABI and CPU behavior remain unchanged.

### WORD16 byte-order boundary (P2-W21B implemented)

Byte lanes are an explicit conversion policy, not an attribute of the C host or a
global property of the CPU. The private `src/bus/byte_order/` owner converts one
16-bit word to or from exactly two lanes under a per-call BIG or LITTLE selector.
Encode results own both bytes; decode borrows two readable bytes only during the
call and returns a copied word. Invalid order and argument outcomes carry zero named
payload fields.

This helper does not select the encoding of a program image, the physical lanes of
shared RAM, or a serialization format. Those owners must choose byte order
explicitly and may differ. The common core therefore supports both conversions
without claiming that either is the architectural or Nokia device default. Host
casts, unions, packed layouts, and native-layout copies are outside the contract.

The completed private leaf exhaustively round-trips all 65,536 word values under
both orders and independently pins encode and decode lane vectors so mutually
compensating conversion errors cannot hide. It also proves null-before-order status
precedence, zero named error payloads, and borrowed-input nonmutation through both
decode branches. Focused normal and sanitizer tests and the full standalone Make
gate pass; the public ABI and CPU behavior remain unchanged.

### Resumable WORD16 transaction protocol (P2-W21C implemented)

P2-W21C is the implemented private bus leaf under `src/bus/transaction/`, with mirrored
tests under `tests/unit/bus/transaction/`. It is a pure, callback-free,
allocation-free reducer; no public ABI, CPU path, request type, byte-order codec,
host memory pointer, arbitration, device, timing, observer, diagnostic,
environment, or global behavior enters this slice.

The caller declares canonical `{0}` transaction storage. Lifecycle-only states are
`IDLE=0`, `ISSUE_READY`, `IN_FLIGHT`, `COMPLETION_READY`, `COMPLETE`, and `FAULT`.
Address-effect `PRE_ACCESS`/`POST_ACCESS` phases are not reused here; future
`src/sequencer/memory/` work owns memory-effect phase binding. The transaction
object is the pending token and copies a fresh nonzero caller-generated
`uint64_t transaction_id`, a valid W21A request, staged read word, and staged
`ti_c54x_bus_status`. The caller maintains one authoritative storage instance and
must not copy it by value while active. Fresh IDs are a caller precondition for
reuse while stale observations could exist; IDs cannot detect duplicated state
bearing the same ID or deliberate ID reuse.

`prepare` copies a borrowed valid request and ID only from `IDLE` and enters
`ISSUE_READY`. `begin` returns a by-value issue result whose exact named fields are
`status`, `transaction_id`, and `request`; only a success transition to `IN_FLIGHT`
authorizes one external issue. `poll` accepts copied-ID observations with `INVALID`,
`WAIT`, `READY`, or `FAULT` tags: repeated canonical WAIT returns transaction status
WAIT and remains `IN_FLIGHT`, the first READY returns OK, and the first FAULT returns
transaction status FAULT. READY and FAULT each latch one observation and enter
`COMPLETION_READY`; later polls are rejected. `complete` returns a by-value result
whose exact named fields are `status`, `bus_status`, and `read_value_word`. While
IN_FLIGHT, a matching nonzero ID returns WAIT unchanged; zero and nonmatching IDs
still return INVALID_ARGUMENT and STALE_TRANSACTION. With a matching ID in
COMPLETION_READY it publishes one canonical result and enters COMPLETE or FAULT.
Failed issue and completion results zero every named payload field. `reset` permits
only IDLE, COMPLETE, or FAULT and rejects active states, so no cancellation or abort
semantics are implied.

The exact status set is `OK`, `WAIT`, `FAULT`, `INVALID_ARGUMENT`,
`INVALID_STATE`, and `STALE_TRANSACTION`. Null pointers precede canonical state
and lifecycle checks, zero IDs are invalid arguments, nonmatching nonzero IDs are
stale, and observation payload validation is last. FAULT carries only a known
non-OK `ti_c54x_bus_status` (`UNMAPPED`, `FAULT`, or `NO_CALLBACK`); WAIT and READY
carry `TI_C54X_BUS_OK`. WAIT/FAULT read payloads are zero, READY writes publish no
read word, READY reads may publish any WORD16, and failed outputs are zeroed.
Rejected transitions preserve every named transaction field without a padding-byte
claim.

The exactly-once statement is protocol-only: one issue authorization and one
terminal completion per authoritative token under ownership and unique-ID
preconditions. A separate planned external adapter owner must prove actual
callback/MMIO/RAM issue and completion counts and physical visibility across
wait-state suspension; deterministic multi-master arbitration remains another later
proof. W21C does not fabricate an adapter count or claim shared-memory visibility.
The lifecycle and adversarial tests pass as `ti_c54x.bus_word16_transaction` and
`ti_c54x.bus_word16_transaction_contract`.

The completion review recorded TDD RED before implementation, independent
lifecycle/adversarial ownership, no major defect, and closed proof gaps. The
implementation commit is `a3edba970cb2143bfb9d3eb437b9882084352a0d`; production
files are 227/113 lines, headers 115/40, lifecycle test 453, and adversarial test
499. Future growth splits tests rather than exceeding the review threshold.
Actual callback/MMIO/RAM exactly-once visibility, arbitration, timing, memory
effects, CPU/ISA integration, and public ABI behavior remain outside this leaf.
No knobs or integration behavior were added. Follow-up publish/remote
verification precedes the next ISA slice; external adapter work stays deferred
until a concrete memory opcode requires it.

## Single-indirect address-effect slice

The first indirect-addressing slice is deliberately a private, checked planner
under `src/address/`. It accepts a copied ARF-selected index, a 16-bit current
auxiliary-register value, one of `MOD0` through `MOD3`, a canonical word16 or
long32 data-operand descriptor, and a read/write access kind. It returns a
value-owned operand shape plus at most one explicit AR update with a `NONE`,
`PRE_ACCESS`, or `POST_ACCESS` phase. It does not own CPU state, ARP or CMPT
validation, bus transfers, opcode decoding, circular or bit-reversed
addressing, dual-indirect forms, or execution scheduling.

The evidenced word effects are: MOD0 uses the old AR value with no update; MOD1
uses the old value and reports a one-word post-access decrement; MOD2 uses the
old value and reports a one-word post-access increment; and MOD3 reports a
one-word pre-access increment, uses the new value, and accepts writes only. The
returned effect carries a tagged, value-owned AR source snapshot (selected AR
index plus value-before) so ARF selection is testable without pretending that
this planner owns architectural register storage. The
CMPT=0 ARP=0 invariant and absence of ARP writeback are now enforced at the
sequencer application seam: effects tagged `CMPT0_ARP0` require known CMPT=0 and
ARP=0 before the selected-AR update is applied, and ARP is never written back by
that seam. For the implemented long32 slice, MOD0 uses the first data-word
address with no update; MOD1 and MOD2 use that address with post-access
decrement/increment by two data words; and MOD3 is write-only, pre-increments by
two, and uses the updated first address. The long32 operand stores first/MSW and second/LSW
addresses explicitly: even first addresses pair with the next higher word and
odd first addresses with the previous lower word. The data-operand validator
owns canonical width and pair-shape checks; it does not select address wrap.
Circular, bit-reversed, and dual-indirect forms remain planned or unresolved.
The constructor/parity proof is owned by `src/address/data_operand.*`; the
sequencer validation proof is separate under `src/sequencer/` and anchored at
`ti_c54x_sequencer_apply_address_result`.

The project contract is separate from TI claims: checked invalid planner inputs,
unsupported MOD3 reads, and unresolved boundaries return explicit statuses and
all-zero effects. Data-operand constructors return value-owned descriptors, and
the validator returns false for zero or unknown widths and noncanonical
first/second-word relationships. Constructors retain no borrow; the validator
temporarily borrows a read-only descriptor for the call and neither path mutates
caller storage.
Endpoint cases that would overflow an unsigned 16-bit auxiliary address are
rejected under the project contract; this is not a claim that hardware wraps.
The project contract does not elevate those API details into architectural
evidence.

The sequencer consumes that planner result without redoing its arithmetic. Its
private read-only validation entry first rejects invalid result containers,
then delegates state-free operand/source/update/policy/phase checks to the
intrinsic owner and mode/source state reads to the dependency owner. Only after
both owners succeed does it publish either a zero-valued NONE descriptor or one
discriminated AR commit descriptor. A known selected AR must still equal the
effect's expected before value (or the no-update effective address); otherwise
validation returns a stale-state status. Unknown AR state is reported
separately. The application seam owns the sole checked AR write; a valid NONE
descriptor does not mutate state. Every rejection preserves the complete
private state byte-for-byte. This is a project sequencing contract: it does not
add ARP, bus, memory, timing, opcode, or address-boundary behavior, and it
deliberately does not require `value_after` to differ from `value_before` by one
or two.

### AR0-indexed WORD16 slice

The next narrow address slice models CMPT=0 single-indirect MOD5 and MOD6
effects. TI `SPRU131G` §5.5.3/Table 5-4 (printed p. 5-13) records the ordering:
MOD5 uses the old ARx for the operand and then forms ARx−AR0, while MOD6 uses
the old ARx and then forms ARx+AR0. Section 5.5.3.3 (printed p. 5-15) describes
AR0 as indexing another auxiliary register. Existing §5.5.1 and §5.5.5 records
continue to supply the CMPT=0 ARF selection and ARP=0 policy.

The planner admits selected AR1–AR7 only. It uses the old selected-AR value as
the WORD16 operand and schedules one POST_ACCESS update to that same selected
AR, copying the AR0 value as a second source without making AR0 writable by the
effect. MOD5 is admitted only where ARx≥AR0; MOD6 only where ARx+AR0≤FFFFh.
These are safe unsigned domains chosen by the project contract, not a claim of
hardware wrap or circular arithmetic. Selected-AR0 alias ordering is explicitly
unsupported pending stronger evidence. LONG32 indexed operands, MOD4/MOD7 and
other modes, offset/circular/bit-reversed forms, buses, timing, and opcode
integration remain outside this slice.

Together, the COMMON_TI and project-contract address tests cover both access
kinds, all selected AR1–AR7 values, complete single-source dimensions,
representative opposite-source vectors, and exact admitted/rejected boundary
classes. The COMMON_TI test stays inside the admitted safe domain; the project
contract owns checked rejection outside it. This is intentionally not a full
Cartesian claim over both 16-bit source dimensions. The sequencer tests
separately prove the tagged two-source shape, intrinsic validation and
precedence, generic unknown/stale status for either dependency, bytewise
transactional rejection, one selected-AR commit, and AR0 nonmutation.

### Signed long-offset MOD13 slice

The signed long-offset slice models CMPT=0 `*+ARx(lk)` MOD13 effects for both
WORD16 and LONG32 operands. `SPRU131G` §5.5.3/Table 5-4 (printed p. 5-14)
explicitly identifies `lk` as a signed 16-bit offset, adds it to ARx before
access, and uses the resulting value both as the operand address and the
replacement ARx value. Section 5.5.3.2 (printed pp. 5-14–5-15) supplies the
raw extension/update context; it is recorded without selecting instruction
fetch, repeat, or machine-cycle behavior. `SPRU172C` classes 9B and 13B
(printed pp. 3-21 and 3-29) cross-check long-offset indirect long-word read/write
operand forms. Existing CMPT=0/ARP=0 and LONG32 pairing evidence remains the
authority for those independent shape constraints.

The planner copies the raw offset word, performs explicit two's-complement
sign extension, and accepts only mathematical sums in the 16-bit data-word
domain. A successful effect carries a canonical WORD16 or LONG32 descriptor,
the copied selected-AR source, and exactly one PRE_ACCESS update to the
computed value. The project contract rejects invalid fields and unsafe
unsigned endpoints with explicit statuses and an all-zero result; hardware
wrap is not chosen. The sequencer validates this intrinsic shape before the
CMPT=0/ARP=0 and selected-AR state checks, then commits at most the described
selected-AR write without recomputing arithmetic. MOD12 remains an explicit
`UNRESOLVED` signedness requirement because its source text does not classify
the 16-bit carrier as signed or unsigned. MOD4/MOD7 reverse carry, circular
and bit-reversed modes, MOD15, extension fetch, opcode decode, buses, timing,
and emulator integration remain outside this slice.

The MOD13 address test sweeps both access kinds and widths across every
selected AR index and complete single-source dimensions, with independent
cross-source and exact signed-boundary vectors. The effect-application test
separately exercises the tagged source shape, LONG32 parity, mode/source
validation and precedence, unknown/stale selected-AR state, transactional
rejection, and one PRE_ACCESS commit. These are independent source-dimension
sweeps and representative combinations, not a full Cartesian claim over AR
and offset.

### Reverse-carry WORD16 slice

The reverse-carry foundation is isolated in `src/foundation/` and implements
the documented bit-propagation primitive without owning architectural state.
The Table 5-4 record `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7` is used
only for MOD4's old-AR then subtract-AR0 and MOD7's old-AR then add-AR0
ordering; it does not establish width scaling or access restrictions.
`E-TI-C54X-SPRU131G-S5.5.3.5-P5-18-5-19-REVERSE-CARRY` records AR0 as
2^(N-1), left-to-right (bit 15/MSB toward bit 0/LSB) carry propagation, and the
worked sequence. The planner admits WORD16 safe domains and carries value-owned
selected-AR and AR0 snapshots. The sequencer applies at most one post-access
selected-AR update.

The project-only records `E-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT`,
`E-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT`, and
`E-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` own validation and transactional
behavior. Zero/non-one-hot and unsafe results are rejected by those contracts,
not presented as TI behavior. `E-TI-C54X-SPRU131G-S5.5.5-T5-9-P5-23-AR0-SYNTAX`
records `*AR0-0B` and `*AR0+0B` as legal syntax only; selected-AR0 alias
ordering, final propagation/wrap, and LONG32 applicability/update remain
`UNRESOLVED` and are intentionally untested.

## Private architectural register-state seed

`src/state/` owns a private fixed-width register representation used by later
address and sequencer seams. `src/state/register_layout.h` owns the raw ST0,
ST1, and PMST masks; it deliberately uses no C bitfields. ST0 is represented as
ARP[15:13], TC[12], C[11], OVA[10], OVB[9], and DP[8:0]. ST1 is represented as
BRAF[15], CPL[14], XF[13], HM[12], INTM[11], reserved bit 10, OVM[9], SXM[8],
C16[7], FRCT[6], CMPT[5], and ASM[4:0]. PMST is represented as IPTR[15:7],
MP/MC[6], OVLY[5], AVIS[4], DROM[3], CLKOFF[2], SMUL[1], and SST[0]. The
corresponding layout evidence is pinned as
`E-TI-C54X-SPRU131G-S4.1.1-P4-2-4-5` and
`E-TI-C54X-SPRU131G-S4.1.2-P4-6-4-8`.

`ti_c54x_arch_state_reset_common()` seeds only the documented common reset
values: 16-bit PC `FF80h`, raw 16-bit IFR `0`, 16-bit RC `0`, ST0 `1800h`, ST1
`2900h`, and PMST `FF80h` with known masks `FFFFh`, `FBFFh`, and `FFBCh`.
`E-TI-C54X-SPRU131G-S6.2-T6-2-P6-4`,
`E-TI-C54X-SPRU131G-S6.7-P6-20`, and
`E-TI-C54X-SPRU131G-S6.10.1-P6-27-6-28` pin the register widths and reset
actions; the §6.9 record remains the authority for the complete listed reset
subset. The private state keeps separate knownness masks, so zero-filled
AR0–AR7 and SP storage is not treated as an architectural value. Their reset
values are not asserted; callers establish them through checked writes.

The same private state owns checked field access for ARP, DP, CPL, and CMPT.
ARP is the three-bit ST0 field at bits 15–13, DP is ST0 bits 8–0, CPL is the
one-bit ST1 field at bit 14, and CMPT is ST1 bit 5. Valid writes replace only
their field bits and establish field knownness, while unknown or invalid reads
preserve caller outputs and rejected writes preserve the complete state. The
DP/CPL accessors are used by direct-source application but remain private state
contracts; they do not widen the common reset claim or add raw status-write,
timing, bus, or memory behavior.

The direct DP/SP branches of the shared project-owned address-effect application
seam consume those planner results. A planner result carries a value-owned
DP9/dma7 or SP16/dma7 source snapshot and the shared seam checks the tagged
source arm, canonical field ranges, recomputed WORD16 operand, and matching CPL
policy before reading state. Malformed payloads and non-OK planner results are
rejected first. Unreadable CPL, unreadable DP/SP, and mismatching known DP/SP
snapshots retain distinct statuses, and every rejection is bytewise
transactional. These checks prove interface ordering and state ownership only;
they do not claim direct long32, address wrap, buses, timing, or diagnostic
behavior.

The public `ti_c54x_config.reset_pc_word` remains a project bootstrap setting
owned by the CPU shell. It is not the TI hardware-reset value and is intentionally
not migrated or embedded by this work piece. A future integration must preserve
these two policies as separate contracts rather than making one field represent
both a caller-selected start address and the documented `FF80h` reset vector.

## Circular-buffer size register foundation

The common state foundation now carries BK as a private 16-bit architectural
value. TI `SPRU131G` §3.3.4.8 (printed p. 3-28) defines BK as the circular-buffer
size register used by the ARAUs; §3.3.4/Table 3-3 (printed p. 3-27) maps the CPU
memory-mapped register to data-memory address `0x19`, but MMR routing remains a
planned `src/bus/` responsibility. The original BK foundation deliberately made
no circular-arithmetic claim; the later fixed-step slice below consumes separate
§5.5.3/Table 5-4 and §5.5.3.4 evidence without widening BK reset or MMR-routing
behavior.

`src/state/block_size_register.*` owns checked BK read/write accessors with
explicit knownness and rejection statuses. Reset leaves the physical BK storage
zeroed but unknown under the project contract; this work piece makes no TI reset
claim. Memory-mapped register routing remains a separate future ownership slice.

## Circular fixed-step WORD16 foundation

`src/address/indirect_circular.*` owns pure CMPT=0 single-operand WORD16
planning for MOD8 and MOD10. Table 5-4 supplies the old-AR operand followed by
circular decrement or increment and the one-word fixed step; §5.5.3.4 supplies
BK size, alignment, index, and signed-step rules. The planner accepts copied
scalar values only, supports AR0–AR7 for read and write operands, and returns a
value-owned circular source snapshot rather than reading architectural state.

For positive BK, the planner chooses the least `N` for which `2^N > BK`, clears
the low `N` selected-AR bits to obtain EFB, and requires the initial low-`N`
index to be strictly less than BK. It adds `-1` for MOD8 or `+1` for MOD10,
subtracts BK on an upper crossing, and adds BK on a negative crossing. Wider
intermediates and an explicit 32-bit mask make the `N=16` case defined C. An
initial AR outside the documented buffer returns the unresolved-boundary status;
the implementation does not invent normalization or underlying hardware
behavior outside the source's first-access precondition.

BK zero is represented as the documented no-modification case: the old AR still
addresses the WORD16 operand, the circular source retains both AR and BK
snapshots, and the effect carries a canonical NONE update. This deliberately
avoids a same-value register write while still requiring the sequencer to prove
that CMPT, ARP, selected AR, and BK match the planned inputs.

Intrinsic circular shape and arithmetic are revalidated by the address owner.
The sequencer then preserves the shared order: CMPT, ARP, selected AR, and BK.
Unknown and stale selected AR or BK remain distinct transactional failures;
positive BK can produce exactly one selected-AR write and BK is never an update
target. The generic apply owner remains the sole mutation boundary.

This slice does not claim initial out-of-buffer behavior, LONG32 circular step
scaling, MOD9/MOD11 AR0 steps, MOD14 offset behavior, CMPT=1, dual operands,
MMR routing, opcode decode, memory access, buses, repeat/pipeline behavior, or
machine-cycle timing. Those remain explicit unresolved or future requirements.

## Evidence and conformance rules

TI sources are identified by literature number, revision, ISO publication/revision
dates, official URL, scope, and access date. Evidence records are immutable and
carry a narrow locator plus an independently phrased assertion. A correction gets
a new source/evidence ID; Git review preserves the history, but the registry does
not claim to detect historical edits by itself. No manual prose, figures, opcode
tables, implementation code, or data are copied here.

Requirement state and test state are separate: requirements are `PLANNED`,
`IMPLEMENTED`, or `UNRESOLVED`; tests are `PLANNED` or `PASSING`. `PASSING` means
the path is under `tests/`, has a declared runner registered in Make and CTest (or
Python unittest discovery), and the current repository gate executes it. The
commit/CI ledger is run evidence, not a mutable timestamp stored in this registry.
An implemented requirement must link to at least one existing passing test. The
current CPU shell, 40-bit helpers, observer noninterference, and registry
governance rows are `PROJECT_CONTRACT` records, not TI-conformance claims.

Retired identifiers are never silently reused. The corrected direct-addressing
evidence supersedes `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9`; its tombstone records the
ABI-major lifetime and replacement ID.
