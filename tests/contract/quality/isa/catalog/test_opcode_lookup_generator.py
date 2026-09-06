#!/usr/bin/env python3
"""TDD contracts for the deterministic opcode lookup generator.

Owns: synthetic mapping, rendering, stale-artifact, CLI, and build-boundary
checks for the catalog lookup derivative.  Does not own: catalog parsing,
runtime C consumers, instruction semantics, or architectural evidence.
"""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from tools.isa.catalog.generate_opcode_lookup import (
    UINT16_WORD_COUNT,
    build_lookup,
    render_lookup,
)
from tools.isa.catalog.opcode_catalog_schema import OpcodeCatalogError, OpcodeRow, parse_catalog


LAYOUT_NONE = "TI_C54X_OPERAND_LAYOUT_NONE"
REPEAT_UNRESOLVED = "TI_C54X_REPEATABILITY_UNRESOLVED"


def _row(
    symbol: str,
    handler: str,
    value: int,
    mask: int,
    form: str = "MASKED",
) -> OpcodeRow:
    return OpcodeRow(symbol, handler, value, mask, 1, LAYOUT_NONE,
                     REPEAT_UNRESOLVED, form, 1)


class OpcodeLookupGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.tool = cls.root / "tools/isa/catalog/generate_opcode_lookup.py"
        cls.catalog = cls.root / "src/isa/catalog/opcode_catalog.def"
        cls.artifact = cls.root / "generated/isa/catalog/opcode_first_word.inc"

    def test_logical_lookup_is_exactly_65536_words_and_empty_is_unknown(self) -> None:
        lookup = build_lookup(())
        self.assertEqual(len(lookup), UINT16_WORD_COUNT)
        self.assertTrue(all(identity == 0 for identity in lookup))

    def test_masked_pattern_expands_and_ids_follow_catalog_order(self) -> None:
        rows = (
            _row("LOW", "low", 0x1200, 0xFF00),
            _row("HIGH", "high", 0x3000, 0xF000),
        )
        lookup = build_lookup(rows)
        self.assertEqual(lookup[0x1200], 1)
        self.assertEqual(lookup[0x12FF], 1)
        self.assertEqual(lookup[0x3000], 2)
        self.assertEqual(lookup[0x3ABC], 2)
        self.assertEqual(lookup[0x1300], 0)

    def test_exact_and_disjoint_masked_patterns_coexist(self) -> None:
        rows = (
            _row("CLASS", "class_op", 0x4000, 0xF000),
            _row("EXACT", "exact_op", 0x0123, 0xFFFF, "EXACT"),
        )
        lookup = build_lookup(rows)
        self.assertEqual(lookup[0x4ABC], 1)
        self.assertEqual(lookup[0x0123], 2)
        self.assertEqual(lookup[0x0122], 0)

    def test_directly_supplied_overlapping_rows_are_rejected(self) -> None:
        rows = (
            _row("FIRST", "first", 0x1200, 0xFF00),
            _row("SECOND", "second", 0x1234, 0xFFFF, "EXACT"),
        )
        with self.assertRaisesRegex(OpcodeCatalogError, r"overlapping.*0x1234"):
            build_lookup(rows)

    def test_identity_count_overflow_is_rejected_before_expansion(self) -> None:
        row = _row("ONLY", "only", 0x0000, 0xFFFF, "EXACT")
        with self.assertRaisesRegex(OpcodeCatalogError, r"65535"):
            build_lookup((row,) * UINT16_WORD_COUNT)

    def test_render_is_deterministic_sparse_and_symbolic(self) -> None:
        rows = (
            _row("LOW", "low", 0x1200, 0xFF00),
            _row("EXACT", "exact_op", 0x0123, 0xFFFF, "EXACT"),
        )
        first = render_lookup(rows)
        second = render_lookup(rows)
        self.assertEqual(first, second)
        self.assertIn("Generated", first)
        self.assertIn("DO NOT EDIT", first)
        self.assertIn("[0x0123] = (uint16_t)TI_C54X_OPCODE_ID_EXACT,", first)
        self.assertIn("[0x1200] = (uint16_t)TI_C54X_OPCODE_ID_LOW,", first)
        self.assertEqual(first.count("= (uint16_t)"), 257)
        self.assertNotIn(str(self.root), first)

    def test_production_artifact_matches_generator_and_contains_only_nop(self) -> None:
        rows = parse_catalog(self.catalog.read_text(encoding="utf-8"))
        expected = render_lookup(rows)
        actual = self.artifact.read_text(encoding="utf-8")
        self.assertEqual(actual, expected)
        self.assertEqual(actual.count("= (uint16_t)"), 1)
        self.assertIn("[0xF495] = (uint16_t)TI_C54X_OPCODE_ID_NOP,", actual)

    def test_cli_generate_and_check_with_explicit_paths(self) -> None:
        source = (
            "TI_C54X_OPCODE_EXACT(TEST, test, 0x0042, 1, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            catalog = temp / "catalog.def"
            output = temp / "nested/out.inc"
            catalog.write_text(source, encoding="utf-8")
            generated = subprocess.run(
                [sys.executable, str(self.tool), "--catalog", str(catalog), "--output", str(output)],
                cwd=self.root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(generated.returncode, 0, generated.stderr)
            self.assertTrue(output.exists())
            checked = subprocess.run(
                [sys.executable, str(self.tool), "--catalog", str(catalog), "--output", str(output), "--check"],
                cwd=self.root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(checked.returncode, 0, checked.stderr)
            output.write_text(output.read_text(encoding="utf-8") + "\n", encoding="utf-8")
            stale_bytes = output.read_bytes()
            stale = subprocess.run(
                [sys.executable, str(self.tool), "--catalog", str(catalog), "--output", str(output), "--check"],
                cwd=self.root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(stale.returncode, 0)
            self.assertEqual(output.read_bytes(), stale_bytes)
            output.unlink()
            missing = subprocess.run(
                [sys.executable, str(self.tool), "--catalog", str(catalog), "--output", str(output), "--check"],
                cwd=self.root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(missing.returncode, 0)
            self.assertFalse(output.exists())

    def test_check_mode_never_writes_or_creates_missing_artifact(self) -> None:
        source = (
            "TI_C54X_OPCODE_EXACT(TEST, test, 0x0042, 1, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            catalog = temp / "catalog.def"
            output = temp / "not-created.inc"
            catalog.write_text(source, encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(self.tool), "--catalog", str(catalog), "--output", str(output), "--check"],
                cwd=self.root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(output.exists())

    def test_normal_build_manifests_do_not_invoke_python_generator(self) -> None:
        ignored = {".git", "build", "build-fast", "build-cmake", "generated"}
        manifests: list[Path] = []
        for path in self.root.rglob("*"):
            if not path.is_file():
                continue
            relative = path.relative_to(self.root)
            if any(part in ignored for part in relative.parts):
                continue
            if (
                relative.name == "Makefile"
                or relative.name == "CMakeLists.txt"
                or relative.suffix in {".mk", ".cmake"}
            ):
                manifests.append(relative)

        self.assertTrue(manifests)
        for relative in sorted(manifests):
            with self.subTest(name=str(relative)):
                contents = (self.root / relative).read_text(encoding="utf-8")
                self.assertNotIn("generate_opcode_lookup.py", contents)


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
