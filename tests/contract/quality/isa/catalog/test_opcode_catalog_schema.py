#!/usr/bin/env python3
"""TDD contracts for the shared exact/masked opcode catalog schema.

Owns: synthetic parser and validation cases for the catalog schema.  Does not
own: the production catalog, C macro consumers, generated lookup artifacts,
or instruction semantics.
"""

from __future__ import annotations

import dataclasses
import unittest

from tools.isa.catalog.opcode_catalog_schema import (
    OpcodeCatalogError,
    OpcodeRow,
    parse_catalog,
    validate_catalog_rows,
)


LAYOUT_NONE = "TI_C54X_OPERAND_LAYOUT_NONE"
LAYOUT_RPT_K = "TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K"
LAYOUT_RPT_LK = "TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK"
REPEAT_UNRESOLVED = "TI_C54X_REPEATABILITY_UNRESOLVED"
REPEAT_ELIGIBLE = "TI_C54X_REPEATABILITY_ELIGIBLE"
REPEAT_INELIGIBLE = "TI_C54X_REPEATABILITY_INELIGIBLE"


class OpcodeCatalogSchemaTests(unittest.TestCase):
    def test_exact_is_normalized_to_full_mask_and_retains_form_and_line(self) -> None:
        rows = parse_catalog(
            "// heading\n\n"
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, {LAYOUT_NONE}, "
            f"{REPEAT_UNRESOLVED})\n"
        )
        self.assertEqual(rows, [OpcodeRow(
            "NOP", "nop", 0xF495, 0xFFFF, 1, LAYOUT_NONE,
            REPEAT_UNRESOLVED, "EXACT", 3)])
        with self.assertRaises(dataclasses.FrozenInstanceError):
            rows[0].first_word_value = 0  # type: ignore[misc]

    def test_masked_rows_preserve_value_mask_form_and_line(self) -> None:
        rows = parse_catalog(
            "/* c */\n"
            f"TI_C54X_OPCODE_MASKED(LOAD, load, 0x1200, 0xFF00, 2, "
            f"{LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
        )
        self.assertEqual(rows[0], OpcodeRow(
            "LOAD", "load", 0x1200, 0xFF00, 2, LAYOUT_NONE,
            REPEAT_UNRESOLVED, "MASKED", 2))

    def test_disjoint_masks_are_accepted(self) -> None:
        rows = parse_catalog(
            "\n".join(
                (
                    f"TI_C54X_OPCODE_MASKED(LOW, low, 0x0000, 0xF000, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                    f"TI_C54X_OPCODE_MASKED(HIGH, high, 0x1000, 0xF000, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                )
            )
        )
        self.assertEqual(len(rows), 2)

    def test_all_pattern_collision_classes_are_rejected_with_lowest_witness(self) -> None:
        cases = (
            (
                f"TI_C54X_OPCODE_EXACT(AA, aa, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
                f"TI_C54X_OPCODE_EXACT(BB, bb, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                "0x1234",
            ),
            (
                f"TI_C54X_OPCODE_EXACT(AA, aa, 0x1234, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
                f"TI_C54X_OPCODE_MASKED(BB, bb, 0x1200, 0xFF00, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                "0x1234",
            ),
            (
                f"TI_C54X_OPCODE_MASKED(AA, aa, 0x1200, 0xFF00, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
                f"TI_C54X_OPCODE_MASKED(BB, bb, 0x1004, 0xF0FF, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})",
                "0x1204",
            ),
        )
        for text, witness in cases:
            with self.subTest(text=text):
                with self.assertRaisesRegex(OpcodeCatalogError, witness):
                    parse_catalog(text)

    def test_overlap_diagnostic_is_independent_of_declaration_order(self) -> None:
        first = (
            f"TI_C54X_OPCODE_MASKED(ZZ, zz, 0x1200, 0xFF00, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})\n"
            f"TI_C54X_OPCODE_MASKED(AA, aa, 0x1004, 0xF0FF, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED})"
        )
        second = "\n".join(reversed(first.splitlines()))
        with self.assertRaises(OpcodeCatalogError) as first_error:
            parse_catalog(first)
        with self.assertRaises(OpcodeCatalogError) as second_error:
            parse_catalog(second)
        self.assertEqual(str(first_error.exception), str(second_error.exception))

    def test_invalid_rows_ranges_canonicality_reserved_and_duplicates_are_rejected(self) -> None:
        cases = (
            "TI_C54X_OPCODE(NOP, nop, 0xF495, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495)",
            "TI_C54X_OPCODE_MASKED(NOP, nop, 0x1200, 0xFF00)",
            "TI_C54X_OPCODE_MASKED(NOP, nop, 0x1200, 0x0000, 1)",
            "TI_C54X_OPCODE_MASKED(NOP, nop, 0x1200, 0xFFFF, 1)",
            "TI_C54X_OPCODE_MASKED(NOP, nop, 0x1201, 0xFF00, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0x10000, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0x-1, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 0)",
            "TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 256)",
            "TI_C54X_OPCODE_EXACT(unknown, nop, 0xF495, 1)",
            "TI_C54X_OPCODE_EXACT(NOP, NOP, 0xF495, 1)",
        )
        for text in cases:
            with self.subTest(text=text):
                with self.assertRaises(OpcodeCatalogError):
                    parse_catalog(text)

        duplicate_id = (
            "TI_C54X_OPCODE_EXACT(AA, aa, 0x1111, 1)\n"
            "TI_C54X_OPCODE_EXACT(AA, bb, 0x2222, 1)"
        )
        duplicate_handler = (
            "TI_C54X_OPCODE_EXACT(AA, aa, 0x1111, 1)\n"
            "TI_C54X_OPCODE_EXACT(BB, aa, 0x2222, 1)"
        )
        for text in (duplicate_id, duplicate_handler):
            with self.assertRaises(OpcodeCatalogError):
                parse_catalog(text)

    def test_metadata_tokens_are_required_fully_namespaced_and_known(self) -> None:
        valid_prefix = "TI_C54X_OPCODE_EXACT(VALID, valid, 0x1234, 1,"
        invalid = (
            f"{valid_prefix} TI_C54X_OPERAND_LAYOUT_UNKNOWN, {REPEAT_UNRESOLVED})",
            f"{valid_prefix} OPERAND_LAYOUT_NONE, {REPEAT_UNRESOLVED})",
            f"{valid_prefix} {LAYOUT_NONE}, TI_C54X_REPEATABILITY_COUNT)",
            f"{valid_prefix} {LAYOUT_NONE}, REPEATABILITY_UNRESOLVED)",
        )
        for text in invalid:
            with self.subTest(text=text):
                with self.assertRaises(OpcodeCatalogError):
                    parse_catalog(text)

        for repeatability in (
            REPEAT_UNRESOLVED,
            REPEAT_ELIGIBLE,
            REPEAT_INELIGIBLE,
        ):
            with self.subTest(repeatability=repeatability):
                rows = parse_catalog(
                    f"{valid_prefix} {LAYOUT_NONE}, {repeatability})"
                )
                self.assertEqual(rows[0].repeatability, repeatability)

        for layout in (LAYOUT_RPT_K, LAYOUT_RPT_LK):
            with self.subTest(layout=layout):
                rows = parse_catalog(
                    f"{valid_prefix} {layout}, {REPEAT_UNRESOLVED})"
                )
                self.assertEqual(rows[0].operand_layout, layout)

    def test_malformed_comment_is_rejected_and_comments_blanks_are_allowed(self) -> None:
        parse_catalog(
            "/* before */\n\n"
            f"TI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1, {LAYOUT_NONE}, {REPEAT_UNRESOLVED}) // trailing\n"
        )
        with self.assertRaises(OpcodeCatalogError):
            parse_catalog("/* unterminated\nTI_C54X_OPCODE_EXACT(NOP, nop, 0xF495, 1)")

    def test_direct_rows_apply_the_same_lexical_and_reserved_contract(self) -> None:
        valid = OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 1,
                          LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1)
        malformed = (
            OpcodeRow("not_upper", "valid", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("UNKNOWN", "valid", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("COUNT", "valid", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "Not_lower", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid-handler", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
        )
        self.assertEqual(validate_catalog_rows((valid,)), (valid,))
        for row in malformed:
            with self.subTest(row=row):
                with self.assertRaises(OpcodeCatalogError):
                    validate_catalog_rows((row,))

    def test_direct_rows_reject_bad_types_ranges_forms_and_widths(self) -> None:
        valid = OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 1,
                          LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1)
        malformed = (
            object(),
            OpcodeRow("VALID", "valid", -1, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x10000, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x1234, -1, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x1234, 0x10000, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x1234, 0xFF00, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x1200, 0, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "MASKED", 1),
            OpcodeRow("VALID", "valid", 0x1200, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "MASKED", 1),
            OpcodeRow("VALID", "valid", 0x1201, 0xFF00, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "MASKED", 1),
            OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 0,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 256,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "OTHER", 1),
            OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 0),
            OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", -1),
        )
        for row in malformed:
            with self.subTest(row=row):
                with self.assertRaises(OpcodeCatalogError):
                    validate_catalog_rows((row,))  # type: ignore[arg-type]

    def test_direct_rows_return_an_immutable_tuple_and_report_deterministic_overlap(self) -> None:
        first = OpcodeRow("ZZ", "zz", 0x1200, 0xFF00, 1,
                          LAYOUT_NONE, REPEAT_UNRESOLVED, "MASKED", 7)
        second = OpcodeRow("AA", "aa", 0x1004, 0xF0FF, 1,
                           LAYOUT_NONE, REPEAT_UNRESOLVED, "MASKED", 3)
        result = validate_catalog_rows((first,))
        self.assertIsInstance(result, tuple)
        with self.assertRaises(TypeError):
            result[0] = second  # type: ignore[index]
        with self.assertRaisesRegex(OpcodeCatalogError, r"AA and ZZ at 0x1204"):
            validate_catalog_rows((first, second))
        with self.assertRaisesRegex(OpcodeCatalogError, r"AA and ZZ at 0x1204"):
            validate_catalog_rows((second, first))

    def test_direct_rows_reject_duplicate_ids_and_handlers(self) -> None:
        duplicate_id = (
            OpcodeRow("AA", "aa", 0x1111, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("AA", "bb", 0x2222, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 2),
        )
        duplicate_handler = (
            OpcodeRow("AA", "aa", 0x1111, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 1),
            OpcodeRow("BB", "aa", 0x2222, 0xFFFF, 1,
                      LAYOUT_NONE, REPEAT_UNRESOLVED, "EXACT", 2),
        )
        for rows in (duplicate_id, duplicate_handler):
            with self.assertRaises(OpcodeCatalogError):
                validate_catalog_rows(rows)

    def test_direct_rows_reject_unknown_layout_and_repeatability_tokens(self) -> None:
        for layout, repeatability in (
            ("TI_C54X_OPERAND_LAYOUT_UNKNOWN", REPEAT_UNRESOLVED),
            (LAYOUT_NONE, "TI_C54X_REPEATABILITY_COUNT"),
            (None, REPEAT_UNRESOLVED),
            ([], REPEAT_UNRESOLVED),
            (LAYOUT_NONE, None),
            (LAYOUT_NONE, []),
        ):
            row = OpcodeRow("VALID", "valid", 0x1234, 0xFFFF, 1,
                            layout, repeatability, "EXACT", 1)
            with self.subTest(row=row):
                with self.assertRaises(OpcodeCatalogError):
                    validate_catalog_rows((row,))


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
