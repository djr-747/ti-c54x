#!/usr/bin/env python3
"""Project-contract checks for the single-source opcode catalog.

Owns: production-catalog and source-shape checks that prove the catalog/decode
seam derives from the sole exact/masked inventory. Does not own: schema
parsing, decoder behavior, opcode semantics, build manifests, or TI
architectural evidence.
"""

from __future__ import annotations

import dataclasses
import re
import unittest
from pathlib import Path

from tools.isa.catalog.opcode_catalog_schema import OpcodeRow, parse_catalog


LAYOUT_NONE = "TI_C54X_OPERAND_LAYOUT_NONE"
REPEAT_UNRESOLVED = "TI_C54X_REPEATABILITY_UNRESOLVED"


class OpcodeCatalogContractTests(unittest.TestCase):
    def test_parser_accepts_the_frozen_exact_row(self) -> None:
        self.assertEqual(
            parse_catalog(
                f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, {LAYOUT_NONE}, "
                f"{REPEAT_UNRESOLVED})\n"
            ),
            [OpcodeRow("NOP", "nop", 0xF495, 0xFFFF, 1, LAYOUT_NONE,
                       REPEAT_UNRESOLVED, "EXACT", 1)],
        )

    def test_parser_rejects_malformed_rows(self) -> None:
        malformed = (
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, extra)",
            "TI_C54X_OPCODE(NOP, nop, 0xF495, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, one)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1) trailing",
        )
        for text in malformed:
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_parser_rejects_duplicate_and_reserved_ids(self) -> None:
        duplicate = "\n".join(
            (
                f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                f"TI_C54X_OPCODE_EXACT(NOP, nop, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            )
        )
        for symbol in ("UNKNOWN", "COUNT"):
            with self.subTest(symbol=symbol):
                with self.assertRaises(ValueError):
                    parse_catalog(
                        f"TI_C54X_OPCODE_EXACT({symbol}, nop, 0x0000, 1, "
                        f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED})"
                    )
        with self.assertRaises(ValueError):
            parse_catalog(duplicate)

    def test_parser_rejects_duplicate_first_words_and_handler_tokens(self) -> None:
        duplicate_word = "\n".join(
            (
                f"TI_C54X_OPCODE_EXACT(FOO, foo, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                f"TI_C54X_OPCODE_EXACT(BAR, bar, 0x1234, 2, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            )
        )
        duplicate_handler = "\n".join(
            (
                f"TI_C54X_OPCODE_EXACT(FOO, shared, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                f"TI_C54X_OPCODE_EXACT(BAR, shared, 0x5678, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            )
        )
        for text in (duplicate_word, duplicate_handler):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_parser_rejects_handler_tokens_and_bad_ranges(self) -> None:
        invalid = (
            "TI_C54X_OPCODE_EXACT(NOP, NOP, 0xF495, 1, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            "TI_C54X_OPCODE_EXACT(NOP, nop-handler, 0xF495, 1, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0x10000, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0x-1, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 0, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 256, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
        )
        for text in invalid:
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_real_catalog_contains_nop_and_status_load_rows(self) -> None:
        root = Path(__file__).resolve().parents[3]
        rows = parse_catalog((root / "src/isa/catalog/opcode_catalog.def").read_text())
        self.assertEqual(len(rows), 4)
        self.assertEqual(
            dataclasses.replace(rows[0], source_line=0),
            OpcodeRow("NOP", "nop", 0xF495, 0xFFFF, 1, LAYOUT_NONE,
                      REPEAT_UNRESOLVED, "EXACT", 0),
        )
        self.assertGreater(rows[0].source_line, 0)
        self.assertEqual(rows[1].symbol, "LD_IMMEDIATE_DP")
        self.assertEqual(rows[1].first_word_value, 0xEA00)
        self.assertEqual(rows[1].first_word_mask, 0xFE00)
        self.assertEqual(rows[2].symbol, "LD_IMMEDIATE_ASM")
        self.assertEqual(rows[2].first_word_value, 0xED00)
        self.assertEqual(rows[2].first_word_mask, 0xFFE0)
        self.assertEqual(rows[3].symbol, "LD_IMMEDIATE_ARP")
        self.assertEqual(rows[3].first_word_value, 0xF4A0)
        self.assertEqual(rows[3].first_word_mask, 0xFFF8)

    def test_catalog_has_distinct_x_macro_consumers_and_generated_lookup(self) -> None:
        root = Path(__file__).resolve().parents[3]
        catalog = root / "src/isa/catalog/opcode_catalog.def"
        source_paths = tuple((root / "src/isa").rglob("*.c")) + tuple(
            (root / "src/isa").rglob("*.h")
        )
        consumers = {
            path: path.read_text()
            for path in source_paths
            if path != catalog and "opcode_catalog.def" in path.read_text()
        }
        self.assertGreaterEqual(len(consumers), 3)
        for path, text in consumers.items():
            with self.subTest(path=path):
                self.assertIn("#define TI_C54X_OPCODE_EXACT", text)
                self.assertIn("#define TI_C54X_OPCODE_MASKED", text)

        all_sources = "\n".join(path.read_text() for path in source_paths)
        self.assertIn("TI_C54X_OPCODE_EXACT", all_sources)
        self.assertIn("TI_C54X_OPCODE_MASKED", all_sources)
        self.assertIn("static const uint16_t", all_sources)
        self.assertRegex(all_sources, r"\[\s*UINT16_MAX\s*\+\s*1U\s*\]")
        self.assertIn("_Static_assert", all_sources)
        self.assertIn("first_word_value", all_sources)
        self.assertIn("ti_c54x_op_##handler", all_sources)

        catalog_source = (root / "src/isa/catalog/opcode_catalog.c").read_text()
        self.assertIn(
            '#include "../../../generated/isa/catalog/opcode_first_word.inc"',
            catalog_source,
        )
        self.assertEqual(catalog_source.count("ti_c54x_opcode_direct_lookup"), 4)
        self.assertEqual(catalog_source.count('#include "opcode_catalog.def"'), 1)
        self.assertEqual(
            catalog_source.count(
                '#include "../../../generated/isa/catalog/opcode_first_word.inc"'
            ),
            1,
        )

        generated = root / "generated/isa/catalog/opcode_first_word.inc"
        self.assertTrue(generated.is_file())
        self.assertIn("TI_C54X_OPCODE_ID_NOP", generated.read_text())
        handwritten = "\n".join(
            path.read_text() for path in source_paths
            if path != catalog
        )
        self.assertIsNone(
            re.search(r"0\s*x\s*f\s*4\s*9\s*5", handwritten, flags=re.IGNORECASE)
        )


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
