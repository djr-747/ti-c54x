### P3-WAVE7: managed leaf fan-out (planned)

All arch seams are complete (Seam 1 syn4 SHIFT, Seam 2
RMW-ADD, Seam 3 Smem-lk). 86 forms remain UNIMPLEMENTED.
Readiness classes:

- COVERED (maintainer binds, no worker): the 8 rounded
  R-variants (MPYR/MACR/MACAR/MASR/MASAR single-memory) already
  execute through the existing leaves with `rounding` set and
  are tested there; they need form bindings plus evidence only.
- LEAF-ONLY pilot (worker W7A): RND/SAT accumulator unary
  (T2-06-13/14) on the existing ACCUMULATOR_SOURCE_DESTINATION
  layout with arithmetic40 add/saturate helpers. TI worked
  examples exist for both.
- Deferred seams (maintainer first, workers after): parallel
  dual-memory (T2-22/23/24/25), dual-Xmem BIT (T2-11-01),
  Smem-lk TC/store mini-seam for BITF/CMPM/ST-lk
  (INDIRECT_ADDR7_EXTENSION16 + shared leaf core), XPC far
  branches/returns (T2-12/13/15), stack family (T2-17),
  MMR/pmad moves (T2-26), control/interrupt (T2-14/18),
  conditional stores (T2-21: S48 seam landed; S49 leaves next), filter ops (T2-06-04/06/07/12),
  remaining arithmetic (ADDC/ADDS/SUBC/DADD-family/ABDST/ABS/
  NEG/NORM/SFTA/LD/STL/LTD).

Worker protocol (bounded ownership, maintainer merge):

- Each worker gets an isolated worktree, one family, and a
  maintainer TI brief with exact opcodes and semantics.
- Workers create NEW files only: `src/isa/ops/<fam>/op_<name>.c`,
  `src/isa/ops/<fam>/op_<name>.h`, and
  `tests/unit/isa/ops/<fam>/test_<name>.c`. No edits to shared
  files (catalog, dispatch, layouts, build manifests, config,
  docs), no commits, no pushes.
- Leaves mirror the named sibling file structure (checked plus
  trusted entries, transactional rejection, module contracts).
  Tests mirror the sibling test harness and must build plus
  pass in the worker's tree (`make build-fast/<test>`).
- Workers report the file list plus test output; the
  maintainer wires rows, dispatch includes, build manifests,
  inventory, evidence, and count bumps, then runs the full
  gates before the impl commit.
- One family per worker; workers never revert each other and
  never touch another worker's files.
