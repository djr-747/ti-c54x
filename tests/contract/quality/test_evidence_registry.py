#!/usr/bin/env python3
"""Negative fixtures for the evidence registry quality gate.

Owns: temporary JSON/Markdown registry fixtures and semantic failure coverage.
Does not own: the canonical registry, evidence documents, or checker policy.
"""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path

from tools.quality.check_evidence import check_repository, main
from tools.quality.evidence_runner import validate_test_runners


COMMON_EXCLUSIONS = [
    "XPC and any extended program-page or extended-address semantics",
    "Device-specific ROM, RAM, memory maps, wait states, and boot maps",
    "Exact IFR/IMR interrupt sources, vector tables, and device interrupt wiring",
    "Peripherals, PLL and clock setup, electrical behavior, and silicon errata",
    "Machine-cycle or pipeline timing guarantees until timing evidence and tests are added",
    "VC5402-specific behavior, which belongs to the separate profile target",
]


class EvidenceRegistryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)
        self.registry = self.make_registry()
        self.write_registry()
        self.write_docs()

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def make_registry(self) -> dict:
        common = "C54X_CORE_COMMON_V1"
        profile = "C54X_PROFILE_V1"
        return {
            "schema": {"name": "ti-c54x-evidence-registry", "version": 1, "abi_major": 0, "status": "baseline", "access_date": "2026-09-06"},
            "primary_target_id": common,
            "targets": [
                {"id": common, "kind": "COMMON", "state": "ACTIVE", "parent_target_id": None, "name": "Common core", "description": "Device-neutral common CPU", "inclusions": ["common CPU"], "exclusions": COMMON_EXCLUSIONS, "allowed_claim_kinds": ["ARCHITECTURAL", "PROJECT_CONTRACT"], "program_address_carrier": {"bits": 32, "unit": "program words", "semantic_scope": "CARRIER_ONLY", "owner_path": "include/ti_c54x/cpu.h", "claim": "Carrier only; no XPC implementation."}},
                {"id": profile, "kind": "PROFILE", "state": "PLANNED", "parent_target_id": common, "name": "Profile", "description": "Later profile", "inclusions": [], "exclusions": ["Profile-only behavior"], "allowed_claim_kinds": ["PROFILE"]},
            ],
            "sources": [
                {"id": "SRC-TI-SPRU000A", "kind": "TI_PRIMARY", "normative_scope": "COMMON_ALLOWED", "state": "ACTIVE", "authority": "Texas Instruments", "literature_number": "SPRU000A", "title": "CPU guide", "revision": "A", "publication_date": "2001-03", "revision_date": None, "official_url": "https://www.ti.com/lit/ug/spru000a/x.pdf", "scope": "Common CPU", "access_date": "2026-09-06", "applicable_target_ids": [common]},
                {"id": "SRC-TI-SPRS000A", "kind": "TI_PRIMARY", "normative_scope": "PROFILE_ONLY", "state": "ACTIVE", "authority": "Texas Instruments", "literature_number": "SPRS000A", "title": "Profile guide", "revision": "A", "publication_date": "2001-03", "revision_date": None, "official_url": "https://www.ti.com/lit/ds/profile.pdf", "scope": "Profile only", "access_date": "2026-09-06", "applicable_target_ids": [profile]},
                {"id": "SRC-PROJECT-CONTRACT", "kind": "PROJECT_CONTRACT_ONLY", "normative_scope": "PROJECT_CONTRACT_ONLY", "state": "ACTIVE", "authority": "ti-c54x project", "literature_number": None, "title": "Project contracts", "revision": "repository state", "publication_date": None, "revision_date": None, "official_url": None, "scope": "Project only", "access_date": "2026-09-06", "applicable_target_ids": [common]},
            ],
            "evidence": [
                {"id": "E-TI-SPRU000A-S1-P1", "immutable": True, "state": "PINNED", "classification": "TI_PRIMARY", "scope": "COMMON_TI", "source_id": "SRC-TI-SPRU000A", "applicable_target_ids": [common], "locator": {"chapter": "1", "section": "1.1", "printed_page_start": "1-1", "printed_page_end": "1-1"}, "assertion": "Common fact."},
                {"id": "E-TI-SPRS000A-S1-P1", "immutable": True, "state": "PINNED", "classification": "TI_PRIMARY", "scope": "PROFILE_ONLY", "source_id": "SRC-TI-SPRS000A", "applicable_target_ids": [profile], "locator": {"chapter": "1", "section": "1.1", "printed_page_start": "1-1", "printed_page_end": "1-1"}, "assertion": "Profile fact."},
                {"id": "E-PROJECT-CONTRACT", "immutable": True, "state": "PINNED", "classification": "PROJECT_CONTRACT_ONLY", "scope": "PROJECT_CONTRACT_ONLY", "source_id": "SRC-PROJECT-CONTRACT", "applicable_target_ids": [common], "locator": {"path": "docs/contract.txt", "anchor": "CONTRACT-ANCHOR"}, "assertion": "Project fact."},
            ],
            "requirements": [
                {"id": "REQ-COMMON-PLANNED", "state": "PLANNED", "target_id": common, "claim_kind": "ARCHITECTURAL", "owner_path": "src/sequencer/", "claim": "Common sequential behavior.", "evidence_ids": ["E-TI-SPRU000A-S1-P1"], "test_ids": ["T-COMMON-PLANNED"]},
                {"id": "REQ-COMMON-UNRESOLVED", "state": "UNRESOLVED", "target_id": common, "claim_kind": "ARCHITECTURAL", "owner_path": "src/address/", "claim": "Unknown common behavior.", "evidence_ids": ["E-TI-SPRU000A-S1-P1"], "test_ids": []},
                {"id": "REQ-PROJECT-IMPLEMENTED", "state": "IMPLEMENTED", "target_id": common, "claim_kind": "PROJECT_CONTRACT", "owner_path": "tools/quality/", "claim": "Project contract.", "evidence_ids": ["E-PROJECT-CONTRACT"], "test_ids": ["T-PROJECT-PASS"]},
                {"id": "REQ-PROFILE-PLANNED", "state": "PLANNED", "target_id": profile, "claim_kind": "PROFILE", "owner_path": "src/devices/", "claim": "Profile behavior.", "evidence_ids": ["E-TI-SPRS000A-S1-P1"], "test_ids": ["T-PROFILE-PLANNED"]},
            ],
            "tests": [
                {"id": "T-COMMON-PLANNED", "state": "PLANNED", "scope": "COMMON_TI", "path": "tests/unit/sequencer/test_future.c", "requirement_ids": ["REQ-COMMON-PLANNED"], "runner": None},
                {"id": "T-PROJECT-PASS", "state": "PASSING", "scope": "PROJECT_CONTRACT_ONLY", "path": "tests/contract/quality/test_passing.py", "requirement_ids": ["REQ-PROJECT-IMPLEMENTED"], "runner": {"kind": "PYTHON_UNITTEST_DISCOVERY", "make_goal": "test", "ctest_name": "quality.discovery"}},
                {"id": "T-PROFILE-PLANNED", "state": "PLANNED", "scope": "PROFILE", "path": "tests/unit/devices/test_future.c", "requirement_ids": ["REQ-PROFILE-PLANNED"], "runner": None},
            ],
            "tombstones": {"targets": [], "sources": [], "evidence": [], "requirements": [], "tests": []},
        }

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def write_registry(self) -> None:
        self.write("config/evidence.json", json.dumps(self.registry, indent=2) + "\n")
        self.write("docs/contract.txt", "CONTRACT-ANCHOR\n")
        self.write("tests/contract/quality/test_passing.py", "# passing\n")
        self.write("Makefile", ".PHONY: test quality-test\ntest: quality-test\nquality-test:\n\tpython3 -m unittest discover -s tests/contract/quality -t .\n")
        self.write("tests/CMakeLists.txt", "add_test(NAME quality.discovery COMMAND python3 -m unittest discover -s \"${PROJECT_SOURCE_DIR}/tests/contract/quality\" -t \"${PROJECT_SOURCE_DIR}\")\n")

    def write_docs(self) -> None:
        targets = self.registry["targets"]
        sources = self.registry["sources"]
        evidence = self.registry["evidence"]
        requirements = self.registry["requirements"]
        tests = self.registry["tests"]
        carrier = targets[0]["program_address_carrier"]
        architecture = "# Architecture\n\n" + "\n".join([target["id"] + "\n" + "\n".join(target["exclusions"]) for target in targets])
        architecture += f"\nsemantic_scope: {carrier['semantic_scope']}\n{carrier['bits']} bits\nunit `{carrier['unit']}`\n{carrier['claim']}\n"
        target_rows = ["| Target ID | Kind | State | Parent | Allowed claim kinds |", "|---|---|---|---|---|"]
        target_rows.extend("| %s | %s | %s | %s | %s |" % (t["id"], t["kind"], t["state"], t["parent_target_id"] or "—", ", ".join(t["allowed_claim_kinds"])) for t in targets)
        source_rows = ["| Source ID | State | Kind | Normative scope | Literature / revision | Applicable target IDs | Official source |", "|---|---|---|---|---|---|---|"]
        for source in sources:
            lit = "repository state" if source["kind"] != "TI_PRIMARY" else "%s / %s, %s%s" % (source["literature_number"], source["revision"], source["publication_date"], "; revised " + source["revision_date"] if source["revision_date"] else "")
            official = "[%s](%s)" % (source["title"], source["official_url"]) if source["official_url"] else "Local repository contract"
            source_rows.append("| %s | %s | %s | %s | %s | %s | %s |" % (source["id"], source["state"], source["kind"], source["normative_scope"], lit, ", ".join(source["applicable_target_ids"]), official))
        evidence_rows = ["| Evidence ID | State | Scope | Classification | Source | Applicable target IDs | Locator |", "|---|---|---|---|---|---|---|"]
        for item in evidence:
            locator = item["locator"]
            if "chapter" in locator:
                loc = "§%s, `%s`" % (locator["section"], locator["printed_page_start"] if locator["printed_page_start"] == locator["printed_page_end"] else locator["printed_page_start"] + "–" + locator["printed_page_end"])
            else:
                loc = "`%s` anchor `%s`" % (locator["path"], locator["anchor"])
            evidence_rows.append("| %s | %s | %s | %s | `%s` | %s | %s |" % (item["id"], item["state"], item["scope"], item["classification"], item["source_id"], ", ".join(item["applicable_target_ids"]), loc))
        rows = ["| Requirement ID | State | Claim kind | Target | Owner | Evidence IDs | Test IDs |", "|---|---|---|---|---|---|---|"]
        rows.extend("| %s | %s | %s | %s | %s | %s | %s |" % (r["id"], r["state"], r["claim_kind"], r["target_id"], r["owner_path"], ", ".join(r["evidence_ids"]), ", ".join(r["test_ids"])) for r in requirements)
        test_rows = ["| Test ID | State | Scope | Path | Requirement IDs | Runner |", "|---|---|---|---|---|---|"]
        test_rows.extend("| %s | %s | %s | %s | %s | %s |" % (t["id"], t["state"], t["scope"], t["path"], ", ".join(t["requirement_ids"]), json.dumps(t["runner"], separators=(",", ":")) if t["runner"] is not None else "null") for t in tests)
        all_ids = "\n".join(record["id"] for category in ("targets", "sources", "evidence") for record in self.registry[category])
        self.write("docs/architecture-baseline.md", architecture)
        self.write("docs/evidence-index.md", all_ids + "\n\n" + "\n".join(target_rows + ["", *source_rows, "", *evidence_rows, "", *rows, "", *test_rows]) + "\n")

    def errors(self) -> list[str]:
        return check_repository(self.root)

    def test_baseline_fixture_passes(self) -> None:
        self.assertEqual(self.errors(), [])

    def test_schema_exact_keys_and_metadata(self) -> None:
        self.registry["schema"]["name"] = "wrong"
        self.write_registry()
        self.assertTrue(any("schema.name" in error for error in self.errors()))

    def test_tombstone_collision_is_rejected(self) -> None:
        self.registry["tombstones"]["tests"].append({"id": "T-PROJECT-PASS", "retired_abi_major": 0, "reason": "old", "replacement_id": None})
        self.write_registry()
        self.assertTrue(any("reuse" in error for error in self.errors()))

    def test_missing_project_anchor_or_passing_path_is_rejected(self) -> None:
        self.registry["evidence"][2]["locator"]["anchor"] = "MISSING"
        self.registry["tests"][1]["path"] = "tests/contract/quality/missing.py"
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("anchor" in error for error in errors))
        self.assertTrue(any("does not exist" in error for error in errors))

    def test_profile_evidence_cannot_satisfy_common_requirement(self) -> None:
        self.registry["requirements"][0]["evidence_ids"] = ["E-TI-SPRS000A-S1-P1"]
        self.write_registry()
        self.assertTrue(any("leaks across claim scope" in error for error in self.errors()))

    def test_common_timing_claim_is_rejected(self) -> None:
        self.registry["requirements"][0]["claim"] = "The common target guarantees exact cycle timing."
        self.write_registry()
        self.assertTrue(any("positive timing" in error or "positive cycle" in error for error in self.errors()))

    def test_implemented_without_passing_test_and_unresolved_with_test_are_rejected(self) -> None:
        self.registry["requirements"][0]["state"] = "IMPLEMENTED"
        self.registry["requirements"][0]["test_ids"] = ["T-COMMON-PLANNED"]
        self.registry["requirements"][1]["test_ids"] = ["T-COMMON-PLANNED"]
        self.registry["tests"][0]["requirement_ids"].append("REQ-COMMON-UNRESOLVED")
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("IMPLEMENTED" in error for error in errors))
        self.assertTrue(any("UNRESOLVED" in error for error in errors))

    def test_reverse_link_drift_is_rejected(self) -> None:
        self.registry["requirements"][0]["test_ids"] = []
        self.write_registry()
        self.assertTrue(any("reverse links drift" in error for error in self.errors()))

    def test_planned_requirement_needs_linked_planned_test(self) -> None:
        self.registry["requirements"][0]["test_ids"] = []
        self.write_registry()
        self.assertTrue(any("PLANNED requirement" in error for error in self.errors()))

    def test_duplicate_requirement_link_ids_are_rejected_by_schema(self) -> None:
        self.registry["requirements"][0]["evidence_ids"].append(self.registry["requirements"][0]["evidence_ids"][0])
        self.write_registry()
        self.assertTrue(any("duplicate ID" in error for error in self.errors()))

    def test_docs_omission_and_drift_are_rejected(self) -> None:
        self.write("docs/evidence-index.md", "# stale\n")
        self.assertTrue(any(error.startswith("DOC:") for error in self.errors()))

    def test_undeclared_pseudo_target_and_invalid_owner_are_rejected(self) -> None:
        self.registry["requirements"][0]["target_id"] = "PROJECT_CONTRACT"
        self.registry["requirements"][0]["owner_path"] = "misc/"
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("missing target" in error for error in errors))
        self.assertTrue(any("outside" in error for error in errors))

    def test_duplicate_active_id_and_parent_cycle_are_rejected(self) -> None:
        self.registry["tests"].append(copy.deepcopy(self.registry["tests"][0]))
        self.registry["targets"][0]["parent_target_id"] = self.registry["targets"][1]["id"]
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("duplicate active ID" in error for error in errors))
        self.assertTrue(any("parent cycle" in error for error in errors))

    def test_classification_mismatch_and_planned_pinned_source_are_rejected(self) -> None:
        self.registry["evidence"][0]["classification"] = "PROJECT_CONTRACT_ONLY"
        self.registry["sources"][1]["state"] = "PLANNED"
        self.registry["evidence"][1]["state"] = "PINNED"
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("classification disagrees" in error for error in errors))
        self.assertTrue(any("cannot use PLANNED source" in error for error in errors))

    def test_implemented_requires_all_passing_tests_and_tests_need_requirements(self) -> None:
        extra = {"id": "T-PROJECT-PLANNED", "state": "PLANNED", "scope": "PROJECT_CONTRACT_ONLY", "path": "tests/contract/quality/future.py", "requirement_ids": ["REQ-PROJECT-IMPLEMENTED"], "runner": None}
        self.registry["tests"].append(extra)
        self.registry["requirements"][2]["test_ids"].append(extra["id"])
        self.registry["tests"].append({"id": "T-EMPTY", "state": "PLANNED", "scope": "COMMON_TI", "path": "tests/unit/future.c", "requirement_ids": [], "runner": None})
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("IMPLEMENTED" in error for error in errors))
        self.assertTrue(any("must link at least one requirement" in error for error in errors))

    def test_tombstone_replacement_must_resolve_and_not_self_reference(self) -> None:
        self.registry["tombstones"]["tests"] = [
            {"id": "T-RETIRED", "retired_abi_major": 0, "reason": "old", "replacement_id": "T-MISSING"},
            {"id": "T-SELF", "retired_abi_major": 0, "reason": "old", "replacement_id": "T-SELF"},
            {"id": "T-CYCLE-A", "retired_abi_major": 0, "reason": "old", "replacement_id": "T-CYCLE-B"},
            {"id": "T-CYCLE-B", "retired_abi_major": 0, "reason": "old", "replacement_id": "T-CYCLE-A"},
        ]
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("does not resolve" in error for error in errors))
        self.assertTrue(any("self-reference" in error for error in errors))
        self.assertTrue(any("replacement cycle" in error for error in errors))

    def test_normative_banned_tokens_in_evidence_and_requirement_are_rejected(self) -> None:
        self.registry["evidence"][0]["assertion"] = "This legacy emulator assertion is forbidden."
        self.registry["requirements"][0]["claim"] = "The common architecture matches emulator behavior."
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("TI evidence" in error for error in errors))
        self.assertTrue(any("normative requirement" in error for error in errors))

    def test_ti_page_chapter_and_order_are_rejected(self) -> None:
        self.registry["evidence"][0]["locator"]["chapter"] = "9"
        self.write_registry()
        self.assertTrue(any("printed pages" in error for error in self.errors()))
        self.registry["evidence"][0]["locator"]["chapter"] = "1"
        self.registry["evidence"][0]["locator"]["printed_page_end"] = "1-0"
        self.write_registry()
        self.assertTrue(any("printed pages" in error for error in self.errors()))

    def test_ti_id_and_revision_metadata_are_rejected(self) -> None:
        self.registry["sources"][0]["id"] = "SRC-TI-WRONG"
        self.registry["sources"][0]["revision"] = "B"
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("ID must correspond" in error for error in errors))
        self.assertTrue(any("revision must match" in error for error in errors))

    def test_common_term_word_boundaries_do_not_reject_innocent_words(self) -> None:
        self.registry["requirements"][0]["claim"] = "The common sequencer remains clockwise and deviceagnostic in its interface."
        self.write_registry()
        self.assertFalse(any(error.startswith("LEAKAGE:") for error in self.errors()))

    def test_profile_can_combine_common_and_profile_evidence(self) -> None:
        self.registry["requirements"][3]["evidence_ids"] = ["E-TI-SPRU000A-S1-P1", "E-TI-SPRS000A-S1-P1"]
        self.write_registry()
        self.write_docs()
        self.assertEqual(self.errors(), [])

    def test_profile_requires_profile_evidence(self) -> None:
        self.registry["requirements"][3]["evidence_ids"] = ["E-TI-SPRU000A-S1-P1"]
        self.write_registry()
        self.assertTrue(any("needs PROFILE_ONLY evidence" in error for error in self.errors()))

    def test_profile_unit_test_must_mirror_owner(self) -> None:
        self.registry["tests"][2]["path"] = "tests/unit/sequencer/test_profile.c"
        self.write_registry()
        self.assertTrue(any("normative test T-PROFILE-PLANNED does not mirror owner" in error for error in self.errors()))

    def test_source_url_parity_is_rejected(self) -> None:
        docs = (self.root / "docs/evidence-index.md").read_text(encoding="utf-8")
        docs = docs.replace("https://www.ti.com/lit/ug/spru000a/x.pdf", "https://www.ti.com/wrong.pdf")
        docs = docs.replace("Local repository contract", "https://example.invalid/project")
        self.write("docs/evidence-index.md", docs)
        errors = self.errors()
        self.assertTrue(any("SRC-TI-SPRU000A official source URL drifts" in error for error in errors))
        self.assertTrue(any("project source must not contain" in error for error in errors))

    def test_target_claim_and_applicability_contracts_are_rejected(self) -> None:
        self.registry["requirements"][3]["claim_kind"] = "ARCHITECTURAL"
        self.registry["sources"][0]["applicable_target_ids"] = []
        self.registry["evidence"][0]["applicable_target_ids"] = ["C54X_PROFILE_V1"]
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("claim kind is not allowed" in error for error in errors))
        self.assertTrue(any("must declare applicable targets" in error for error in errors))
        self.assertTrue(any("applicability must be non-empty" in error for error in errors))

    def test_profile_source_and_evidence_cannot_bind_sibling_profiles(self) -> None:
        sibling = copy.deepcopy(self.registry["targets"][1])
        sibling["id"] = "C54X_PROFILE_V2"
        self.registry["targets"].append(sibling)
        self.registry["sources"][1]["applicable_target_ids"] = ["C54X_PROFILE_V1", "C54X_PROFILE_V2"]
        self.registry["evidence"][1]["applicable_target_ids"] = ["C54X_PROFILE_V1", "C54X_PROFILE_V2"]
        self.write_registry()
        self.write_docs()
        errors = self.errors()
        self.assertTrue(any("must bind exactly one profile target" in error for error in errors))

    def test_runner_contract_rejects_planned_and_unregistered_passing_paths(self) -> None:
        self.registry["tests"][0]["runner"] = {"kind": "PYTHON_UNITTEST_DISCOVERY", "make_goal": "test", "ctest_name": "quality.discovery"}
        self.registry["tests"][1]["runner"]["kind"] = "C_EXECUTABLE"
        self.registry["tests"][1]["path"] = "README.md"
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("must be null for PLANNED tests" in error for error in errors))
        self.registry["tests"][0]["runner"] = None
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("normalized and repository-relative" in error or "must be under tests/unit or tests/contract" in error for error in errors))

    def test_make_test_recipe_must_execute_c_binary(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write("Makefile", ".PHONY: test\nFAKE_TEST := build/fake\ntest: build\n\t$(FAKE_TEST)\nbuild: $(FAKE_TEST)\n$(FAKE_TEST): tests/unit/cpu/test_fake.c\n\tcc $< -o $@\n")
        self.write("tests/CMakeLists.txt", "add_executable(fake tests/unit/cpu/test_fake.c)\nadd_test(NAME fake.ctest COMMAND fake)\n")
        tests = {"T-FAKE": {"state": "PASSING", "path": "tests/unit/cpu/test_fake.c", "runner": {"kind": "C_EXECUTABLE", "make_goal": "test", "ctest_name": "fake.ctest"}}}
        errors: list[str] = []
        validate_test_runners(self.root, tests, errors)
        self.assertEqual(errors, [])
        self.write("Makefile", ".PHONY: test\nFAKE_TEST := build/fake\ntest: build\nbuild: $(FAKE_TEST)\n$(FAKE_TEST): tests/unit/cpu/test_fake.c\n\tcc $< -o $@\n")
        errors = []
        validate_test_runners(self.root, tests, errors)
        self.assertTrue(any("not reachable" in error for error in errors))

    def test_python_quality_test_prerequisite_and_cmake_command_are_exact(self) -> None:
        tests = {"T-PY": {"state": "PASSING", "path": "tests/contract/quality/test_passing.py", "runner": {"kind": "PYTHON_UNITTEST_DISCOVERY", "make_goal": "test", "ctest_name": "quality.discovery"}}}
        errors: list[str] = []
        validate_test_runners(self.root, tests, errors)
        self.assertEqual(errors, [])
        self.write("Makefile", ".PHONY: test quality-test\ntest: build\nquality-test:\n\tpython3 -m unittest discover -s tests/contract/quality -t .\n")
        self.write("tests/CMakeLists.txt", "add_test(NAME quality.discovery COMMAND python3 -m unittest discover -s \"/not-real/tests/contract/quality\" -t \"${PROJECT_SOURCE_DIR}\")\n")
        errors = []
        validate_test_runners(self.root, tests, errors)
        self.assertTrue(any("Make test/quality-test" in error for error in errors))
        self.assertTrue(any("ctest is not registered" in error for error in errors))
        self.write("Makefile", ".PHONY: test quality-test\ntest: quality-test\nquality-test:\n\tpython3 -m unittest discover -s tests/contract/quality -t .\n")
        self.write("tests/CMakeLists.txt", "add_test(NAME quality.discovery COMMAND python3 -m unittest discover -s \"${PROJECT_SOURCE_DIR}/tests/contract/quality\" -t \"/not-real\")\n")
        errors = []
        validate_test_runners(self.root, tests, errors)
        self.assertTrue(any("ctest is not registered" in error for error in errors))

    def test_additional_common_hardware_claim_terms_are_rejected(self) -> None:
        self.registry["requirements"][0]["claim"] = "The common architecture guarantees ROM wait-state and pipeline timing."
        self.write_registry()
        errors = self.errors()
        self.assertTrue(any("positive rom" in error or "positive wait state" in error or "positive pipeline" in error for error in errors))

    def test_duplicate_evidence_index_row_is_rejected(self) -> None:
        docs = (self.root / "docs/evidence-index.md").read_text(encoding="utf-8")
        row = next(line for line in docs.splitlines() if line.startswith("| C54X_CORE_COMMON_V1 |"))
        self.write("docs/evidence-index.md", docs + row + "\n")
        self.assertTrue(any("duplicates target row" in error for error in self.errors()))

    def test_target_source_and_evidence_doc_scalar_drift_is_rejected(self) -> None:
        self.write("docs/evidence-index.md", (self.root / "docs/evidence-index.md").read_text(encoding="utf-8").replace("| C54X_PROFILE_V1 | PROFILE | PLANNED |", "| C54X_PROFILE_V1 | PROFILE | ACTIVE |").replace("| SRC-TI-SPRU000A | ACTIVE |", "| SRC-TI-SPRU000A | PLANNED |").replace("| E-TI-SPRU000A-S1-P1 | PINNED | COMMON_TI", "| E-TI-SPRU000A-S1-P1 | PINNED | PROFILE_ONLY"))
        errors = self.errors()
        self.assertTrue(any("C54X_PROFILE_V1 state drifts" in error for error in errors))
        self.assertTrue(any("SRC-TI-SPRU000A state drifts" in error for error in errors))
        self.assertTrue(any("E-TI-SPRU000A-S1-P1 scope drifts" in error for error in errors))

    def test_unit_architectural_test_must_mirror_owner(self) -> None:
        self.registry["tests"][0]["path"] = "tests/unit/address/test_future.c"
        self.write_registry()
        self.assertTrue(any("does not mirror owner" in error for error in self.errors()))

    def test_cli_has_stable_pass_line(self) -> None:
        self.assertEqual(main(["--root", str(self.root)]), 0)


if __name__ == "__main__":
    unittest.main()
