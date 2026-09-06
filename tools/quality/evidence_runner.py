"""Build/test registration checks for PASSING evidence tests.

Module contract: owns runner-shape, ownership-location, aggregate membership,
and static reachability policy for evidence tests.  The read-only manifest
graph/parser lives in :mod:`manifest_graph`; this module performs no builds,
file generation, environment reads, or mutable process-global bookkeeping.
"""

from __future__ import annotations

import re
from pathlib import Path, PurePosixPath
from typing import Any

try:  # Package import for tests and the repository checker.
    from .manifest_graph import (
        MakeTestRegistration,
        cmake_test_block,
        cmake_test_registrations,
        make_root_text,
        make_test_manifest_calls,
        make_test_target_variable,
        ownership_cmake_fragment,
        reachable_cmake_fragments,
        reachable_make_fragments,
    )
except ImportError:  # pragma: no cover - direct CLI execution.
    from manifest_graph import (  # type: ignore[no-redef]
        MakeTestRegistration,
        cmake_test_block,
        cmake_test_registrations,
        make_root_text,
        make_test_manifest_calls,
        make_test_target_variable,
        ownership_cmake_fragment,
        reachable_cmake_fragments,
        reachable_make_fragments,
    )


def _add(errors: list[str], message: str) -> None:
    errors.append(f"RUNNER: {message}")


def _path(root: Path, value: Any, identifier: str, errors: list[str], require_exists: bool) -> Path | None:
    if not isinstance(value, str) or not value or "\\" in value:
        _add(errors, f"test {identifier} path is not normalized")
        return None
    pure = PurePosixPath(value)
    if pure.is_absolute() or "." in pure.parts or ".." in pure.parts or str(pure) != value or not (value.startswith("tests/unit/") or value.startswith("tests/contract/")):
        _add(errors, f"test {identifier} path must be under tests/unit or tests/contract")
        return None
    resolved = root.joinpath(*pure.parts)
    if require_exists and not resolved.is_file():
        _add(errors, f"test {identifier} PASSING path does not exist: {value}")
    return resolved


def _make_target_body(make: str, name: str) -> str | None:
    match = re.search(rf"^{re.escape(name)}\s*:[^\n]*(?:\n(?:[ \t].*|\\\n))*", make, re.MULTILINE)
    return match.group(0) if match else None


def _make_registration_is_aggregated(
    root: Path,
    calls: list[MakeTestRegistration],
    reachable: set[Path],
    call: MakeTestRegistration,
    target_variable: str | None,
    make_text: str,
) -> bool:
    """Prove a registration joins the aggregate executed by root Make.

    Macro registrations are proved by the generic macro contract. Direct
    rules have no parameter expansion to rely on: their exact target spelling
    must occur in an assignment to the aggregate in the root or a reachable
    ownership fragment.
    """

    target_or_name = call.target_or_name
    if target_variable is None:
        return False
    if call.kind == "direct":
        aggregate_text = "\n".join(
            [make_text]
            + [
                path.read_text(encoding="utf-8", errors="ignore")
                for path in sorted(reachable, key=lambda item: item.as_posix())
                if path.is_file() and path.suffix == ".mk"
            ]
        )
        assignment = re.compile(
            rf"(?m)^\s*{re.escape(target_variable)}\s*(?::=|\+=|\?=|=)\s*([^\n]*(?:\\\n[ \t][^\n]*)*)"
        )
        target_token = re.compile(rf"(?<!\S){re.escape(target_or_name)}(?!\S)")
        return any(target_token.search(match.group(1).replace("\\\n", " ")) for match in assignment.finditer(aggregate_text))
    if call.kind != "macro":
        return False
    macro = re.search(r"define\s+ti_c54x_add_test\b(.*?)\bendef", make_text, re.DOTALL)
    return macro is not None and re.search(
        rf"{re.escape(target_variable)}\s*\+=\s*[^\n]*test_\$\(1\)", macro.group(1)
    ) is not None


def _owner_matches_make_fragment(root: Path, call: MakeTestRegistration) -> bool:
    fragment, _, owner, filename = call.fragment, call.target_or_name, call.owner, call.filename
    try:
        relative = fragment.relative_to(root).parts
    except ValueError:
        return False
    owner_parts = relative[2:-1] if len(relative) >= 4 else ()
    owner_path = "/".join(owner_parts)
    return (
        len(relative) >= 4
        and relative[:2] == ("tests", "unit")
        and relative[-1] == "tests.mk"
        and owner_path == owner
        and all(part not in {".", ".."} for part in owner_parts)
        and "." not in PurePosixPath(filename).parts
        and ".." not in PurePosixPath(filename).parts
    )


def _owner_matches_cmake_fragment(root: Path, fragment: Path, source_path: str) -> bool:
    try:
        fragment_parent = fragment.parent.relative_to(root).as_posix()
    except ValueError:
        return False
    source_parent = source_path.rsplit("/", 1)[0]
    return fragment_parent == source_parent and source_parent.startswith("tests/unit/")


def _registered_c(root: Path, path: str, runner: dict[str, Any], errors: list[str]) -> None:
    root_make, make_text, _, reachable_make = make_root_text(root)
    calls = make_test_manifest_calls(root, reachable_make)
    expected_calls = [call for call in calls if f"tests/unit/{call.owner}/{call.filename}" == path]
    if root_make is None or not expected_calls:
        _add(errors, f"C test {path} must be registered in a reachable ownership *.mk fragment")
    for call in expected_calls:
        if not _owner_matches_make_fragment(root, call):
            _add(errors, f"C test {path} ownership fragment does not match its tests/unit owner")
    macro = re.search(r"define\s+ti_c54x_add_test\b(.*?)\bendef", make_text, re.DOTALL)
    if macro is None:
        _add(errors, "root Makefile must define ti_c54x_add_test")
    target_variable = make_test_target_variable(make_text)
    build_body = _make_target_body(make_text, "build")
    test_body = _make_target_body(make_text, "test")
    variable_reference = (
        rf"\$\({re.escape(target_variable)}\)"
        if target_variable
        else r"\$\([A-Za-z][A-Za-z0-9_]*test_targets\)"
    )
    if build_body is None or not re.search(variable_reference, build_body):
        _add(errors, f"C test {path} is not reachable from Make build target")
    if test_body is None or not re.search(
        re.escape(target_variable) if target_variable else r"[A-Za-z][A-Za-z0-9_]*test_targets",
        test_body,
    ):
        _add(errors, f"C test {path} is not reachable from Make test target")
    if expected_calls and not any(
        _make_registration_is_aggregated(root, calls, reachable_make, call, target_variable, make_text)
        for call in expected_calls
    ):
        _add(errors, f"C test {path} target is not a member of the executed Make test aggregate")

    all_cmake, reachable_cmake = reachable_cmake_fragments(root)
    source_to_executable, ctest_to_executable, source_to_fragment, helper_valid = cmake_test_registrations(root, reachable_cmake)
    executable = source_to_executable.get(path)
    if executable is None:
        all_source_to_executable, _, _, _ = cmake_test_registrations(root, all_cmake)
        if path in all_source_to_executable:
            _add(errors, f"C test {path} is declared in an unreachable tests/**/CMakeLists.txt")
        else:
            _add(errors, f"C test {path} must be registered in a reachable tests/**/CMakeLists.txt")
    elif path not in source_to_fragment or not _owner_matches_cmake_fragment(root, source_to_fragment[path], path):
        _add(errors, f"C test {path} CMake ownership fragment does not match its tests/unit owner")
    ctest_name = str(runner.get("ctest_name", ""))
    if not helper_valid and any(
        "ti_c54x_add_unit_test" in item.read_text(encoding="utf-8", errors="ignore")
        for item in reachable_cmake
    ):
        _add(errors, "tests CMake helper ti_c54x_add_unit_test is incomplete")
    if executable is None or ctest_to_executable.get(ctest_name) != executable:
        _add(errors, f"C test {path} ctest/executable registration does not match {ctest_name}")


def _registered_python(root: Path, path: str, runner: dict[str, Any], errors: list[str]) -> None:
    if not re.fullmatch(r"tests/contract/quality/test_[^/]+\.py", path):
        _add(errors, f"Python discovery test path is not a contract quality test: {path}")
    _, make, _, _ = make_root_text(root)
    test_body = _make_target_body(make, "test")
    quality_body = _make_target_body(make, "quality-test")
    if test_body is None or not re.search(r"(?:^|\s)quality-test(?:\s|$)", test_body) or quality_body is None or not re.search(r"unittest\s+discover\s+-s\s+tests/contract/quality", quality_body):
        _add(errors, "Python discovery tests must be run by Make test/quality-test")
    ctest = runner.get("ctest_name", "")
    _, reachable_cmake = reachable_cmake_fragments(root)
    cmake = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for path in sorted(reachable_cmake, key=lambda item: item.as_posix())
    )
    ctest_block = cmake_test_block(cmake, ctest)
    source_path = r"-s\s+\"?\$\{PROJECT_SOURCE_DIR\}/tests/contract/quality\"?"
    top_path = r"-t\s+\"?\$\{PROJECT_SOURCE_DIR\}\"?"
    if not ctest or ctest_block is None or not re.search(r"-m\s+unittest\s+discover", ctest_block) or not re.search(source_path, ctest_block) or not re.search(top_path, ctest_block):
        _add(errors, f"Python discovery ctest is not registered: {ctest}")


def _orphan_manifests(root: Path, errors: list[str]) -> None:
    all_make, reachable_make = reachable_make_fragments(root)
    for fragment in sorted(all_make - reachable_make, key=lambda item: item.as_posix()):
        _add(errors, f"orphan Make ownership fragment is not reachable: {fragment.relative_to(root).as_posix()}")
    all_cmake, reachable_cmake = reachable_cmake_fragments(root)
    for fragment in sorted(all_cmake - reachable_cmake, key=lambda item: item.as_posix()):
        if ownership_cmake_fragment(root, fragment):
            _add(errors, f"orphan CMake ownership fragment is not reachable: {fragment.relative_to(root).as_posix()}")


def validate_test_runners(root: Path, tests: dict[str, dict[str, Any]], errors: list[str]) -> None:
    """Validate static runner proofs for all tests and manifest reachability."""

    _orphan_manifests(root, errors)
    for identifier, test in tests.items():
        runner = test.get("runner")
        path = _path(root, test.get("path"), identifier, errors, test.get("state") == "PASSING")
        if test.get("state") == "PLANNED":
            if runner is not None:
                _add(errors, f"PLANNED test {identifier} runner must be null")
            continue
        if path is None or not isinstance(runner, dict):
            continue
        runner_kind = runner.get("kind")
        if runner_kind == "C_EXECUTABLE":
            if not str(test.get("path", "")).endswith(".c"):
                _add(errors, f"C runner {identifier} requires a .c source path")
            else:
                _registered_c(root, str(test.get("path")), runner, errors)
        elif runner_kind == "PYTHON_UNITTEST_DISCOVERY":
            _registered_python(root, str(test.get("path")), runner, errors)
        else:
            _add(errors, f"test {identifier} has unknown runner kind")
