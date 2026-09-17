"""Ownership-mirrored Make/CMake manifest graph and registration parsing.

Module contract: owns deterministic, read-only parsing of repository-local
Make and CMake ownership fragments, include/subdirectory reachability, and
literal source/target registration facts.  It does not own evidence JSON,
runner policy, builds, command execution, or mutable process state.  All paths
returned are absolute ``Path`` values inside the supplied repository root.
"""

from __future__ import annotations

import re
import shlex
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


# ".muse" holds agent worktrees and session state: never deliverable content.
# Ignoring it keeps the gate green while isolated leaf workers are in flight.
IGNORED_TREE_NAMES = {".git", "build", "build-fast", "build-cmake", "generated", "external", ".muse"}


@dataclass(frozen=True)
class MakeTestRegistration:
    """One test registration found in a reachable Make ownership fragment.

    ``kind`` is deliberately explicit: a ``ti_c54x_add_test`` macro call is
    proved through the macro's append contract, while a direct rule must prove
    that its exact target expression is appended to the executed aggregate.
    """

    fragment: Path
    target_or_name: str
    owner: str
    filename: str
    kind: str


def repo_files(root: Path, suffix: str) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob(f"*{suffix}"):
        if not path.is_file():
            continue
        relative = path.relative_to(root)
        if any(part in IGNORED_TREE_NAMES for part in relative.parts):
            continue
        files.append(path)
    return sorted(files, key=lambda item: item.relative_to(root).as_posix())


def normalise_literal_path(root: Path, base: Path, token: str) -> Path | None:
    value = token.strip().strip('"\'')
    if not value or "\\" in value or "$" in value or "{" in value or "}" in value:
        return None
    pure = PurePosixPath(value)
    if pure.is_absolute() or "." in pure.parts or ".." in pure.parts or str(pure) != value:
        return None
    candidate = (base / value).resolve()
    try:
        candidate.relative_to(root.resolve())
    except ValueError:
        return None
    return candidate if candidate.is_file() else None


def _make_include_targets(root: Path, path: Path, text: str) -> list[Path]:
    targets: list[Path] = []
    for raw_line in text.splitlines():
        line = raw_line.split("#", 1)[0].strip()
        match = re.match(r"(?:-?include)\s+(.+)$", line)
        if not match:
            continue
        token = match.group(1).strip()
        wildcard = re.fullmatch(r"\$\(wildcard\s+([^\)]+)\)", token)
        if wildcard:
            pattern = wildcard.group(1).strip()
            if "$" in pattern or any(part in {".", ".."} for part in PurePosixPath(pattern).parts):
                continue
            targets.extend(sorted((path.parent / candidate).resolve() for candidate in path.parent.glob(pattern) if candidate.is_file()))
            continue
        target = normalise_literal_path(root, path.parent, token)
        if target is None:
            target = normalise_literal_path(root, root, token)
        if target is not None:
            targets.append(target)
    return targets


def reachable_make_fragments(root: Path) -> tuple[set[Path], set[Path]]:
    """Return all ``*.mk`` files and those included by the root Makefile."""

    all_fragments = set(repo_files(root, ".mk"))
    root_make = next((root / name for name in ("Makefile", "GNUmakefile", "makefile") if (root / name).is_file()), None)
    reachable: set[Path] = set()
    if root_make is None:
        return all_fragments, reachable
    queue = [root_make.resolve()]
    while queue:
        current = queue.pop(0)
        if current in reachable or not current.is_file():
            continue
        reachable.add(current)
        try:
            text = current.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        queue.extend(target for target in _make_include_targets(root, current, text) if target not in reachable)
    return all_fragments, reachable


def make_test_manifest_calls(root: Path, reachable: set[Path]) -> list[MakeTestRegistration]:
    """Return typed macro/direct registrations from reachable ``*.mk`` files."""

    calls: list[MakeTestRegistration] = []
    pattern = re.compile(
        r"\$\(eval\s+\$\(call\s+ti_c54x_add_test\s*,\s*([^,\s]+)\s*,\s*([^,\s]+)\s*,\s*([^\)\s]+)\s*\)\s*\)"
    )
    direct_rule = re.compile(r"(?m)^\s*([^:\n]+):[^\n]*\b(tests/unit/(?:[^\s\\]+/)*test_[^\s\\]+\.c)\b")
    for fragment in sorted(reachable, key=lambda item: item.as_posix()):
        if fragment.suffix != ".mk":
            continue
        try:
            text = fragment.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for match in pattern.finditer(text):
            calls.append(MakeTestRegistration(fragment, match.group(1), match.group(2), match.group(3), "macro"))
        for match in direct_rule.finditer(text):
            owner, _, filename = match.group(2).removeprefix("tests/unit/").rpartition("/")
            if owner and filename:
                calls.append(MakeTestRegistration(fragment, match.group(1).strip(), owner, filename, "direct"))
    return calls


def make_test_target_variable(make_text: str) -> str | None:
    macro = re.search(r"define\s+ti_c54x_add_test\b(.*?)\bendef", make_text, re.DOTALL)
    if macro is None:
        return None
    match = re.search(r"(?m)^\s*([A-Za-z][A-Za-z0-9_]*)\s*\+=\s*[^\n]*test_", macro.group(1))
    return match.group(1) if match else None


def make_root_text(root: Path) -> tuple[Path | None, str, set[Path], set[Path]]:
    all_fragments, reachable = reachable_make_fragments(root)
    root_make = next((root / name for name in ("Makefile", "GNUmakefile", "makefile") if (root / name).is_file()), None)
    if root_make is None:
        return None, "", all_fragments, reachable
    try:
        return root_make, root_make.read_text(encoding="utf-8"), all_fragments, reachable
    except (OSError, UnicodeDecodeError):
        return root_make, "", all_fragments, reachable


def _cmake_commands(text: str, name: str) -> list[str]:
    commands: list[str] = []
    pattern = re.compile(rf"\b{re.escape(name)}\s*\(", re.IGNORECASE)
    for match in pattern.finditer(text):
        line_start = text.rfind("\n", 0, match.start()) + 1
        if text[line_start:match.start()].lstrip().startswith("#"):
            continue
        open_index = text.find("(", match.start(), match.end())
        depth = 1
        index = open_index + 1
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
            commands.append(text[open_index + 1 : index - 1])
    return commands


def _cmake_include_target(root: Path, path: Path, token: str) -> Path | None:
    value = token.strip().strip('"\'')
    if value.endswith("/CMakeLists.txt"):
        return normalise_literal_path(root, path.parent, value)
    if value.startswith("${CMAKE_CURRENT_SOURCE_DIR}/"):
        return normalise_literal_path(root, path.parent, value.removeprefix("${CMAKE_CURRENT_SOURCE_DIR}/"))
    if value.startswith("${PROJECT_SOURCE_DIR}/"):
        return normalise_literal_path(root, root, value.removeprefix("${PROJECT_SOURCE_DIR}/"))
    return None


def reachable_cmake_fragments(root: Path) -> tuple[set[Path], set[Path]]:
    """Return all test CMakeLists and files reachable from top-level CMake."""

    all_fragments = set(repo_files(root / "tests", "CMakeLists.txt")) if (root / "tests").is_dir() else set()
    top = root / "CMakeLists.txt"
    starts = [top] if top.is_file() else ([root / "tests" / "CMakeLists.txt"] if (root / "tests" / "CMakeLists.txt").is_file() else [])
    reachable: set[Path] = set()
    queue = [item.resolve() for item in starts]
    while queue:
        current = queue.pop(0)
        if current in reachable or not current.is_file():
            continue
        reachable.add(current)
        try:
            text = current.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for body in _cmake_commands(text, "add_subdirectory"):
            token = next(iter(body.split()), "")
            if not token or "$" in token:
                continue
            target = normalise_literal_path(root, current.parent, token.rstrip("/"))
            if target is None:
                candidate = (current.parent / token.rstrip("/") / "CMakeLists.txt").resolve()
                try:
                    candidate.relative_to(root.resolve())
                except ValueError:
                    candidate = None
                target = candidate if candidate is not None and candidate.is_file() else None
            if target is not None and target not in reachable:
                queue.append(target)
        for body in _cmake_commands(text, "include"):
            token = next(iter(body.split()), "")
            target = _cmake_include_target(root, current, token)
            if target is not None and target not in reachable:
                queue.append(target)
    return all_fragments, reachable


def cmake_source_token(root: Path, fragment: Path, token: str) -> Path | None:
    value = token.strip().strip('"\'')
    if not value.endswith(".c"):
        return None
    if value.startswith("${CMAKE_CURRENT_SOURCE_DIR}/"):
        value = value.removeprefix("${CMAKE_CURRENT_SOURCE_DIR}/")
        base = fragment.parent
    elif value.startswith("${PROJECT_SOURCE_DIR}/"):
        value = value.removeprefix("${PROJECT_SOURCE_DIR}/")
        base = root
    else:
        base = fragment.parent
    pure = PurePosixPath(value)
    if pure.is_absolute() or "\\" in value or "." in pure.parts or ".." in pure.parts or str(pure) != value:
        return None
    candidate = (base / value).resolve()
    try:
        candidate.relative_to(root.resolve())
    except ValueError:
        return None
    return candidate


def _cmake_words(body: str) -> list[str]:
    try:
        return shlex.split(body, comments=True, posix=True)
    except ValueError:
        return body.replace("\n", " ").split()


def cmake_test_registrations(root: Path, reachable: set[Path]) -> tuple[dict[str, str], dict[str, str], dict[str, Path], bool]:
    """Return source->exe, ctest->exe, source->owner-fragment, helper-valid."""

    source_to_executable: dict[str, str] = {}
    ctest_to_executable: dict[str, str] = {}
    source_to_fragment: dict[str, Path] = {}
    helper_valid = False
    helper_pattern = re.compile(r"add_executable\s*\(.*?\$\{target\}.*?\$\{CMAKE_CURRENT_SOURCE_DIR\}/\$\{source_name\}.*?\)", re.DOTALL)
    test_helper_pattern = re.compile(r"add_test\s*\(.*?NAME\s+\"?ti_c54x\.\$\{test_name\}\"?.*?COMMAND\s+\$\{target\}.*?\)", re.DOTALL)
    for fragment in sorted(reachable, key=lambda item: item.as_posix()):
        if fragment.name != "CMakeLists.txt":
            continue
        try:
            text = fragment.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        if helper_pattern.search(text) and test_helper_pattern.search(text):
            helper_valid = True
        for body in _cmake_commands(text, "add_executable"):
            words = _cmake_words(body)
            if not words:
                continue
            executable = words[0]
            for token in words[1:]:
                source = cmake_source_token(root, fragment, token)
                if source is not None:
                    source_key = source.relative_to(root).as_posix()
                    source_to_executable[source_key] = executable
                    source_to_fragment[source_key] = fragment
        for body in _cmake_commands(text, "ti_c54x_add_unit_test"):
            words = _cmake_words(body)
            if len(words) < 2 or not helper_valid:
                continue
            source = cmake_source_token(root, fragment, words[1])
            if source is None:
                continue
            source_key = source.relative_to(root).as_posix()
            executable = f"ti_c54x_test_{words[0]}"
            source_to_executable[source_key] = executable
            source_to_fragment[source_key] = fragment
            ctest_to_executable[f"ti_c54x.{words[0]}"] = executable
        for body in _cmake_commands(text, "add_test"):
            words = _cmake_words(body)
            if not words:
                continue
            try:
                name_index = words.index("NAME")
                command_index = words.index("COMMAND")
                ctest_to_executable[words[name_index + 1]] = words[command_index + 1]
            except (ValueError, IndexError):
                continue
    return source_to_executable, ctest_to_executable, source_to_fragment, helper_valid


def cmake_test_block(cmake: str, name: str) -> str | None:
    for body in _cmake_commands(cmake, "add_test"):
        if re.search(rf"\bNAME\s+{re.escape(name)}\b", body):
            return body
    return None


def ownership_cmake_fragment(root: Path, path: Path) -> bool:
    relative = path.relative_to(root).parts
    return relative == ("tests", "CMakeLists.txt") or (len(relative) >= 3 and relative[:2] == ("tests", "unit"))
