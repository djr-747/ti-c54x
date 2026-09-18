# Implementation runbook

How a vertical slice gets built in this repository. This distills
`AGENTS.md` plus the procedure every recent slice (W50–W59) actually
followed, so future sessions do not re-infer it. When this document
and `AGENTS.md` disagree, `AGENTS.md` wins.

## 1. The slice recipe

One instruction form (or one infrastructure piece) per slice, in
this order. Do not skip steps; do not reorder RED and GREEN.

1. **Research.** Pin the TI ground truth first, dual-sourced where
   the repo already does so: SPRU172C (mnemonic) plus SPRU179C
   (algebraic) for opcodes/semantics, SPRU131G for addressing.
   Use the vendored text set (`vendor/ti/*.txt`, see
   `vendor/ti/README.md`) — never re-fetch from the web when the
   vendor copy exists. Verify bit diagrams numerically (expand
   every bit to hex independently; a misread low byte cost a full
   rework in W57). Never invent opcode values; never copy the
   legacy emulator core (black-box differential oracle only, later).
2. **Plan anchor.** Append an immutable `(planned)` section to
   `docs/implementation-plan.md` **before** the `### P2-W21` line.
   It names the form, value/mask/encodings, layout, resources,
   repeatability, semantics with pinned vectors, ownership
   requirements, the four RED test paths + CTest names, and
   projected closure numbers. Numbers are projections until the
   slice lands; verify each before committing.
3. **Evidence.** Add records to `config/evidence.json` preserving
   its exact style (4-space items, compact nested dicts):
   TI opcode record, TI semantics record (with caveats, e.g. "no
   worked example pins…"), T2-28 repeatability *boundary* (never a
   positive rule), project plan-anchor + activation records, six
   requirements (CATALOG, DECODE, EFFECTS, TRUSTED-BINDING,
   CPU-RETIREMENT, INVENTORY), four test records; extend the two
   back-link arrays; update `FORM-*` + `catalog_bindings` in
   `config/isa/inventory.json`; mirror every row in
   `docs/evidence-index.md` (evidence, requirements, tests tables
   plus the two back-link rows).
4. **RED.** Write the four test files (catalog metadata, decode,
   ops semantics, CPU retirement), register each in `tests.mk` and
   `CMakeLists.txt`, build one target, and observe the authentic
   failure (absent identity) before implementing.
5. **GREEN.** Catalog row, layout, dispatch include, handler,
   manifests, regenerated lookup. Smallest diff that satisfies
   the RED contracts; mirror the closest sibling slice.
6. **Fixtures.** Bump every count the new row moves (see §5).
   Let test failures guide you, but prefer the full list below.
7. **Validation** (§6). All gates, not just `make test`.
8. **Commits.** Atomic implementation commit, then a ledger-only
   commit (§7).

## 2. Ground-truth rules

- Cite TI chapter/section/printed page in every TI record.
- Record negative evidence explicitly ("Examples 1–6 cover
  syntaxes 1, 2, 3, 6, 10 only").
- Status-bit notes in the manual are generic across syntaxes;
  follow the closest *evidenced sibling slice* for fixed-vs-
  variable-shift OVM treatment, and document the decision.
- Assembler notes ("assembles as syntax N") constrain the
  assembler, never decode.
- Repeatability: Table 2-28 lists only what it lists. Boundary
  records only; `UNRESOLVED` unless a positive rule exists.

## 3. Test lanes and naming

| Lane | Path pattern | CTest name pattern |
|---|---|---|
| Catalog metadata | `tests/unit/isa/catalog/test_<sym>_metadata.c` | `ti_c54x.<sym>_metadata` |
| Decode | `tests/unit/isa/decode/test_<sym>_decode.c` | `ti_c54x.<sym>_decode` |
| Ops semantics | `tests/unit/isa/ops/<fam>/test_<sym>.c` | `ti_c54x.<sym>` |
| CPU retirement | `tests/unit/cpu/isa/<fam>/test_cpu_<sym>.c` | `ti_c54x.cpu_<sym>` |

- Metadata: exact value/mask/count/layout/resources/repeatability,
  `ID_COUNT`, 2-or-N population, full 65,536-word disjointness
  sweep, `~mask` identity.
- Decode: every encoding decodes with bundle copy + extraction
  order; malformed inputs reject transactionally (memcmp-clean
  output); fixed-bit flips pinned — sibling rows by identity,
  all others `UNKNOWN_WORD`. **Cross-slice hazard:** a flip that
  is UNKNOWN today may become a valid row tomorrow (W57→W58);
  the activating slice updates the older test.
- Ops: independent oracle (recompute, never copy production),
  exhaustive over the varying domain where affordable, hostile
  seeds with memcmp parity across checked/trusted/dispatch
  paths, unknown-input rejection, per-row identity forgery
  rejection, trusted-operand rejection.
- CPU: local bus fixture (program reads + endpoint counts),
  retirement shape, state preservation, unknown-input rejection.
  Fixture counters must be reset between scenarios on a shared
  fixture (W58 bug class).

## 4. Implementation seams

- `src/isa/catalog/opcode_catalog.def`: one
  `TI_C54X_OPCODE_MASKED(...)` line. IDs, binding, and checked
  dispatch derive from it via X-macro — no other wiring except
  the handler `#include` in `src/isa/ops/opcode_dispatch.c`.
- Layouts: enum in `opcode_catalog.h`, token in
  `tools/isa/catalog/opcode_catalog_schema.py`, set in
  `src/isa/decode/operand_layout_sets.c` (+ extern in the
  internal header), plan + resolve case in
  `src/isa/decode/operand_layout.c`. Keep each file under 500
  lines; split before 800 (requires an explicit decision).
- Handlers in `src/isa/ops/<fam>/op_<sym>.[ch]` with module
  contracts; pure checked/trusted leaves, per-row identity
  validation, transactional rejection (output untouched on
  failure). Register in the family `sources.cmake` + `sources.mk`.
- Regenerate `generated/isa/catalog/opcode_first_word.inc` via
  `tools/isa/catalog/generate_opcode_lookup.py` (no hand edits).
- Resource combos: reuse an existing `TI_C54X_INPUT_RESOURCES_*`
  macro; add a new combination only when no honest one exists.
- No `getenv`, file I/O, globals, or logging in the core path;
  no new knobs without a `docs/knobs.md` entry.

## 5. Fixture inventory (per new row)

With R rows (excluding UNKNOWN), `ID_COUNT` = R+1, lookup =
prior + encodings, bindings +1, family/strict recompute as
N/205 with 3-decimal percents:

- `TI_C54X_OPCODE_ID_COUNT == N` in 18 files (the catalog
  `test_*_metadata.c` set plus
  `test_single_repeat_body_eligibility.c`) — watch the
  no-space `==N` spelling in the reverse-carry file.
- Populated-lookup counts (`4667U`-style, several catalog
  metadata files + `test_opcode_lookup_generator.py` + two
  inventory binding tests).
- `len(catalog_rows)`, `len(rows)+1` (UNKNOWN retains identity
  zero), binding counts, distinct-symbol counts, `COMPLETE`
  sums, `covered` dicts, percent strings, `N/205` report
  strings in `tests/contract/quality/`.
- Scalar check afterwards: `grep -rn "==14[01]\|4669\|4701"
  tests/unit`, plus the percent/`covered` patterns.

## 6. Validation gates

Run **all** of these; each catches what the others miss.

1. `make build-fast/test_<new...>` × 4 — focused GREEN.
2. `make test` — full rebuild. **Caveat (W58 finding):** the
   `test` recipe runs the Python quality gate *before* the C
   binaries, so a C regression hides behind the known
   environmental quality failure. Never claim "C suite green"
   from `make test` output alone.
3. All C binaries explicitly: `make build -j$(nproc)`, then run
   every `build-fast/test_*` (≈300) and require zero failures.
4. Quality checkers individually (unittest short-circuits them):
   `check_knobs.py`, `check_source_boundaries.py`,
   `check_build_manifests.py`, `check_evidence.py`,
   `check_isa_inventory.py`, `check_observer_artifact.py
   --artifact build-fast/observer-off/libti_c54x.a --nm nm`.
5. Focused ASan/UBSan: `make -B TI_C54X_ENABLE_SANITIZERS=1`
   on the new tests, run with
   `ASAN_OPTIONS=detect_leaks=0` (LSan cannot run under this
   container's ptrace — pre-existing limitation). Then
   `make clean` — sanitizer objects poison later normal links.
6. Fresh CMake/CTest Debug (CI's configuration):
   `cmake -S . -B /tmp/<name> -DCMAKE_BUILD_TYPE=Debug`,
   build, `ctest`. Expect 100% except `quality.python`.
7. Full sanitizer CMake/CTest
   (`-DTI_C54X_BUILD_EXAMPLES=ON
   -DTI_C54X_ENABLE_SANITIZERS=ON`, `ctest -E quality.python`,
   LSan off): expect 100%.
8. `git diff --check` (whitespace) before each commit.

Known-good baseline (not regressions): the four
`-m32`/ILP32 SIGSYS environmental failures
(`test_fixed_effect_and_root_sizes_ilp32_when_supported` +
three `test_supported_m32_*`), caused by missing 32-bit exec
support in this container. Do **not** use a `-O3` Release
cmake configure: `repeat_pending_request` has a pre-existing
struct-padding sensitivity that fails there (CI uses Debug).

## 7. Commit and ledger discipline

- The main agent is orchestrator and commit owner; delegated
  agents never commit. Never commit, amend, push, or tag unless
  the session's work calls for it under these rules.
- Atomic commits: implementation (or standalone refactor/docs
  piece) first, full suite green at that commit. Name staged
  files explicitly; never `git clean` user-untracked files.
- Message format: `feat: <what> (P2-Wnn)` with a body stating
  the form, value/mask, layout, resources, semantics, and the
  realized registry numbers (rows, ID_COUNT, lookup, bindings,
  family N/205 + %, strict N/205).
- Immediately after: a ledger-only commit
  (`docs: ledger Wnn <what> checkpoint`) appending one entry to
  `docs/development-ledger.md` with the implementation hash,
  validation evidence, decisions, knob changes (usually none),
  and follow-up work. Never rewrite an old entry; append a
  correction.
- Independent review (adversarial, read-only) before commit for
  slices with new semantics; a caught finding (W57's bit-1
  misread) is recorded in the ledger, not hidden.

## 8. README step when finishing a slice

Read this section top to bottom every time a slice is ready to
land. It is the definition of done; a slice is not finished until
every line checks out.

1. **Numbers agree.** The plan anchor projections (§1, step 2) now match
   reality: rows, `ID_COUNT`, lookup entries, bindings, family
   N/205 + 3-decimal %, strict N/205, evidence/requirement/test
   counts. `grep` the anchor numbers against
   `config/evidence.json`, `config/isa/inventory.json`, and the
   test output; fix the anchor, never the tests.
2. **All gates green** (§6). No gate may be skipped because an
   earlier one passed; record the actual commands and results
   for the ledger entry.
3. **Diff review.** `git status --short` shows only files the
   slice owns; `git diff --check` is clean; no stray debug,
   no `git clean` of user-untracked files. New handwritten
   files carry module contracts; no file silently passed 500
   lines without a split decision.
4. **Implementation commit.** Stage files by name, commit
   `feat: <what> (P2-Wnn)` with the realized numbers in the
   body. Full suite green at this commit.
5. **Ledger commit.** Append one entry to
   `docs/development-ledger.md` (hash, evidence, decisions,
   knobs, follow-ups) and commit alone as
   `docs: ledger Wnn <what> checkpoint`. Never rewrite history.
6. **Runbook + plan hygiene.** Move the slice's line out of §9
   below (or mark it closed with the landing hash); flip the
   plan anchor heading from `(planned)` to the landed state
   only if that slice's convention says so. If the slice taught
   a new bug class, fixture, or gate caveat, add it to this
   runbook in the same commit as the lesson (separate docs
   commit, never folded into the next slice).

## 9. Live follow-ups (update as they close)

- P3 5110-DSP execution focus (harness landed 2026-09-19 at user
  direction; profile facts sourced read-only from the legacy core
  review, no source/tables/comments copied): the `### P3` plan
  section sequences control flow → STM/STH → repeat → MAC/MPY →
  MMR/stack → bit ops → shifts/misc → port I/O from a 36,202-word
  mnemonic census over the sibling `re/dsp-5110` disassembly.
  `tools/hosted/ti_c54x_run_image.c` runs caller-supplied images
  with explicit argv config and a `nokia-ti-lead-rom4` profile doc;
  no sibling source ingested, no `.bin` bytes used as vectors
  (P0-W4 licensing still open).
- LD syntax 5 (`FORM-T2-19-06` Xmem/SHFT, P2-W59): CLOSED by
  `fcc9ead` — X-only dual seam landed (one D-bus read, X commit
  only, no CMPT/ARP observation).
- ST/STH/STL stores; ADD/SUB remainder; MAC/MPY families.
- Consider fixing the `make test` recipe order (§6 caveat).
