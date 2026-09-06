#!/usr/bin/env python3
"""Project-contract checks for the single-source opcode catalog.

Owns: the deliberately narrow parser and source-shape checks that prove the
catalog/decode seam is generated from one exact-row inventory.  Does not own:
decoder behavior, opcode semantics, build manifests, or TI architectural
evidence.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path


_ROW = re.compile(
    r"^TI_C54X_OPCODE_EXACT\(\s*"
    r"([A-Z][A-Z0-9_]*)\s*,\s*"
    r"([a-z][a-z0-9_]*)\s*,\s*"
    r"(0x[0-9A-Fa-f]+)\s*,\s*"
    r"([0-9]+)\s*\)$"
)
_ID = re.compile(r"^[A-Z][A-Z0-9_]*$")
_HANDLER = re.compile(r"^[a-z][a-z0-9_]*$")


def parse_catalog(text: str) -> list[tuple[str, str, int, int]]:
    """Parse only the one-line EXACT form accepted by ``opcode_catalog.def``.

    Comments and blank lines are allowed for the module contract.  Every other
    non-comment line must be exactly one row; this avoids accidentally growing
    a second, undocumented inventory syntax.
    """

    without_comments = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    rows: list[tuple[str, str, int, int]] = []
    seen_ids: set[str] = set()
    seen_words: set[int] = set()
    seen_handlers: set[str] = set()
    for line_number, raw_line in enumerate(without_comments.splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith("//"):
            continue
        match = _ROW.fullmatch(line)
        if match is None:
            raise ValueError(f"malformed catalog row at line {line_number}")
        symbol, handler, word_text, width_text = match.groups()
        if not _ID.fullmatch(symbol):
            raise ValueError(f"invalid opcode ID at line {line_number}")
        if symbol in {"UNKNOWN", "COUNT"} or symbol in seen_ids:
            raise ValueError(f"duplicate or reserved opcode ID at line {line_number}")
        if not _HANDLER.fullmatch(handler):
            raise ValueError(f"invalid handler token at line {line_number}")
        first_word = int(word_text, 16)
        width = int(width_text, 10)
        if not 0 <= first_word <= 0xFFFF:
            raise ValueError(f"first word out of range at line {line_number}")
        if not 1 <= width <= 0xFF:
            raise ValueError(f"program-word width out of range at line {line_number}")
        if first_word in seen_words:
            raise ValueError(f"duplicate first word at line {line_number}")
        if handler in seen_handlers:
            raise ValueError(f"duplicate handler token at line {line_number}")
        seen_ids.add(symbol)
        seen_words.add(first_word)
        seen_handlers.add(handler)
        rows.append((symbol, handler, first_word, width))
    if not rows:
        raise ValueError("catalog has no EXACT rows")
    return rows


class OpcodeCatalogContractTests(unittest.TestCase):
    def test_parser_accepts_the_frozen_exact_row(self) -> None:
        self.assertEqual(
            parse_catalog("TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1)\n"),
            [("NOP", "nop", 0xF495, 1)],
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
                "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1)",
                "TI_C54X_OPCODE_EXACT(NOP, nop, 0x1234, 1)",
            )
        )
        for symbol in ("UNKNOWN", "COUNT"):
            with self.subTest(symbol=symbol):
                with self.assertRaises(ValueError):
                    parse_catalog(f"TI_C54X_OPCODE_EXACT({symbol}, nop, 0x0000, 1)")
        with self.assertRaises(ValueError):
            parse_catalog(duplicate)

    def test_parser_rejects_duplicate_first_words_and_handler_tokens(self) -> None:
        duplicate_word = "\n".join(
            (
                "TI_C54X_OPCODE_EXACT(FOO, foo, 0x1234, 1)",
                "TI_C54X_OPCODE_EXACT(BAR, bar, 0x1234, 2)",
            )
        )
        duplicate_handler = "\n".join(
            (
                "TI_C54X_OPCODE_EXACT(FOO, shared, 0x1234, 1)",
                "TI_C54X_OPCODE_EXACT(BAR, shared, 0x5678, 1)",
            )
        )
        for text in (duplicate_word, duplicate_handler):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_parser_rejects_handler_tokens_and_bad_ranges(self) -> None:
        invalid = (
            "TI_C54X_OPCODE_EXACT(NOP, NOP, 0xF495, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop-handler, 0xF495, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0x10000, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0x-1, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 0)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 256)",
        )
        for text in invalid:
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_catalog(text)

    def test_real_catalog_contains_only_the_frozen_nop_row(self) -> None:
        root = Path(__file__).resolve().parents[3]
        rows = parse_catalog((root / "src/isa/catalog/opcode_catalog.def").read_text())
        self.assertEqual(rows, [("NOP", "nop", 0xF495, 1)])

    def test_catalog_has_distinct_x_macro_consumers_and_no_second_inventory(self) -> None:
        root = Path(__file__).resolve().parents[3]
        catalog = root / "src/isa/catalog/opcode_catalog.def"
        consumers = "\n".join(
            path.read_text()
            for path in tuple((root / "src/isa").rglob("*.c"))
            + tuple((root / "src/isa").rglob("*.h"))
            if path != catalog
        )
        self.assertGreaterEqual(consumers.count("opcode_catalog.def"), 4)
        self.assertGreaterEqual(
            consumers.count("#define TI_C54X_OPCODE_EXACT"), 4
        )
        self.assertIn("TI_C54X_OPCODE_EXACT", consumers)
        self.assertIn("static const uint16_t", consumers)
        self.assertRegex(consumers, r"\[\s*UINT16_MAX\s*\+\s*1U\s*\]")
        self.assertIn("_Static_assert", consumers)
        self.assertIn("first_word_value", consumers)
        self.assertIn("ti_c54x_op_##handler", consumers)
        handwritten = "\n".join(
            path.read_text()
            for path in tuple((root / "src/isa").rglob("*.c"))
            + tuple((root / "src/isa").rglob("*.h"))
            if path != catalog
        )
        self.assertIsNone(
            re.search(r"0\s*x\s*f\s*4\s*9\s*5", handwritten, flags=re.IGNORECASE)
        )


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
