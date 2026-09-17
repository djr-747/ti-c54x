#!/usr/bin/env python3
"""Owns repository-surface checks for the machine-readable knob registry.

Schema and lifecycle semantics live in ``knob_schema.py``.  This module owns
build/source switch discovery, public trace-header proof, Markdown parity, and
the command-line entry point; it does not own build configuration or runtime
behavior.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Any, Iterable

try:  # Supports both ``python -m tools.quality.check_knobs`` and direct CLI use.
    from .knob_schema import (  # type: ignore[import-not-found]
        ALLOWED_CLASSES,
        ALLOWED_DISPOSITIONS,
        ALLOWED_STATES,
        RegistryError,
        _parse_mask,
        load_registry,
        validate_registry,
    )
except ImportError:  # pragma: no cover - exercised by the direct CLI.
    from knob_schema import (  # type: ignore[no-redef]
        ALLOWED_CLASSES,
        ALLOWED_DISPOSITIONS,
        ALLOWED_STATES,
        RegistryError,
        _parse_mask,
        load_registry,
        validate_registry,
    )


PROJECT_COMPILE_RE = re.compile(r"\bTI_C54X_[A-Z][A-Z0-9_]*\b")
PREPROCESSOR_RE = re.compile(
    r"^\s*#\s*(?:if|ifdef|ifndef|elif)\b(?P<body>.*)$"
)
BUILD_FILENAMES = {"CMakeLists.txt", "Makefile", "GNUmakefile", "makefile"}


def _require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def _strip_hash_comments(text: str) -> str:
    """Strip shell/CMake/Make comments while preserving quoted build values."""

    text = re.sub(
        r"#\[=*\[.*?\]=*\]",
        lambda match: "".join("\n" if char == "\n" else " "
                               for char in match.group(0)),
        text,
        flags=re.DOTALL,
    )
    output: list[str] = []
    quote = ""
    escaped = False
    for char in text:
        if quote == "comment":
            if char == "\n":
                quote = ""
                output.append(char)
            else:
                output.append(" ")
            continue
        if quote in {'"', "'"}:
            if escaped:
                output.append(char)
                escaped = False
            elif char == "\\":
                output.append(char)
                escaped = True
            elif char == quote:
                output.append(char)
                quote = ""
            else:
                output.append(char)
            continue
        if escaped:
            output.append(char)
            escaped = False
            continue
        if char == "\\":
            output.append(char)
            escaped = True
            continue
        if char in {'"', "'"}:
            quote = char
            output.append(char)
            continue
        if char == "#":
            output.append("\n")
            quote = "comment"
            continue
        output.append(char)
    return "".join(output)


def _strip_c_comments(text: str) -> str:
    """Strip C comments while preserving line positions for preprocessor scans."""

    text = re.sub(
        r"/\*.*?\*/",
        lambda match: "".join("\n" if char == "\n" else " "
                               for char in match.group(0)),
        text,
        flags=re.DOTALL,
    )
    return re.sub(r"//[^\n]*", "", text)


def _iter_project_files(root: Path) -> Iterable[Path]:
    for path in root.rglob("*"):
        # ".muse" holds agent worktrees and session state: never deliverable content.
        if not path.is_file() or ".git" in path.parts or ".muse" in path.parts:
            continue
        if path.name in BUILD_FILENAMES or path.suffix in {".cmake", ".mk"}:
            yield path
        elif path.suffix in {".c", ".h"} and any(
                part in path.parts for part in ("include", "src", "tests", "examples")):
            yield path


def scan_project_compile_knobs(root: Path) -> dict[str, list[str]]:
    """Find project compile switches and return identifier -> source locations."""

    found: dict[str, list[str]] = {}
    for path in _iter_project_files(root):
        try:
            text = path.read_text(encoding="utf-8")
        except OSError:
            continue
        is_source = path.suffix in {".c", ".h"}
        scan_text = _strip_c_comments(text) if is_source else _strip_hash_comments(text)
        for line_number, line in enumerate(scan_text.splitlines(), start=1):
            if is_source:
                match = PREPROCESSOR_RE.search(line)
                names = [] if match is None else PROJECT_COMPILE_RE.findall(match.group("body"))
                names = [name for name in names
                         if not name.endswith("_H") and not name.endswith("_H_")]
            else:
                names = PROJECT_COMPILE_RE.findall(line)
            for name in names:
                location = f"{path.relative_to(root)}:{line_number}"
                found.setdefault(name, []).append(location)
    return found


def _parse_trace_header(path: Path) -> tuple[dict[str, int], dict[str, int], list[str]]:
    """Parse public trace kind and mask definitions from *path*."""

    if not path.is_file():
        return {}, {}, [f"missing public trace header: {path}"]
    try:
        text = _strip_c_comments(path.read_text(encoding="utf-8"))
    except OSError as exc:
        return {}, {}, [f"cannot read public trace header {path}: {exc}"]
    kinds = {
        match.group(1): int(match.group(2), 10)
        for match in re.finditer(
            r"\b(TI_C54X_TRACE_[A-Z][A-Z0-9_]*)\s*=\s*(\d+)\s*(?:,|})", text
        )
    }
    masks = {
        match.group(1): int(match.group(2), 16)
        for match in re.finditer(
            r"^\s*#\s*define\s+(TI_C54X_TRACE_MASK_[A-Z][A-Z0-9_]*)\s+"
            r"UINT64_C\(0x([0-9a-fA-F]{1,16})\)", text, flags=re.MULTILINE
        )
    }
    return kinds, masks, []


def check_trace_header(root: Path, registry: dict[str, Any]) -> list[str]:
    """Prove each active registry bit is represented by the public trace header."""

    path = root / "include" / "ti_c54x" / "trace.h"
    kinds, masks, errors = _parse_trace_header(path)
    active_bits = [bit for bit in registry.get("observer_bits", [])
                   if bit.get("state") == "ACTIVE"]
    expected_kinds = {bit.get("kind_c_identifier") for bit in active_bits}
    expected_masks = {bit.get("mask_c_identifier") for bit in active_bits}
    for identifier in sorted(set(kinds) - expected_kinds):
        errors.append(f"trace header has unregistered observer kind identifier: {identifier}")
    for identifier in sorted(set(masks) - expected_masks):
        if identifier != "TI_C54X_TRACE_MASK_ALL":
            errors.append(f"trace header has unregistered observer mask identifier: {identifier}")
    for bit in registry.get("observer_bits", []):
        if bit.get("state") != "ACTIVE":
            continue
        kind_identifier = bit.get("kind_c_identifier")
        if kind_identifier not in kinds:
            errors.append(f"trace header is missing observer kind identifier: {kind_identifier}")
        elif kinds[kind_identifier] != bit.get("bit"):
            errors.append(
                f"trace header value for {kind_identifier} does not match registry bit {bit.get('bit')}"
            )
        mask_identifier = bit.get("mask_c_identifier")
        if mask_identifier not in masks:
            errors.append(f"trace header is missing observer mask identifier: {mask_identifier}")
        else:
            expected = _parse_mask(bit.get("mask"), f"observer bit {bit.get('id')}.mask", errors)
            if expected is not None and masks[mask_identifier] != expected:
                errors.append(
                    f"trace header literal for {mask_identifier} does not match registry mask {bit.get('mask')}"
                )
    return errors


def _markdown_rows(text: str) -> dict[str, tuple[str, str, str]]:
    rows: dict[str, tuple[str, str, str]] = {}
    header_seen = False
    for line in text.splitlines():
        if "| Identifier |" in line and "| State |" in line:
            header_seen = True
            continue
        if not header_seen or not line.lstrip().startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) < 4 or cells[0] in {"---", ""}:
            continue
        if cells[1] not in ALLOWED_STATES or cells[2] not in ALLOWED_CLASSES \
                or cells[3] not in ALLOWED_DISPOSITIONS:
            continue
        identifier = cells[0].strip("`")
        if identifier:
            rows[identifier] = (cells[1], cells[2], cells[3])
    return rows


def _observer_markdown_rows(text: str) -> dict[str, tuple[str, str, str, int, int]]:
    """Parse the observer allocation table into normalized, comparable values."""

    rows: dict[str, tuple[str, str, str, int, int]] = {}
    header_seen = False
    for line in text.splitlines():
        if "| Logical identifier |" in line and "| Mask C identifier |" in line:
            header_seen = True
            continue
        if not header_seen or not line.lstrip().startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) < 6 or cells[0] in {"---", ""}:
            continue
        try:
            bit = int(cells[4])
            mask = int(cells[5].strip("`"), 0)
        except ValueError:
            continue
        identifier = cells[0].strip("`")
        if identifier:
            rows[identifier] = (
                cells[1].strip("`"),
                cells[2].strip("`"),
                cells[3],
                bit,
                mask,
            )
    return rows


def check_documentation(registry: dict[str, Any], docs_text: str) -> list[str]:
    errors: list[str] = []
    rows = _markdown_rows(docs_text)
    for entry in registry["knobs"]:
        identifier = entry["id"]
        expected = (entry["state"], entry["class"], entry["disposition"])
        _require(rows.get(identifier) == expected,
                 f"docs/knobs.md row for {identifier} must contain state/class/disposition {expected!r}",
                 errors)
    registry_ids = {entry["id"] for entry in registry["knobs"]}
    for identifier in rows:
        _require(identifier in registry_ids,
                 f"docs/knobs.md contains unregistered knob row: {identifier}", errors)
    _require("Observer-bit allocations" in docs_text,
             "docs/knobs.md must contain an Observer-bit allocations section", errors)
    observer_rows = _observer_markdown_rows(docs_text)
    observer_ids = {bit["id"] for bit in registry["observer_bits"]}
    for identifier in observer_rows:
        _require(identifier in observer_ids,
                 f"docs/knobs.md contains unregistered observer-bit row: {identifier}", errors)
    for bit in registry["observer_bits"]:
        expected = (
            bit["kind_c_identifier"],
            bit["mask_c_identifier"],
            bit["state"],
            bit["bit"],
            int(bit["mask"], 16),
        )
        _require(observer_rows.get(bit["id"]) == expected,
                 f"docs/knobs.md observer row for {bit['id']} does not match registry", errors)
    for tombstone in registry["observer_bit_tombstones"]:
        _require(tombstone["id"] in docs_text,
                 f"docs/knobs.md is missing observer-bit tombstone {tombstone['id']}", errors)
    for tombstone in registry.get("knob_tombstones", []):
        _require(tombstone["id"] in docs_text,
                 f"docs/knobs.md is missing knob tombstone {tombstone['id']}", errors)
    return errors


def check_repository(root: Path) -> list[str]:
    """Return all registry, source/build, trace-header, and documentation errors."""

    try:
        registry = load_registry(root / "config" / "knobs.json")
    except RegistryError as exc:
        return str(exc).splitlines()
    errors: list[str] = []
    found = scan_project_compile_knobs(root)
    declared = {entry["id"]: entry for entry in registry["knobs"]}
    for identifier, locations in sorted(found.items()):
        entry = declared.get(identifier)
        if entry is None:
            errors.append(f"unregistered project compile knob {identifier}: {', '.join(locations)}")
        elif entry["state"] != "ACTIVE":
            errors.append(f"PLANNED knob {identifier} has project call sites: {', '.join(locations)}")
    for identifier, entry in sorted(declared.items()):
        if entry["surface"] == "compile" and entry["state"] == "ACTIVE" and identifier not in found:
            errors.append(f"ACTIVE compile knob {identifier} has no project build/source call site")
    errors.extend(check_trace_header(root, registry))
    try:
        docs_text = (root / "docs" / "knobs.md").read_text(encoding="utf-8")
    except OSError as exc:
        errors.append(f"cannot read {root / 'docs' / 'knobs.md'}: {exc}")
    else:
        errors.extend(check_documentation(registry, docs_text))
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args(argv)
    root = args.root.resolve()
    errors = check_repository(root)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    registry = load_registry(root / "config" / "knobs.json")
    print(f"knob registry OK: {len(registry['knobs'])} knobs, "
          f"{len(registry['observer_bits'])} active observer bits, "
          f"{len(registry['observer_bit_tombstones'])} observer-bit tombstones, "
          f"{len(registry['knob_tombstones'])} knob tombstones")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
