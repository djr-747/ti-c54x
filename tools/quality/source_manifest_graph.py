#!/usr/bin/env python3
"""Parse and expand the production source-manifest ownership graph.

Owns source-manifest records, normalized Make/CMake include and source
parsing, and ordered graph expansion.  It deliberately does not own build
policy (object derivation, observer policy, or diagnostics); callers provide
an error sink and decide how the resulting canonical source sequence is
validated.
"""

from __future__ import annotations

import re
import shlex
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Iterable


OWNER_MANIFEST_RE = re.compile(r"^src/([^/][^/]*(?:/[^/][^/]*)*)/sources\.(?:mk|cmake)$")


@dataclass
class ManifestRecord:
    path: Path
    operations: list[tuple[str, str | Path]]

    @property
    def entries(self) -> list[str]:
        return [value for kind, value in self.operations if kind == "source" and isinstance(value, str)]

    @property
    def includes(self) -> list[Path]:
        return [value for kind, value in self.operations if kind == "include" and isinstance(value, Path)]


def _error(errors: list[str], code: str, message: str) -> None:
    errors.append(f"MANIFEST:{code}: {message}")


def relative_path(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def manifest_owner(root: Path, path: Path) -> str | None:
    match = OWNER_MANIFEST_RE.match(relative_path(root, path))
    return match.group(1) if match is not None else None


def logical_make_lines(text: str) -> Iterable[tuple[int, str]]:
    pending = ""
    start = 0
    for number, raw in enumerate(text.splitlines(), start=1):
        line = raw.split("#", 1)[0].rstrip()
        if pending:
            pending += line.lstrip()
        else:
            pending = line
            start = number
        if pending.endswith("\\"):
            pending = pending[:-1] + " "
            continue
        yield start, pending
        pending = ""
    if pending:
        yield start, pending


def _safe_relative_source(root: Path, base: Path, token: str) -> str | None:
    value = token.strip().strip('"\'')
    if not value or "\\" in value or "$" in value or "{" in value or "}" in value:
        return None
    pure = PurePosixPath(value)
    if pure.is_absolute() or any(part in {"", ".", ".."} for part in pure.parts):
        return None
    if not value.endswith(".c"):
        return None
    candidate = (base / value).resolve()
    try:
        relative = candidate.relative_to(root.resolve())
    except ValueError:
        return None
    if not candidate.is_file():
        return relative_path(root, candidate)
    return relative.as_posix()


def _source_token_error(root: Path, manifest: Path, token: str) -> str:
    value = token.strip().strip('"\'')
    if ".." in PurePosixPath(value).parts or value.startswith("/") or "\\" in value:
        return f"{relative_path(root, manifest)} contains traversal or absolute source token {value!r}"
    if "$" in value or "{" in value or "}" in value:
        return f"{relative_path(root, manifest)} contains unresolved source token {value!r}"
    return f"{relative_path(root, manifest)} contains malformed source token {value!r}"


def _include_candidate(token: str, suffix: str) -> bool:
    value = token.strip().strip('"\'')
    return value.endswith(f"sources.{suffix}") or f"/sources.{suffix}" in value


def _safe_include(
    root: Path,
    manifest: Path,
    token: str,
    suffix: str,
    errors: list[str],
) -> Path | None:
    value = token.strip().strip('"\'')
    base = manifest.parent
    if suffix == "mk":
        if value.startswith("src/"):
            base = root
    elif value.startswith("${CMAKE_CURRENT_LIST_DIR}/"):
        value = value.removeprefix("${CMAKE_CURRENT_LIST_DIR}/")
        base = manifest.parent
    elif value.startswith("${CMAKE_CURRENT_SOURCE_DIR}/"):
        value = value.removeprefix("${CMAKE_CURRENT_SOURCE_DIR}/")
        base = root
    else:
        _error(errors, "TOKEN", f"{relative_path(root, manifest)} contains malformed manifest include {value!r}")
        return None
    if (
        not value
        or "\\" in value
        or "$" in value
        or "{" in value
        or "}" in value
        or value.startswith("/")
        or any(part in {"", ".", ".."} for part in value.split("/"))
    ):
        _error(errors, "TOKEN", f"{relative_path(root, manifest)} contains malformed manifest include {value!r}")
        return None
    pure = PurePosixPath(value)
    if pure.is_absolute() or not value.endswith(f"sources.{suffix}"):
        _error(errors, "TOKEN", f"{relative_path(root, manifest)} contains malformed manifest include {value!r}")
        return None
    candidate = (base / value).resolve()
    try:
        relative = candidate.relative_to(root.resolve()).as_posix()
    except ValueError:
        _error(errors, "TOKEN", f"{relative_path(root, manifest)} contains malformed manifest include {value!r}")
        return None
    if OWNER_MANIFEST_RE.fullmatch(relative) is None:
        _error(errors, "TOKEN", f"{relative_path(root, manifest)} contains malformed manifest include {value!r}")
        return None
    return candidate


def _append_include(
    root: Path,
    manifest: Path,
    token: str,
    suffix: str,
    operations: list[tuple[str, str | Path]],
    errors: list[str],
) -> None:
    if not _include_candidate(token, suffix):
        return
    target = _safe_include(root, manifest, token, suffix, errors)
    if target is None:
        return
    if any(kind == "include" and value == target for kind, value in operations):
        _error(
            errors,
            "DUPLICATE",
            f"{relative_path(root, manifest)} includes {relative_path(root, target)} more than once",
        )
    operations.append(("include", target))


def _parse_make_file(root: Path, path: Path, errors: list[str], *, sources: bool) -> ManifestRecord:
    operations: list[tuple[str, str | Path]] = []
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        _error(errors, "READ", f"cannot read {relative_path(root, path)}: {exc}")
        return ManifestRecord(path, operations)
    for line_number, line in logical_make_lines(text):
        include = re.match(r"(?:-?include)\s+(.+)$", line)
        if include is not None:
            try:
                tokens = shlex.split(include.group(1), posix=True)
            except ValueError as exc:
                _error(errors, "SYNTAX", f"{relative_path(root, path)}:{line_number}: {exc}")
                continue
            for token in tokens:
                _append_include(root, path, token, "mk", operations, errors)
            continue
        if not sources:
            continue
        match = re.search(r"\bti_c54x_core_sources\s*\+=\s*(.*)$", line)
        if match is None:
            continue
        try:
            tokens = shlex.split(match.group(1), posix=True)
        except ValueError as exc:
            _error(errors, "SYNTAX", f"{relative_path(root, path)}:{line_number}: {exc}")
            continue
        for token in tokens:
            base = root if token.startswith("src/") else path.parent
            normalized = _safe_relative_source(root, base, token)
            if normalized is None:
                _error(errors, "TOKEN", _source_token_error(root, path, token))
                continue
            operations.append(("source", normalized))
    return ManifestRecord(path, operations)


def _cmake_commands(text: str) -> Iterable[tuple[str, str]]:
    pattern = re.compile(r"\b(include|list)\s*\(", re.IGNORECASE)
    for match in pattern.finditer(text):
        command = match.group(1).lower()
        opening = text.find("(", match.start(), match.end())
        depth = 1
        index = opening + 1
        quote = False
        escaped = False
        while index < len(text) and depth:
            char = text[index]
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quote = not quote
            elif not quote and char == "(":
                depth += 1
            elif not quote and char == ")":
                depth -= 1
            index += 1
        if depth == 0:
            yield command, text[opening + 1 : index - 1]


def _parse_cmake_file(root: Path, path: Path, errors: list[str], *, sources: bool) -> ManifestRecord:
    operations: list[tuple[str, str | Path]] = []
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        _error(errors, "READ", f"cannot read {relative_path(root, path)}: {exc}")
        return ManifestRecord(path, operations)
    text = re.sub(r"#[^\n]*", "", text)
    for command, body in _cmake_commands(text):
        try:
            tokens = shlex.split(body, posix=True)
        except ValueError as exc:
            _error(errors, "SYNTAX", f"{relative_path(root, path)}: {exc}")
            continue
        if command == "include":
            for token in tokens:
                _append_include(root, path, token, "cmake", operations, errors)
            continue
        if not sources or len(tokens) < 3 or tokens[0].upper() != "APPEND" or tokens[1] != "ti_c54x_core_sources":
            continue
        for token in tokens[2:]:
            if token.startswith("${CMAKE_CURRENT_LIST_DIR}/"):
                token = token.removeprefix("${CMAKE_CURRENT_LIST_DIR}/")
                normalized = _safe_relative_source(root, path.parent, token)
            else:
                normalized = _safe_relative_source(root, root, token)
            if normalized is None:
                _error(errors, "TOKEN", _source_token_error(root, path, token))
                continue
            operations.append(("source", normalized))
    return ManifestRecord(path, operations)


def _parse_file(root: Path, path: Path, errors: list[str], kind: str, *, sources: bool) -> ManifestRecord:
    if kind == "mk":
        return _parse_make_file(root, path, errors, sources=sources)
    return _parse_cmake_file(root, path, errors, sources=sources)


def _manifests(root: Path, suffix: str) -> list[Path]:
    source_root = root / "src"
    if not source_root.is_dir():
        return []
    return sorted(path for path in source_root.rglob(f"sources.{suffix}") if path.is_file())


def discover_manifests(root: Path, errors: list[str]) -> tuple[list[Path], list[Path]]:
    make = _manifests(root, "mk")
    cmake = _manifests(root, "cmake")
    for kind, paths in (("mk", make), ("cmake", cmake)):
        for path in paths:
            relative = relative_path(root, path)
            if OWNER_MANIFEST_RE.match(relative) is None:
                _error(errors, "LOCATION", f"{relative} is not a src/<owner>/sources.{kind} manifest")
    make_names = {relative_path(root, path).removesuffix(".mk") for path in make}
    cmake_names = {relative_path(root, path).removesuffix(".cmake") for path in cmake}
    for missing in sorted(make_names - cmake_names):
        _error(errors, "PAIR", f"missing CMake pair for {missing}.mk")
    for missing in sorted(cmake_names - make_names):
        _error(errors, "PAIR", f"missing Make pair for {missing}.cmake")
    return make, cmake


def expand_manifest_graph(
    root: Path,
    manifests: Iterable[Path],
    kind: str,
    errors: list[str],
) -> tuple[list[Path], list[str], list[tuple[Path, list[str]]]]:
    """Return manifest preorder, expanded source order, and direct entries."""

    records = {path.resolve(): _parse_file(root, path, errors, kind, sources=True) for path in manifests}
    for record in records.values():
        if not record.entries and not record.includes:
            _error(errors, "EMPTY", f"{relative_path(root, record.path)} has no source or child manifest")

    incoming: dict[Path, Path] = {}
    for record in records.values():
        for child in record.includes:
            previous = incoming.get(child)
            if previous is not None and previous != record.path:
                _error(errors, "DUPLICATE", f"{relative_path(root, child)} is included by both {relative_path(root, previous)} and {relative_path(root, record.path)}")
            else:
                incoming[child] = record.path

    root_path = root / ("Makefile" if kind == "mk" else "CMakeLists.txt")
    root_record = _parse_file(root, root_path, errors, kind, sources=False)
    for child in root_record.includes:
        previous = incoming.get(child)
        if previous is not None:
            _error(errors, "DUPLICATE", f"{relative_path(root, child)} is included by both {relative_path(root, previous)} and {root_path.name}")
        else:
            incoming[child] = root

    state: dict[Path, int] = {}
    order: list[Path] = []
    expanded: list[str] = []

    def visit(path: Path, stack: tuple[Path, ...]) -> None:
        path = path.resolve()
        if path not in records:
            parent = stack[-1] if stack else root
            _error(errors, "ORPHAN", f"{relative_path(root, parent)} includes missing source manifest {relative_path(root, path)}")
            return
        current = state.get(path, 0)
        if current == 1:
            cycle = " -> ".join(relative_path(root, item) for item in (*stack, path))
            _error(errors, "CYCLE", f"source manifest include cycle: {cycle}")
            return
        if current == 2:
            return
        state[path] = 1
        order.append(path)
        for operation, value in records[path].operations:
            if operation == "source":
                expanded.append(value)  # type: ignore[arg-type]
            else:
                visit(value, (*stack, path))  # type: ignore[arg-type]
        state[path] = 2

    for child in root_record.includes:
        visit(child, ())
    for path in records:
        if path not in state:
            _error(errors, "DISCONNECTED", f"{relative_path(root, path)} is not reachable from the root {root_path.name}")
    entries = [(record.path, record.entries) for record in records.values()]
    return order, expanded, entries
