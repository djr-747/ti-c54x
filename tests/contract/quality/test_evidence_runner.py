#!/usr/bin/env python3
"""Contract tests for ownership-mirrored evidence test runners.

Owns: temporary split Make/CMake manifest fixtures and static runner
reachability, aggregate-membership, and ownership checks.  Does not own the
evidence registry schema, canonical evidence documents, or checker policy.
"""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.quality.evidence_runner import validate_test_runners


class EvidenceRunnerTests(unittest.TestCase):
    """Exercise runner proofs against small repository-shaped fixtures."""

    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def write_build_manifests(self, *, include_make_fragment: bool = True) -> None:
        """Write an ownership-mirrored split Make/CMake test fixture."""

        include = "include tests/unit/cpu/tests.mk\n" if include_make_fragment else ""
        self.write(
            "Makefile",
            ".PHONY: test build quality-test\n"
            "TI_C54X_TEST_TARGETS :=\n"
            "define ti_c54x_add_test\n"
            "TI_C54X_TEST_TARGETS += $(BUILD_DIR)/test_$(1)\n"
            "$(BUILD_DIR)/test_$(1): tests/unit/$(2)/$(3)\n"
            "\t@true\n"
            "endef\n"
            f"{include}"
            "BUILD_DIR := build\n"
            "build: $(TI_C54X_TEST_TARGETS)\n"
            "test: build quality-test\n"
            "\t@set -e; $(foreach test,$(TI_C54X_TEST_TARGETS),$(test);)\n"
            "quality-test:\n"
            "\tpython3 -m unittest discover -s tests/contract/quality -t .\n",
        )
        self.write("CMakeLists.txt", "add_subdirectory(tests)\n")
        self.write(
            "tests/CMakeLists.txt",
            "add_subdirectory(unit)\n"
            "add_test(NAME quality.discovery COMMAND python3 -m unittest discover -s \"${PROJECT_SOURCE_DIR}/tests/contract/quality\" -t \"${PROJECT_SOURCE_DIR}\")\n",
        )
        self.write("tests/unit/CMakeLists.txt", "add_subdirectory(cpu)\n")
        self.write("tests/unit/cpu/tests.mk", "$(eval $(call ti_c54x_add_test,fake,cpu,test_fake.c))\n")

    @staticmethod
    def passing_test(path: str = "tests/unit/cpu/test_fake.c", ctest_name: str = "fake.ctest") -> dict:
        return {
            "T-FAKE": {
                "state": "PASSING",
                "path": path,
                "runner": {"kind": "C_EXECUTABLE", "make_goal": "test", "ctest_name": ctest_name},
            }
        }

    def cmake_fake(self) -> None:
        self.write("tests/unit/cpu/CMakeLists.txt", "add_executable(fake test_fake.c)\nadd_test(NAME fake.ctest COMMAND fake)\n")

    def test_make_test_recipe_must_execute_c_binary(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.cmake_fake()
        tests = self.passing_test()
        errors: list[str] = []
        validate_test_runners(self.root, tests, errors)
        self.assertEqual(errors, [])
        self.write("Makefile", (self.root / "Makefile").read_text(encoding="utf-8").replace("build: $(TI_C54X_TEST_TARGETS)", "build:").replace("\n\t@set -e; $(foreach test,$(TI_C54X_TEST_TARGETS),$(test);)", ""))
        errors = []
        validate_test_runners(self.root, tests, errors)
        self.assertTrue(any("not reachable" in error for error in errors))

    def test_split_manifests_are_accepted(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.cmake_fake()
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertEqual(errors, [])

    def test_missing_make_fragment_include_is_rejected(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests(include_make_fragment=False)
        self.cmake_fake()
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("ownership *.mk" in error for error in errors))

    def test_missing_cmake_test_registration_is_rejected(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("tests/unit/cpu/CMakeLists.txt", "add_executable(fake test_fake.c)\n")
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("ctest/executable" in error for error in errors))

    def test_unreachable_cmake_fragment_is_rejected(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.cmake_fake()
        self.write("tests/unit/CMakeLists.txt", "# missing add_subdirectory(cpu)\n")
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("unreachable tests/**/CMakeLists" in error for error in errors))

    def test_direct_make_rule_must_join_executed_aggregate(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("tests/unit/cpu/tests.mk", "ti_c54x_test_targets += $(OTHER_TEST)\n$(FAKE_TEST): tests/unit/cpu/test_fake.c\n")
        self.cmake_fake()
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("not a member of the executed Make test aggregate" in error for error in errors))

    def test_direct_literal_make_rule_must_join_executed_aggregate(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("tests/unit/cpu/tests.mk", "build/test_fake: tests/unit/cpu/test_fake.c\n")
        self.cmake_fake()
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("not a member of the executed Make test aggregate" in error for error in errors))
        self.write("tests/unit/cpu/tests.mk", "TI_C54X_TEST_TARGETS += build/test_fake\nbuild/test_fake: tests/unit/cpu/test_fake.c\n")
        errors = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertEqual(errors, [])

    def test_make_owner_fragment_must_match_source_owner(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("Makefile", (self.root / "Makefile").read_text(encoding="utf-8").replace("include tests/unit/cpu/tests.mk", "include tests/unit/state/tests.mk"))
        self.write("tests/unit/state/tests.mk", "$(eval $(call ti_c54x_add_test,fake,cpu,test_fake.c))\n")
        self.cmake_fake()
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("ownership fragment does not match" in error for error in errors))

    def test_cmake_owner_fragment_must_match_source_owner(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("tests/unit/cpu/CMakeLists.txt", "# intentionally empty\n")
        self.write("tests/unit/state/CMakeLists.txt", "add_executable(fake ${PROJECT_SOURCE_DIR}/tests/unit/cpu/test_fake.c)\nadd_test(NAME fake.ctest COMMAND fake)\n")
        self.write("tests/unit/CMakeLists.txt", "add_subdirectory(cpu)\nadd_subdirectory(state)\n")
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("CMake ownership fragment does not match" in error for error in errors))

    def test_orphan_manifests_and_non_normalized_literals_are_rejected(self) -> None:
        self.write("tests/unit/cpu/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("tests/unit/orphan/tests.mk", "# disconnected owner\n")
        self.write("tests/unit/orphan/CMakeLists.txt", "# disconnected owner\n")
        self.write("Makefile", (self.root / "Makefile").read_text(encoding="utf-8").replace("include tests/unit/cpu/tests.mk", "include tests/unit/./cpu/tests.mk"))
        self.write("tests/CMakeLists.txt", (self.root / "tests/CMakeLists.txt").read_text(encoding="utf-8").replace("add_subdirectory(unit)", "add_subdirectory(./unit)"))
        self.write("tests/unit/cpu/CMakeLists.txt", "add_executable(fake ./test_fake.c)\nadd_test(NAME fake.ctest COMMAND fake)\n")
        errors: list[str] = []
        validate_test_runners(self.root, self.passing_test(), errors)
        self.assertTrue(any("orphan Make ownership fragment" in error for error in errors))
        self.assertTrue(any("orphan CMake ownership fragment" in error for error in errors))
        self.assertTrue(any("reachable tests/**/CMakeLists" in error for error in errors))

    def test_nested_owner_manifests_are_accepted(self) -> None:
        self.write("tests/unit/isa/decode/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("Makefile", (self.root / "Makefile").read_text(encoding="utf-8").replace("include tests/unit/cpu/tests.mk", "include tests/unit/cpu/tests.mk\ninclude tests/unit/isa/decode/tests.mk"))
        self.write("tests/unit/isa/decode/tests.mk", "$(eval $(call ti_c54x_add_test,nested,isa/decode,test_fake.c))\n")
        self.write("tests/unit/CMakeLists.txt", "add_subdirectory(cpu)\nadd_subdirectory(isa)\n")
        self.write("tests/unit/isa/CMakeLists.txt", "add_subdirectory(decode)\n")
        self.write("tests/unit/isa/decode/CMakeLists.txt", "add_executable(nested test_fake.c)\nadd_test(NAME nested.ctest COMMAND nested)\n")
        errors: list[str] = []
        validate_test_runners(self.root, {"T-NESTED": {"state": "PASSING", "path": "tests/unit/isa/decode/test_fake.c", "runner": {"kind": "C_EXECUTABLE", "make_goal": "test", "ctest_name": "nested.ctest"}}}, errors)
        self.assertEqual(errors, [])

    def test_cross_nested_owner_registration_is_rejected(self) -> None:
        self.write("tests/unit/isa/ops/test_fake.c", "int main(void) { return 0; }\n")
        self.write_build_manifests()
        self.write("Makefile", (self.root / "Makefile").read_text(encoding="utf-8").replace("include tests/unit/cpu/tests.mk", "include tests/unit/cpu/tests.mk\ninclude tests/unit/isa/decode/tests.mk"))
        self.write("tests/unit/isa/decode/tests.mk", "$(eval $(call ti_c54x_add_test,cross,isa/ops,test_fake.c))\n")
        self.write("tests/unit/CMakeLists.txt", "add_subdirectory(cpu)\nadd_subdirectory(isa)\n")
        self.write("tests/unit/isa/CMakeLists.txt", "add_subdirectory(decode)\nadd_subdirectory(ops)\n")
        self.write("tests/unit/isa/decode/CMakeLists.txt", "# wrong owner source is deliberately not registered here\n")
        self.write("tests/unit/isa/ops/CMakeLists.txt", "add_executable(cross test_fake.c)\nadd_test(NAME cross.ctest COMMAND cross)\n")
        errors: list[str] = []
        validate_test_runners(self.root, {"T-CROSS": {"state": "PASSING", "path": "tests/unit/isa/ops/test_fake.c", "runner": {"kind": "C_EXECUTABLE", "make_goal": "test", "ctest_name": "cross.ctest"}}}, errors)
        self.assertTrue(any("ownership fragment does not match" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
