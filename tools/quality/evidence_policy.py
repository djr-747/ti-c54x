"""Cross-record and repository policy for the evidence registry.

Owns: target/source/evidence/requirement/test semantics, path/link parity,
scope leakage checks, and Markdown evidence-index parity.  Does not own: JSON
record shape, CLI handling, or build-system configuration.
"""

from __future__ import annotations

import re
from pathlib import Path, PurePosixPath
from typing import Any

try:  # Package import for tests; direct import for the standalone CLI.
    from .evidence_schema import ID_RE
except ImportError:  # pragma: no cover - exercised by direct CLI execution.
    from evidence_schema import ID_RE  # type: ignore[no-redef]


_NORMATIVE_BANNED = re.compile(r"\b(?:legacy|gnu|qemu|osmocom|calypso|binutils|emulator)\b", re.IGNORECASE)
_COMMON_EXCLUSION_GROUPS = (
    ("xpc", "extended"),
    ("device", "memory"),
    ("interrupt", "vector"),
    ("peripheral",),
    ("pll", "clock"),
    ("electrical",),
    ("errata",),
    ("timing",),
    ("cycle",),
    ("vc5402",),
)
_COMMON_CLAIM_BANNED = (
    "device",
    "xpc",
    "map",
    "memory map",
    "vector",
    "peripheral",
    "pll",
    "clock",
    "electrical",
    "errata",
    "timing",
    "cycle",
    "rom",
    "ram",
    "wait state",
    "wait-state",
    "interrupt source",
    "boot map",
    "device-specific",
    "pipeline",
    "machine-cycle",
)
_OWNER_PREFIXES = ("include/ti_c54x", "src/cpu", "src/foundation", "src/bus", "src/address", "src/isa/decode", "src/isa/ops", "src/sequencer", "src/devices", "src/state", "src/debug", "tools/quality", "config")
_ID_PREFIXES = {
    "targets": "C54X_",
    "sources": "SRC-",
    "evidence": "E-",
    "requirements": "REQ-",
    "tests": "T-",
}


def _add(errors: list[str], code: str, message: str) -> None:
    errors.append(f"{code}: {message}")


def _index(records: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    return {record.get("id"): record for record in records if isinstance(record, dict) and isinstance(record.get("id"), str)}


def _normalized_repo_path(root: Path, value: Any, location: str, errors: list[str], require_exists: bool = False) -> Path | None:
    if not isinstance(value, str) or not value or "\\" in value:
        _add(errors, "PATH", f"{location} must be a normalized repository-relative POSIX path")
        return None
    pure = PurePosixPath(value)
    if pure.is_absolute() or ".." in pure.parts or "." in pure.parts or str(pure) != value:
        _add(errors, "PATH", f"{location} must be normalized and repository-relative")
        return None
    path = root / Path(*pure.parts)
    if require_exists and not path.is_file():
        _add(errors, "PATH", f"{location} does not exist: {value}")
    return path


def _owner_path(value: Any, location: str, errors: list[str]) -> None:
    if not isinstance(value, str) or not value or "\\" in value:
        _add(errors, "OWNER", f"{location} is not a repository ownership path")
        return
    candidate = value[:-1] if value.endswith("/") else value
    pure = PurePosixPath(candidate)
    if pure.is_absolute() or "." in pure.parts or ".." in pure.parts or str(pure) != candidate:
        _add(errors, "OWNER", f"{location} is not a normalized repository ownership path")
        return
    if not any(value == prefix or value.startswith(prefix + "/") or value.startswith(prefix) and prefix.endswith("/") for prefix in _OWNER_PREFIXES):
        _add(errors, "OWNER", f"{location} is outside the declared ownership hierarchy: {value}")


def _positive_common_term(claim: str, term: str) -> bool:
    lower = claim.lower()
    matcher = re.compile(rf"(?<!\w){re.escape(term.lower())}(?!\w)")
    for found in matcher.finditer(lower):
        position = found.start()
        before = lower[max(0, position - 45) : position]
        after = lower[position + len(term) : position + len(term) + 70]
        if not re.search(r"\b(no|not|without|outside|excluded|exclude|never|later|profile|does not|unresolved)\b", before) and not re.search(r"\b(outside|excluded|exclude|later|profile-specific)\b", after):
            return True
    return False


def _strings(value: Any) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, dict):
        return [part for key, item in value.items() for part in (_strings(key) + _strings(item))]
    if isinstance(value, list):
        return [part for item in value for part in _strings(item)]
    return []


def _has_normative_banned(value: Any) -> bool:
    return any(_NORMATIVE_BANNED.search(part) for part in _strings(value))


def _valid_ti_pages(locator: dict[str, Any]) -> bool:
    chapter = str(locator.get("chapter", ""))
    pattern = re.compile(rf"^{re.escape(chapter)}-(\d+)$")
    start = pattern.fullmatch(str(locator.get("printed_page_start", "")))
    end = pattern.fullmatch(str(locator.get("printed_page_end", "")))
    return bool(start and end and int(start.group(1)) <= int(end.group(1)))


def _normalized_ids(cell: str) -> set[str]:
    return {match.group(0) for match in re.finditer(r"[A-Z][A-Z0-9]*(?:[-_][A-Z0-9][A-Z0-9._]*)+", cell)}


def validate_repository(root: Path, registry: dict[str, Any]) -> list[str]:
    """Return deterministic cross-record, path, semantic, and docs errors."""

    errors: list[str] = []
    categories = ("targets", "sources", "evidence", "requirements", "tests")
    records = {category: _index(registry.get(category, [])) for category in categories}
    tombstones = registry.get("tombstones", {})
    active_ids: set[str] = set()
    seen_ids: set[str] = set()
    for category, values in records.items():
        expected_prefix = _ID_PREFIXES[category]
        raw_records = registry.get(category, [])
        raw_ids = [item.get("id") for item in raw_records if isinstance(item, dict)]
        for identifier in sorted({item for item in raw_ids if isinstance(item, str) and raw_ids.count(item) > 1}):
            _add(errors, "ID", f"duplicate active ID: {identifier}")
        for identifier in values:
            if not identifier.startswith(expected_prefix):
                _add(errors, "ID", f"{category}.{identifier} has wrong namespace")
            if identifier in seen_ids:
                _add(errors, "ID", f"duplicate active ID: {identifier}")
            seen_ids.add(identifier)
            active_ids.add(identifier)
    for category, values in records.items():
        tombstone_values = tombstones.get(category, []) if isinstance(tombstones, dict) else []
        tomb_ids = [item.get("id") for item in tombstone_values if isinstance(item, dict)]
        if len(tomb_ids) != len(set(tomb_ids)):
            _add(errors, "ID", f"duplicate tombstone ID in {category}")
        for identifier in tomb_ids:
            if not isinstance(identifier, str) or not identifier.startswith(_ID_PREFIXES[category]):
                _add(errors, "ID", f"{category} tombstone has wrong namespace: {identifier}")
            if identifier in active_ids or identifier in seen_ids:
                _add(errors, "ID", f"active/tombstone ID reuse: {identifier}")
            seen_ids.add(identifier)
            replacement = next((item.get("replacement_id") for item in tombstone_values if isinstance(item, dict) and item.get("id") == identifier), None)
            if replacement is not None:
                namespace = set(values) | set(tomb_ids)
                if replacement == identifier:
                    _add(errors, "ID", f"tombstone {identifier} replacement_id cannot self-reference")
                elif replacement not in namespace:
                    _add(errors, "ID", f"tombstone {identifier} replacement_id does not resolve: {replacement}")
    for category in records:
        tombstone_values = tombstones.get(category, []) if isinstance(tombstones, dict) else []
        replacements = {item.get("id"): item.get("replacement_id") for item in tombstone_values if isinstance(item, dict) and item.get("replacement_id") is not None}
        for start in replacements:
            trail: set[str] = set()
            current = start
            while current in replacements:
                if current in trail:
                    _add(errors, "ID", f"tombstone replacement cycle in {category} includes {current}")
                    break
                trail.add(current)
                current = replacements[current]

    targets = records["targets"]
    primary = registry.get("primary_target_id")
    if primary not in targets or targets.get(primary, {}).get("state") != "ACTIVE" or targets.get(primary, {}).get("kind") != "COMMON":
        _add(errors, "TARGET", "primary_target_id must resolve to an active COMMON target")
    for identifier, target in targets.items():
        expected_claims = ["ARCHITECTURAL", "PROJECT_CONTRACT"] if target.get("kind") == "COMMON" else ["PROFILE"]
        if target.get("allowed_claim_kinds") != expected_claims:
            _add(errors, "TARGET", f"target {identifier} allowed_claim_kinds drift")
        parent = target.get("parent_target_id")
        if target.get("kind") == "COMMON" and parent is not None:
            _add(errors, "TARGET", f"COMMON target {identifier} cannot have a parent")
        if target.get("kind") == "PROFILE":
            if parent not in targets or targets.get(parent, {}).get("kind") != "COMMON":
                _add(errors, "TARGET", f"PROFILE target {identifier} parent must resolve to COMMON")
        if target.get("kind") == "COMMON":
            exclusions = " ".join(str(item) for item in target.get("exclusions", [])).lower()
            for group in _COMMON_EXCLUSION_GROUPS:
                if not all(term in exclusions for term in group):
                    _add(errors, "TARGET", f"COMMON target {identifier} is missing exclusion coverage for {'/'.join(group)}")
            carrier = target.get("program_address_carrier", {})
            if carrier.get("semantic_scope") != "CARRIER_ONLY" or carrier.get("bits") != 32 or carrier.get("unit") != "program words":
                _add(errors, "TARGET", f"COMMON target {identifier} has an invalid program-address carrier contract")
    for identifier in targets:
        trail: set[str] = set()
        current = identifier
        while current is not None:
            if current in trail:
                _add(errors, "TARGET", f"target parent cycle includes {identifier}")
                break
            trail.add(current)
            current = targets.get(current, {}).get("parent_target_id")

    sources = records["sources"]
    for identifier, source in sources.items():
        applicable = source.get("applicable_target_ids", [])
        if not applicable:
            _add(errors, "SOURCE", f"source {identifier} must declare applicable targets")
        if source.get("kind") == "TI_PRIMARY":
            expected_kind = "COMMON" if source.get("normative_scope") == "COMMON_ALLOWED" else "PROFILE"
            if any(targets.get(target_id, {}).get("kind") != expected_kind for target_id in applicable):
                _add(errors, "SOURCE", f"source {identifier} applicability does not match normative scope")
            if source.get("normative_scope") == "PROFILE_ONLY" and len(applicable) != 1:
                _add(errors, "SOURCE", f"PROFILE_ONLY source {identifier} must bind exactly one profile target")
        elif applicable != [primary]:
            _add(errors, "SOURCE", f"project source {identifier} must apply to the primary target only")
        if source.get("normative_scope") != "PROJECT_CONTRACT_ONLY" and _has_normative_banned(source):
            _add(errors, "SOURCE", f"normative source {identifier} carries forbidden legacy/platform lineage")
        if source.get("kind") == "TI_PRIMARY":
            literature = str(source.get("literature_number", ""))
            if identifier != f"SRC-TI-{literature}":
                _add(errors, "SOURCE", f"TI source {identifier} ID must correspond to literature number {literature}")
            if not re.fullmatch(r"[A-Za-z]", str(source.get("revision", ""))) or not literature or source["revision"].upper() != literature[-1].upper():
                _add(errors, "SOURCE", f"TI source {identifier} revision must match literature suffix")

    evidence = records["evidence"]
    for identifier, item in evidence.items():
        source = sources.get(item.get("source_id"))
        if source is None:
            _add(errors, "EVIDENCE", f"{identifier} references missing source {item.get('source_id')}")
            continue
        if source.get("kind") == "TI_PRIMARY" and _has_normative_banned(item):
            _add(errors, "SOURCE", f"TI evidence {identifier} carries forbidden legacy/platform lineage")
        expected_class = source.get("kind")
        if item.get("classification") != expected_class:
            _add(errors, "EVIDENCE", f"{identifier} classification disagrees with source {source.get('id')}")
        if item.get("state") == "PINNED" and source.get("state") != "ACTIVE":
            _add(errors, "STATE", f"PINNED evidence {identifier} cannot use PLANNED source {source.get('id')}")
        expected_scope = "PROJECT_CONTRACT_ONLY" if source.get("kind") == "PROJECT_CONTRACT_ONLY" else ("PROFILE_ONLY" if source.get("normative_scope") == "PROFILE_ONLY" else "COMMON_TI")
        if item.get("scope") != expected_scope:
            _add(errors, "EVIDENCE", f"{identifier} scope disagrees with source {source.get('id')}")
        applicable = item.get("applicable_target_ids", [])
        source_targets = set(source.get("applicable_target_ids", []))
        if not applicable or not set(applicable) <= source_targets:
            _add(errors, "EVIDENCE", f"evidence {identifier} applicability must be non-empty and within its source")
        if source.get("kind") == "PROJECT_CONTRACT_ONLY" and applicable != [primary]:
            _add(errors, "EVIDENCE", f"project evidence {identifier} must apply to the primary target")
        elif source.get("normative_scope") == "COMMON_ALLOWED" and any(targets.get(target_id, {}).get("kind") != "COMMON" for target_id in applicable):
            _add(errors, "EVIDENCE", f"common evidence {identifier} must apply only to COMMON targets")
        elif source.get("normative_scope") == "PROFILE_ONLY" and any(targets.get(target_id, {}).get("kind") != "PROFILE" for target_id in applicable):
            _add(errors, "EVIDENCE", f"profile evidence {identifier} must apply only to PROFILE targets")
        if source.get("normative_scope") == "PROFILE_ONLY" and len(applicable) != 1:
            _add(errors, "EVIDENCE", f"PROFILE_ONLY evidence {identifier} must bind exactly one profile target")
        locator = item.get("locator", {})
        if source.get("kind") == "TI_PRIMARY":
            literature = str(source.get("literature_number", ""))
            if literature and literature.lower() not in identifier.lower():
                _add(errors, "EVIDENCE", f"TI evidence {identifier} ID must include source literature number {literature}")
            if not all(isinstance(locator.get(key), str) and locator.get(key).strip() for key in ("chapter", "section", "printed_page_start", "printed_page_end")):
                _add(errors, "LOCATOR", f"TI evidence {identifier} requires non-empty chapter/section/printed page start/end")
            elif not _valid_ti_pages(locator):
                _add(errors, "LOCATOR", f"TI evidence {identifier} printed pages must match chapter and be ordered")
        else:
            path = _normalized_repo_path(root, locator.get("path"), f"evidence.{identifier}.locator.path", errors, require_exists=True)
            if path is not None:
                try:
                    if locator.get("anchor") not in path.read_text(encoding="utf-8"):
                        _add(errors, "LOCATOR", f"project evidence {identifier} anchor is not present in {locator.get('path')}")
                except (OSError, UnicodeDecodeError):
                    _add(errors, "LOCATOR", f"project evidence {identifier} locator path is not readable text")

    tests = records["tests"]
    requirements = records["requirements"]
    for identifier, test in tests.items():
        path = _normalized_repo_path(root, test.get("path"), f"tests.{identifier}.path", errors, require_exists=test.get("state") == "PASSING")
        if test.get("state") == "PASSING" and path is not None and not path.is_file():
            _add(errors, "TEST", f"PASSING test path does not exist: {test.get('path')}")
        if not test.get("requirement_ids"):
            _add(errors, "LINK", f"test {identifier} must link at least one requirement")
        for req_id in test.get("requirement_ids", []):
            if req_id not in requirements:
                _add(errors, "LINK", f"test {identifier} references missing requirement {req_id}")
    try:
        from .evidence_runner import validate_test_runners
    except ImportError:  # pragma: no cover - direct CLI execution
        from evidence_runner import validate_test_runners  # type: ignore[no-redef]
    validate_test_runners(root, tests, errors)

    for identifier, requirement in requirements.items():
        _owner_path(requirement.get("owner_path"), f"requirement.{identifier}.owner_path", errors)
        target = targets.get(requirement.get("target_id"))
        if target is None:
            _add(errors, "LINK", f"requirement {identifier} references missing target {requirement.get('target_id')}")
            continue
        refs_evidence = requirement.get("evidence_ids", [])
        refs_tests = requirement.get("test_ids", [])
        for evidence_id in refs_evidence:
            if evidence_id not in evidence:
                _add(errors, "LINK", f"requirement {identifier} references missing evidence {evidence_id}")
        for test_id in refs_tests:
            if test_id not in tests:
                _add(errors, "LINK", f"requirement {identifier} references missing test {test_id}")
        pinned_active = [
            evidence[evidence_id] for evidence_id in refs_evidence
            if evidence_id in evidence
            and evidence[evidence_id].get("state") == "PINNED"
            and sources.get(evidence[evidence_id].get("source_id"), {}).get("state") == "ACTIVE"
        ]
        if not refs_evidence or len(pinned_active) != len(refs_evidence):
            _add(errors, "STATE", f"requirement {identifier} needs only non-empty PINNED evidence from ACTIVE sources")
        if requirement.get("state") == "IMPLEMENTED":
            if target.get("state") != "ACTIVE" or not refs_tests or not all(tests.get(test_id, {}).get("state") == "PASSING" and (root / tests[test_id].get("path", "")).is_file() for test_id in refs_tests if test_id in tests) or any(test_id not in tests for test_id in refs_tests):
                _add(errors, "STATE", f"IMPLEMENTED requirement {identifier} requires all linked tests to be PASSING with existing files")
        elif requirement.get("state") == "PLANNED":
            if not refs_tests or any(test_id not in tests or tests[test_id].get("state") != "PLANNED" for test_id in refs_tests):
                _add(errors, "STATE", f"PLANNED requirement {identifier} needs at least one linked PLANNED test")
        elif requirement.get("state") == "UNRESOLVED" and (not refs_evidence or refs_tests):
            _add(errors, "STATE", f"UNRESOLVED requirement {identifier} needs evidence and zero tests")
        claim_kind = requirement.get("claim_kind")
        if claim_kind not in target.get("allowed_claim_kinds", []):
            _add(errors, "SCOPE", f"requirement {identifier} claim kind is not allowed by target {target.get('id')}")
        if claim_kind == "PROJECT_CONTRACT" and (target.get("id") != primary or target.get("kind") != "COMMON"):
            _add(errors, "SCOPE", f"project requirement {identifier} must bind the primary COMMON target")
        if claim_kind in {"ARCHITECTURAL", "PROFILE"} and _has_normative_banned({"id": identifier, "claim": requirement.get("claim", "")}):
            _add(errors, "SOURCE", f"normative requirement {identifier} carries forbidden legacy/platform lineage")
        if target.get("kind") == "PROFILE" and claim_kind != "PROFILE":
            _add(errors, "SCOPE", f"profile target requirement {identifier} must use PROFILE claim kind")
        if target.get("kind") == "COMMON" and claim_kind == "PROFILE":
            _add(errors, "SCOPE", f"common target requirement {identifier} cannot use PROFILE claim kind")
        expected_evidence = {"ARCHITECTURAL": {"COMMON_TI"}, "PROJECT_CONTRACT": {"PROJECT_CONTRACT_ONLY"}, "PROFILE": {"COMMON_TI", "PROFILE_ONLY"}}.get(claim_kind, set())
        expected_test = {"ARCHITECTURAL": "COMMON_TI", "PROJECT_CONTRACT": "PROJECT_CONTRACT_ONLY", "PROFILE": "PROFILE"}.get(claim_kind)
        if claim_kind == "PROFILE" and not any(evidence_id in evidence and evidence[evidence_id].get("scope") == "PROFILE_ONLY" for evidence_id in refs_evidence):
            _add(errors, "SCOPE", f"profile requirement {identifier} needs PROFILE_ONLY evidence")
        for evidence_id in refs_evidence:
            if evidence_id in evidence and evidence[evidence_id].get("scope") not in expected_evidence:
                _add(errors, "SCOPE", f"requirement {identifier} evidence {evidence_id} leaks across claim scope")
            if evidence_id in evidence:
                evidence_targets = set(evidence[evidence_id].get("applicable_target_ids", []))
                parent = target.get("parent_target_id")
                if claim_kind == "PROFILE":
                    if evidence[evidence_id].get("scope") == "PROFILE_ONLY" and evidence_targets != {target.get("id")}:
                        _add(errors, "SCOPE", f"profile requirement {identifier} profile evidence {evidence_id} must apply exactly to its profile target")
                    elif target.get("id") not in evidence_targets and parent not in evidence_targets:
                        _add(errors, "SCOPE", f"profile requirement {identifier} evidence {evidence_id} does not apply to target or parent")
                elif target.get("id") not in evidence_targets:
                    _add(errors, "SCOPE", f"requirement {identifier} evidence {evidence_id} does not apply to target")
        for test_id in refs_tests:
            if test_id in tests and tests[test_id].get("scope") != expected_test:
                _add(errors, "SCOPE", f"requirement {identifier} test {test_id} leaks across claim scope")
        if target.get("kind") == "COMMON" and claim_kind == "ARCHITECTURAL":
            for term in _COMMON_CLAIM_BANNED:
                if _positive_common_term(requirement.get("claim", ""), term):
                    _add(errors, "LEAKAGE", f"common architectural requirement {identifier} makes a positive {term} claim")

        if claim_kind in {"ARCHITECTURAL", "PROFILE"}:
            for test_id in refs_tests:
                if test_id not in tests:
                    continue
                test_path = tests[test_id].get("path", "")
                if not test_path.startswith("tests/unit/"):
                    continue
                owners = {req.get("owner_path", "") for req in requirements.values() if test_id in req.get("test_ids", []) and req.get("claim_kind") in {"ARCHITECTURAL", "PROFILE"}}
                if len(owners) > 1:
                    if not test_path.startswith("tests/contract/"):
                        _add(errors, "OWNER", f"normative test {test_id} links differing owners and must use tests/contract")
                elif owners:
                    owner = next(iter(owners)).rstrip("/")
                    if owner.startswith("src/"):
                        expected_path = "tests/unit/" + owner[4:] + "/"
                        if not test_path.startswith(expected_path):
                            _add(errors, "OWNER", f"normative test {test_id} does not mirror owner {owner}/")

    for identifier, requirement in requirements.items():
        declared = set(requirement.get("test_ids", []))
        reverse = {test_id for test_id, test in tests.items() if identifier in test.get("requirement_ids", [])}
        if declared != reverse:
            _add(errors, "LINK", f"requirement/test reverse links drift for {identifier}")
    for identifier, test in tests.items():
        declared = set(test.get("requirement_ids", []))
        reverse = {req_id for req_id, requirement in requirements.items() if identifier in requirement.get("test_ids", [])}
        if declared != reverse:
            _add(errors, "LINK", f"test/requirement reverse links drift for {identifier}")
        for req_id in declared:
            if requirements.get(req_id, {}).get("state") == "UNRESOLVED":
                _add(errors, "STATE", f"UNRESOLVED requirement {req_id} cannot be referenced by test {identifier}")

    try:
        from .evidence_docs import validate_docs
    except ImportError:  # pragma: no cover - direct CLI execution
        from evidence_docs import validate_docs  # type: ignore[no-redef]
    validate_docs(root, registry, records, errors)
    return sorted(set(errors))
