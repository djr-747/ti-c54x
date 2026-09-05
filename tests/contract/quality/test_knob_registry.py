#!/usr/bin/env python3
"""Owns contract fixtures for the machine-readable knob-registry checker.

These tests own checker behavior only; they do not build the C library or define
production configuration semantics.
"""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
from tools.quality import check_knobs  # noqa: E402


class RegistryValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.registry = json.loads(
            (ROOT / "config" / "knobs.json").read_text(encoding="utf-8")
        )

    def test_repository_registry_is_consistent(self) -> None:
        self.assertEqual(check_knobs.check_repository(ROOT), [])

    def test_duplicate_knob_id_is_rejected(self) -> None:
        duplicate = copy.deepcopy(self.registry["knobs"][0])
        self.registry["knobs"].append(duplicate)
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("duplicate knob id" in error for error in errors))

    def test_unknown_enum_is_rejected(self) -> None:
        self.registry["knobs"][0]["class"] = "DIAGNOSTIC"
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("class is not a supported" in error for error in errors))

    def test_duplicate_observer_mask_is_rejected(self) -> None:
        self.registry["observer_bits"][1]["mask"] = self.registry["observer_bits"][0]["mask"]
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("duplicate observer-bit mask" in error for error in errors))

    def test_duplicate_observer_c_identifier_is_rejected(self) -> None:
        self.registry["observer_bits"][1]["kind_c_identifier"] = self.registry["observer_bits"][0]["kind_c_identifier"]
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("duplicate observer-bit kind C identifier" in error for error in errors))

    def test_missing_observer_mask_identifier_is_rejected(self) -> None:
        del self.registry["observer_bits"][0]["mask_c_identifier"]
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("mask_c_identifier" in error for error in errors))

    def test_duplicate_observer_mask_c_identifier_is_rejected(self) -> None:
        self.registry["observer_bits"][1]["mask_c_identifier"] = self.registry["observer_bits"][0]["mask_c_identifier"]
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("duplicate observer-bit mask C identifier" in error
                            for error in errors))

    def test_observer_bit_and_mask_must_agree(self) -> None:
        self.registry["observer_bits"][0]["bit"] = 1
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("does not match" in error for error in errors))

    def test_architectural_effect_must_be_non_architectural(self) -> None:
        self.registry["knobs"][0]["architectural_effect"] = "changes PC"
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("architectural_effect must begin" in error for error in errors))

    def test_ab_knob_requires_comparison_metadata(self) -> None:
        ab_knob = copy.deepcopy(self.registry["knobs"][0])
        ab_knob["id"] = "fixture.ab"
        ab_knob["class"] = "A/B"
        self.registry["knobs"].append(ab_knob)
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("implementations must name two" in error for error in errors))
        self.assertTrue(any("comparison_corpus" in error for error in errors))

    def test_knob_tombstone_cannot_reuse_active_id(self) -> None:
        self.registry["knob_tombstones"] = [{
            "id": "TI_C54X_BUILD_TESTS",
            "retired_abi_major": 0,
            "reason": "fixture",
            "replacement": None,
        }]
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("reuses active identifier" in error for error in errors))

    def test_knob_tombstone_requires_replacement_metadata(self) -> None:
        self.registry["knob_tombstones"] = [{
            "id": "old.knob",
            "retired_abi_major": 0,
            "reason": "fixture",
        }]
        errors = check_knobs.validate_registry(self.registry)
        self.assertTrue(any("replacement" in error for error in errors))

    def test_trace_header_mask_literal_must_match_registry(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            header = root / "include" / "ti_c54x" / "trace.h"
            header.parent.mkdir(parents=True)
            text = (ROOT / "include" / "ti_c54x" / "trace.h").read_text(encoding="utf-8")
            text = text.replace(
                "TI_C54X_TRACE_MASK_FETCH UINT64_C(0x0000000000000002)",
                "TI_C54X_TRACE_MASK_FETCH UINT64_C(0x0000000000000008)",
            )
            header.write_text(text, encoding="utf-8")
            errors = check_knobs.check_trace_header(root, self.registry)
            self.assertTrue(any("literal for TI_C54X_TRACE_MASK_FETCH" in error
                                for error in errors))

    def test_trace_header_kind_identifier_is_required(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            header = root / "include" / "ti_c54x" / "trace.h"
            header.parent.mkdir(parents=True)
            text = (ROOT / "include" / "ti_c54x" / "trace.h").read_text(encoding="utf-8")
            text = text.replace("TI_C54X_TRACE_FETCH = 1,", "TI_C54X_TRACE_FETCH_REMOVED = 1,")
            header.write_text(text, encoding="utf-8")
            errors = check_knobs.check_trace_header(root, self.registry)
            self.assertTrue(any("missing observer kind identifier: TI_C54X_TRACE_FETCH" in error
                                for error in errors))

    def test_docs_state_class_and_disposition_are_authoritative(self) -> None:
        docs = (ROOT / "docs" / "knobs.md").read_text(encoding="utf-8")
        docs = docs.replace(
            "| `TI_C54X_BUILD_TESTS` | ACTIVE | TEST | KEEP |",
            "| `TI_C54X_BUILD_TESTS` | PLANNED | TEST | KEEP |",
        )
        errors = check_knobs.check_documentation(self.registry, docs)
        self.assertTrue(any("TI_C54X_BUILD_TESTS" in error for error in errors))

    def test_unregistered_project_compile_knob_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "config").mkdir()
            (root / "docs").mkdir()
            (root / "src").mkdir()
            (root / "CMakeLists.txt").write_text(
                "option(TI_C54X_UNREGISTERED_SWITCH \"fixture\" OFF)\n",
                encoding="utf-8",
            )
            (root / "config" / "knobs.json").write_text(
                json.dumps(self.registry), encoding="utf-8"
            )
            (root / "docs" / "knobs.md").write_text(
                (ROOT / "docs" / "knobs.md").read_text(encoding="utf-8"),
                encoding="utf-8",
            )
            errors = check_knobs.check_repository(root)
            self.assertTrue(any("TI_C54X_UNREGISTERED_SWITCH" in error for error in errors))

    def test_planned_compile_knob_cannot_have_a_call_site(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "config").mkdir()
            (root / "docs").mkdir()
            (root / "CMakeLists.txt").write_text(
                "option(TI_C54X_ENABLE_TEST_HOOKS \"fixture\" OFF)\n",
                encoding="utf-8",
            )
            (root / "config" / "knobs.json").write_text(
                json.dumps(self.registry), encoding="utf-8"
            )
            (root / "docs" / "knobs.md").write_text(
                (ROOT / "docs" / "knobs.md").read_text(encoding="utf-8"),
                encoding="utf-8",
            )
            errors = check_knobs.check_repository(root)
            self.assertTrue(any("PLANNED knob TI_C54X_ENABLE_TEST_HOOKS" in error
                                for error in errors))

    def test_comments_do_not_register_compile_knobs(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "CMakeLists.txt").write_text(
                "# option(TI_C54X_COMMENTED_SWITCH \"fixture\" OFF)\n"
                "# set(comment TI_C54X_ALSO_COMMENTED)\n",
                encoding="utf-8",
            )
            found = check_knobs.scan_project_compile_knobs(root)
            self.assertNotIn("TI_C54X_COMMENTED_SWITCH", found)
            self.assertNotIn("TI_C54X_ALSO_COMMENTED", found)

    def test_conditional_line_registers_every_project_symbol(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "src" / "cpu" / "fixture.h"
            source.parent.mkdir(parents=True)
            source.write_text(
                "#if defined(TI_C54X_FIRST_SWITCH) && defined(TI_C54X_SECOND_SWITCH)\n"
                "#endif\n",
                encoding="utf-8",
            )
            found = check_knobs.scan_project_compile_knobs(root)
            self.assertIn("TI_C54X_FIRST_SWITCH", found)
            self.assertIn("TI_C54X_SECOND_SWITCH", found)

    def test_extra_trace_kind_in_public_header_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            header = root / "include" / "ti_c54x" / "trace.h"
            header.parent.mkdir(parents=True)
            text = (ROOT / "include" / "ti_c54x" / "trace.h").read_text(encoding="utf-8")
            text = text.replace(
                "    TI_C54X_TRACE_STOP = 2\n}",
                "    TI_C54X_TRACE_STOP = 2,\n    TI_C54X_TRACE_EXTRA = 3\n}",
            )
            header.write_text(text, encoding="utf-8")
            errors = check_knobs.check_trace_header(root, self.registry)
            self.assertTrue(any("unregistered observer kind identifier: TI_C54X_TRACE_EXTRA" in error
                                for error in errors))

    def test_extra_trace_mask_in_public_header_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            header = root / "include" / "ti_c54x" / "trace.h"
            header.parent.mkdir(parents=True)
            text = (ROOT / "include" / "ti_c54x" / "trace.h").read_text(encoding="utf-8")
            text = text.replace(
                "#define TI_C54X_TRACE_MASK_STOP UINT64_C(0x0000000000000004)",
                "#define TI_C54X_TRACE_MASK_STOP UINT64_C(0x0000000000000004)\n"
                "#define TI_C54X_TRACE_MASK_EXTRA UINT64_C(0x0000000000000008)",
            )
            header.write_text(text, encoding="utf-8")
            errors = check_knobs.check_trace_header(root, self.registry)
            self.assertTrue(any("unregistered observer mask identifier: TI_C54X_TRACE_MASK_EXTRA" in error
                                for error in errors))

    def test_observer_markdown_drift_is_rejected(self) -> None:
        docs = (ROOT / "docs" / "knobs.md").read_text(encoding="utf-8")
        docs = docs.replace(
            "`trace.event.fetch` | `TI_C54X_TRACE_FETCH` | `TI_C54X_TRACE_MASK_FETCH` | ACTIVE | 1 |",
            "`trace.event.fetch` | `TI_C54X_TRACE_DRIFT` | `TI_C54X_TRACE_MASK_FETCH` | ACTIVE | 1 |",
        )
        errors = check_knobs.check_documentation(self.registry, docs)
        self.assertTrue(any("observer row for trace.event.fetch" in error for error in errors))

    def test_tombstone_ids_must_be_visible_in_markdown(self) -> None:
        self.registry["observer_bit_tombstones"] = [{
            "id": "trace.event.retired",
            "bit": 3,
            "mask": "0x0000000000000008",
            "kind_c_identifier": "TI_C54X_TRACE_RETIRED",
            "mask_c_identifier": "TI_C54X_TRACE_MASK_RETIRED",
            "retired_abi_major": 0,
            "reason": "fixture",
            "replacement": None,
        }]
        self.registry["knob_tombstones"] = [{
            "id": "fixture.retired",
            "retired_abi_major": 0,
            "reason": "fixture",
            "replacement": None,
        }]
        docs = (ROOT / "docs" / "knobs.md").read_text(encoding="utf-8")
        errors = check_knobs.check_documentation(self.registry, docs)
        self.assertTrue(any("trace.event.retired" in error for error in errors))
        self.assertTrue(any("fixture.retired" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
