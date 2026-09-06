#!/usr/bin/env python3
"""Contract fixtures for the production-source manifest checker.

Owns: standalone fixtures proving source-manifest ownership, build-graph
reachability, canonical ordering, and observer-off derivation.  Does not own
the Make/CMake implementations or production C source behavior.
"""

from __future__ import annotations

import re
import tempfile
import unittest
from pathlib import Path

from tools.quality.check_build_manifests import check_repository


class BuildManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def fixture(self, owners: tuple[str, ...] = ("cpu",), cmake_order: tuple[str, ...] | None = None) -> None:
        if cmake_order is None:
            cmake_order = owners
        for owner in owners:
            source_name = owner.rsplit("/", 1)[-1]
            source = f"src/{owner}/{source_name}.c"
            self.write(source, "int fixture_source;\n")
            self.write(
                f"src/{owner}/sources.mk",
                f"ti_c54x_core_sources += {source}\n",
            )
            self.write(
                f"src/{owner}/sources.cmake",
                "list(APPEND ti_c54x_core_sources\n"
                f"    \"${{CMAKE_CURRENT_LIST_DIR}}/{source_name}.c\"\n"
                ")\n",
            )
        make_includes = "\n".join(f"include src/{owner}/sources.mk" for owner in owners)
        cmake_includes = "\n".join(
            f"include(${{CMAKE_CURRENT_SOURCE_DIR}}/src/{owner}/sources.cmake)"
            for owner in cmake_order
        )
        self.write(
            "Makefile",
            "ti_c54x_core_sources :=\n"
            f"{make_includes}\n"
            "LIB_OBJECTS := $(patsubst src/%.c,build/%.o,$(ti_c54x_core_sources))\n"
            "OBSERVER_OFF_OBJECTS := $(patsubst src/%.c,build-off/%.o,$(ti_c54x_core_sources))\n",
        )
        self.write(
            "CMakeLists.txt",
            "set(ti_c54x_core_sources)\n"
            f"{cmake_includes}\n"
            "add_library(ti_c54x STATIC ${ti_c54x_core_sources})\n",
        )
        self.write(
            "tests/CMakeLists.txt",
            "add_library(ti_c54x_observer_off STATIC ${ti_c54x_core_sources})\n"
            "add_library(ti_c54x_observer_enabled STATIC ${ti_c54x_core_sources})\n",
        )

    def hierarchy_fixture(self, *, cmake_child_order: tuple[str, ...] = ("catalog", "decode")) -> None:
        for owner, source_name in (("isa/catalog", "catalog.c"), ("isa/decode", "decode.c")):
            source = f"src/{owner}/{source_name}"
            self.write(source, "int fixture_source;\n")
            self.write(
                f"src/{owner}/sources.mk",
                f"ti_c54x_core_sources += {source}\n",
            )
            self.write(
                f"src/{owner}/sources.cmake",
                "list(APPEND ti_c54x_core_sources\n"
                f"    \"${{CMAKE_CURRENT_LIST_DIR}}/{source_name}\"\n"
                ")\n",
            )
        self.write(
            "src/isa/sources.mk",
            "include src/isa/catalog/sources.mk\n"
            "include src/isa/decode/sources.mk\n",
        )
        cmake_children = "\n".join(
            f"include(${{CMAKE_CURRENT_LIST_DIR}}/{child}/sources.cmake)"
            for child in cmake_child_order
        )
        self.write("src/isa/sources.cmake", f"{cmake_children}\n")
        self.write(
            "Makefile",
            "ti_c54x_core_sources :=\n"
            "include src/isa/sources.mk\n"
            "LIB_OBJECTS := $(patsubst src/%.c,build/%.o,$(ti_c54x_core_sources))\n"
            "OBSERVER_OFF_OBJECTS := $(patsubst src/%.c,build-off/%.o,$(ti_c54x_core_sources))\n",
        )
        self.write(
            "CMakeLists.txt",
            "set(ti_c54x_core_sources)\n"
            "include(${CMAKE_CURRENT_SOURCE_DIR}/src/isa/sources.cmake)\n"
            "add_library(ti_c54x STATIC ${ti_c54x_core_sources})\n",
        )
        self.write(
            "tests/CMakeLists.txt",
            "add_library(ti_c54x_observer_off STATIC ${ti_c54x_core_sources})\n"
            "add_library(ti_c54x_observer_enabled STATIC ${ti_c54x_core_sources})\n",
        )

    @staticmethod
    def codes(errors: list[str]) -> set[str]:
        return {error.split(":", 2)[1] for error in errors}

    def test_repository_manifests_pass(self) -> None:
        self.assertEqual(check_repository(Path(__file__).resolve().parents[3]), [])

    def test_clean_fixture_passes(self) -> None:
        self.fixture()
        self.assertEqual(check_repository(self.root), [])

    def test_make_and_cmake_root_order_must_match(self) -> None:
        self.fixture(("cpu", "debug"), cmake_order=("debug", "cpu"))
        errors = check_repository(self.root)
        self.assertIn("ORDER", self.codes(errors))

    def test_nested_owner_manifests_pass(self) -> None:
        self.fixture(("isa/decode", "isa/ops"))
        self.assertEqual(check_repository(self.root), [])

    def test_nested_owner_order_is_not_collapsed_to_top_level(self) -> None:
        self.fixture(("isa/decode", "isa/ops"), cmake_order=("isa/ops", "isa/decode"))
        errors = check_repository(self.root)
        self.assertIn("ORDER", self.codes(errors))

    def test_nested_owner_cross_listing_is_rejected(self) -> None:
        self.fixture(("isa/decode", "isa/ops"))
        self.write(
            "src/isa/decode/sources.mk",
            "ti_c54x_core_sources += src/isa/ops/ops.c\n",
        )
        errors = check_repository(self.root)
        self.assertIn("OWNER", self.codes(errors))

    def test_aggregate_cannot_claim_descendant_source_directly(self) -> None:
        self.hierarchy_fixture()
        self.write(
            "src/isa/sources.mk",
            "ti_c54x_core_sources += src/isa/catalog/catalog.c\n"
            "include src/isa/decode/sources.mk\n",
        )
        errors = check_repository(self.root)
        self.assertIn("OWNER", self.codes(errors))

    def test_aggregate_nested_manifests_expand_in_root_order(self) -> None:
        self.hierarchy_fixture()
        self.assertEqual(check_repository(self.root), [])

    def test_aggregate_nested_order_must_match_between_build_systems(self) -> None:
        self.hierarchy_fixture(cmake_child_order=("decode", "catalog"))
        errors = check_repository(self.root)
        self.assertIn("ORDER", self.codes(errors))

    def test_manifest_include_cycle_is_rejected(self) -> None:
        self.hierarchy_fixture()
        self.write(
            "src/isa/catalog/sources.mk",
            "include src/isa/sources.mk\n",
        )
        self.write(
            "src/isa/catalog/sources.cmake",
            "include(${CMAKE_CURRENT_LIST_DIR}/../sources.cmake)\n",
        )
        errors = check_repository(self.root)
        self.assertIn("CYCLE", self.codes(errors))

    def test_duplicate_manifest_include_is_rejected(self) -> None:
        self.hierarchy_fixture()
        self.write(
            "src/isa/sources.mk",
            "include src/isa/catalog/sources.mk\n"
            "include src/isa/catalog/sources.mk\n"
            "include src/isa/decode/sources.mk\n",
        )
        self.write(
            "src/isa/sources.cmake",
            "include(${CMAKE_CURRENT_LIST_DIR}/catalog/sources.cmake)\n"
            "include(${CMAKE_CURRENT_LIST_DIR}/catalog/sources.cmake)\n"
            "include(${CMAKE_CURRENT_LIST_DIR}/decode/sources.cmake)\n",
        )
        errors = check_repository(self.root)
        self.assertIn("DUPLICATE", self.codes(errors))

    def test_root_and_aggregate_duplicate_include_is_rejected(self) -> None:
        self.hierarchy_fixture()
        self.write(
            "Makefile",
            "ti_c54x_core_sources :=\n"
            "include src/isa/sources.mk\n"
            "include src/isa/catalog/sources.mk\n"
            "LIB_OBJECTS := $(patsubst src/%.c,build/%.o,$(ti_c54x_core_sources))\n"
            "OBSERVER_OFF_OBJECTS := $(patsubst src/%.c,build-off/%.o,$(ti_c54x_core_sources))\n",
        )
        errors = check_repository(self.root)
        self.assertIn("DUPLICATE", self.codes(errors))

    def test_empty_leaf_manifest_is_rejected(self) -> None:
        self.hierarchy_fixture()
        self.write("src/isa/decode/sources.mk", "")
        self.write("src/isa/decode/sources.cmake", "")
        errors = check_repository(self.root)
        self.assertIn("EMPTY", self.codes(errors))

    def test_manifest_include_traversal_is_rejected(self) -> None:
        self.hierarchy_fixture()
        self.write(
            "src/isa/sources.mk",
            "include src/isa/catalog/sources.mk\n"
            "include src/isa/../isa/decode/sources.mk\n",
        )
        errors = check_repository(self.root)
        self.assertIn("TOKEN", self.codes(errors))

    def test_manifest_order_and_sets_must_match(self) -> None:
        self.fixture()
        self.write("src/cpu/sources.cmake", "list(APPEND ti_c54x_core_sources\n)\n")
        errors = check_repository(self.root)
        self.assertIn("EMPTY", self.codes(errors))
        self.assertIn("MISSING", self.codes(errors))

    def test_duplicate_source_is_rejected(self) -> None:
        self.fixture()
        self.write(
            "src/cpu/sources.mk",
            "ti_c54x_core_sources += src/cpu/cpu.c src/cpu/cpu.c\n",
        )
        errors = check_repository(self.root)
        self.assertIn("DUPLICATE", self.codes(errors))

    def test_cross_owner_source_is_rejected(self) -> None:
        self.fixture(("cpu", "debug"))
        self.write(
            "src/cpu/sources.mk",
            "ti_c54x_core_sources += src/debug/debug.c\n",
        )
        errors = check_repository(self.root)
        self.assertIn("OWNER", self.codes(errors))

    def test_traversal_source_token_is_rejected(self) -> None:
        self.fixture()
        self.write("src/cpu/sources.mk", "ti_c54x_core_sources += ../cpu.c\n")
        errors = check_repository(self.root)
        self.assertIn("TOKEN", self.codes(errors))

    def test_unreachable_owner_manifest_is_rejected(self) -> None:
        self.fixture()
        self.write("src/debug/debug.c", "int fixture_debug;\n")
        self.write("src/debug/sources.mk", "ti_c54x_core_sources += src/debug/debug.c\n")
        self.write(
            "src/debug/sources.cmake",
            "list(APPEND ti_c54x_core_sources\n"
            "    \"${CMAKE_CURRENT_LIST_DIR}/debug.c\"\n)\n",
        )
        errors = check_repository(self.root)
        self.assertIn("DISCONNECTED", self.codes(errors))

    def test_lib_objects_must_reuse_canonical_make_list(self) -> None:
        self.fixture()
        self.write(
            "Makefile",
            "ti_c54x_core_sources :=\ninclude src/cpu/sources.mk\n"
            "LIB_OBJECTS := build/cpu.o\n"
            "OBSERVER_OFF_OBJECTS := $(patsubst src/%.c,build-off/%.o,$(ti_c54x_core_sources))\n",
        )
        errors = check_repository(self.root)
        self.assertIn("CANONICAL", self.codes(errors))

    def test_observer_off_objects_must_reuse_canonical_make_list(self) -> None:
        self.fixture()
        self.write(
            "Makefile",
            "ti_c54x_core_sources :=\ninclude src/cpu/sources.mk\n"
            "LIB_OBJECTS := $(patsubst src/%.c,build/%.o,$(ti_c54x_core_sources))\n"
            "OBSERVER_OFF_OBJECTS := build-off/cpu.o\n",
        )
        errors = check_repository(self.root)
        self.assertIn("CANONICAL", self.codes(errors))

    def test_observer_on_linkage_uses_canonical_archive_and_distinct_gate(self) -> None:
        repository = Path(__file__).resolve().parents[3]
        makefile = (repository / "Makefile").read_text(encoding="utf-8")
        debug_manifest = (repository / "tests" / "unit" / "debug" / "tests.mk").read_text(
            encoding="utf-8"
        )

        self.assertRegex(
            makefile,
            r"(?m)^OBSERVER_ON_DIR := \$\(BUILD_DIR\)/observer-on$",
        )
        self.assertRegex(
            makefile,
            r"(?m)^OBSERVER_ON_OBJECTS := \$\(patsubst src/%\.c,\$\(OBSERVER_ON_DIR\)/obj/%.o,\$\(ti_c54x_core_sources\)\)$",
        )
        self.assertRegex(
            makefile,
            r"(?m)^OBSERVER_ON_LIB := \$\(OBSERVER_ON_DIR\)/libti_c54x\.a$",
        )
        self.assertRegex(
            makefile,
            r"(?m)^OBSERVER_OFF_OBJECTS := \$\(patsubst src/%\.c,\$\(OBSERVER_OFF_DIR\)/obj/%.o,\$\(ti_c54x_core_sources\)\)$",
        )
        self.assertNotEqual(
            self._assignment(makefile, "OBSERVER_ON_DIR"),
            self._assignment(makefile, "OBSERVER_OFF_DIR"),
        )
        self.assertIn("$(OBSERVER_ON_OBJECTS)", makefile)
        self.assertIn("$(OBSERVER_OFF_OBJECTS)", makefile)
        self.assertRegex(
            makefile,
            r"-UTI_C54X_ENABLE_OBSERVERS\s*\\\s*-DTI_C54X_ENABLE_OBSERVERS=1",
        )
        self.assertIn("-UTI_C54X_ENABLE_OBSERVERS", makefile)
        self.assertIn("-DTI_C54X_ENABLE_OBSERVERS=0", makefile)

        self.assertNotIn("src/cpu/cpu.c", debug_manifest)
        self.assertNotIn("src/debug/observer.c", debug_manifest)
        self.assertRegex(
            debug_manifest,
            r"(?m)^\$\(OBSERVER_TEST\): tests/unit/debug/test_observer\.c \$\(OBSERVER_ON_LIB\)$",
        )
        self.assertIn("$(OBSERVER_ON_LIB)", debug_manifest)
        self.assertIn("-UTI_C54X_ENABLE_OBSERVERS", debug_manifest)
        self.assertIn("-DTI_C54X_ENABLE_OBSERVERS=1", debug_manifest)

    @staticmethod
    def _assignment(text: str, variable: str) -> str:
        pattern = re.compile(rf"(?m)^{re.escape(variable)}\s*:=\s*(.+)$")
        match = pattern.search(text)
        if match is None:
            return ""
        return match.group(1).strip()

    def test_concrete_root_source_paths_are_rejected(self) -> None:
        self.fixture()
        self.write(
            "CMakeLists.txt",
            "set(ti_c54x_core_sources)\n"
            "include(${CMAKE_CURRENT_SOURCE_DIR}/src/cpu/sources.cmake)\n"
            "add_library(ti_c54x STATIC ${ti_c54x_core_sources} src/cpu/cpu.c)\n",
        )
        errors = check_repository(self.root)
        self.assertIn("DUPLICATED", self.codes(errors))


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
