### P2-W48C: DLD repeatability correction (implemented; local verification green)

This heading is the FIT/ownership anchor for the unsupported-DLD
repeatability classification correction. Implementation commit
`b0144d16d4de6d559c26210c356a67031f025094` corrects only
`DLD_LONG32_MOD13` repeatability from `INELIGIBLE` to `UNRESOLVED`. Its
checked semantic shape validator and the DLD/DST metadata/inventory tests
now preserve the exact asymmetry; `DST_LONG32_MOD13` remains `INELIGIBLE`.

Evidence basis (`E-PROJECT-P2-W48C-DLD-REPEATABILITY-CORRECTION`, anchored
in `p2-w20a-rpt-evidence-and-instruction-transaction-architecture.md`): a
complete primary-source audit of TI SPRU172C §2.5/Table 2-28, SPRU131G
§6.7/Table 6-18, and the DLD instruction entry found no DLD prohibition or
positive authorization. The negative tables do not say that every omitted
instruction is repeatable, and the timing-class tables include separately
nonrepeatable instructions. DLD therefore cannot truthfully be either
`INELIGIBLE` or `ELIGIBLE`; `UNRESOLVED` is the only supported state.
Existing immutable evidence was not rewritten.

The catalog holds 84 `UNRESOLVED`, 16 directly evidenced `INELIGIBLE`, and
zero `ELIGIBLE` rows. A new contract proves DLD maps to the distinct
non-admitting unresolved decision, DST maps to reject-ineligible, and both
retain canonical trusted bindings and non-null handlers. No runtime repeat
body or trusted repeat cache is activated without positive evidence.

Ledger-recorded validation (2026-09-09 entry): focused Make C tests pass;
focused CMake/CTest passes 5/5; affected inventory/catalog Python tests
pass 4/4; authoritative `make test` gate passes. Performance and execution
behavior are unchanged: no opcode value/mask, layout, resource, handler
output, lookup entry, inventory binding, CPU route, packet/effect/input
carrier, bus transaction, observer, public ABI, or benchmark binary
changed. No hot-path operation, allocation, I/O, environment access,
mutable global, or knob was added. Runtime RPT remains evidence-gated.
