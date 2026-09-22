"""Contract: the hosted benchmark counts every catalog row form.

bench/ti_c54x_bench.c includes opcode_catalog.def directly with a
counting X-macro per row form, and tools/perf/capture.py recounts
the rows with its own regex to cross-check the benchmark output. A
catalog form missing from either side breaks the benchmark lane (as
MASKED2 once did), so this test pins that every form spelled in the
.def has a matching #define/#undef pair in the bench and appears in
the capture recount.
"""

# Module contract
#
# Owns: deterministic proof that the benchmark's direct catalog
# inclusion covers every row form in the .def.
# Does not own: CPU semantics, benchmark timing, or core build behavior.

import re
import unittest
from pathlib import Path

FORM_USE_RE = re.compile(r"^TI_C54X_OPCODE_([A-Z0-9]+)\(", re.MULTILINE)


class BenchCatalogFormsTests(unittest.TestCase):
    @staticmethod
    def _root() -> Path:
        return Path(__file__).resolve().parents[4]

    def test_bench_defines_every_catalog_row_form(self) -> None:
        root = self._root()
        catalog = (
            root / "src" / "isa" / "catalog" / "opcode_catalog.def"
        ).read_text()
        bench = (root / "bench" / "ti_c54x_bench.c").read_text()
        forms = set(FORM_USE_RE.findall(catalog))
        self.assertGreaterEqual(forms, {"EXACT", "MASKED"})
        define_re = re.compile(
            r"^#define TI_C54X_OPCODE_([A-Z0-9]+)\(\.\.\.\)", re.MULTILINE
        )
        defined = set(define_re.findall(bench))
        capture = (
            root / "tools" / "perf" / "capture.py"
        ).read_text()
        recount = capture.split("def _catalog_count", 1)[1].split(
            "def ", 1
        )[0]
        for form in sorted(forms):
            with self.subTest(form=form):
                self.assertIn(form, defined)
                self.assertIn(f"#undef TI_C54X_OPCODE_{form}", bench)
                self.assertIn(form, recount)


if __name__ == "__main__":
    unittest.main()
