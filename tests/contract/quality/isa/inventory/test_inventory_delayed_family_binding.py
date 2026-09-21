#!/usr/bin/env python3
"""Slice-1 denominator contract: delayed variants share family forms.

The 205-form denominator counts TI summary-table families, never delay
slots. Every DELAYED-subset catalog binding must share its form with a
non-delayed sibling under the same form ID. Future delayed rows
(RPTBD, RETFD, FCALLD) extend
EXPECTED_DELAYED and compose the same way; a delayed-only form ID is a
denominator change and must fail here first.
"""
from pathlib import Path
import unittest
from tools.isa.inventory.schema import load_inventory

EXPECTED_DELAYED = {
    "BACCD": "FORM-T2-12-02",
    "BD": "FORM-T2-12-01",
    "CALAD": "FORM-T2-13-01",
    "BCD": "FORM-T2-12-04",
    "BANZD": "FORM-T2-12-03",
    "CALLD": "FORM-T2-13-02",
    "CCD": "FORM-T2-13-03",
    "RETD": "FORM-T2-15-04",
    "RETED": "FORM-T2-15-05",
    "RCD": "FORM-T2-15-03",
    "FBD": "FORM-T2-12-05",
    "FBACCD": "FORM-T2-12-06",
}


def _is_delayed(binding) -> bool:
    subset = binding.get("addressing_subset") or ""
    return "DELAYED" in subset and "NONDELAYED" not in subset


class DelayedFamilyBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def test_delayed_bindings_share_family_forms(self):
        d = load_inventory(self.path)
        by_symbol = {b["catalog_symbol"]: b for b in d["catalog_bindings"]}
        by_form = {}
        for b in d["catalog_bindings"]:
            by_form.setdefault(b["form_id"], []).append(b)
        self.assertEqual(
            {s for s, b in by_symbol.items() if _is_delayed(b)},
            set(EXPECTED_DELAYED),
        )
        for symbol, fid in EXPECTED_DELAYED.items():
            self.assertEqual(by_symbol[symbol]["form_id"], fid)
            siblings = [s for s in by_form[fid] if not _is_delayed(s)]
            self.assertTrue(siblings, (symbol, fid))


if __name__ == "__main__":
    unittest.main()
