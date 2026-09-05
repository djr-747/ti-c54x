#!/usr/bin/env python3
"""Run deterministic schema and repository checks for config/evidence.json.

Owns: quality-check orchestration, CLI status, and stable error ordering.  Does
not own: JSON record shape or cross-record policy, which live in the dedicated
evidence schema/policy modules.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable

try:  # Package import for tests; direct import for the standalone CLI.
    from .evidence_policy import validate_repository
    from .evidence_schema import load_registry, validate_schema
except ImportError:  # pragma: no cover - exercised by direct CLI execution.
    from evidence_policy import validate_repository  # type: ignore[no-redef]
    from evidence_schema import load_registry, validate_schema  # type: ignore[no-redef]


def check_repository(root: Path | str) -> list[str]:
    """Return sorted schema and repository errors for *root*."""

    resolved_root = Path(root).expanduser().resolve()
    if not resolved_root.is_dir():
        return ["ROOT: repository root does not exist or is not a directory"]
    loaded = load_registry(resolved_root)
    errors = list(loaded.errors)
    if loaded.registry is not None and not errors:
        errors.extend(validate_repository(resolved_root, loaded.registry))
    return sorted(set(errors))


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repository root containing config/evidence.json",
    )
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(list(argv) if argv is not None else None)
    root = Path(args.root).expanduser().resolve()
    errors = check_repository(root)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        status = 2 if any(error.startswith("ROOT:") or "cannot parse registry" in error for error in errors) else 1
        print("evidence registry: ERROR" if status == 2 else f"evidence registry: FAIL ({len(errors)} error(s))", file=sys.stderr)
        return status
    loaded = load_registry(root)
    registry = loaded.registry or {}
    print(
        "evidence registry: PASS "
        f"(targets={len(registry.get('targets', []))}, "
        f"sources={len(registry.get('sources', []))}, "
        f"requirements={len(registry.get('requirements', []))}, "
        f"tests={len(registry.get('tests', []))})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
