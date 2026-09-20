#!/usr/bin/env python3
"""Projection contract for the DLD/DST LONG32 MOD13 inventory lane.

Owns: the inventory-to-catalog identity projection and bounded-addressing
token for the two-word DLD/DST forms.  Does not own opcode execution,
operand extraction, or semantic state transitions.
"""

from __future__ import annotations

from pathlib import Path
import unittest

from tools.isa.catalog.generate_opcode_lookup import render_lookup
from tools.isa.catalog.opcode_catalog_schema import parse_catalog
from tools.isa.inventory.schema import load_inventory
from tools.quality.check_isa_inventory import report_inventory


SYMBOLS = {
    "DLD_LONG32_MOD13": "FORM-T2-19-01",
    "DLD_SMEM_ABSOLUTE": "FORM-T2-19-01",
    "DST_LONG32_MOD13": "FORM-T2-20-01",
    "DST_SMEM_ABSOLUTE": "FORM-T2-20-01",
}
ADDRESSING_SUBSETS = {
    "DLD_LONG32_MOD13": "LONG_OFFSET_MOD13_CMPT0",
    "DLD_SMEM_ABSOLUTE": "ABSOLUTE_DATA",
    "DST_LONG32_MOD13": "LONG_OFFSET_MOD13_CMPT0",
    "DST_SMEM_ABSOLUTE": "ABSOLUTE_DATA",
}
LAYOUTS = {
    "DLD_LONG32_MOD13": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4",
    "DLD_SMEM_ABSOLUTE": (
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16"
    ),
    "DST_LONG32_MOD13": "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4",
    "DST_SMEM_ABSOLUTE": (
        "TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16"
    ),
}
FIRST_WORD_MASKS = {
    "DLD_LONG32_MOD13": 0xFEF8,
    "DLD_SMEM_ABSOLUTE": 0xFFFF,
    "DST_LONG32_MOD13": 0xFEF8,
    "DST_SMEM_ABSOLUTE": 0xFFFF,
}
REPEATABILITY = {
    "DLD_LONG32_MOD13": "TI_C54X_REPEATABILITY_UNRESOLVED",
    "DLD_SMEM_ABSOLUTE": "TI_C54X_REPEATABILITY_INELIGIBLE",
    "DST_LONG32_MOD13": "TI_C54X_REPEATABILITY_INELIGIBLE",
    "DST_SMEM_ABSOLUTE": "TI_C54X_REPEATABILITY_INELIGIBLE",
}
RESOURCES = {
    "DLD_LONG32_MOD13": "TI_C54X_INPUT_RESOURCES_STATUS_SXM",
    "DLD_SMEM_ABSOLUTE": "TI_C54X_INPUT_RESOURCES_STATUS_SXM",
    "DST_LONG32_MOD13": "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_SST",
    "DST_SMEM_ABSOLUTE": (
        "TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_SST"
    ),
}


class InventoryDldDstLong32ProjectionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.inventory_path = cls.root / "config/isa/inventory.json"
        cls.catalog_path = cls.root / "src/isa/catalog/opcode_catalog.def"

    def test_catalog_rows_project_to_bounded_inventory_forms(self) -> None:
        inventory = load_inventory(self.inventory_path)
        rows = parse_catalog(self.catalog_path.read_text(encoding="utf-8"))
        by_symbol = {row.symbol: row for row in rows}
        by_form = {form["id"]: form for form in inventory["forms"]}
        bindings = {
            binding["catalog_symbol"]: binding
            for binding in inventory["catalog_bindings"]
        }

        self.assertEqual(len(rows), 349)
        self.assertEqual(len(rows) + 1, 350)  # UNKNOWN retains identity zero.
        self.assertEqual(len(inventory["catalog_bindings"]),373)
        self.assertEqual(
            {symbol for symbol in SYMBOLS if symbol in by_symbol},
            set(SYMBOLS),
        )
        for symbol, form_id in SYMBOLS.items():
            with self.subTest(symbol=symbol):
                row = by_symbol[symbol]
                form = by_form[form_id]
                binding = bindings[symbol]
                self.assertEqual(binding["form_id"], form_id)
                self.assertEqual(binding["coverage"], "SUBSET")
                self.assertEqual(
                    binding["addressing_subset"],
                    ADDRESSING_SUBSETS[symbol],
                )
                self.assertEqual(form["scope"], "COMMON_CANDIDATE")
                self.assertEqual(form["coverage"], "SUBSET")
                self.assertEqual(
                    sorted(form["catalog_symbols"]),
                    sorted(
                        other
                        for other, other_form in SYMBOLS.items()
                        if other_form == form_id
                    ),
                )
                self.assertEqual(row.first_word_mask, FIRST_WORD_MASKS[symbol])
                self.assertEqual(row.program_word_count, 2)
                self.assertEqual(row.operand_layout, LAYOUTS[symbol])
                self.assertEqual(row.repeatability, REPEATABILITY[symbol])
                self.assertEqual(row.input_resources, RESOURCES[symbol])

        rendered_lookup = render_lookup(rows)
        self.assertEqual(rendered_lookup.count("= (uint16_t)"), 43096)
        report = report_inventory(self.root)
        self.assertEqual(report["catalog_family_progress"], {"covered": 178, "denominator": 205})
        self.assertEqual(report["strict_complete_lower_bound"], {"covered": 45, "denominator": 205})
        self.assertAlmostEqual(report["catalog_family_percent"], 86.829, places=3)
        self.assertAlmostEqual(report["strict_complete_percent"], 21.951, places=3)


if __name__ == "__main__":
    unittest.main()
