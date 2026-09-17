### P2-W47A: generic LONG32 pair transaction seam (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W47A
activation. W47A is a generic sequencer-only forcing function: it adds no
catalog row, opcode handler, CPU branch, public ABI, or MEMORY effect-carrier
field. It reuses the canonical LONG32 descriptor with `{first=MSW,
second=LSW}` ordering and the private WORD16 reducer, so the pair seam proves
multiword lifecycle behavior without widening an existing architectural
carrier.

The activation contract freezes these decisions before implementation:

- Request order is deterministic: first/MSW, then second/LSW. Descriptors and
  values are copied into the fixed-size private reducer. Each word receives a
  caller-supplied fresh, nonzero, strictly increasing transaction ID;
  `WAIT` resumes the same request with the same ID.
- A first-word fault issues no second request. A second-word fault produces no
  pair read result and no architectural commit. Cancel applies only to the
  active word; a failed cancel preserves reducer state.
- Reads publish only after both words are `READY`. Existing DATA endpoints may
  make each write externally visible: the first write is not rolled back when
  the second fails or is cancelled, while later architectural commit remains
  deferred. W47A makes no external-master exclusion or all-or-none physical
  write claim.
- Malformed, noncanonical, or overlapping pairs reject before endpoint issue.

W47A is implementation-complete. Its private reducer and validation split
preserve the canonical MSW→LSW order, copied descriptors/values, caller-supplied
fresh increasing IDs, same-ID WAIT, first/second FAULT behavior, active-word
cancellation, read publication only after both READY observations, and the
existing endpoint's necessarily partial write visibility. The first write is
not rolled back if the second faults or is cancelled; no external-master
exclusion or all-or-none physical-write claim is made. Malformed,
noncanonical, and overlapping pairs reject before issue.

The four RED lanes are GREEN: shape, read lifecycle, write lifecycle, and
structural hot-path performance. The reducer is allocation-free, fixed-size,
never replans, and constructs exactly two child requests (one in `prepare`, one
in the first-word transition); canonical validation uses direct field checks.
The quality/performance lane is 6/6; native LP64 freezes the request and
transaction at 32/88 bytes, and supported ILP32 at 32/80 bytes. Focused strict
sanitizer validation is 3/3 and the authoritative standalone full gate is
GREEN. No catalog, opcode, CPU, public-ABI, or effect-carrier change was made.
An executable opcode benchmark is deferred to W47B.

W47A closure accounting is 176 evidence records; 294 requirements (257
`IMPLEMENTED`, 8 `PLANNED`, 29 `UNRESOLVED`); 248 tests (240 `PASSING`, 8
`PLANNED`); and 7 sources. The current ISA snapshot remains 98 catalog rows,
2,679 populated first-word entries, 38/205 family progress, and 25/205
strict-complete forms.
