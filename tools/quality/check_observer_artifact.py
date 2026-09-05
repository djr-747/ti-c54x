#!/usr/bin/env python3
"""Prove that an observer-disabled library artifact has no observer symbols.

Owns: the deterministic ``nm`` invocation and symbol-policy check for one
observer-disabled build artifact.  Does not own: compilation, CMake target
selection, source-boundary policy, or the enabled observer implementation.

The checker accepts a static archive, object, or executable.  It examines both
defined and undefined symbols, so an accidentally linked callback is rejected
even when the callback is supplied by another object.  ``nm`` is injected by
the build (``NM`` for Make and ``CMAKE_NM`` for CMake) to keep the check
portable across GNU and LLVM toolchains.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


FORBIDDEN_SYMBOL_RE = re.compile(
    r"(?:^|_)ti_c54x_observer_(?:[A-Za-z0-9_]+)$|"
    r"(?:^|_)TI_C54X_OBSERVE_(?:[A-Za-z0-9_]+)$"
)


@dataclass(frozen=True)
class ArtifactReport:
    """The symbols violating the observer-disabled artifact contract."""

    artifact: Path
    symbols: tuple[str, ...]

    @property
    def ok(self) -> bool:
        return not self.symbols


def _symbol_from_posix_line(line: str) -> str | None:
    """Extract the symbol field from one ``nm --format=posix`` output line."""

    fields = line.split()
    if len(fields) < 2 or line.rstrip().endswith("]:"):
        return None
    # POSIX nm emits ``name type value size``.  Undefined symbols may have
    # blank value/size fields, but still retain the first two fields.
    symbol = fields[0]
    if symbol in {"U", "T", "t", "D", "d", "B", "b", "R", "r", "A", "a"}:
        return None
    return symbol


def parse_nm_symbols(output: str) -> tuple[str, ...]:
    """Return sorted unique symbols from POSIX ``nm`` output."""

    symbols = {
        symbol
        for line in output.splitlines()
        if (symbol := _symbol_from_posix_line(line)) is not None
    }
    return tuple(sorted(symbols))


def forbidden_symbols(symbols: Iterable[str]) -> tuple[str, ...]:
    """Return sorted observer callback/gate symbols from *symbols*."""

    return tuple(sorted(symbol for symbol in set(symbols)
                        if FORBIDDEN_SYMBOL_RE.search(symbol)))


def run_nm(artifact: Path, nm: str | Path = "nm") -> tuple[int, str, str]:
    """Run the injected ``nm`` implementation and return status/stdout/stderr."""

    try:
        completed = subprocess.run(
            [str(nm), "-a", "--format=posix", str(artifact)],
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except OSError as exc:
        return 127, "", str(exc)
    return completed.returncode, completed.stdout, completed.stderr


def check_artifact(artifact: Path, nm: str | Path = "nm") -> tuple[str, ...]:
    """Return deterministic policy errors for one artifact."""

    if not artifact.is_file():
        return (f"observer artifact does not exist: {artifact}",)
    status, stdout, stderr = run_nm(artifact, nm)
    if status != 0:
        detail = stderr.strip() or f"nm exited with status {status}"
        return (f"cannot inspect observer artifact {artifact}: {detail}",)
    violations = forbidden_symbols(parse_nm_symbols(stdout))
    return tuple(
        f"observer-disabled artifact contains forbidden symbol: {symbol}"
        for symbol in violations
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--nm", default="nm", help="nm executable (default: nm)")
    args = parser.parse_args(argv)
    errors = check_artifact(args.artifact.resolve(), args.nm)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"observer-disabled artifact OK: {args.artifact}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
