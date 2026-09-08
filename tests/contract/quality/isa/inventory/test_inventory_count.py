#!/usr/bin/env python3
"""RED-first contracts for the authoritative inventory counts.

Owns: the SPRU172C summary-table vector and normalized mnemonic/parallel
counts.  Does not own: catalog binding policy or report formatting.
"""

from __future__ import annotations

import copy
from pathlib import Path
import unittest

from tools.isa.inventory.schema import load_inventory, validate_inventory


VECTOR = [13, 13, 10, 22, 6, 15, 5, 5, 5, 6, 5, 6, 5, 2, 6, 5, 5, 7, 21, 14, 4, 2, 4, 2, 5, 12]


class InventoryCountTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def test_authoritative_summary_checkpoint(self) -> None:
        inventory = load_inventory(self.path)
        forms = inventory["forms"]
        basis = inventory["basis"]
        self.assertEqual(len(forms), 205)
        self.assertEqual(basis["table_form_counts"], VECTOR)
        self.assertEqual(sum(VECTOR), 205)
        self.assertEqual(len({form["mnemonic"] for form in forms}), 126)
        self.assertEqual(sum(form["parallel"] is True for form in forms), 13)

    def test_count_vector_and_ids_reject_drift(self) -> None:
        inventory = load_inventory(self.path)
        altered = copy.deepcopy(inventory)
        altered["basis"]["table_form_counts"][0] += 1
        errors = validate_inventory(altered)
        self.assertTrue(any("table_form_counts" in error for error in errors))

        altered = copy.deepcopy(inventory)
        altered["forms"][-1]["id"] = "FORM-T2-26-99"
        errors = validate_inventory(altered)
        self.assertTrue(any("id" in error or "ordinal" in error for error in errors))
