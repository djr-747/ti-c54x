"""Markdown parity checks for the evidence registry.

Owns: deterministic table parsing and exact architecture/evidence-index parity.
The registry schema and cross-record semantics remain in sibling modules.
"""

from __future__ import annotations

import re
import json
from pathlib import Path
from typing import Any


def _add(errors: list[str], message: str) -> None:
    errors.append(f"DOC: {message}")


def _ids(cell: str) -> set[str]:
    return {match.group(0) for match in re.finditer(r"[A-Z][A-Z0-9]*(?:[-_][A-Z0-9][A-Z0-9._]*)+", cell)}


def _tables(text: str) -> list[tuple[dict[str, int], list[list[str]]]]:
    lines = text.splitlines()
    found: list[tuple[dict[str, int], list[list[str]]]] = []
    index = 0
    while index + 1 < len(lines):
        if not lines[index].lstrip().startswith("|") or not lines[index + 1].lstrip().startswith("|"):
            index += 1
            continue
        headers = [cell.strip().strip("`").lower() for cell in lines[index].strip().strip("|").split("|")]
        separator = [cell.strip() for cell in lines[index + 1].strip().strip("|").split("|")]
        if not headers or not all(set(cell) <= {"-", ":", " "} for cell in separator):
            index += 1
            continue
        rows: list[list[str]] = []
        index += 2
        while index < len(lines) and lines[index].lstrip().startswith("|"):
            rows.append([cell.strip() for cell in lines[index].strip().strip("|").split("|")])
            index += 1
        found.append(({header: number for number, header in enumerate(headers)}, rows))
    return found


def _row(tables: list[tuple[dict[str, int], list[list[str]]]], identifier: str) -> tuple[dict[str, int], list[str]] | None:
    id_names = {"id", "target id", "source id", "evidence id", "requirement", "requirement id", "test", "test id"}
    for headers, rows in tables:
        column = next((number for name, number in headers.items() if name in id_names), None)
        if column is None:
            continue
        for cells in rows:
            if column < len(cells) and identifier in _ids(cells[column]):
                return headers, cells
    return None


def _row_count(tables: list[tuple[dict[str, int], list[list[str]]]], identifier: str) -> int:
    return sum(1 for headers, rows in tables for cells in rows if any(identifier in _ids(cell) for cell in cells[:1]))


def _cell(headers: dict[str, int], cells: list[str], *names: str) -> str:
    column = next((number for name, number in headers.items() if name in names), None)
    return cells[column].strip() if column is not None and column < len(cells) else ""


def _clean(value: str) -> str:
    return value.strip().strip("`").strip()


def _same(actual: str, expected: Any) -> bool:
    return _clean(actual) == str(expected)


def _set_cell(value: str) -> set[str]:
    return {part.strip().strip("`") for part in value.split(",") if part.strip().strip("`")}


def _runner_cell(value: str) -> Any:
    text = _clean(value)
    if text in {"", "—", "null", "None"}:
        return None
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return text


def _source_lit(value: str) -> tuple[str, str, str, str] | None:
    text = _clean(value)
    if text == "repository state":
        return (text, "", "", "project")
    match = re.fullmatch(r"([A-Za-z0-9]+)\s*/\s*([A-Za-z])\s*,\s*(\d{4}-\d{2})(?:;\s*revised\s*(\d{4}-\d{2}))?", text)
    return (match.group(1), match.group(2), match.group(3), match.group(4) or "") if match else None


def _source_url(value: str) -> str:
    match = re.search(r"\((https?://[^)]+)\)", value)
    return match.group(1) if match else ""


def _locator(value: str) -> tuple[str, ...] | None:
    section = re.search(r"§\s*([^,]+)", value)
    pages = re.findall(r"\b(\d+-\d+)\b", value)
    if section and pages:
        section_text = section.group(1).strip()
        chapter = pages[0].split("-", 1)[0]
        if section_text.startswith(chapter + " "):
            section_text = section_text[len(chapter) + 1 :]
        return ("ti", section_text, pages[0], pages[-1])
    project = re.search(r"`([^`]+)`\s+anchor\s+`([^`]+)`", value)
    return ("project", project.group(1), project.group(2)) if project else None


def _scalar_checks(identifier: str, headers: dict[str, int], cells: list[str], expected: dict[str, str], errors: list[str]) -> None:
    aliases = {
        "state": {"state"}, "kind": {"kind"}, "parent": {"parent", "parent target"},
        "scope": {"scope", "normative scope"}, "claim kind": {"claim kind", "claim_kind"},
        "target": {"target", "target id"}, "owner": {"owner", "owner path", "owner_path"},
        "path": {"path"}, "classification": {"classification"}, "source": {"source", "source id"},
    }
    for field, value in expected.items():
        names = aliases.get(field, {field})
        actual = _cell(headers, cells, *names)
        if not _same(actual, value):
            _add(errors, f"{identifier} {field} drifts (expected {value!r}, got {actual!r})")


def validate_docs(root: Path, registry: dict[str, Any], records: dict[str, dict[str, dict[str, Any]]], errors: list[str]) -> None:
    """Check exact registry parity in architecture-baseline and evidence-index."""

    try:
        architecture = (root / "docs" / "architecture-baseline.md").read_text(encoding="utf-8")
        index = (root / "docs" / "evidence-index.md").read_text(encoding="utf-8")
    except OSError as exc:
        _add(errors, f"cannot read architecture/evidence docs: {exc}")
        return
    architecture_flat = " ".join(architecture.split())
    primary = registry.get("primary_target_id")
    if primary not in architecture:
        _add(errors, "architecture baseline omits primary target")
    for identifier, target in records["targets"].items():
        if identifier not in architecture:
            _add(errors, f"architecture baseline omits target {identifier}")
        for exclusion in target.get("exclusions", []):
            if exclusion not in architecture:
                _add(errors, f"architecture baseline omits exclusion for {identifier}: {exclusion}")
        if target.get("kind") == "COMMON":
            carrier = target.get("program_address_carrier", {})
            checks = ("semantic_scope: " + str(carrier.get("semantic_scope")), f"{carrier.get('bits')} bits", f"unit `{carrier.get('unit')}`", str(carrier.get("claim")))
            for phrase in checks:
                if " ".join(phrase.split()) not in architecture_flat:
                    _add(errors, f"architecture baseline omits exact common carrier field for {identifier}: {phrase}")

    tables = _tables(index)
    for category, values in records.items():
        for identifier in values:
            if identifier not in index:
                _add(errors, f"evidence index omits {category[:-1]} {identifier}")
            if _row_count(tables, identifier) > 1:
                _add(errors, f"evidence index duplicates {category[:-1]} row {identifier}")

    for identifier, target in records["targets"].items():
        found = _row(tables, identifier)
        if found is None:
            _add(errors, f"evidence index lacks target row {identifier}")
            continue
        headers, cells = found
        parent = target.get("parent_target_id")
        parent_cell = _cell(headers, cells, "parent", "parent target")
        expected_parent = "—" if parent is None else parent
        _scalar_checks(identifier, headers, cells, {"kind": target.get("kind"), "state": target.get("state"), "parent": expected_parent}, errors)
        if _set_cell(_cell(headers, cells, "allowed claim kinds", "allowed_claim_kinds")) != set(target.get("allowed_claim_kinds", [])):
            _add(errors, f"{identifier} allowed claim kinds drift")

    for identifier, source in records["sources"].items():
        found = _row(tables, identifier)
        if found is None:
            _add(errors, f"evidence index lacks source row {identifier}")
            continue
        headers, cells = found
        _scalar_checks(identifier, headers, cells, {"state": source.get("state"), "kind": source.get("kind"), "scope": source.get("normative_scope")}, errors)
        if _ids(_cell(headers, cells, "applicable target ids", "applicable_target_ids", "applicable targets")) != set(source.get("applicable_target_ids", [])):
            _add(errors, f"{identifier} applicable targets drift")
        expected_lit = (source.get("literature_number"), source.get("revision"), source.get("publication_date"), source.get("revision_date") or "") if source.get("kind") == "TI_PRIMARY" else ("repository state", "", "", "project")
        if _source_lit(_cell(headers, cells, "literature / revision", "literature/revision")) != expected_lit:
            _add(errors, f"{identifier} literature/revision/date drifts")
        url_cell = _cell(headers, cells, "official source", "official url", "official_url")
        if source.get("kind") == "TI_PRIMARY":
            if _source_url(url_cell) != source.get("official_url"):
                _add(errors, f"{identifier} official source URL drifts")
        elif re.search(r"https?://", url_cell, re.IGNORECASE):
            _add(errors, f"{identifier} project source must not contain an HTTP URL")

    for identifier, item in records["evidence"].items():
        found = _row(tables, identifier)
        if found is None:
            _add(errors, f"evidence index lacks evidence row {identifier}")
            continue
        headers, cells = found
        _scalar_checks(identifier, headers, cells, {"state": item.get("state"), "scope": item.get("scope"), "classification": item.get("classification"), "source": item.get("source_id")}, errors)
        if _ids(_cell(headers, cells, "applicable target ids", "applicable_target_ids", "applicable targets")) != set(item.get("applicable_target_ids", [])):
            _add(errors, f"{identifier} applicable targets drift")
        expected = item.get("locator", {})
        parsed = _locator(_cell(headers, cells, "locator"))
        if set(expected) == {"chapter", "section", "printed_page_start", "printed_page_end"}:
            wanted = ("ti", expected["section"], expected["printed_page_start"], expected["printed_page_end"])
        else:
            wanted = ("project", expected.get("path"), expected.get("anchor"))
        if parsed != wanted:
            _add(errors, f"{identifier} locator drifts")

    for identifier, requirement in records["requirements"].items():
        found = _row(tables, identifier)
        if found is None:
            _add(errors, f"evidence index lacks requirement row {identifier}")
            continue
        headers, cells = found
        _scalar_checks(identifier, headers, cells, {"state": requirement.get("state"), "claim kind": requirement.get("claim_kind"), "target": requirement.get("target_id"), "owner": requirement.get("owner_path")}, errors)
        for field, expected in (("evidence", set(requirement.get("evidence_ids", []))), ("test", set(requirement.get("test_ids", [])))):
            names = ("evidence ids", "evidence_ids", "evidence") if field == "evidence" else ("test ids", "test_ids", "test", "tests")
            if _ids(_cell(headers, cells, *names)) != expected:
                _add(errors, f"{identifier} {field} links drift")

    for identifier, test in records["tests"].items():
        found = _row(tables, identifier)
        if found is None:
            _add(errors, f"evidence index lacks test row {identifier}")
            continue
        headers, cells = found
        _scalar_checks(identifier, headers, cells, {"state": test.get("state"), "scope": test.get("scope"), "path": test.get("path")}, errors)
        if _ids(_cell(headers, cells, "requirement ids", "requirement_ids", "requirements")) != set(test.get("requirement_ids", [])):
            _add(errors, f"{identifier} requirement links drift")
        if _runner_cell(_cell(headers, cells, "runner")) != test.get("runner"):
            _add(errors, f"{identifier} runner drifts")

    expected_tombstones = {(category, item["id"]): item for category, items in registry.get("tombstones", {}).items() for item in items}
    tombstone_rows: dict[tuple[str, str], dict[str, str]] = {}
    for headers, rows in tables:
        if not {"category", "id", "retired abi", "reason", "replacement"} <= set(headers):
            continue
        for cells in rows:
            category = _clean(_cell(headers, cells, "category"))
            identifier = _clean(_cell(headers, cells, "id"))
            if identifier in {"", "—"} and category in {"targets", "sources", "evidence", "requirements", "tests"}:
                continue
            key = (category, identifier)
            if key in tombstone_rows:
                _add(errors, f"tombstone row duplicates {category}:{identifier}")
            tombstone_rows[key] = {"retired_abi_major": _clean(_cell(headers, cells, "retired abi")), "reason": _clean(_cell(headers, cells, "reason")), "replacement_id": _clean(_cell(headers, cells, "replacement"))}
    for key, item in expected_tombstones.items():
        actual = tombstone_rows.get(key)
        wanted = {"retired_abi_major": str(item["retired_abi_major"]), "reason": item["reason"], "replacement_id": "—" if item["replacement_id"] is None else item["replacement_id"]}
        if actual != wanted:
            _add(errors, f"tombstone row drifts or is missing: {key[0]}:{key[1]}")
    for key in tombstone_rows:
        if key not in expected_tombstones:
            _add(errors, f"undeclared tombstone row: {key[0]}:{key[1]}")
