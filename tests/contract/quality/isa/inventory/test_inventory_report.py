#!/usr/bin/env python3
"""RED-first contracts for the provisional ISA inventory report.

Owns: report measures and denominator language.  Does not own: report I/O,
catalog parsing, or the semantic implementation of any instruction form.
"""

from __future__ import annotations

from pathlib import Path
import copy
import unittest
from unittest.mock import patch

from tools.quality.check_isa_inventory import check_repository, report_inventory


class InventoryReportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]

    def test_repository_check_is_clean_and_report_is_provisional(self) -> None:
        self.assertEqual(check_repository(self.root), [])
        report = report_inventory(self.root)
        self.assertEqual(report["catalog_family_progress"], {"covered": 88, "denominator": 205})
        self.assertEqual(report["strict_complete_lower_bound"], {"covered": 33, "denominator": 205})
        self.assertAlmostEqual(report["catalog_family_percent"], 42.927, places=3)
        self.assertAlmostEqual(report["strict_complete_percent"], 16.098, places=3)
        self.assertNotIn(65536, report.values())
        rendered = report["text"]
        self.assertIn("88/205", rendered)
        self.assertIn("33/205", rendered)
        self.assertNotIn("1487/65536", rendered)

    def test_report_rejects_invalid_repository_root(self) -> None:
        errors = check_repository(self.root / "does-not-exist")
        self.assertTrue(errors)
        self.assertTrue(any("root" in error.lower() for error in errors))

    def test_report_counts_distinct_bound_forms_and_complete_form_states(self) -> None:
        # This synthetic alias is intentionally invalid under schema v1.  The
        # report arithmetic is still tested in isolation to ensure a future
        # schema relaxation cannot inflate progress by counting raw bindings.
        from tools.quality import check_isa_inventory as checker

        inventory = checker.load_inventory(self.root / "config/isa/inventory.json")
        alias = copy.deepcopy(inventory["catalog_bindings"][0])
        alias["catalog_symbol"] = "CMPL_ACC_ALIAS"
        inventory["catalog_bindings"].append(alias)
        with patch.object(checker, "check_repository", return_value=[]), patch.object(
            checker, "load_inventory", return_value=inventory
        ):
            report = report_inventory(self.root)
        self.assertEqual(report["catalog_family_progress"], {"covered": 88, "denominator": 205})
        self.assertEqual(report["strict_complete_lower_bound"], {"covered": 33, "denominator": 205})
