### P2-W76: RETF fast-return slice (implemented; local verification green)

This heading is the FIT/ownership anchor for the Table 2-15 fast-return
row. Implementation commit `1f98043` binds one row under `FORM-T2-15-06`:
`RETF` at first-word value `0xF49B` under mask `0xFFFF`, one program word,
one encoding, no operands, reusing `NONE` with the new
`STACK_POINTER_RETURN_REGISTER` inputs and `INELIGIBLE` repeatability.

The slice adds the CPU-internal 16-bit RTN fast-return register (SPRU131G
§6, SPRU172C §4 RETF[D]): checked read/write/knownness accessors,
common-reset zero-but-unknown seeding, copied input collection, and a
`STACK_POP_RTN_WORD16` MEMORY kind whose planned READ never issues. Push
commits stage the pushed return address into RTN atomically (CALL PC+2,
delayed calls PC+4); `RETF` retires with no bus traffic, PC from frozen
RTN, SP+1, and the shared INTM-clearing completion. No new effect kind
beyond the RTN pop address kind, no new address kind beyond it, no timing
claim. `FORM-T2-15-06` lands `SUBSET` with a lone `RETF` symbol under an
`RTN_POP_NONDELAYED` binding.

Ledger-recorded validation (2026-09-16 entry): seven new executable
contracts (metadata with single-encoding disjointness, decode with
near-miss audit, handler staging with SP/RTN gating, CPU retirement with
worked-example vector and CALL-then-RETF chain, input collection, extended
state knownness); `make test` exits 0 with all 379 C test binaries passing;
contract quality unittest fully OK (268); knobs, source boundaries,
manifests, evidence, inventory (159 catalog rows, `ID_COUNT=160`, 6,854
lookup entries, 159 bindings, family coverage 60/205 at 29.268%, 33/205
strict), and observer-disabled structure pass; ASan/UBSan `make test`
exits 0 with no findings. No knob was added or changed. `RETFD` (delayed
fast return) stays deferred, as does the remaining wave-1 tail per the
approved P3 slice order.
