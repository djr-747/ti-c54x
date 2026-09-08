#!/usr/bin/env python3
"""RED-first contracts for the ISA inventory schema.

Owns: root/record shape, state vocabulary, stable form identities, and the
separation between audit metadata and opcode authority.  Does not own: the
authoritative form data, catalog parsing, or the quality report.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path
import unittest

from tools.isa.inventory.schema import load_inventory, validate_inventory


TABLE_FORM_COUNTS = [13, 13, 10, 22, 6, 15, 5, 5, 5, 6, 5, 6, 5, 2, 6, 5, 5, 7, 21, 14, 4, 2, 4, 2, 5, 12]
ROOT_KEYS = {"schema", "target_id", "source_id", "basis", "forms", "catalog_bindings"}
FORM_KEYS = {
    "id", "ordinal", "table", "table_ordinal", "mnemonic", "parallel",
    "scope", "coverage", "catalog_symbols",
}
BINDING_KEYS = {"catalog_symbol", "form_id", "coverage", "addressing_subset"}
SCOPES = {"COMMON_CANDIDATE", "PROFILE_ONLY", "OUT_OF_SCOPE", "UNRESOLVED"}
COVERAGE = {"COMPLETE", "SUBSET", "UNIMPLEMENTED"}


class InventorySchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[5]
        cls.path = cls.root / "config/isa/inventory.json"

    def inventory(self) -> dict:
        return load_inventory(self.path)

    def test_production_inventory_has_exact_root_and_basis_contract(self) -> None:
        inventory = self.inventory()
        self.assertEqual(set(inventory), ROOT_KEYS)
        self.assertEqual(inventory["schema"], {"name": "ti-c54x-isa-inventory", "version": 1})
        self.assertEqual(inventory["target_id"], "C54X_CORE_COMMON_V1")
        self.assertEqual(inventory["source_id"], "SRC-TI-SPRU172C")
        self.assertEqual(
            inventory["basis"],
            {
                "chapter": "2.1-2.4",
                "tables": "2-1 through 2-26",
                "printed_pages": "2-2 through 2-18",
                "table_form_counts": TABLE_FORM_COUNTS,
                "form_count": 205,
                "mnemonic_count": 126,
                "parallel_bundle_count": 13,
            },
        )

    def test_form_ids_ordinals_and_record_keys_are_stable(self) -> None:
        forms = self.inventory()["forms"]
        self.assertEqual(len(forms), 205)
        self.assertEqual([form["ordinal"] for form in forms], list(range(1, 206)))
        self.assertEqual(
            [form["id"] for form in forms[:3]],
            ["FORM-T2-01-01", "FORM-T2-01-02", "FORM-T2-01-03"],
        )
        self.assertEqual(
            [form["id"] for form in forms[-3:]],
            ["FORM-T2-26-10", "FORM-T2-26-11", "FORM-T2-26-12"],
        )
        self.assertEqual({key for form in forms for key in form}, FORM_KEYS)
        self.assertEqual(
            [sum(form["table"] == f"2-{index}" for form in forms) for index in range(1, 27)],
            TABLE_FORM_COUNTS,
        )
        for index, count in enumerate(TABLE_FORM_COUNTS, start=1):
            table_forms = [form for form in forms if form["table"] == f"2-{index}"]
            self.assertEqual([form["table_ordinal"] for form in table_forms], list(range(1, count + 1)))
        self.assertTrue(all(set(binding) == BINDING_KEYS for binding in self.inventory()["catalog_bindings"]))

    def test_states_and_parallel_metadata_are_closed(self) -> None:
        forms = self.inventory()["forms"]
        self.assertTrue({form["scope"] for form in forms} <= SCOPES)
        self.assertTrue({form["coverage"] for form in forms} <= COVERAGE)
        self.assertEqual(sum(form["parallel"] is True for form in forms), 13)
        self.assertEqual(len({form["mnemonic"] for form in forms}), 126)
        for form in forms:
            self.assertIsInstance(form["mnemonic"], str)
            self.assertTrue(form["mnemonic"])
            self.assertIsInstance(form["parallel"], bool)
            self.assertIsInstance(form["catalog_symbols"], list)

    def test_inventory_contains_no_opcode_authority_fields(self) -> None:
        raw = json.loads(self.path.read_text(encoding="utf-8"))
        forbidden = {"opcode", "opcode_value", "opcode_mask", "first_word_value", "first_word_mask", "mask", "value"}

        def walk(value: object, location: str = "root") -> list[str]:
            found: list[str] = []
            if isinstance(value, dict):
                found.extend(f"{location}.{key}" for key in value if key in forbidden)
                for key, child in value.items():
                    found.extend(walk(child, f"{location}.{key}"))
            elif isinstance(value, list):
                for index, child in enumerate(value):
                    found.extend(walk(child, f"{location}[{index}]"))
            return found

        self.assertEqual(walk(raw), [])

    def test_malformed_root_and_record_shapes_are_rejected_deterministically(self) -> None:
        valid = self.inventory()
        cases: list[dict] = []
        missing = copy.deepcopy(valid)
        del missing["basis"]
        cases.append(missing)
        bad_scope = copy.deepcopy(valid)
        bad_scope["forms"][0]["scope"] = "COMMON"
        cases.append(bad_scope)
        bad_coverage = copy.deepcopy(valid)
        bad_coverage["forms"][0]["coverage"] = "DONE"
        cases.append(bad_coverage)
        duplicate_id = copy.deepcopy(valid)
        duplicate_id["forms"][1]["id"] = duplicate_id["forms"][0]["id"]
        cases.append(duplicate_id)
        for malformed in cases:
            first = validate_inventory(malformed)
            second = validate_inventory(malformed)
            self.assertTrue(first)
            self.assertEqual(first, second)

    def test_table_counts_are_derived_and_reject_a_moved_form(self) -> None:
        altered = copy.deepcopy(self.inventory())
        moved = altered["forms"][0]
        # Keep the locator internally coherent while moving the form from 2-1
        # to the end of 2-2.  Only the form-derived table counts should expose
        # this drift; the source checkpoint remains unchanged.
        moved["table"] = "2-2"
        moved["table_ordinal"] = 14
        moved["id"] = "FORM-T2-02-14"
        errors = validate_inventory(altered)
        self.assertTrue(any("table_form_counts" in error for error in errors))

    def test_table_ordinals_must_be_contiguous(self) -> None:
        altered = copy.deepcopy(self.inventory())
        altered["forms"][0]["table_ordinal"] = 14
        altered["forms"][0]["id"] = "FORM-T2-01-14"
        errors = validate_inventory(altered)
        self.assertTrue(any("contiguous" in error for error in errors))

    def test_form_and_binding_coverage_cannot_be_swapped(self) -> None:
        altered = copy.deepcopy(self.inventory())
        binding = next(item for item in altered["catalog_bindings"] if item["coverage"] == "COMPLETE")
        binding["coverage"] = "SUBSET"
        binding["addressing_subset"] = "STANDARD_INDIRECT_MOD0_MOD3"
        errors = validate_inventory(altered)
        self.assertTrue(any("coverage" in error for error in errors))

    def test_bound_form_requires_exactly_one_binding(self) -> None:
        altered = copy.deepcopy(self.inventory())
        binding = copy.deepcopy(altered["catalog_bindings"][0])
        binding["catalog_symbol"] = "CMPL_ACC_ALIAS"
        altered["catalog_bindings"].append(binding)
        errors = validate_inventory(altered)
        self.assertTrue(any("exactly one" in error for error in errors))

    def test_unimplemented_form_cannot_have_a_binding(self) -> None:
        altered = copy.deepcopy(self.inventory())
        form = next(item for item in altered["forms"] if item["coverage"] == "UNIMPLEMENTED")
        altered["catalog_bindings"].append(
            {
                "catalog_symbol": "UNIMPLEMENTED_ALIAS",
                "form_id": form["id"],
                "coverage": "COMPLETE",
                "addressing_subset": "ALL",
            }
        )
        errors = validate_inventory(altered)
        self.assertTrue(any("must not have catalog bindings" in error for error in errors))

    def test_subset_addressing_locator_accepts_future_uppercase_tokens(self) -> None:
        altered = copy.deepcopy(self.inventory())
        binding = next(item for item in altered["catalog_bindings"] if item["coverage"] == "SUBSET")
        binding["addressing_subset"] = "FUTURE_SUBSET"
        self.assertEqual(validate_inventory(altered), [])

    def test_addressing_locator_rejects_malformed_tokens(self) -> None:
        altered = copy.deepcopy(self.inventory())
        binding = next(item for item in altered["catalog_bindings"] if item["coverage"] == "SUBSET")
        binding["addressing_subset"] = "fixed-mod2"
        errors = validate_inventory(altered)
        self.assertTrue(any("uppercase token" in error for error in errors))

    def test_malformed_enum_types_return_deterministic_errors(self) -> None:
        altered = copy.deepcopy(self.inventory())
        altered["forms"][0]["scope"] = []
        altered["forms"][0]["coverage"] = {}
        altered["catalog_bindings"][0]["coverage"] = []
        altered["catalog_bindings"][0]["addressing_subset"] = {}
        first = validate_inventory(altered)
        second = validate_inventory(altered)
        self.assertTrue(first)
        self.assertEqual(first, second)
