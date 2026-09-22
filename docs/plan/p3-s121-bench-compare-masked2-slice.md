# P3-S121: bench-compare MASKED2 gap slice

### P3-S121: count MASKED2 rows in the benchmark lane (implemented; local verification green)

Scope: `make bench-compare` broke when MASKED2 catalog rows
arrived — two sites only knew the EXACT|MASKED forms.
`bench/ti_c54x_bench.c` includes `opcode_catalog.def` directly
with a counting X-macro per row form but never defined
`TI_C54X_OPCODE_MASKED2`, failing the benchmark build; and
`tools/perf/capture.py::_catalog_count` recounted only
EXACT|MASKED rows, failing the capture consistency check once
the build was fixed. TI vectors: none — tooling-only fix, no
opcode, operand, sequencing, or timing semantics change; the
oracle is the previously-red `make bench-compare` going green.

Approach: define/undef the MASKED2 counting macro in the bench,
extend the capture recount alternation, and add contract test
`test_bench_catalog_forms.py` pinning that every form spelled in
the .def has a bench macro pair and appears in the capture
recount (so a future fourth form fails there first).

Gates: new contract test RED→GREEN (observed), `make
bench-compare` exit 0 with a full comparison report, `make test`
exit 0, evidence registry PASS. No evidence registry changes (no
new ISA tests, requirements, or behavior).
