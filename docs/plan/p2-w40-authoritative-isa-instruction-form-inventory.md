### P2-W40: authoritative ISA instruction-form inventory (planned)

This heading is the immutable FIT/ownership anchor for the inventory audit. The
authoritative SPRU172C summary/source-universe checkpoint is §§2.1–2.4, Tables
2-1 through 2-26 (printed pp. 2-2–2-18): 205 syntax forms, 126 normalized
mnemonic tokens, and 13 parallel bundles, with the pinned per-table vector
`[13,13,10,22,6,15,5,5,5,6,5,6,5,2,6,5,5,7,21,14,4,2,4,2,5,12]`.

W40 adds only audit metadata and quality contracts. `config/isa/inventory.json`
will describe one record per syntax form, normalized mnemonic and parallel
bundle metadata, explicit `COMPLETE`/`SUBSET`/`UNIMPLEMENTED` coverage, and
explicit `COMMON_CANDIDATE`/`PROFILE_ONLY`/`OUT_OF_SCOPE`/`UNRESOLVED` scope;
205 is not yet an exact common-ISA denominator until those scope decisions are
evidenced.

`opcode_catalog.def` remains the sole production opcode value/mask source;
inventory records must bind to catalog identity rather than duplicate opcode
authority. Unresolved candidate forms remain in the provisional denominator,
and any exclusion needs explicit evidence. The report is provisional and must
state 35/205 = 17.073%
catalog-family progress and the strict 25/205 = 12.195% complete-form lower
bound; populated-first-word ratios are not ISA coverage.

The planned implementation owns `tools/isa/inventory/schema.py` and
`tools/quality/check_isa_inventory.py`, with RED-first contract tests under
`tests/contract/quality/isa/inventory/`. This work has no runtime, hot-path, or
public-ABI change; timing and profile-specific forms remain outside this slice.

Implementation is complete. The value-owned JSON inventory carries all 205
stable table/ordinal form locators, the exact 126-mnemonic and 13-parallel-form
checkpoints, 35 catalog bindings, and 170 explicitly unimplemented forms. The
current bindings classify 25 forms as `COMPLETE` and ten fixed-MOD2 Smem forms
as `SUBSET`; covered forms are `COMMON_CANDIDATE`, while the remaining 170 keep
scope `UNRESOLVED`. The validator derives table counts from form records,
requires contiguous stable IDs, checks form/binding coverage bidirectionally,
and compares every binding with the sole production catalog without copying
opcode values or masks. Report arithmetic counts distinct form IDs, so aliases
cannot inflate progress. The checker is an explicit Make and CTest quality
gate, and 21 focused schema/count/binding/report tests are green.

Execution history is recorded in `docs/development-ledger.md`. This plan describes
current intent; the ledger records completed work and decisions without rewriting
history.
