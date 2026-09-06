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
import sys
from pathlib import Path
from pathlib import PurePosixPath

try:
    from tools.quality.source_manifest_graph import (
        OWNER_MANIFEST_RE as _OWNER_MANIFEST_RE,
        discover_manifests,
        expand_manifest_graph,
        logical_make_lines,
        manifest_owner as _manifest_owner,
        relative_path as _relative,
    )
except ModuleNotFoundError:  # direct script execution from tools/quality
    from source_manifest_graph import (  # type: ignore[no-redef]
        OWNER_MANIFEST_RE as _OWNER_MANIFEST_RE,
        discover_manifests,
        expand_manifest_graph,
        logical_make_lines,
        manifest_owner as _manifest_owner,
        relative_path as _relative,
    )


_SOURCE_RE = re.compile(r"(?:^|[^A-Za-z0-9_.-])([^\s\\\"']+\.c)(?:$|[^A-Za-z0-9_.-])")
_CONCRETE_SOURCE_RE = re.compile(
    r"(?<![%$A-Za-z0-9_.-])src/[A-Za-z0-9_./-]+\.c(?![A-Za-z0-9_.-])"
)


def _error(errors: list[str], code: str, message: str) -> None:
    errors.append(f"MANIFEST:{code}: {message}")


def _source_files(root: Path) -> list[str]:
    source_root = root / "src"
    if not source_root.is_dir():
        return []
    return sorted(
        _relative(root, path)
        for path in source_root.rglob("*.c")
        if path.is_file()
    )


def _check_owner_paths(root: Path, entries: list[tuple[Path, list[str]]], errors: list[str]) -> None:
    seen: dict[str, Path] = {}
    for manifest, paths in entries:
        relative = _relative(root, manifest)
        owner = _manifest_owner(root, manifest)
        for source in paths:
            if owner is not None and PurePosixPath(source).parent.as_posix() != f"src/{owner}":
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
        for _, line in logical_make_lines(text):
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
    make, cmake = discover_manifests(root, errors)
    make_order, make_sources, make_entries = expand_manifest_graph(root, make, "mk", errors)
    cmake_order, cmake_sources, cmake_entries = expand_manifest_graph(root, cmake, "cmake", errors)
    if [_manifest_owner(root, path) or _relative(root, path) for path in make_order] != [
        _manifest_owner(root, path) or _relative(root, path) for path in cmake_order
    ]:
        _error(errors, "ORDER", "Make and CMake root manifest include orders differ")

    _check_owner_paths(root, make_entries, errors)
    _check_owner_paths(root, cmake_entries, errors)
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
