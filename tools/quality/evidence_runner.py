"""Build/test registration checks for PASSING evidence tests.

Owns: runner shape beyond JSON, path ownership, and static Make/CMake
registration proofs. It performs no builds and records no mutable run state.
"""

from __future__ import annotations

import re
from pathlib import Path, PurePosixPath
from typing import Any


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


def _registered_c(root: Path, path: str, runner: dict[str, Any], errors: list[str]) -> None:
    make = (root / "Makefile").read_text(encoding="utf-8") if (root / "Makefile").is_file() else ""
    cmake = (root / "tests" / "CMakeLists.txt").read_text(encoding="utf-8") if (root / "tests" / "CMakeLists.txt").is_file() else ""
    cmake_path = path.removeprefix("tests/")
    if path not in make or cmake_path not in cmake:
        _add(errors, f"C test {path} must be registered in Makefile and tests/CMakeLists.txt")
    executable = None
    for match in re.finditer(r"add_executable\s*\(\s*([^\s)]+)\s+([^\s)]+)\s*\)", cmake):
        if match.group(2) in {path, cmake_path}:
            executable = match.group(1)
            break
    ctest_block = _cmake_test_block(cmake, runner.get("ctest_name", ""))
    if executable is None or ctest_block is None or not re.search(rf"\bCOMMAND\s+{re.escape(executable)}\b", ctest_block):
        _add(errors, f"C test {path} ctest/executable registration does not match {runner.get('ctest_name')}")
    variable = next((candidate for candidate in re.findall(r"^([A-Z][A-Z0-9_]*)\s*:=\s*[^\n]+$", make, re.MULTILINE) if re.search(rf"^\$\({re.escape(candidate)}\):[^\n]*{re.escape(path)}", make, re.MULTILINE)), None)
    build_body = _make_target_body(make, "build")
    test_body = _make_target_body(make, "test")
    reachable = variable is not None and build_body is not None and re.search(rf"\$\({re.escape(variable)}\)", build_body) is not None and test_body is not None and re.search(rf"^[ \t]*@?\$\({re.escape(variable)}\)\s*$", test_body, re.MULTILINE) is not None
    if executable is None or variable is None or not reachable:
        _add(errors, f"C test {path} is not reachable from Make test target")


def _registered_python(root: Path, path: str, runner: dict[str, Any], errors: list[str]) -> None:
    if not re.fullmatch(r"tests/contract/quality/test_[^/]+\.py", path):
        _add(errors, f"Python discovery test path is not a contract quality test: {path}")
    make = (root / "Makefile").read_text(encoding="utf-8") if (root / "Makefile").is_file() else ""
    cmake = (root / "tests" / "CMakeLists.txt").read_text(encoding="utf-8") if (root / "tests" / "CMakeLists.txt").is_file() else ""
    test_body = _make_target_body(make, "test")
    quality_body = _make_target_body(make, "quality-test")
    if test_body is None or not re.search(r"(?:^|\s)quality-test(?:\s|$)", test_body) or quality_body is None or not re.search(r"unittest\s+discover\s+-s\s+tests/contract/quality", quality_body):
        _add(errors, "Python discovery tests must be run by Make test/quality-test")
    ctest = runner.get("ctest_name", "")
    ctest_block = _cmake_test_block(cmake, ctest)
    source_path = r"-s\s+\"?\$\{PROJECT_SOURCE_DIR\}/tests/contract/quality\"?"
    top_path = r"-t\s+\"?\$\{PROJECT_SOURCE_DIR\}\"?"
    if not ctest or ctest_block is None or not re.search(r"-m\s+unittest\s+discover", ctest_block) or not re.search(source_path, ctest_block) or not re.search(top_path, ctest_block):
        _add(errors, f"Python discovery ctest is not registered: {ctest}")


def _make_target_body(make: str, name: str) -> str | None:
    match = re.search(rf"^{re.escape(name)}\s*:[^\n]*(?:\n(?:[ \t].*|\\\n))*", make, re.MULTILINE)
    return match.group(0) if match else None


def _cmake_test_block(cmake: str, name: str) -> str | None:
    for match in re.finditer(r"add_test\s*\((.*?)\)", cmake, re.DOTALL):
        block = match.group(0)
        if re.search(rf"\bNAME\s+{re.escape(name)}\b", block):
            return block
    return None


def validate_test_runners(root: Path, tests: dict[str, dict[str, Any]], errors: list[str]) -> None:
    """Validate static runner proofs for all tests."""

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
