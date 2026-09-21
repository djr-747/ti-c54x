#!/usr/bin/env python3
"""RED-first contracts for W46B atomic logical inventory bindings.

Owns: audit-only one-symbol-per-form bindings for ANDM, ORM, XORM, and ADDM
over the neutral MOD0--MOD3 composition. Does not own opcode values, decode,
execution semantics, or catalog generation.
"""

from __future__ import annotations

import copy
from pathlib import Path
import unittest

from tools.isa.inventory.schema import load_inventory, validate_inventory
from tools.quality.check_isa_inventory import report_inventory


CASES = {
    "ANDM_SMEM_MOD0_3": "FORM-T2-07-05",
    "ORM_SMEM_MOD0_3": "FORM-T2-08-05",
    "XORM_SMEM_MOD0_3": "FORM-T2-09-05",
    "ADDM_SMEM_MOD0_3": "FORM-T2-01-12",
}
TOKEN = "STANDARD_INDIRECT_MOD0_MOD3"
SMEM_CASES = {
    "AND_SMEM": "FORM-T2-07-01",
    "OR_SMEM": "FORM-T2-08-01",
    "XOR_SMEM": "FORM-T2-09-01",
}
SMEM_TOKEN = "DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3"


class InventoryAtomicLogicalMultiBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def inventory(self) -> dict:
        return load_inventory(self.path)

    def test_each_atomic_logical_form_has_one_subset_binding(self) -> None:
        inventory = self.inventory()
        bindings = inventory["catalog_bindings"]
        self.assertEqual(len(bindings),376)
        by_symbol = {binding["catalog_symbol"]: binding for binding in bindings}
        for symbol, form_id in CASES.items():
            self.assertIn(symbol, by_symbol)
            binding = by_symbol[symbol]
            self.assertEqual(binding["form_id"], form_id)
            self.assertEqual(binding["coverage"], "SUBSET")
            self.assertEqual(binding["addressing_subset"], TOKEN)
            form = next(item for item in inventory["forms"] if item["id"] == form_id)
            if form_id == "FORM-T2-01-12":
                # P3-S76: the ADDM family form also binds the absolute
                # ADDM_LK_SMEM_ABSOLUTE row.
                self.assertEqual(
                    form["catalog_symbols"],
                    [symbol, "ADDM_LK_SMEM_ABSOLUTE"],
                )
            else:
                self.assertEqual(form["catalog_symbols"], [symbol])
        for symbol, form_id in SMEM_CASES.items():
            self.assertIn(symbol, by_symbol)
            binding = by_symbol[symbol]
            self.assertEqual(binding["form_id"], form_id)
            self.assertEqual(binding["coverage"], "SUBSET")
            self.assertEqual(binding["addressing_subset"], SMEM_TOKEN)
            form = next(item for item in inventory["forms"] if item["id"] == form_id)
            if form_id == "FORM-T2-08-01":
                # P3-S81: the OR family form also binds the absolute
                # OR_SMEM_ABSOLUTE row.
                self.assertEqual(
                    form["catalog_symbols"],
                    [symbol, "OR_SMEM_ABSOLUTE"],
                )
            else:
                self.assertEqual(form["catalog_symbols"], [symbol])

    def test_report_counts_atomic_families_once(self) -> None:
        report = report_inventory(self.root)
        self.assertEqual(report["catalog_family_progress"],
                         {"covered": 181, "denominator": 205})
        self.assertEqual(report["strict_complete_lower_bound"],
                         {"covered": 48, "denominator": 205})
        self.assertAlmostEqual(report["catalog_family_percent"], 88.293,
                               places=3)
        self.assertAlmostEqual(report["strict_complete_percent"], 23.415,
                               places=3)

    def test_duplicate_symbol_or_pair_remains_invalid(self) -> None:
        duplicate = copy.deepcopy(self.inventory())
        duplicate["catalog_bindings"].append(
            copy.deepcopy(duplicate["catalog_bindings"][-1]))
        errors = validate_inventory(duplicate)
        self.assertTrue(any("duplicate" in error.lower() for error in errors))


if __name__ == "__main__":
    unittest.main()
