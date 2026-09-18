#!/usr/bin/env python3
"""RED-first contracts for W44B circular inventory multi-binding semantics.

Owns: audit-only representation of one stable TI form bound to its neutral,
MOD5/MOD6, and MOD8/MOD10 catalog rows, with deduplicated coverage arithmetic.
Does not own: opcode values or masks, catalog generation, decode, or execution
semantics.
"""

from __future__ import annotations

import copy
from pathlib import Path
import unittest

from tools.isa.inventory.schema import load_inventory, validate_inventory
from tools.quality.check_isa_inventory import report_inventory


FAMILIES = {
    "LD_SMEM": "FORM-T2-19-02",
    "LD_SMEM_TS": "FORM-T2-19-03",
    "STL_SMEM": "FORM-T2-20-09",
    "LDU_SMEM": "FORM-T2-19-20",
    "LD_T_SMEM": "FORM-T2-19-12",
    "ST_IMMEDIATE_SMEM": "FORM-T2-20-04",
    "LD_DP_SMEM": "FORM-T2-19-13",
    "LD_ASM_SMEM": "FORM-T2-19-17",
    "LD_SMEM_SHIFT16": "FORM-T2-19-04",
    "ST_T_SMEM": "FORM-T2-20-02",
}
TOKEN = "STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10"


class InventoryCircularMultiBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def inventory(self) -> dict:
        return load_inventory(self.path)

    def test_each_form_has_five_deduplicated_subset_bindings(self) -> None:
        inventory = self.inventory()
        bindings = inventory["catalog_bindings"]
        self.assertEqual(len(bindings), 272)
        self.assertEqual(len({binding["catalog_symbol"] for binding in bindings}), 272)
        by_form: dict[str, list[dict]] = {}
        for binding in bindings:
            by_form.setdefault(binding["form_id"], []).append(binding)
        for family, form_id in FAMILIES.items():
            rows = by_form[form_id]
            self.assertEqual(len(rows), 7)
            self.assertEqual(
                {row["catalog_symbol"] for row in rows},
                {
                    family,
                    f"{family}_MOD5",
                    f"{family}_MOD6",
                    f"{family}_MOD4",
                    f"{family}_MOD7",
                    f"{family}_MOD8",
                    f"{family}_MOD10",
                },
            )
            self.assertTrue(all(row["coverage"] == "SUBSET" for row in rows))
            self.assertEqual({row["addressing_subset"] for row in rows}, {"STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10"})
            form = next(item for item in inventory["forms"] if item["id"] == form_id)
            self.assertEqual(
                set(form["catalog_symbols"]),
                {
                    family,
                    f"{family}_MOD5",
                    f"{family}_MOD6",
                    f"{family}_MOD4",
                    f"{family}_MOD7",
                    f"{family}_MOD8",
                    f"{family}_MOD10",
                },
            )

    def test_report_counts_forms_once_after_circular_multi_binding(self) -> None:
        report = report_inventory(self.root)
        self.assertEqual(report["catalog_family_progress"], {"covered": 114, "denominator": 205})
        self.assertEqual(report["strict_complete_lower_bound"], {"covered": 40, "denominator": 205})
        self.assertAlmostEqual(report["catalog_family_percent"], 55.610, places=3)
        self.assertAlmostEqual(report["strict_complete_percent"], 19.512, places=3)

    def test_duplicate_symbol_or_pair_remains_invalid(self) -> None:
        inventory = self.inventory()
        duplicate = copy.deepcopy(inventory)
        duplicate["catalog_bindings"].append(copy.deepcopy(duplicate["catalog_bindings"][-1]))
        errors = validate_inventory(duplicate)
        self.assertTrue(any("duplicate" in error.lower() for error in errors))


if __name__ == "__main__":
    unittest.main()
