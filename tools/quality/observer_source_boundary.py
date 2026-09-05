"""Observer-specific source-boundary checks.

Owns: observer gate placement, event-mask access, public diagnostic-knob
exposure, and the enabled/disabled macro-shape proof.  Does not own: generic C
lexing, path ownership, report types, or build configuration.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Callable, Iterable

try:  # Package import for tests; direct import for the standalone CLI.
    from .c_source_lexer import strip_literals_and_comments, tokens
except ImportError:  # pragma: no cover - exercised by direct CLI execution.
    from c_source_lexer import strip_literals_and_comments, tokens  # type: ignore[no-redef]


_OBSERVE_DEFINE_RE = re.compile(r"^\s*#\s*define\s+TI_C54X_OBSERVE_ENABLED\b")
IssueSink = Callable[[Path, int, str, str], None]


def check_observer_file(
    path: Path,
    root: Path,
    text: str,
    add_issue: IssueSink,
) -> None:
    """Check one production file against the observer ownership seam."""

    relative = path.relative_to(root).parts
    in_debug = len(relative) >= 2 and relative[:2] == ("src", "debug")
    in_call_seam = len(relative) >= 2 and relative[0] == "src" and relative[1] in {
        "cpu",
        "sequencer",
    }
    source_tokens = tokens(text)
    for line_number, line in enumerate(strip_literals_and_comments(text).splitlines(), start=1):
        if _OBSERVE_DEFINE_RE.match(line) and not in_debug:
            add_issue(path, line_number, "OBSERVE_BOUNDARY", "observation gate definitions are owned by src/debug")
    for index, (token, line_number) in enumerate(source_tokens):
        previous = source_tokens[index - 1][0] if index else ""
        if token == "event_mask" and previous in {".", "->"} and not in_debug:
            add_issue(path, line_number, "OBSERVE_BOUNDARY", "event-mask storage is owned by src/debug")
        if token == "TI_C54X_OBSERVE_ENABLED" and not (in_debug or in_call_seam):
            add_issue(path, line_number, "OBSERVE_BOUNDARY", "observation gates may be defined in src/debug or called by cpu/sequencer")

    if relative[0] == "include" and len(relative) >= 2 and relative[1] == "ti_c54x":
        for token, line_number in source_tokens:
            if token.startswith("TI_C54X_") and ("TEST" in token or "TEMP" in token):
                add_issue(path, line_number, "PUBLIC_KNOB", "TEST/TEMP knobs must not enter installed public headers")


def _macro_definitions(text: str, name: str) -> list[str]:
    lines = strip_literals_and_comments(text).splitlines()
    definitions: list[str] = []
    index = 0
    while index < len(lines):
        if not re.match(rf"^\s*#\s*define\s+{re.escape(name)}\b", lines[index]):
            index += 1
            continue
        body = [lines[index]]
        while body[-1].rstrip().endswith("\\") and index + 1 < len(lines):
            index += 1
            body.append(lines[index])
        definitions.append(" ".join(part.rstrip().rstrip("\\") for part in body))
        index += 1
    return definitions


def check_observer_gate_proof(
    root: Path,
    production_files: Iterable[Path],
    add_issue: IssueSink,
) -> None:
    """Require exactly one enabled and one disabled observer gate definition."""

    definitions: list[str] = []
    for path in production_files:
        relative = path.relative_to(root).parts
        if len(relative) >= 2 and relative[:2] == ("src", "debug"):
            definitions.extend(
                _macro_definitions(path.read_text(encoding="utf-8"), "TI_C54X_OBSERVE_ENABLED")
            )
    enabled = sum(
        re.search(
            r"event_mask\s*!=\s*UINT64_C\s*\(\s*0\s*\)\s*&&.*event_mask.*&",
            body,
        )
        is not None
        for body in definitions
    )
    disabled = sum(re.search(r"\bfalse\s*&&", body) is not None for body in definitions)
    if len(definitions) != 2 or enabled != 1 or disabled != 1:
        add_issue(
            root / "src/debug",
            1,
            "OBSERVE_GATE_PROOF",
            "src/debug must define enabled nonzero-mask-first && and disabled false && observer gates exactly once",
        )
