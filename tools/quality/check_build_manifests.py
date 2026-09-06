#!/usr/bin/env python3
"""Check the standalone production-source ownership manifests.

Owns: static validation of ``src/<owner>/sources.mk`` and
``src/<owner>/sources.cmake`` plus their root build reachability.  It does not
own compiler flags, test registration, or source semantics.  The checker is
deliberately a small build-file reader; it does not evaluate Make or CMake.

The canonical source list is the ordered concatenation of the owner manifests.
Both normal and observer-off targets must consume that same list.  Paths are
normalized to repository-relative POSIX paths before comparison, so a source
cannot be hidden by spelling, traversal, or build-system-specific prefixes.
"""

from __future__ import annotations

import argparse
import re
import shlex
import sys
from pathlib import Path, PurePosixPath
from typing import Iterable


_SOURCE_RE = re.compile(r"(?:^|[^A-Za-z0-9_.-])([^\s\\\"']+\.c)(?:$|[^A-Za-z0-9_.-])")
_CONCRETE_SOURCE_RE = re.compile(
    r"(?<![%$A-Za-z0-9_.-])src/[A-Za-z0-9_./-]+\.c(?![A-Za-z0-9_.-])"
)
_OWNER_MANIFEST_RE = re.compile(r"^src/([^/][^/]*(?:/[^/][^/]*)*)/sources\.(?:mk|cmake)$")


def _error(errors: list[str], code: str, message: str) -> None:
    errors.append(f"MANIFEST:{code}: {message}")


def _relative(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def _manifest_owner(root: Path, path: Path) -> str | None:
    match = _OWNER_MANIFEST_RE.match(_relative(root, path))
    return match.group(1) if match is not None else None


def _source_files(root: Path) -> list[str]:
    source_root = root / "src"
    if not source_root.is_dir():
        return []
    return sorted(
        _relative(root, path)
        for path in source_root.rglob("*.c")
        if path.is_file()
    )


def _logical_make_lines(text: str) -> Iterable[tuple[int, str]]:
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
    """Normalize one manifest token, returning None for a malformed token."""

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
        return _relative(root, candidate)
    return relative.as_posix()


def _manifest_source_token_errors(root: Path, manifest: Path, token: str) -> str:
    value = token.strip().strip('"\'')
    if ".." in PurePosixPath(value).parts or value.startswith("/") or "\\" in value:
        return f"{_relative(root, manifest)} contains traversal or absolute source token {value!r}"
    if "$" in value or "{" in value or "}" in value:
        return f"{_relative(root, manifest)} contains unresolved source token {value!r}"
    return f"{_relative(root, manifest)} contains malformed source token {value!r}"


def _parse_make_manifest(root: Path, manifest: Path, errors: list[str]) -> list[str]:
    entries: list[str] = []
    try:
        text = manifest.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        _error(errors, "READ", f"cannot read {_relative(root, manifest)}: {exc}")
        return entries
    for line_number, line in _logical_make_lines(text):
        match = re.search(r"\bti_c54x_core_sources\s*\+=\s*(.*)$", line)
        if match is None:
            continue
        try:
            tokens = shlex.split(match.group(1), posix=True)
        except ValueError as exc:
            _error(errors, "SYNTAX", f"{_relative(root, manifest)}:{line_number}: {exc}")
            continue
        for token in tokens:
            base = root if token.startswith("src/") else manifest.parent
            normalized = _safe_relative_source(root, base, token)
            if normalized is None:
                _error(errors, "TOKEN", _manifest_source_token_errors(root, manifest, token))
                continue
            entries.append(normalized)
    if not entries:
        _error(errors, "EMPTY", f"{_relative(root, manifest)} has no canonical source entries")
    return entries


def _cmake_commands(text: str, name: str) -> Iterable[str]:
    pattern = re.compile(rf"\b{re.escape(name)}\s*\(", re.IGNORECASE)
    for match in pattern.finditer(text):
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
            yield text[opening + 1 : index - 1]


def _parse_cmake_manifest(root: Path, manifest: Path, errors: list[str]) -> list[str]:
    entries: list[str] = []
    try:
        text = manifest.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        _error(errors, "READ", f"cannot read {_relative(root, manifest)}: {exc}")
        return entries
    text = re.sub(r"#[^\n]*", "", text)
    for body in _cmake_commands(text, "list"):
        tokens = shlex.split(body, posix=True)
        if len(tokens) < 3 or tokens[0].upper() != "APPEND" or tokens[1] != "ti_c54x_core_sources":
            continue
        for token in tokens[2:]:
            if token.startswith("${CMAKE_CURRENT_LIST_DIR}/"):
                token = token.removeprefix("${CMAKE_CURRENT_LIST_DIR}/")
                normalized = _safe_relative_source(root, manifest.parent, token)
            else:
                normalized = _safe_relative_source(root, root, token)
            if normalized is None:
                _error(errors, "TOKEN", _manifest_source_token_errors(root, manifest, token))
                continue
            entries.append(normalized)
    if not entries:
        _error(errors, "EMPTY", f"{_relative(root, manifest)} has no canonical source entries")
    return entries


def _owner_manifests(root: Path, suffix: str) -> list[Path]:
    source_root = root / "src"
    if not source_root.is_dir():
        return []
    return sorted(
        path for path in source_root.rglob(f"sources.{suffix}") if path.is_file()
    )


def _check_manifest_locations(root: Path, errors: list[str]) -> tuple[list[Path], list[Path]]:
    make = _owner_manifests(root, "mk")
    cmake = _owner_manifests(root, "cmake")
    by_kind = {"mk": make, "cmake": cmake}
    for kind, paths in by_kind.items():
        for path in paths:
            relative = _relative(root, path)
            match = _OWNER_MANIFEST_RE.match(relative)
            if match is None:
                _error(errors, "LOCATION", f"{relative} is not a src/<owner>/sources.{kind} manifest")
    make_names = {_relative(root, path).removesuffix(".mk") for path in make}
    cmake_names = {_relative(root, path).removesuffix(".cmake") for path in cmake}
    for missing in sorted(make_names - cmake_names):
        _error(errors, "PAIR", f"missing CMake pair for {missing}.mk")
    for missing in sorted(cmake_names - make_names):
        _error(errors, "PAIR", f"missing Make pair for {missing}.cmake")
    return make, cmake


def _make_includes(root: Path, path: Path) -> list[Path]:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []
    targets: list[Path] = []
    for raw in text.splitlines():
        line = raw.split("#", 1)[0].strip()
        match = re.match(r"(?:-?include)\s+([^\s]+)$", line)
        if match is None:
            continue
        token = match.group(1).strip('"\'')
        if "$" in token or "*" in token:
            continue
        candidate = (path.parent / token).resolve()
        if candidate.is_file():
            targets.append(candidate)
            continue
        candidate = (root / token).resolve()
        if candidate.is_file():
            targets.append(candidate)
    return targets


def _cmake_includes(root: Path, path: Path) -> list[Path]:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []
    text = re.sub(r"#[^\n]*", "", text)
    targets: list[Path] = []
    for body in _cmake_commands(text, "include"):
        token = body.strip().strip('"\'')
        if token.startswith("${CMAKE_CURRENT_SOURCE_DIR}/"):
            token = token.removeprefix("${CMAKE_CURRENT_SOURCE_DIR}/")
            candidate = (path.parent / token).resolve()
        elif token.startswith("${PROJECT_SOURCE_DIR}/"):
            token = token.removeprefix("${PROJECT_SOURCE_DIR}/")
            candidate = (root / token).resolve()
        else:
            continue
        if candidate.is_file():
            targets.append(candidate)
    return targets


def _reachable(root: Path, make: bool) -> set[Path]:
    start = root / "Makefile" if make else root / "CMakeLists.txt"
    if not start.is_file():
        return set()
    seen: set[Path] = set()
    queue = [start.resolve()]
    while queue:
        current = queue.pop(0)
        if current in seen or not current.is_file():
            continue
        seen.add(current)
        targets = _make_includes(root, current) if make else _cmake_includes(root, current)
        queue.extend(target for target in targets if target not in seen)
    return seen


def _ordered_reachable_manifests(
    root: Path, manifests: Iterable[Path], make: bool
) -> list[Path]:
    """Return owner manifests in root include order."""

    candidates = {path.resolve() for path in manifests}
    start = root / "Makefile" if make else root / "CMakeLists.txt"
    if not start.is_file():
        return []
    seen: set[Path] = set()
    ordered: list[Path] = []
    queue = [start.resolve()]
    while queue:
        current = queue.pop(0)
        if current in seen or not current.is_file():
            continue
        seen.add(current)
        targets = _make_includes(root, current) if make else _cmake_includes(root, current)
        for target in targets:
            resolved = target.resolve()
            if resolved in candidates:
                if resolved not in ordered:
                    ordered.append(resolved)
            elif resolved not in seen:
                queue.append(resolved)
    return ordered


def _check_root_graph(root: Path, manifests: Iterable[Path], make: bool, errors: list[str]) -> list[Path]:
    reachable = _reachable(root, make)
    for manifest in manifests:
        if manifest.resolve() not in reachable:
            _error(errors, "DISCONNECTED", f"{_relative(root, manifest)} is not reachable from the root {('Makefile' if make else 'CMakeLists.txt')}")
    if not reachable:
        _error(errors, "ROOT", f"missing root {'Makefile' if make else 'CMakeLists.txt'}")
    return _ordered_reachable_manifests(root, manifests, make)


def _check_owner_paths(root: Path, entries: list[tuple[Path, list[str]]], errors: list[str]) -> None:
    seen: dict[str, Path] = {}
    for manifest, paths in entries:
        relative = _relative(root, manifest)
        owner = _manifest_owner(root, manifest)
        for source in paths:
            if owner is not None and not source.startswith(f"src/{owner}/"):
                _error(errors, "OWNER", f"{relative} lists cross-owner source {source}")
            if source not in _source_files(root):
                _error(errors, "EXTRA", f"{relative} lists missing source {source}")
            previous = seen.get(source)
            if previous is not None:
                _error(errors, "DUPLICATE", f"source {source} is listed by both {_relative(root, previous)} and {relative}")
            else:
                seen[source] = manifest


def _check_build_variable_derivation(root: Path, errors: list[str]) -> None:
    make_path = root / "Makefile"
    if make_path.is_file():
        text = make_path.read_text(encoding="utf-8")
        if len(re.findall(r"(?m)^\s*ti_c54x_core_sources\s*:=", text)) != 1:
            _error(errors, "CANONICAL", "Makefile must define ti_c54x_core_sources exactly once")
        assignments: dict[str, list[str]] = {
            "LIB_OBJECTS": [],
            "OBSERVER_OFF_OBJECTS": [],
        }
        for _, line in _logical_make_lines(text):
            for variable in assignments:
                match = re.match(
                    rf"^\s*{re.escape(variable)}\s*(?::=|\+=|\?=|=)\s*(.*)$",
                    line,
                )
                if match is not None:
                    assignments[variable].append(match.group(1))
        for variable, bodies in assignments.items():
            if not bodies:
                _error(errors, "CANONICAL", f"Makefile must assign {variable}")
            elif any("$(ti_c54x_core_sources)" not in body for body in bodies):
                _error(
                    errors,
                    "CANONICAL",
                    f"Makefile {variable} assignments must derive from ti_c54x_core_sources",
                )
        concrete = [match.group(0) for match in _CONCRETE_SOURCE_RE.finditer(text)]
        if concrete:
            _error(errors, "DUPLICATED", f"Makefile contains concrete source paths outside owner manifests: {sorted(set(concrete))}")
    cmake_paths = [root / "CMakeLists.txt", root / "tests" / "CMakeLists.txt"]
    cmake_text = "\n".join(
        path.read_text(encoding="utf-8") for path in cmake_paths if path.is_file()
    )
    if cmake_text.count("${ti_c54x_core_sources}") < 3:
        _error(errors, "CANONICAL", "CMake normal and observer-off targets must derive from ti_c54x_core_sources")
    concrete = [match.group(0) for match in _CONCRETE_SOURCE_RE.finditer(cmake_text)]
    if concrete:
        _error(errors, "DUPLICATED", f"root CMake files contain concrete source paths outside owner manifests: {sorted(set(concrete))}")


def check_repository(root: Path) -> list[str]:
    """Return deterministic manifest/build-graph errors for *root*."""

    root = root.resolve()
    errors: list[str] = []
    source_files = _source_files(root)
    make, cmake = _check_manifest_locations(root, errors)
    make_order = _check_root_graph(root, make, True, errors)
    cmake_order = _check_root_graph(root, cmake, False, errors)
    if [_manifest_owner(root, path) or _relative(root, path) for path in make_order] != [
        _manifest_owner(root, path) or _relative(root, path) for path in cmake_order
    ]:
        _error(errors, "ORDER", "Make and CMake root manifest include orders differ")

    make_entries = [(path, _parse_make_manifest(root, path, errors)) for path in make]
    cmake_entries = [(path, _parse_cmake_manifest(root, path, errors)) for path in cmake]
    _check_owner_paths(root, make_entries, errors)
    _check_owner_paths(root, cmake_entries, errors)
    make_sources = [source for _, paths in make_entries for source in paths]
    cmake_sources = [source for _, paths in cmake_entries for source in paths]
    if make_sources != cmake_sources:
        _error(errors, "ORDER", "Make and CMake canonical source lists differ in set or order")
    for kind, sources in (("Make", make_sources), ("CMake", cmake_sources)):
        if set(sources) != set(source_files) or len(sources) != len(source_files):
            _error(errors, "MISSING", f"{kind} canonical manifests do not contain every handwritten src/**/*.c exactly once")
    _check_build_variable_derivation(root, errors)
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if not root.is_dir():
        print(f"MANIFEST:ROOT: not a directory: {root}", file=sys.stderr)
        return 2
    errors = check_repository(root)
    for error in errors:
        print(error, file=sys.stderr)
    if not errors:
        print("build manifests: ok")
        return 0
    return 1


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
