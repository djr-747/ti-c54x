# ti-c54x

`ti-c54x` is a standalone C11 implementation of the TI C54x DSP foundations. It
is being developed as a small, test-first library that can eventually be vendored
by emulators and other tools without pulling in a phone model, firmware image, or
host application.

The repository is intentionally independent of the Nokia DCT3 emulator. The
emulator may provide optional black-box traces and externally supplied DSP images
for later validation, but it is not a build or test dependency. Integration is
deferred until the standalone contracts and provenance review are complete.

## Status

Foundations are under active development. The public API, supported C54x variant,
opcode inventory, and distribution license are not final. In particular, the
provenance and licensing review is still open; this repository does not currently
make a license-selection or compatibility claim. See the [provenance review](docs/provenance.md),
[architecture baseline](docs/architecture-baseline.md), and [evidence index](docs/evidence-index.md).
The machine-readable [`config/evidence.json`](config/evidence.json) registry is checked by
the standalone Python quality gate.

The standalone foundations and generated exact/masked decode infrastructure are
green. Only the evidence-backed NOP is registered, and its semantics remain
private to the core. The end-to-end CPU NOP transaction is implemented and green:
fetch, decode, dispatch, private program-PC staging, and transactional retirement
are covered by executable tests. P2-W19A is implemented and green: the checked,
value-owned operand-field extractor proves exhaustive one-word slices plus
multiword, non-contiguous, 64-bit, borrowed-input, and transactional cases.
P2-W19B is implemented and green: the standalone transactional extension-word
fetch helper and width-aware program-PC span/commit seams are covered by focused
CPU and sequencer tests. It adds no production opcode, handler, catalog row, or
CPU count-two retirement integration. P2-W20A is complete: TI evidence now pins the
two immediate RPT forms, operand+1 execution cardinality, repeatability rules, and
post-decode IRQ/NMI deferral with the reset exception. P2-W20B is implemented and
green: one decode-owned copied instruction bundle now moves through fetch, checked
completion, and generic dispatch while preserving one-callback NOP behavior. The
CPU gained no opcode-specific branch. P2-W20C1 is implemented and green: every
catalog row now owns explicit scalar layout/repeatability metadata, carried through
the copied decoded value. P2-W20C2 is implemented and green: decode now owns
immutable zero/one/multi-operand layout sets, shared descriptor validation, and
transactional copied extraction. The one- and two-word RPT immediate layouts are
proven but remain inert; production decode still registers only NOP. P2-W20C is
complete. P2-W20D is implemented and green as a narrow project-contract proof for
a typed, value-owned `REPEAT_START` effect; it does not claim RC mutation or
activate RPT in production decode. P2-W20E is implemented and green as three pure,
owner-separated repeat-sequencer policy proofs for exhaustive `uint32_t`
cardinality derivation, preclassified-body admission, and repeat-only request
gating. Actual body execution, production classification, and IRQ integration
remain planned. A follow-up TI evidence audit confirmed that the manuals do not
define the decrement timing edge or final visible RC required by this integration
contract, single-repeat PC flow, or arbitration for a request already pending
before RPT decode, so W20F CPU integration is deliberately deferred rather than
guessed. The broader
implementation is split
into canonical instruction-bundle, catalog-layout, opcode-effect, repeat-control,
and later CPU-integration proofs; unresolved RC/PC/arbitration details are not
guessed. P2-W21A is implemented and green as a private, value-owned WORD16
bus-request foundation. P2-W21B is implemented and green as a separate exhaustive
BIG/LITTLE WORD16 byte-lane codec; resumable transactions begin next under P2-W21C,
while arbitration and memory effects remain later owners. Timing, shared-memory
mutation, the full ISA, device
profiles, and emulator integration remain deferred. See the
[implementation plan](docs/implementation-plan.md) and [development
ledger](docs/development-ledger.md) for the current work state.

## Build and test

The C-only compile/link path requires only a C11 compiler and `make`:
`make build` does not invoke CMake, Python, the emulator, firmware, or a DSP
image. The default `make test` gate additionally runs the repository's Python
3 standard-library quality checkers and therefore requires a `python3`-compatible
interpreter (override it with `PYTHON=/path/to/python`). CMake 3.21 or newer and
CTest are used for packaging, IDE/CI validation, and the same Python quality
checks when tests are enabled.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The equivalent fast path is:

```sh
make test
```

For the compiler-only path, use:

```sh
make build
```

Useful options are:

```text
TI_C54X_BUILD_TESTS=ON             standalone unit tests (default)
TI_C54X_BUILD_EXAMPLES=OFF        minimal consumer example (default while API settles)
TI_C54X_ENABLE_SANITIZERS=OFF     AddressSanitizer + UBSan where supported
TI_C54X_ENABLE_OBSERVERS=OFF      structured lifecycle observers (test/diagnostic builds)
TI_C54X_WARNINGS_AS_ERRORS=ON     strict warnings (default)
PYTHON=python3                     quality-check interpreter used by `make test`
```

For example:

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTI_C54X_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Observer callbacks receive borrowed, structured event values only. The core
copies the event mask and sink descriptor during initialization; the sink
context and callback must remain valid until deinitialization. Callbacks must
not re-enter or mutate the running CPU. Observer support is disabled in the
default library build and is exercised by a separate enabled test variant.

The install tree exports the CMake package target `ti_c54x::core`. A consumer
should link that target and include only the installed public headers; internal
implementation headers are not part of the vendor surface.

The current implementation lineage and evidence boundary are pinned in
[`docs/provenance.md`](docs/provenance.md); the distribution-license decision
remains open pending the documented provenance review.

## Design boundaries

The core owns architectural state and instruction semantics. A caller owns memory,
I/O, device timing, logging, and persistence through explicit interfaces. Runtime
state is instance-owned: there are no environment reads, file loads, mutable
process-wide emulator hooks, or hidden firmware paths in the execution path.

Requirements are recorded with evidence identifiers and are accompanied by
executable tests. Pure arithmetic and address helpers are tested before opcode
families; decode coverage, sequencing, interrupt behavior, and timing are added as
their specifications become pinned. The dispatcher follows the mGBA pattern of a
declarative instruction definition expanded into generated wrappers and dispatch
tables, while keeping semantic implementations readable and independently
testable. The [architecture baseline](docs/architecture-baseline.md) describes
the current ownership contracts, while the [evidence index](docs/evidence-index.md)
maps requirements to their sources and tests. `config/evidence.json` is the
machine-checked counterpart; it is validated without emulator or firmware gates.

## External inputs

Tests that need a firmware or DSP image will accept an explicit path and skip when
one is not supplied. Images are never committed, downloaded, or embedded by the
build. This keeps a clean checkout fast and reproducible.
