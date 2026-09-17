### P2-W31B: transactional data-memory endpoint and fixed-MOD2 load/store pair (planned)

State: W31B1 and W31B2 are implemented with focused strict and ASan/UBSan
tests, independent review, and the full standalone local gate green. P2-W31P's
exact hosted stability follow-up remains open but is no longer a prerequisite.
W31B2 added the two catalog/decode rows, pure generic MEMORY effects, and CPU
end-to-end retirement. Only the immediate-READY benchmark follow-up remains
pending. No catalog row or opcode handler was added during W31B1.

The public boundary is deliberately small and vendorable:

- `include/ti_c54x/bus_endpoint.h` owns the generic WORD16 request, observation,
  status, phase, endpoint, and cancellation vocabulary. A request names
  PROGRAM, DATA, or IO space, READ or WRITE, a `uint32_t address_word`, and a
  write value; DATA and IO addresses are checked against `UINT16_MAX`, and a
  READ has `write_value == 0`. The W31B slice copies one `data_endpoint` by
  value into `ti_c54x_bus`; it enforces DATA requests. `bus.h` owns topology
  only. The unused synchronous public data callbacks are retired; synchronous
  program and IO callbacks remain until their own wait-capable work is
  evidenced. There is no attach/detach API.
- An endpoint descriptor contains one borrowed context pointer, one `advance`
  callback, and one `cancel` callback; the caller continues to own the context
  and everything reachable through it for the CPU lifetime. Both callbacks are
  required or the complete descriptor is the canonical all-zero absent value.
  `ti_c54x_bus_word16_endpoint_advance_fn` returns `void` and receives the
  borrowed context, BEGIN or RESUME, a stable nonzero `uint64_t` transaction
  ID, the same immutable request on every phase, and a caller-owned observation
  output that the core zero-initializes before each callback. Observations are exactly
  INVALID, WAIT, READY, or FAULT: WAIT/READY carry `TI_C54X_BUS_OK`, FAULT
  carries a non-OK status, and a read value is meaningful only for READ READY;
  WAIT, FAULT, and WRITE READY carry a zero read value. INVALID is the all-zero
  sentinel and is never a legal callback result. A missing callback for a
  memory instruction reports `BUS_FAULT` plus `TI_C54X_BUS_NO_CALLBACK` before
  an ID is allocated. Malformed callback output leaves the root transaction
  pending and reports internal invalid state; it is not a TI behavior claim.
- `ti_c54x_bus_word16_endpoint_cancel_fn` receives the same borrowed context,
  active ID, and immutable request, and returns
  `TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED` or
  `TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED`; the all-zero
  `TI_C54X_BUS_WORD16_CANCEL_RESULT_INVALID` is malformed. CANCELLED authorizes the
  private reducer to clear the pending transaction. FAILED or INVALID maps to
  `TI_C54X_ERROR_BUS_CANCEL_FAILED`, preserves CPU and transaction state, and
  permits a later reset/deinit call to retry cancellation. Cancel is called
  only for active pending work, never for IDLE, READY, or FAULT.
- External visibility is commit-at-READY. WAIT, FAULT, and cancellation never
  publish a store; the first READY publishes it once before returning. The
  adapter owns physical storage, mapping, arbitration, and byte-lane conversion.
  Posted-device writes, device/MMR behavior, and arbitration policy are outside
  this slice.
- CPU initialization copies the endpoint once. `ti_c54x_cpu_init` requires fresh
  or successfully deinitialized storage; live-storage overwrite/reinitialization
  is retired and no replacement reinit API is added. `ti_c54x_cpu_deinit`
  returns `ti_c54x_error` (ignoring the return remains valid C); `deinit(NULL)`
  is a successful no-op. Reset and deinit cancel a pending WAIT. Cancellation
  failure is explicit and preserves CPU plus pending transaction state. IDs begin
  at one, monotonically increase, never reset or reuse within one CPU lifetime,
  and exhaustion is detected before issue. Reset preserves that sequence.

The execution seam is a persistent root instruction transaction under
`src/sequencer/instruction/`. It composes the existing bus reducer,
`effect_transaction`, address planner, and program-PC bridge. The CPU owns one
root transaction and resumes it before fetching a new word. A first call that
fetches/dispatches and reaches WAIT reports attempted=1, retired=0; a resume
that remains WAIT reports 0/0; a resume that reaches READY reports 0/1. It
never refetches, redecodes, or reissues. `run(0)` does neither begin nor poll.
`TI_C54X_STOP_BUS_WAIT` is a distinct public stop reason; BUS_WAIT emits no
terminal STOP observer, while FETCH is emitted once. A bus fault is
`BUS_FAULT` with no PC, AR, accumulator, or status commit. This is an
instruction work-unit suspension contract, not a machine-cycle API.

The generic memory carrier is defined in a small owned subtree under
`src/isa/ops/memory/` and included as one tagged `MEMORY` arm of
`ti_c54x_op_effect`; there is no second effect channel. It carries a generic
DATA READ/WRITE, the initial `SINGLE_INDIRECT_WORD16` MOD2+ARF address
specification, a store word, and a static process-lifetime completion callback.
It carries neither a resolved address nor AR/ARP/CMPT copies.
`src/sequencer/memory/` resolves that spec against the embedded `arch_state`,
using the old selected AR and staging the post-access AR+1 before issue. The
completion callback receives the copied decoded instruction, copied op-input,
and READY read word, and returns an ordinary next effect; the sequencer never
switches on LD/STL identity or a transform enum. LD owns the callback and uses
captured D plus SXM to emit an accumulator write. STL precomputes the
accumulator low 16 bits and has no completion callback. All PC, address/AR,
counter, and effect candidates are validated before BEGIN so a READY store can
commit atomically. After WRITE READY, no fallible validation or external
callback remains: publication uses only the already-prepared AR, PC, and
retirement assignments. Forged or invalid address, effect, PC, and counter
candidates are rejected before BEGIN. WAIT, FAULT, and cancellation leave
architectural state untouched.

The first evidence-pinned pair is deliberately narrow:

| Form | Encoding contract | Semantic contract |
|---|---|---|
| `LD *ARx+,dst` | value `0x1090`, mask `0xFEF8`; D bit 8, ARF bits 2:0, fixed I=1/MOD2; one program word and 16 first-word encodings | Read the old selected AR, zero-extend the 16-bit word when SXM=0 or sign-extend it into the 40-bit accumulator when SXM=1, then postincrement AR by one word. No encoded shift. |
| `STL src,*ARx+` | value `0x8090`, mask `0xFEF8`; S bit 8, ARF bits 2:0, fixed I=1/MOD2; one program word and 16 first-word encodings | Store the accumulator low 16 bits, then postincrement AR by one word. This unshifted form has no SXM-dependent transform. |

Both forms require CMPT=0 and the existing ARP=0 policy. The old selected AR
is used for the transfer and the post-access AR is committed only with READY.
`AR=0xffff` boundary behavior remains unresolved and is rejected by the current
checked planner. After W31B2 the expected catalog is 24 rows and 1319 populated
first words, with reset-runnable coverage still 4/24; these are lookup
population counts, not a distinct-instruction count or timing claim.

Ownership and TDD order mirror the tree. RED tests are created first, then each
owner goes GREEN without editing another owner’s files:

1. `tests/unit/bus/adapter/test_word16_endpoint_contract.c` proves the copied
   public endpoint shape and canonical callback protocol. The existing
   `tests/unit/bus/adapter/test_word16_exactly_once.c` is activated for the
   external visibility/issue proof; `tests/unit/bus/transaction/` owns reducer
   cancellation.
2. `tests/unit/sequencer/instruction/test_memory_instruction_lifecycle.c` and
   `test_memory_instruction_commit.c` prove persistent
   WAIT/RESUME/READY/FAULT/cancel and publication phases, while
   `tests/unit/sequencer/memory/test_memory_address_resolution.c` proves the
   generic MOD2 address dependency and staged AR result.
3. `tests/unit/cpu/memory/test_cpu_memory_lifecycle.c` and
   `test_cpu_memory_pending.c` prove endpoint copy, resume-before-fetch,
   counter phases, BUS_WAIT, reset/deinit cancellation, re-entry exclusion,
   and monotonic IDs.
4. Catalog and decode owners prove the two rows in
   `tests/unit/isa/catalog/test_fixed_mod2_memory_metadata.c` and
   `tests/unit/isa/decode/test_fixed_mod2_memory_decode.c`.
5. `tests/unit/isa/ops/load/test_ld_smem.c` and
   `tests/unit/isa/ops/store/test_stl_smem.c` prove family-local pure effects;
   `tests/unit/cpu/isa/load/test_cpu_ld_smem.c` and
   `tests/unit/cpu/isa/store/test_cpu_stl_smem.c` prove end-to-end retirement.

The common target does not claim machine-cycle cost, repeatability,
`AR=0xffff` wrap, device/MMR/program/IO spaces, byte-order policy beyond the
adapter contract, dual data reads, parallel issue, posted writes, or any
fault/recovery behavior not represented by the project endpoint protocol.
Those remain explicit GAP or UNRESOLVED work for later evidence freezes.
