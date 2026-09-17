#!/usr/bin/env python3
"""Inventory/catalog projection for the ADD/SUB accumulator-shift and SUB Smem rows.

Owns: exact row identity, form binding, and generated lookup growth
for W49B and P3-S19.  Does not own opcode execution, CPU integration,
or catalog parsing.
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
        "mask": 0xFCE0,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "SUB_ACC_SHIFT": {
        "form": "FORM-T2-02-09",
        "base": 0xF420,
        "mask": 0xFCE0,
        "layout": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "COMPLETE",
        "addressing": "ALL",
    },
    "SUB_SMEM": {
        "form": "FORM-T2-02-01",
        "base": 0x0800,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUB_SMEM_TS": {
        "form": "FORM-T2-02-02",
        "base": 0x0C00,
        "mask": 0xFE00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
    },
    "SUB_SMEM_SH16": {
        "form": "FORM-T2-02-03",
        "base": 0x4000,
        "mask": 0xFC00,
        "layout": "TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7",
        "resources": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM",
        "coverage": "SUBSET",
        "addressing": "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3",
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

        self.assertEqual(len(rows), 258)
        self.assertEqual(len(rows) + 1, 259)
        self.assertEqual(len(inventory["catalog_bindings"]), 258)
        for symbol, expected in ROWS.items():
            with self.subTest(symbol=symbol):
                row = by_symbol[symbol]
                form = by_form[expected["form"]]
                binding = bindings[symbol]
                self.assertEqual(row.first_word_value, expected["base"])
                self.assertEqual(row.first_word_mask, expected["mask"])
                self.assertEqual(row.program_word_count, 1)
                self.assertEqual(row.operand_layout, expected["layout"])
                self.assertEqual(
                    row.repeatability, "TI_C54X_REPEATABILITY_UNRESOLVED"
                )
                self.assertEqual(row.input_resources, expected["resources"])
                self.assertEqual(form["scope"], "COMMON_CANDIDATE")
                self.assertEqual(form["coverage"], expected["coverage"])
                self.assertEqual(form["catalog_symbols"], [symbol])
                self.assertEqual(binding["form_id"], expected["form"])
                self.assertEqual(binding["coverage"], expected["coverage"])
                self.assertEqual(
                    binding["addressing_subset"], expected["addressing"]
                )

        self.assertEqual(render_lookup(rows).count("= (uint16_t)"), 24593)
        report = report_inventory(self.root)
        self.assertEqual(
            report["catalog_family_progress"], {"covered": 100, "denominator": 205}
        )
        self.assertEqual(
            report["strict_complete_lower_bound"],
            {"covered": 34, "denominator": 205},
        )


if __name__ == "__main__":
    unittest.main()
