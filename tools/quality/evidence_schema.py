"""Schema and record-shape validation for the evidence registry.

Owns: JSON root/record keys, scalar types, identifiers, dates, and locator
shapes.  Does not own: repository path resolution, cross-record semantics,
Markdown parity, or command-line behavior.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Any


SCHEMA_NAME = "ti-c54x-evidence-registry"
SCHEMA_VERSION = 1
ABI_MAJOR = 0
ID_RE = re.compile(r"^[A-Z][A-Z0-9]*(?:[-_][A-Z0-9][A-Z0-9._]*)+$")
DATE_RE = re.compile(r"^\d{4}-\d{2}(?:-\d{2})?$")
ROOT_KEYS = {"schema", "primary_target_id", "targets", "sources", "evidence", "requirements", "tests", "tombstones"}
SCHEMA_KEYS = {"name", "version", "abi_major", "status", "access_date"}
TARGET_BASE_KEYS = {"id", "kind", "state", "parent_target_id", "name", "description", "inclusions", "exclusions", "allowed_claim_kinds"}
CARRIER_KEYS = {"bits", "unit", "owner_path", "semantic_scope", "claim"}
COMMON_TARGET_KEYS = TARGET_BASE_KEYS | {"program_address_carrier"}
PROFILE_TARGET_KEYS = TARGET_BASE_KEYS
SOURCE_KEYS = {"id", "kind", "normative_scope", "state", "authority", "literature_number", "title", "revision", "publication_date", "revision_date", "official_url", "scope", "access_date", "applicable_target_ids"}
EVIDENCE_KEYS = {"id", "immutable", "state", "classification", "scope", "source_id", "locator", "assertion", "applicable_target_ids"}
REQUIREMENT_KEYS = {"id", "state", "target_id", "claim_kind", "owner_path", "claim", "evidence_ids", "test_ids"}
TEST_KEYS = {"id", "state", "scope", "path", "requirement_ids", "runner"}
TOMBSTONE_KEYS = {"id", "retired_abi_major", "reason", "replacement_id"}


@dataclass(frozen=True)
class SchemaResult:
    """Parsed registry value and deterministic schema errors."""

    registry: dict[str, Any] | None
    errors: tuple[str, ...]


def _error(errors: list[str], location: str, message: str) -> None:
    errors.append(f"{location}: {message}")


def _keys(value: Any, expected: set[str], location: str, errors: list[str]) -> bool:
    if not isinstance(value, dict):
        _error(errors, location, "must be an object")
        return False
    actual = set(value)
    if actual != expected:
        _error(errors, location, f"keys must be exactly {sorted(expected)} (got {sorted(actual)})")
        return False
    return True


def _string(value: Any, location: str, errors: list[str], nonempty: bool = True) -> bool:
    if not isinstance(value, str) or (nonempty and not value.strip()):
        _error(errors, location, "must be a non-empty string")
        return False
    return True


def _id(value: Any, location: str, errors: list[str]) -> bool:
    if not _string(value, location, errors):
        return False
    if not ID_RE.fullmatch(value):
        _error(errors, location, "must be a namespaced uppercase ID")
        return False
    return True


def _date(value: Any, location: str, errors: list[str], allow_null: bool = False) -> None:
    if value is None and allow_null:
        return
    if not _string(value, location, errors) or not DATE_RE.fullmatch(value):
        _error(errors, location, "must be an ISO date (YYYY-MM-DD or YYYY-MM)")
        return
    try:
        date.fromisoformat(value if len(value) == 10 else f"{value}-01")
    except ValueError:
        _error(errors, location, "is not a valid ISO calendar date")


def _id_list(value: Any, location: str, errors: list[str]) -> None:
    if not isinstance(value, list):
        _error(errors, location, "must be a list")
        return
    seen: set[str] = set()
    for index, item in enumerate(value):
        _id(item, f"{location}[{index}]", errors)
        if isinstance(item, str) and item in seen:
            _error(errors, location, f"must not contain duplicate ID {item}")
        seen.add(item)


def validate_schema(registry: Any) -> list[str]:
    """Return deterministic errors for registry structure and scalar shape."""

    errors: list[str] = []
    if not _keys(registry, ROOT_KEYS, "root", errors):
        return sorted(errors)
    schema = registry["schema"]
    if _keys(schema, SCHEMA_KEYS, "schema", errors):
        if schema["name"] != SCHEMA_NAME:
            _error(errors, "schema.name", f"must equal {SCHEMA_NAME}")
        if schema["version"] != SCHEMA_VERSION or type(schema["version"]) is not int:
            _error(errors, "schema.version", f"must equal integer {SCHEMA_VERSION}")
        if schema["abi_major"] != ABI_MAJOR or type(schema["abi_major"]) is not int:
            _error(errors, "schema.abi_major", f"must equal integer {ABI_MAJOR}")
        _string(schema["status"], "schema.status", errors)
        _date(schema["access_date"], "schema.access_date", errors)
    _string(registry["primary_target_id"], "primary_target_id", errors)
    for name in ("targets", "sources", "evidence", "requirements", "tests"):
        if not isinstance(registry[name], list):
            _error(errors, name, "must be a list")
    if not _keys(registry["tombstones"], {"targets", "sources", "evidence", "requirements", "tests"}, "tombstones", errors):
        return sorted(errors)
    for category in ("targets", "sources", "evidence", "requirements", "tests"):
        records = registry[category]
        if not isinstance(records, list):
            continue
        for index, record in enumerate(records):
            location = f"{category}[{index}]"
            if category == "targets":
                expected = COMMON_TARGET_KEYS if isinstance(record, dict) and record.get("kind") == "COMMON" else PROFILE_TARGET_KEYS
            else:
                expected = {"sources": SOURCE_KEYS, "evidence": EVIDENCE_KEYS, "requirements": REQUIREMENT_KEYS, "tests": TEST_KEYS}[category]
            if not _keys(record, expected, location, errors):
                continue
            _id(record["id"], f"{location}.id", errors)
            if category == "targets":
                _validate_target(record, location, errors)
            elif category == "sources":
                _validate_source(record, location, errors)
            elif category == "evidence":
                _validate_evidence(record, location, errors)
            elif category == "requirements":
                _validate_requirement(record, location, errors)
            else:
                _validate_test(record, location, errors)
        tombstones = registry["tombstones"][category]
        if not isinstance(tombstones, list):
            _error(errors, f"tombstones.{category}", "must be a list")
            continue
        for index, record in enumerate(tombstones):
            location = f"tombstones.{category}[{index}]"
            if _keys(record, TOMBSTONE_KEYS, location, errors):
                _id(record["id"], f"{location}.id", errors)
                if type(record["retired_abi_major"]) is not int or record["retired_abi_major"] < 0 or record["retired_abi_major"] > ABI_MAJOR:
                    _error(errors, f"{location}.retired_abi_major", "must be an integer no greater than current ABI major")
                _string(record["reason"], f"{location}.reason", errors)
                if record["replacement_id"] is not None:
                    _id(record["replacement_id"], f"{location}.replacement_id", errors)
    return sorted(errors)


def _validate_target(record: dict[str, Any], location: str, errors: list[str]) -> None:
    if record["kind"] not in {"COMMON", "PROFILE"}:
        _error(errors, f"{location}.kind", "must be COMMON or PROFILE")
    if record["state"] not in {"ACTIVE", "PLANNED"}:
        _error(errors, f"{location}.state", "must be ACTIVE or PLANNED")
    if record["parent_target_id"] is not None:
        _id(record["parent_target_id"], f"{location}.parent_target_id", errors)
    for key in ("name", "description"):
        _string(record[key], f"{location}.{key}", errors)
    for key in ("inclusions", "exclusions"):
        if not isinstance(record[key], list) or not all(isinstance(item, str) and item.strip() for item in record[key]):
            _error(errors, f"{location}.{key}", "must be a list of non-empty strings")
    if not isinstance(record["allowed_claim_kinds"], list) or not record["allowed_claim_kinds"] or any(item not in {"ARCHITECTURAL", "PROJECT_CONTRACT", "PROFILE"} for item in record["allowed_claim_kinds"]):
        _error(errors, f"{location}.allowed_claim_kinds", "must be a non-empty list of valid claim kinds")
    elif len(set(record["allowed_claim_kinds"])) != len(record["allowed_claim_kinds"]):
        _error(errors, f"{location}.allowed_claim_kinds", "must not contain duplicates")
    expected_claims = {"COMMON": ["ARCHITECTURAL", "PROJECT_CONTRACT"], "PROFILE": ["PROFILE"]}[record["kind"]] if record["kind"] in {"COMMON", "PROFILE"} else []
    if record["allowed_claim_kinds"] != expected_claims:
        _error(errors, f"{location}.allowed_claim_kinds", f"must equal {expected_claims}")
    if record["kind"] == "COMMON":
        carrier = record.get("program_address_carrier")
        if not _keys(carrier, CARRIER_KEYS, f"{location}.program_address_carrier", errors):
            return
        if carrier["bits"] != 32 or type(carrier["bits"]) is not int:
            _error(errors, f"{location}.program_address_carrier.bits", "must be integer 32")
        if carrier["unit"] != "program words":
            _error(errors, f"{location}.program_address_carrier.unit", "must be program words")
        if carrier["semantic_scope"] != "CARRIER_ONLY":
            _error(errors, f"{location}.program_address_carrier.semantic_scope", "must be CARRIER_ONLY")
        _string(carrier["owner_path"], f"{location}.program_address_carrier.owner_path", errors)
        _string(carrier["claim"], f"{location}.program_address_carrier.claim", errors)


def _validate_source(record: dict[str, Any], location: str, errors: list[str]) -> None:
    if record["kind"] not in {"TI_PRIMARY", "PROJECT_CONTRACT_ONLY"}:
        _error(errors, f"{location}.kind", "must be TI_PRIMARY or PROJECT_CONTRACT_ONLY")
    if record["normative_scope"] not in {"COMMON_ALLOWED", "PROFILE_ONLY", "PROJECT_CONTRACT_ONLY"}:
        _error(errors, f"{location}.normative_scope", "has an invalid scope")
    if record["kind"] == "TI_PRIMARY" and record["normative_scope"] not in {"COMMON_ALLOWED", "PROFILE_ONLY"}:
        _error(errors, f"{location}.normative_scope", "TI_PRIMARY must be COMMON_ALLOWED or PROFILE_ONLY")
    if record["kind"] == "PROJECT_CONTRACT_ONLY" and record["normative_scope"] != "PROJECT_CONTRACT_ONLY":
        _error(errors, f"{location}.normative_scope", "PROJECT_CONTRACT_ONLY must use PROJECT_CONTRACT_ONLY scope")
    if record["state"] not in {"ACTIVE", "PLANNED"}:
        _error(errors, f"{location}.state", "must be ACTIVE or PLANNED")
    for key in ("authority", "title", "revision", "scope"):
        _string(record[key], f"{location}.{key}", errors)
    _id_list(record["applicable_target_ids"], f"{location}.applicable_target_ids", errors)
    _date(record["access_date"], f"{location}.access_date", errors)
    if record["kind"] == "TI_PRIMARY":
        for key in ("literature_number",):
            _string(record[key], f"{location}.{key}", errors)
        _date(record["publication_date"], f"{location}.publication_date", errors)
        _date(record["revision_date"], f"{location}.revision_date", errors, allow_null=True)
        if not isinstance(record["official_url"], str) or not re.fullmatch(r"https://www\.ti\.com/\S+", record["official_url"]):
            _error(errors, f"{location}.official_url", "must be an official https://www.ti.com URL")
    else:
        for key in ("literature_number", "publication_date", "revision_date", "official_url"):
            if record[key] is not None:
                _error(errors, f"{location}.{key}", "must be null for project contracts")


def _validate_evidence(record: dict[str, Any], location: str, errors: list[str]) -> None:
    if record["state"] not in {"PINNED", "PLANNED"}:
        _error(errors, f"{location}.state", "must be PINNED or PLANNED")
    if record["classification"] not in {"TI_PRIMARY", "PROJECT_CONTRACT_ONLY"}:
        _error(errors, f"{location}.classification", "has an invalid classification")
    if record["scope"] not in {"COMMON_TI", "PROFILE_ONLY", "PROJECT_CONTRACT_ONLY"}:
        _error(errors, f"{location}.scope", "has an invalid scope")
    if record["immutable"] is not True:
        _error(errors, f"{location}.immutable", "must be true")
    _id(record["source_id"], f"{location}.source_id", errors)
    _id_list(record["applicable_target_ids"], f"{location}.applicable_target_ids", errors)
    _string(record["assertion"], f"{location}.assertion", errors)
    locator = record["locator"]
    if not isinstance(locator, dict) or set(locator) not in ({"chapter", "section", "printed_page_start", "printed_page_end"}, {"path", "anchor"}):
        _error(errors, f"{location}.locator", "must be exact TI page or project path/anchor shape")
    elif set(locator) == {"chapter", "section", "printed_page_start", "printed_page_end"}:
        for key in ("chapter", "section", "printed_page_start", "printed_page_end"):
            _string(locator[key], f"{location}.locator.{key}", errors)
    elif set(locator) == {"path", "anchor"}:
        _string(locator["path"], f"{location}.locator.path", errors)
        _string(locator["anchor"], f"{location}.locator.anchor", errors)


def _validate_requirement(record: dict[str, Any], location: str, errors: list[str]) -> None:
    if record["state"] not in {"PLANNED", "IMPLEMENTED", "UNRESOLVED"}:
        _error(errors, f"{location}.state", "has an invalid state")
    if record["claim_kind"] not in {"ARCHITECTURAL", "PROJECT_CONTRACT", "PROFILE"}:
        _error(errors, f"{location}.claim_kind", "has an invalid claim kind")
    for key in ("target_id", "owner_path", "claim"):
        _string(record[key], f"{location}.{key}", errors)
    _id_list(record["evidence_ids"], f"{location}.evidence_ids", errors)
    _id_list(record["test_ids"], f"{location}.test_ids", errors)


def _validate_test(record: dict[str, Any], location: str, errors: list[str]) -> None:
    if record["state"] not in {"PLANNED", "PASSING"}:
        _error(errors, f"{location}.state", "must be PLANNED or PASSING")
    if record["scope"] not in {"COMMON_TI", "PROJECT_CONTRACT_ONLY", "PROFILE"}:
        _error(errors, f"{location}.scope", "has an invalid scope")
    _string(record["path"], f"{location}.path", errors)
    _id_list(record["requirement_ids"], f"{location}.requirement_ids", errors)
    runner = record["runner"]
    if record["state"] == "PLANNED":
        if runner is not None:
            _error(errors, f"{location}.runner", "must be null for PLANNED tests")
    elif not isinstance(runner, dict) or set(runner) != {"kind", "make_goal", "ctest_name"}:
        _error(errors, f"{location}.runner", "PASSING runner must have exactly kind, make_goal, ctest_name")
    elif runner["kind"] not in {"C_EXECUTABLE", "PYTHON_UNITTEST_DISCOVERY"} or runner["make_goal"] != "test" or not isinstance(runner["ctest_name"], str) or not runner["ctest_name"].strip():
        _error(errors, f"{location}.runner", "has invalid PASSING runner fields")


def load_registry(root: Path) -> SchemaResult:
    """Load config/evidence.json without network or process configuration."""

    path = root / "config" / "evidence.json"
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return SchemaResult(None, (f"config/evidence.json: cannot parse registry: {exc}",))
    return SchemaResult(value if isinstance(value, dict) else None, tuple(validate_schema(value)))
