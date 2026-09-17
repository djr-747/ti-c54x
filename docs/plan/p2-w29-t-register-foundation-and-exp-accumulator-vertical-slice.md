### P2-W29: T-register foundation and EXP accumulator vertical slice (planned)

Implementation status: complete at `fedbd6a`. All seven RED contracts, focused
tests, two independent reviews, the full `make test` gate, and remote CI run
`34062796872` are green. The clean catalog-sixteen performance record is
captured; an identical-binary diagnostic demonstrates substantial host noise,
so it does not establish a confirmed regression.

P2-W29 is a demand-driven two-stage slice. It is the next architecture seam only
because EXP needs a private T destination and the existing effect path has no
candidate-only root transaction. No unrelated register, status, memory, repeat,
interrupt, timing, diagnostic, environment, or emulator foundation is admitted.

P2-W29A owns the private T foundation. `src/state/temporary/` owns only the raw
`uint16_t` storage, explicit knownness, reset physical-zero/architectural-unknown
boundary, and checked private read/write contract. `src/isa/ops/op_effect.h`
centrally owns the value-only `TI_C54X_OP_EFFECT_T_REGISTER_WRITE` carrier;
`src/sequencer/temporary/` owns only its transactional application. The shared
architecture lane, serialized and centrally reviewed by the orchestrator, owns
`src/sequencer/effect_transaction.[ch]`: a tagged union stages only the affected
candidate state, validates all staged effects, and commits atomically.
`src/cpu/` stores and resets private T then delegates staging/commit; it does not
branch on opcode identity, copy an unconditional whole state, expose T publicly,
read T as opcode input, route the T MMR, or touch data memory.

P2-W29B begins only after P2-W29A is GREEN. EXP_ACC/`exp_acc` is a one-word,
S-bit-selected accumulator form with value `0xF48E`, mask `0xFEFF`, the existing
`ACCUMULATOR_SOURCE` layout, and `ACCUMULATORS` input resources. The accumulator
family lane owns only `src/isa/ops/accumulator/` and its pure test leaf. The
catalog/decode lanes own tests only; the CPU proof lane owns its test only. The
orchestrator owns production catalog rows, generated lookup, dispatch, parent
manifests, CPU files, and shared integration. EXP leaves the selected source
unchanged and writes signed `-8..31` as a `uint16_t` T value: zero `0000`, both
signed 40-bit extrema `FFF8`, `+1` `001E`, `-1` `001F`, `+2^31`
`FFFF`, `+2^32` `FFFE`, `-2^31` `0000`, `-53` `0019`, and
`0x0785432105 -> FFFC`. The summary one-cycle field is evidence only; no timing
API or claim is introduced. EXP repeatability remains `UNRESOLVED` because
absence from Table 2-28's nonrepeatable list is not positive eligibility.

Ownership and sequencing are explicit: state lane owns only the temporary state
leaf and `tests/unit/state/temporary/`; T-effect lane owns only the temporary
sequencer leaf and its test; effect-transaction RED/GREEN is a serialized shared
architecture work piece; EXP owns only its accumulator leaf and pure tests;
catalog/decode and CPU lanes own tests only; the orchestrator owns shared files.
Any lane that needs a new layout, T input, MMR/data-memory route, OVA/OVB,
OVM/SXM, memory, RPT/IRQ, parallel issue, timing, diagnostics/environment,
allocation, public API, or emulator seam must stop and report the exact evidence,
missing contract, owner, tests, and hot-path consequence as a GAP.

#### P2-W29B: EXP accumulator vertical slice (planned)

This subheading is the immutable FIT/ownership anchor for the EXP family. It is not a
permission to broaden the T foundation: EXP consumes the generic T write effect
and root transaction only after their tests pass. The completed slice has 16
catalog rows and 1083 populated first-word encodings, with reset-runnable
coverage remaining 4/16; no timing or repeatability result is implied.
