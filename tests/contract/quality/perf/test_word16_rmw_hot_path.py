"""Structural contract for the W46A WORD16 RMW hot path.

Module contract: owns source-shape checks for the private RMW seam's
allocation-free and generic-dispatch budget. It does not run a benchmark,
claim DSP timing, or inspect generated formatting. W46B owns the executable
opcode benchmark.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
MEMORY_EFFECT_HEADER = ROOT / "src/isa/ops/memory/memory_effect.h"
MEMORY_EFFECT_SOURCE = ROOT / "src/isa/ops/memory/memory_effect.c"
TRANSACTION_SOURCE = ROOT / "src/sequencer/instruction/memory_instruction_transaction.c"
TRANSACTION_DIR = ROOT / "src/sequencer/instruction"
CANDIDATE_SOURCE = TRANSACTION_DIR / "memory_instruction_candidate_validate.c"
CPU_RUN_SOURCE = ROOT / "src/cpu/execution/run.c"


class Word16RmwHotPathTests(unittest.TestCase):
    @staticmethod
    def _read(path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def test_carrier_uses_compact_value_owned_operation_without_callback(self) -> None:
        header = self._read(MEMORY_EFFECT_HEADER)
        for operation in ("NONE", "AND", "OR", "XOR"):
            self.assertRegex(
                header,
                rf"TI_C54X_OP_MEMORY_RMW_OPERATION_{operation}",
            )
        self.assertRegex(header, r"\brmw_operation\b")
        self.assertRegex(header, r"\bvalue_word\b")
        self.assertNotRegex(header, r"(?i)rmw[^\n]*(?:callback|function|fn)")
        self.assertNotRegex(
            self._read(MEMORY_EFFECT_SOURCE),
            r"(?i)rmw[^\n]*(?:callback|function|fn)",
        )

    def test_resume_validation_does_not_replan_the_address(self) -> None:
        source = self._read(CANDIDATE_SOURCE)
        marker = "ti_c54x_memory_instruction_candidate_matches_live"
        self.assertIn(marker, source)
        resume_body = source[source.index(marker) :]
        self.assertNotIn("ti_c54x_sequencer_resolve_memory_address", resume_body)
        self.assertIn("ti_c54x_memory_instruction_candidate_shape_is_valid", resume_body)

    def test_rmw_has_one_bounded_read_and_one_paired_write_advance_site(self) -> None:
        source = self._read(TRANSACTION_SOURCE)
        self.assertEqual(
            source.count("ti_c54x_bus_word16_adapter_advance("),
            2,
        )
        read_path = source[source.index("rmw_read_phase") :]
        self.assertIn("TI_C54X_BUS_WORD16_ADVANCE_BEGIN", read_path)
        self.assertIn("advance_rmw_write", source)
        helper = self._read(TRANSACTION_DIR / "memory_instruction_rmw.c")
        self.assertEqual(helper.count("ti_c54x_bus_word16_adapter_advance("), 0)
        # W76 splits the READY-path commit into memory_instruction_commit.c;
        # the paired-write arm lives with the new commit owner.
        commit = self._read(TRANSACTION_DIR / "memory_instruction_commit.c")
        self.assertIn("TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE", commit)

    def test_cpu_route_has_no_opcode_branch_or_handler_fanout(self) -> None:
        run_source = self._read(CPU_RUN_SOURCE)
        self.assertNotRegex(run_source, r"case\s+TI_C54X_OPCODE_ID_")
        self.assertNotRegex(run_source, r"switch\s*\([^)]*(?:opcode|instruction|id)")
        self.assertNotRegex(run_source, r"\bti_c54x_op_[A-Za-z0-9_]+\s*\(")

    def test_sequencer_and_memory_effect_hot_paths_are_allocation_free(self) -> None:
        paths = [MEMORY_EFFECT_SOURCE, MEMORY_EFFECT_HEADER]
        paths.extend(TRANSACTION_DIR.glob("*.c"))
        paths.extend(TRANSACTION_DIR.glob("*.h"))
        forbidden = re.compile(r"\b(?:malloc|calloc|realloc|free|aligned_alloc)\s*\(")
        for path in paths:
            self.assertIsNone(forbidden.search(self._read(path)), str(path))

    def test_transform_dispatch_is_bounded_and_value_based(self) -> None:
        source = "\n".join(
            self._read(path)
            for path in TRANSACTION_DIR.glob("*")
            if path.suffix in {".c", ".h"}
        )
        operation_uses = len(re.findall(r"\brmw_operation\b", source))
        self.assertGreater(operation_uses, 0)
        self.assertLessEqual(operation_uses, 12)
        self.assertNotRegex(source, r"(?i)rmw[^\n]*(?:callback|function\s*pointer|fn)")


if __name__ == "__main__":
    unittest.main()
