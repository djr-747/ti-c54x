"""Inventory projection contract for the dual ADD/SUB MOD0-MOD3 forms."""

from __future__ import annotations

import unittest
from pathlib import Path

from tools.isa.inventory.schema import load_inventory
from tools.quality.check_isa_inventory import report_inventory


class AddSubDualInventoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.inventory = load_inventory(cls.root / "config/isa/inventory.json")

    def test_forms_are_bounded_subsets(self) -> None:
        forms = {form["id"]: form for form in self.inventory["forms"]}
        self.assertEqual(
            forms["FORM-T2-01-06"]["catalog_symbols"],
            [
                "ADD_DUAL_MOD0",
                "ADD_DUAL_M0_M1",
                "ADD_DUAL_M0_M2",
                "ADD_DUAL_M1_M0",
                "ADD_DUAL_M1_M1",
                "ADD_DUAL_M1_M2",
                "ADD_DUAL_M2_M0",
                "ADD_DUAL_M2_M1",
                "ADD_DUAL_M2_M2",
                "ADD_DUAL_M0_M3",
                "ADD_DUAL_M1_M3",
                "ADD_DUAL_M2_M3",
                "ADD_DUAL_M3_M0",
                "ADD_DUAL_M3_M1",
                "ADD_DUAL_M3_M2",
                "ADD_DUAL_M3_M3",
            ],
        )
        self.assertEqual(
            forms["FORM-T2-02-06"]["catalog_symbols"],
            [
                "SUB_DUAL_MOD0",
                "SUB_DUAL_M0_M1",
                "SUB_DUAL_M0_M2",
                "SUB_DUAL_M1_M0",
                "SUB_DUAL_M1_M1",
                "SUB_DUAL_M1_M2",
                "SUB_DUAL_M2_M0",
                "SUB_DUAL_M2_M1",
                "SUB_DUAL_M2_M2",
                "SUB_DUAL_M0_M3",
                "SUB_DUAL_M1_M3",
                "SUB_DUAL_M2_M3",
                "SUB_DUAL_M3_M0",
                "SUB_DUAL_M3_M1",
                "SUB_DUAL_M3_M2",
                "SUB_DUAL_M3_M3",
            ],
        )
        for form_id in ("FORM-T2-01-06", "FORM-T2-02-06"):
            self.assertEqual(forms[form_id]["scope"], "COMMON_CANDIDATE")
            self.assertEqual(forms[form_id]["coverage"], "SUBSET")

    def test_bindings_and_projection_counts(self) -> None:
        bindings = {
            binding["catalog_symbol"]: binding
            for binding in self.inventory["catalog_bindings"]
        }
        for mnemonic, form_id in (("ADD", "FORM-T2-01-06"), ("SUB", "FORM-T2-02-06")):
            for x_mode in range(4):
                for y_mode in range(4):
                    if x_mode == 0 and y_mode == 0:
                        symbol = f"{mnemonic}_DUAL_MOD0"
                    else:
                        symbol = f"{mnemonic}_DUAL_M{x_mode}_M{y_mode}"
                    self.assertEqual(
                        bindings[symbol],
                        {
                            "catalog_symbol": symbol,
                            "form_id": form_id,
                            "coverage": "SUBSET",
                            "addressing_subset": f"DUAL_INDIRECT_MOD{x_mode}_MOD{y_mode}",
                        },
                    )
        self.assertEqual(len(self.inventory["catalog_bindings"]),334)
        report = report_inventory(self.root)
        self.assertEqual(
            report["catalog_family_progress"], {"covered": 166, "denominator": 205}
        )
        self.assertEqual(
            report["strict_complete_lower_bound"],
            {"covered": 45, "denominator": 205},
        )


if __name__ == "__main__":
    unittest.main()
