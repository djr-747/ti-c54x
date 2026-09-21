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
        self.assertEqual(len(bindings),383)
        self.assertEqual(len({binding["catalog_symbol"] for binding in bindings}), 359)
        by_form: dict[str, list[dict]] = {}
        for binding in bindings:
            by_form.setdefault(binding["form_id"], []).append(binding)
        for family, form_id in FAMILIES.items():
            rows = by_form[form_id]
            expected_symbols = {
                family,
                f"{family}_MOD5",
                f"{family}_MOD6",
                f"{family}_MOD4",
                f"{family}_MOD7",
                f"{family}_MOD8",
                f"{family}_MOD10",
            }
            expected_subsets = {"STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10"}
            if form_id == "FORM-T2-19-20":
                # P3-S66: the LDU family form also binds the direct-DP
                # LDU_DIRECT row outside the standard-indirect token.
                expected_symbols.add("LDU_DIRECT")
                expected_subsets.add("DIRECT_DP")
            if form_id == "FORM-T2-19-02":
                # P3-S75: the LD family form also binds the absolute
                # LD_SMEM_ABSOLUTE row outside the standard-indirect
                # token.
                expected_symbols.add("LD_SMEM_ABSOLUTE")
                expected_subsets.add("ABSOLUTE_DATA")
            if form_id == "FORM-T2-19-04":
                # P3-S80: the LD-shift16 family form also binds the
                # absolute LD_SMEM_ABSOLUTE_SHIFT16 row outside the
                # standard-indirect token.
                expected_symbols.add("LD_SMEM_ABSOLUTE_SHIFT16")
                expected_subsets.add("ABSOLUTE_DATA")
            if form_id == "FORM-T2-20-09":
                # P3-S84: the STL family form also binds the absolute
                # STL_SMEM_ABSOLUTE row outside the standard-indirect
                # token.
                expected_symbols.add("STL_SMEM_ABSOLUTE")
                expected_subsets.add("ABSOLUTE_DATA")
            self.assertEqual(len(rows), len(expected_symbols))
            self.assertEqual(
                {row["catalog_symbol"] for row in rows},
                expected_symbols,
            )
            self.assertTrue(all(row["coverage"] == "SUBSET" for row in rows))
            self.assertEqual({row["addressing_subset"] for row in rows}, expected_subsets)
            form = next(item for item in inventory["forms"] if item["id"] == form_id)
            self.assertEqual(
                set(form["catalog_symbols"]),
                expected_symbols,
            )

    def test_report_counts_forms_once_after_circular_multi_binding(self) -> None:
        report = report_inventory(self.root)
        self.assertEqual(report["catalog_family_progress"], {"covered": 188, "denominator": 205})
        self.assertEqual(report["strict_complete_lower_bound"], {"covered": 55, "denominator": 205})
        self.assertAlmostEqual(report["catalog_family_percent"], 91.707, places=3)
        self.assertAlmostEqual(report["strict_complete_percent"], 26.829, places=3)

    def test_duplicate_symbol_or_pair_remains_invalid(self) -> None:
        inventory = self.inventory()
        duplicate = copy.deepcopy(inventory)
        duplicate["catalog_bindings"].append(copy.deepcopy(duplicate["catalog_bindings"][-1]))
        errors = validate_inventory(duplicate)
        self.assertTrue(any("duplicate" in error.lower() for error in errors))


if __name__ == "__main__":
    unittest.main()
