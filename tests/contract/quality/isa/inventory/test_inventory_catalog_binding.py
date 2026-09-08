#!/usr/bin/env python3
"""Contracts for inventory-to-catalog identity binding.

Owns: one-to-one symbol identities, multi-binding subset forms, and the
frozen 25 complete/50 standard-indirect subset checkpoint. Does not
own: opcode value/mask parsing or instruction execution semantics.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from tools.quality.check_isa_inventory import check_repository
from tools.isa.inventory.schema import load_inventory, validate_inventory


STANDARD_INDIRECT_SUBSET = {
    "LD_SMEM",
    "LD_SMEM_TS",
    "STL_SMEM",
    "LDU_SMEM",
    "LD_T_SMEM",
    "ST_IMMEDIATE_SMEM",
    "LD_DP_SMEM",
    "LD_ASM_SMEM",
    "LD_SMEM_SHIFT16",
    "ST_T_SMEM",
}


class InventoryCatalogBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def inventory(self) -> dict:
        return load_inventory(self.path)

    def test_current_catalog_snapshot_binds_uniquely_with_frozen_coverage(self) -> None:
        inventory = self.inventory()
        bindings = inventory["catalog_bindings"]
        self.assertEqual(len(bindings), 75)
        self.assertEqual(len({binding["catalog_symbol"] for binding in bindings}), 75)
        self.assertEqual(sum(binding["coverage"] == "COMPLETE" for binding in bindings), 25)
        self.assertEqual(sum(binding["coverage"] == "SUBSET" for binding in bindings), 50)
        self.assertEqual(
            {binding["catalog_symbol"] for binding in bindings if binding["coverage"] == "SUBSET"},
            STANDARD_INDIRECT_SUBSET
            | {f"{symbol}_MOD{mode}" for symbol in STANDARD_INDIRECT_SUBSET for mode in (5, 6, 8, 10)},
        )
        for binding in bindings:
            if binding["coverage"] == "COMPLETE":
                self.assertEqual(binding["addressing_subset"], "ALL")
            else:
                self.assertEqual(binding["addressing_subset"], "STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6_MOD8_MOD10")
        self.assertTrue(all(binding["form_id"].startswith("FORM-T2-") for binding in bindings))

    def test_unknown_form_binding_and_duplicate_are_rejected_by_schema(self) -> None:
        inventory = self.inventory()
        unknown_form = copy.deepcopy(inventory)
        unknown_form["catalog_bindings"][0]["form_id"] = "FORM-T2-99-99"
        errors = validate_inventory(unknown_form)
        self.assertTrue(any("form" in error.lower() or "binding" in error.lower() for error in errors))

        duplicate = copy.deepcopy(inventory)
        duplicate["catalog_bindings"].append(copy.deepcopy(duplicate["catalog_bindings"][0]))
        errors = validate_inventory(duplicate)
        self.assertTrue(any("duplicate" in error.lower() or "once" in error.lower() for error in errors))

    def _repository_with_inventory(self, inventory: dict) -> Path:
        tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(tempdir.cleanup)
        root = Path(tempdir.name)
        (root / "config/isa").mkdir(parents=True)
        (root / "src/isa/catalog").mkdir(parents=True)
        shutil.copy2(self.path, root / "config/isa/inventory.json")
        shutil.copy2(self.root / "src/isa/catalog/opcode_catalog.def", root / "src/isa/catalog/opcode_catalog.def")
        (root / "config/isa/inventory.json").write_text(json.dumps(inventory), encoding="utf-8")
        return root

    def test_unknown_catalog_symbol_and_missing_binding_are_checker_errors(self) -> None:
        inventory = self.inventory()
        unknown = copy.deepcopy(inventory)
        unknown["catalog_bindings"][0]["catalog_symbol"] = "NOT_A_CATALOG_SYMBOL"
        errors = check_repository(self._repository_with_inventory(unknown))
        self.assertTrue(any("catalog" in error.lower() or "unknown" in error.lower() for error in errors))

        missing = copy.deepcopy(inventory)
        missing["catalog_bindings"].pop()
        errors = check_repository(self._repository_with_inventory(missing))
        self.assertTrue(any("binding" in error.lower() or "catalog" in error.lower() for error in errors))
