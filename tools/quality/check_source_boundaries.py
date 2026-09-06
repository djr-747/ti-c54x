#!/usr/bin/env python3
"""Check source ownership and host-boundary rules.

Owns: repository boundary policy, report formatting, and production/test path
validation.  Does not own: C lexical parsing, knob registry policy, or build
system configuration; those remain in their dedicated modules.

The checker deliberately uses only Python's standard library.  It is a source
review aid, not a C parser: comments, string literals, and character literals
are removed before token checks so that examples in documentation do not look
like dependencies.  The checks are conservative around declarations at file
scope, while function bodies and local variables are left to the C compiler.

Usage::

    python3 tools/quality/check_source_boundaries.py
    python3 tools/quality/check_source_boundaries.py --root /path/to/ti-c54x

Exit status is zero for a clean tree, one for policy violations, and two for
invalid command-line input or an unusable root.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from functools import partial
from pathlib import Path
from typing import Iterable

try:  # Package import for tests; direct import for the standalone CLI.
    from .c_source_lexer import (
        comments_only,
        iter_includes,
        iter_top_level_statements,
        looks_like_function_pointer,
        line_of,
        strip_literals_and_comments,
        tokens,
    )
    from .source_boundary_policy import (
        ALLOWED_ISA_OWNERS,
        ALLOWED_SRC_OWNERS,
        ENV_API_NAMES,
        FORBIDDEN_API_NAMES,
        FORBIDDEN_INCLUDE_NAMES,
        FORBIDDEN_PLATFORM_NAMES,
        MAX_REVIEW_LINES,
        MAX_SOURCE_LINES,
        PRODUCTION_SUFFIXES,
        TEST_ROOTS,
        TEST_SUFFIXES,
    )
    from .observer_source_boundary import check_observer_file, check_observer_gate_proof
except ImportError:  # pragma: no cover - exercised by direct CLI execution.
    from c_source_lexer import (  # type: ignore[no-redef]
        comments_only,
        iter_includes,
        iter_top_level_statements,
        looks_like_function_pointer,
        line_of,
        strip_literals_and_comments,
        tokens,
    )
    from source_boundary_policy import (  # type: ignore[no-redef]
        ALLOWED_ISA_OWNERS,
        ALLOWED_SRC_OWNERS,
        ENV_API_NAMES,
        FORBIDDEN_API_NAMES,
        FORBIDDEN_INCLUDE_NAMES,
        FORBIDDEN_PLATFORM_NAMES,
        MAX_REVIEW_LINES,
        MAX_SOURCE_LINES,
        PRODUCTION_SUFFIXES,
        TEST_ROOTS,
        TEST_SUFFIXES,
    )
    from observer_source_boundary import check_observer_file, check_observer_gate_proof  # type: ignore[no-redef]

_GUARD_DIRECTIVE_RE = re.compile(r"^\s*#\s*(if|define|ifndef|endif)\b")
@dataclass(frozen=True)
class Violation:
    """One source-boundary policy failure."""

    path: Path
    line: int
    code: str
    message: str

    def format(self, root: Path) -> str:
        try:
            display = self.path.relative_to(root)
        except ValueError:
            display = self.path
        return f"{display}:{self.line}: [{self.code}] {self.message}"


@dataclass
class BoundaryReport:
    """Deterministic checker result, with failures and review warnings."""

    violations: list[Violation] = field(default_factory=list)
    warnings: list[Violation] = field(default_factory=list)
    files_checked: int = 0

    @property
    def ok(self) -> bool:
        return not self.violations


def _record(report: BoundaryReport, path: Path, line: int, code: str, message: str) -> None:
    report.violations.append(Violation(path, line, code, message))


def _production_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for directory in (root / "include", root / "src"):
        if not directory.is_dir():
            continue
        files.extend(
            path
            for path in directory.rglob("*")
            if path.is_file() and path.suffix in PRODUCTION_SUFFIXES
        )
    return sorted(files, key=lambda path: path.relative_to(root).as_posix())


def _test_files(root: Path) -> list[Path]:
    directory = root / "tests"
    if not directory.is_dir():
        return []
    return sorted(
        (
            path
            for path in directory.rglob("*")
            if path.is_file() and path.suffix in TEST_SUFFIXES
        ),
        key=lambda path: path.relative_to(root).as_posix(),
    )


def _check_ownership(path: Path, root: Path, report: BoundaryReport) -> None:
    relative = path.relative_to(root).parts
    if relative[0] == "include":
        if len(relative) < 3 or relative[1] != "ti_c54x":
            report.violations.append(
                Violation(path, 1, "OWNERSHIP", "public headers must live under include/ti_c54x/")
            )
        return
    if relative[0] != "src" or len(relative) < 2:
        report.violations.append(
            Violation(path, 1, "OWNERSHIP", "production source must live under an owning src/ module")
        )
        return
    owner = relative[1]
    if owner not in ALLOWED_SRC_OWNERS and owner != "isa":
        report.violations.append(
            Violation(path, 1, "OWNERSHIP", f"unknown src ownership module: {owner}")
        )
    elif owner == "isa" and (len(relative) < 3 or relative[2] not in ALLOWED_ISA_OWNERS):
        report.violations.append(
            Violation(path, 1, "OWNERSHIP", "src/isa files must belong to src/isa/catalog, src/isa/decode, or src/isa/ops")
        )


def _check_test_ownership(path: Path, root: Path, report: BoundaryReport) -> None:
    """Ensure test C/H files mirror the ownership hierarchy."""

    relative = path.relative_to(root).parts
    if len(relative) < 3 or relative[0] != "tests":
        report.violations.append(
            Violation(path, 1, "TEST_OWNERSHIP", "test C/H files must live under a named tests owner")
        )
        return
    owner = relative[1]
    if owner in TEST_ROOTS or owner == "support":
        return
    if owner != "unit":
        report.violations.append(
            Violation(path, 1, "TEST_OWNERSHIP", f"unknown tests ownership module: {owner}")
        )
        return
    if len(relative) < 4 or relative[2] not in ALLOWED_SRC_OWNERS | {"isa"}:
        report.violations.append(
            Violation(path, 1, "TEST_OWNERSHIP", "unit tests must mirror a src ownership module")
        )
        return
    if relative[2] == "isa" and relative[3] not in ALLOWED_ISA_OWNERS:
        report.violations.append(
            Violation(path, 1, "TEST_OWNERSHIP", "ISA unit tests must mirror catalog, decode, or ops")
        )


def _check_contract(path: Path, text: str, report: BoundaryReport) -> None:
    clean = strip_literals_and_comments(text)
    comments = comments_only(text)
    prefix = "\n".join(comments.splitlines()[:40])
    marker = re.search(r"\bModule\s+contract\b", prefix, re.IGNORECASE)
    owns = re.search(r"\bOwns\s*:", prefix)
    excludes = re.search(r"\bDoes\s+not\s+own\s*:", prefix, re.IGNORECASE)
    if marker is None or owns is None or excludes is None:
        report.violations.append(
            Violation(path, 1, "CONTRACT", "first 40 lines must contain Module contract, Owns:, and Does not own:")
        )
        return

    # Include guards may precede a contract in a header, but an include or C
    # declaration may not.  This keeps the contract at the module boundary.
    marker_line = line_of(comments, marker.start())
    for line_number, line in enumerate(clean.splitlines(), start=1):
        if line_number >= marker_line:
            break
        if not line.strip():
            continue
        if _GUARD_DIRECTIVE_RE.match(line):
            continue
        if line.lstrip().startswith("#"):
            report.violations.append(
                Violation(path, line_number, "CONTRACT", "module contract must precede includes and other directives")
            )
            return
        report.violations.append(
            Violation(path, line_number, "CONTRACT", "module contract must be the first source declaration")
        )
        return


def _check_includes_and_apis(path: Path, text: str, report: BoundaryReport) -> None:
    forbidden_headers = {item[1:-1].lower() for item in FORBIDDEN_INCLUDE_NAMES}
    for include in iter_includes(text):
        lower = include.header.lower()
        if lower in forbidden_headers or any(
            lower.startswith(prefix)
            for prefix in ("sys/", "linux/", "net/", "arpa/")
        ):
            report.violations.append(
                Violation(path, include.line, "HOST_INCLUDE", f"host/process/file header is forbidden: {include.spelling}")
            )

    source_tokens = tokens(text)
    for index, (token, line_number) in enumerate(source_tokens):
        if token not in FORBIDDEN_API_NAMES:
            continue
        following = source_tokens[index + 1][0] if index + 1 < len(source_tokens) else ""
        if following == "(":
            report.violations.append(
                Violation(path, line_number, "HOST_API", f"host/process/file/logging API is forbidden: {token}()")
            )

    identifiers = {token.lower() for token, _ in source_tokens}
    for name in sorted(identifiers & ENV_API_NAMES):
        line_number = next(line for token, line in source_tokens if token.lower() == name)
        report.violations.append(
            Violation(path, line_number, "ENV_API", f"environment/process configuration API is forbidden: {name}")
        )


def _check_file_scope_globals(path: Path, text: str, report: BoundaryReport) -> None:
    """Reject mutable object definitions while ignoring functions and locals."""

    for declaration in iter_top_level_statements(text):
        words = [item[0] for item in declaration.tokens]
        if words:
            first = words[0]
            close_index = words.index("}") if "}" in words else len(words)
            trailing_after_type = words[close_index + 1 :]
            tag_declaration = first in {"struct", "enum", "union"} and (
                (close_index == len(words) and len(words) == 2)
                or (close_index < len(words) and not trailing_after_type)
            )
            star_index = words.index("*") if "*" in words else len(words)
            pointer_to_const = (
                star_index < len(words)
                and "const" in words[:star_index]
                and "const" not in words[star_index + 1 :]
            )
            ignored = {
                "const",
                "register",
                "static_assert",
                "typedef",
            }
            if (
                (first not in ignored or (first == "const" and pointer_to_const))
                and "typedef" not in words
                and ("const" not in words or pointer_to_const)
                and not tag_declaration
                and not ("(" in words and not looks_like_function_pointer(words))
                and first not in {"#", "if", "ifdef", "ifndef"}
            ):
                report.violations.append(
                    Violation(
                        path,
                        declaration.line,
                        "GLOBAL_STATE",
                        "mutable file-scope object definition is forbidden; keep state instance-owned",
                    )
                )


def _check_platform_names(path: Path, text: str, report: BoundaryReport) -> None:
    for token, line_number in tokens(text):
        lower = token.lower()
        if any(name in lower for name in FORBIDDEN_PLATFORM_NAMES):
            report.violations.append(
                Violation(path, line_number, "PLATFORM", f"legacy platform lineage name is forbidden: {token}")
            )


def _check_opcode_debug_boundary(path: Path, root: Path, text: str, report: BoundaryReport) -> None:
    relative = path.relative_to(root).parts
    if len(relative) < 4 or relative[:3] != ("src", "isa", "ops"):
        return
    for include in iter_includes(text):
        included = include.header.lower().replace("\\", "/")
        if "/debug/" in f"/{included}/" or "observer" in included or "trace" in included:
            report.violations.append(
                Violation(path, include.line, "OP_DEBUG", "opcode handlers must not include debug/observer/trace modules")
            )
    for token, line_number in tokens(text):
        lower = token.lower()
        if "observer" in lower or lower in {"debug", "trace", "trace_emit"}:
            report.violations.append(
                Violation(path, line_number, "OP_DEBUG", "opcode handlers must not depend on debug/observer symbols")
            )


def _check_size(path: Path, text: str, report: BoundaryReport) -> None:
    lines = len(text.splitlines())
    if lines > MAX_SOURCE_LINES:
        report.violations.append(
            Violation(path, MAX_SOURCE_LINES + 1, "SIZE", f"handwritten source has {lines} lines; split before {MAX_SOURCE_LINES} lines")
        )
    elif lines > MAX_REVIEW_LINES:
        report.warnings.append(
            Violation(path, MAX_REVIEW_LINES + 1, "SIZE_REVIEW", f"handwritten source has {lines} lines; architecture review is required")
        )


def check_tree(root: Path | str) -> BoundaryReport:
    """Run all source-boundary checks against *root*."""

    resolved_root = Path(root).expanduser().resolve()
    report = BoundaryReport()
    if not resolved_root.is_dir():
        report.violations.append(
            Violation(resolved_root, 1, "ROOT", "repository root does not exist or is not a directory")
        )
        return report
    production_files = _production_files(resolved_root)
    test_files = _test_files(resolved_root)
    observer_sink = partial(_record, report)
    report.files_checked = len(production_files) + len(test_files)
    if not production_files:
        report.violations.append(
            Violation(resolved_root, 1, "ROOT", "repository contains no production C/H files under include/ or src/")
        )
    for path in production_files:
        text = path.read_text(encoding="utf-8")
        _check_ownership(path, resolved_root, report)
        _check_contract(path, text, report)
        _check_includes_and_apis(path, text, report)
        _check_file_scope_globals(path, text, report)
        _check_platform_names(path, text, report)
        _check_opcode_debug_boundary(path, resolved_root, text, report)
        check_observer_file(path, resolved_root, text, observer_sink)
        _check_size(path, text, report)
    if any(path.relative_to(resolved_root).parts[:2] == ("src", "debug") for path in production_files):
        check_observer_gate_proof(resolved_root, production_files, observer_sink)
    for path in test_files:
        text = path.read_text(encoding="utf-8")
        _check_test_ownership(path, resolved_root, report)
        _check_contract(path, text, report)
        _check_size(path, text, report)
    return report


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    default_root = Path(__file__).resolve().parents[2]
    parser.add_argument(
        "--root",
        type=Path,
        default=default_root,
        help=f"repository root to scan (default: {default_root})",
    )
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(list(argv) if argv is not None else None)
    report = check_tree(args.root)
    root = Path(args.root).expanduser().resolve()
    for warning in report.warnings:
        print(f"warning: {warning.format(root)}", file=sys.stderr)
    for violation in report.violations:
        print(violation.format(root), file=sys.stderr)
    if any(violation.code == "ROOT" for violation in report.violations):
        print("source boundaries: ERROR (invalid repository root)", file=sys.stderr)
        return 2
    if report.ok:
        suffix = " with review warnings" if report.warnings else ""
        print(f"source boundaries: PASS ({report.files_checked} files){suffix}")
        return 0
    print(
        f"source boundaries: FAIL ({len(report.violations)} violation(s), {report.files_checked} files)",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
