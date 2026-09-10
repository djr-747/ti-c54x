#!/usr/bin/env python3
"""Inventory/catalog projection for the ADD/SUB accumulator-shift rows.

Owns: exact row identity, complete-form binding, and generated lookup growth
for W49B.  Does not own opcode execution, CPU integration, or catalog parsing.
"""

from __future__ import annotations

from pathlib import Path
import unittest

from tools.isa.catalog.generate_opcode_lookup import render_lookup
from tools.isa.catalog.opcode_catalog_schema import parse_catalog
from tools.isa.inventory.schema import load_inventory
from tools.quality.check_isa_inventory import report_inventory


ROWS = {
    "ADD_ACC_SHIFT": {
        "form": "FORM-T2-01-09",
        "base": 0xF400,
    },
    "SUB_ACC_SHIFT": {
        "form": "FORM-T2-02-09",
        "base": 0xF420,
    },
}


class InventoryAddSubBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.inventory_path = cls.root / "config/isa/inventory.json"
        cls.catalog_path = cls.root / "src/isa/catalog/opcode_catalog.def"

    def test_rows_forms_and_lookup_are_exact(self) -> None:
        inventory = load_inventory(self.inventory_path)
        rows = parse_catalog(self.catalog_path.read_text(encoding="utf-8"))
        by_symbol = {row.symbol: row for row in rows}
        by_form = {form["id"]: form for form in inventory["forms"]}
        bindings = {
            binding["catalog_symbol"]: binding
            for binding in inventory["catalog_bindings"]
        }

        self.assertEqual(len(rows), 141)
        self.assertEqual(len(rows) + 1, 142)
        self.assertEqual(len(inventory["catalog_bindings"]), 141)
        for symbol, expected in ROWS.items():
            with self.subTest(symbol=symbol):
                row = by_symbol[symbol]
                form = by_form[expected["form"]]
                binding = bindings[symbol]
                self.assertEqual(row.first_word_value, expected["base"])
                self.assertEqual(row.first_word_mask, 0xFCE0)
                self.assertEqual(row.program_word_count, 1)
                self.assertEqual(
                    row.operand_layout,
                    "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5",
                )
                self.assertEqual(
                    row.repeatability, "TI_C54X_REPEATABILITY_UNRESOLVED"
                )
                self.assertEqual(
                    row.input_resources,
                    "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
                )
                self.assertEqual(form["scope"], "COMMON_CANDIDATE")
                self.assertEqual(form["coverage"], "COMPLETE")
                self.assertEqual(form["catalog_symbols"], [symbol])
                self.assertEqual(binding["form_id"], expected["form"])
                self.assertEqual(binding["coverage"], "COMPLETE")
                self.assertEqual(binding["addressing_subset"], "ALL")

        self.assertEqual(render_lookup(rows).count("= (uint16_t)"), 5213)
        report = report_inventory(self.root)
        self.assertEqual(
            report["catalog_family_progress"], {"covered": 51, "denominator": 205}
        )
        self.assertEqual(
            report["strict_complete_lower_bound"],
            {"covered": 33, "denominator": 205},
        )


if __name__ == "__main__":
    unittest.main()
