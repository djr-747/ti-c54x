#!/usr/bin/env python3
"""Focused fixtures for source-boundary policy.

Owns: executable boundary fixtures and their temporary roots.  Does not own:
the checker implementation, production source, or build integration.
"""

from __future__ import annotations

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

from tools.quality.check_source_boundaries import check_tree, main


CONTRACT = """/* Module contract
 * Owns: fixture-only behavior.
 * Does not own: host configuration or process state.
 */
"""


class SourceBoundaryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def write(self, relative: str, body: str) -> Path:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
        return path

    def clean_source(self, relative: str = "src/cpu/fixture.c", body: str = "") -> None:
        self.write(relative, CONTRACT + "\n" + body)

    def codes(self, body: str) -> set[str]:
        self.clean_source(body=body)
        return {violation.code for violation in check_tree(self.root).violations}

    def test_clean_tree_passes_from_any_current_directory(self) -> None:
        self.clean_source(body="static void helper(void) { int local = 1; (void)local; }\n")
        report = check_tree(self.root)
        self.assertTrue(report.ok, report.violations)
        self.assertEqual(report.files_checked, 1)

    def test_missing_contract_is_rejected(self) -> None:
        self.write("src/cpu/fixture.c", "int ordinary_function(void) { return 0; }\n")
        self.assertIn("CONTRACT", self.codes_from_report())

    def test_contract_after_a_declaration_is_rejected(self) -> None:
        self.write(
            "src/cpu/fixture.c",
            "int ordinary_function(void) { return 0; }\n" + CONTRACT,
        )
        self.assertIn("CONTRACT", self.codes_from_report())

    def codes_from_report(self) -> set[str]:
        return {violation.code for violation in check_tree(self.root).violations}

    def test_flat_and_unknown_ownership_paths_are_rejected(self) -> None:
        self.clean_source("src/fixture.c")
        self.clean_source("src/misc/fixture.c")
        report = check_tree(self.root)
        self.assertGreaterEqual(sum(v.code == "OWNERSHIP" for v in report.violations), 2)

    def test_isa_requires_a_named_subowner(self) -> None:
        self.clean_source("src/isa/fixture.c")
        self.assertIn("OWNERSHIP", self.codes_from_report())

    def test_isa_catalog_is_a_valid_source_owner(self) -> None:
        self.clean_source("src/isa/catalog/fixture.c")
        self.assertNotIn("OWNERSHIP", self.codes_from_report())

    def test_forbidden_headers_and_calls_are_rejected(self) -> None:
        self.clean_source(body="#include <stdio.h>\nvoid f(void) { printf(\"x\"); }\n")
        codes = self.codes_from_report()
        self.assertIn("HOST_INCLUDE", codes)
        self.assertIn("HOST_API", codes)

    def test_quoted_host_headers_are_also_rejected(self) -> None:
        self.clean_source(body='#include "stdio.h"\n')
        self.assertIn("HOST_INCLUDE", self.codes_from_report())

    def test_include_inside_block_comment_is_ignored(self) -> None:
        self.clean_source(body="/*\n#include <stdio.h>\n*/\n")
        self.assertNotIn("HOST_INCLUDE", self.codes_from_report())

    def test_environment_api_is_rejected_even_without_a_forbidden_header(self) -> None:
        self.clean_source(body="void f(void) { (void)getenv(\"X\"); }\n")
        self.assertIn("ENV_API", self.codes_from_report())

    def test_platform_lineage_names_are_rejected_as_tokens(self) -> None:
        self.clean_source(body="int nokia_state(void) { return 0; }\n")
        self.assertIn("PLATFORM", self.codes_from_report())

    def test_mutable_file_scope_objects_are_rejected(self) -> None:
        self.clean_source(body="static int mutable_counter = 0;\n")
        self.assertIn("GLOBAL_STATE", self.codes_from_report())

    def test_mutable_file_scope_aggregate_and_function_pointer_are_rejected(self) -> None:
        self.clean_source(
            body=(
                "static int mutable_table[] = {1, 2};\n"
                "static void (*mutable_callback)(void);\n"
            )
        )
        report = check_tree(self.root)
        self.assertGreaterEqual(sum(v.code == "GLOBAL_STATE" for v in report.violations), 2)

    def test_nonstatic_tag_object_and_pointer_to_const_are_rejected(self) -> None:
        self.clean_source(
            body=(
                "struct fixture { int value; } mutable_object;\n"
                "const int *mutable_pointer;\n"
            )
        )
        report = check_tree(self.root)
        self.assertGreaterEqual(sum(v.code == "GLOBAL_STATE" for v in report.violations), 2)

    def test_named_tag_objects_and_mutable_externs_are_rejected(self) -> None:
        self.clean_source(
            body=(
                "struct named mutable_struct;\n"
                "union named mutable_union;\n"
                "enum named mutable_enum;\n"
                "extern int process_global;\n"
            )
        )
        report = check_tree(self.root)
        self.assertGreaterEqual(sum(v.code == "GLOBAL_STATE" for v in report.violations), 4)

    def test_extern_const_data_and_function_prototypes_are_allowed(self) -> None:
        self.clean_source(
            body=(
                "extern const int immutable_data;\n"
                "extern int process_function(void);\n"
            )
        )
        self.assertNotIn("GLOBAL_STATE", self.codes_from_report())

    def test_type_definitions_immutable_objects_prototypes_and_locals_are_allowed(self) -> None:
        self.clean_source(
            body=(
                "typedef struct { int value; } fixture_t;\n"
                "struct forward;\n"
                "static const int immutable_value = 1;\n"
                "static const int *const immutable_pointer = 0;\n"
                "static void helper(void);\n"
                "static void helper(void) { static int local_state = 0; (void)local_state; }\n"
            )
        )
        self.assertNotIn("GLOBAL_STATE", self.codes_from_report())

    def test_const_globals_functions_and_locals_are_not_false_positives(self) -> None:
        self.clean_source(
            body=(
                "static const int immutable_value = 1;\n"
                "static void static_helper(void) { static int local_state = 0; (void)local_state; }\n"
                "int public_helper(void) { int local = 2; return local; }\n"
            )
        )
        self.assertNotIn("GLOBAL_STATE", self.codes_from_report())

    def test_comments_and_strings_do_not_trigger_host_or_platform_checks(self) -> None:
        self.clean_source(
            body=(
                "/* getenv printf Nokia QEMU stdio.h */\n"
                "static const char text[] = \"getenv printf Nokia QEMU\";\n"
            )
        )
        report = check_tree(self.root)
        self.assertNotIn("ENV_API", {v.code for v in report.violations})
        self.assertNotIn("HOST_API", {v.code for v in report.violations})
        self.assertNotIn("PLATFORM", {v.code for v in report.violations})

    def test_opcode_handlers_cannot_depend_on_observers(self) -> None:
        self.clean_source(
            "src/isa/ops/fixture.c",
            "#include \"../../debug/observer.h\"\nvoid f(void) { debug(); }\n",
        )
        self.assertIn("OP_DEBUG", self.codes_from_report())

    def test_opcode_include_only_observer_dependency_is_rejected(self) -> None:
        self.clean_source(
            "src/isa/ops/fixture.c",
            '#include "../../debug/observer.h"\n',
        )
        self.assertIn("OP_DEBUG", self.codes_from_report())

    def test_observation_mask_storage_is_owned_by_debug(self) -> None:
        self.clean_source(
            body=(
                "struct fixture { unsigned long event_mask; };\n"
                "void f(struct fixture *state) { (void)state->event_mask; }\n"
            )
        )
        self.assertIn("OBSERVE_BOUNDARY", self.codes_from_report())

    def test_public_headers_cannot_expose_test_or_temp_knobs(self) -> None:
        self.clean_source(
            "include/ti_c54x/fixture.h",
            "#define TI_C54X_ENABLE_TEST_HOOKS 1\n",
        )
        self.assertIn("PUBLIC_KNOB", self.codes_from_report())

    def test_gate_definitions_are_owned_by_debug(self) -> None:
        self.clean_source(
            "src/cpu/fixture.c",
            "#define TI_C54X_OBSERVE_ENABLED(x, y) (false)\n",
        )
        self.assertIn("OBSERVE_BOUNDARY", self.codes_from_report())

    def test_enabled_and_disabled_gate_shapes_are_proven(self) -> None:
        self.clean_source(
            "src/debug/observer.h",
            """#define TI_C54X_OBSERVE_ENABLED(state, bit) \\
    ((state)->event_mask != UINT64_C(0) && ((state)->event_mask & (bit)))
#define TI_C54X_OBSERVE_ENABLED(state, bit) (false && (void)(state))
""",
        )
        self.assertNotIn("OBSERVE_GATE_PROOF", self.codes_from_report())

    def test_malformed_gate_shapes_are_rejected(self) -> None:
        self.clean_source(
            "src/debug/observer.h",
            """#define TI_C54X_OBSERVE_ENABLED(state, bit) \\
    ((state)->event_mask && ((state)->event_mask & (bit)))
#define TI_C54X_OBSERVE_ENABLED(state, bit) (false)
""",
        )
        self.assertIn("OBSERVE_GATE_PROOF", self.codes_from_report())

    def test_extra_gate_definition_is_rejected(self) -> None:
        self.clean_source(
            "src/debug/observer.h",
            """#define TI_C54X_OBSERVE_ENABLED(state, bit) \\
    ((state)->event_mask != UINT64_C(0) && ((state)->event_mask & (bit)))
#define TI_C54X_OBSERVE_ENABLED(state, bit) (false && (void)(state))
#define TI_C54X_OBSERVE_ENABLED(state, bit) (false && (void)(bit))
""",
        )
        self.assertIn("OBSERVE_GATE_PROOF", self.codes_from_report())

    def test_test_c_and_h_paths_mirror_ownership_tree(self) -> None:
        self.clean_source()
        self.write("tests/support/fixture.c", CONTRACT)
        self.write("tests/unit/cpu/fixture.h", CONTRACT)
        self.write("tests/unit/isa/catalog/fixture.c", CONTRACT)
        self.write("tests/unit/isa/ops/fixture.c", CONTRACT)
        self.write("tests/contract/fixture.c", CONTRACT)
        self.write("tests/differential/fixture.h", CONTRACT)
        self.write("tests/fuzz/fixture.c", CONTRACT)
        self.assertNotIn("TEST_OWNERSHIP", self.codes_from_report())

    def test_test_source_requires_module_contract(self) -> None:
        self.clean_source()
        self.write("tests/support/fixture.c", "int test_fixture(void) { return 0; }\n")
        self.assertIn("CONTRACT", self.codes_from_report())

    def test_flat_and_misc_test_c_or_h_paths_are_rejected(self) -> None:
        self.clean_source()
        self.write("tests/fixture.c", "")
        self.write("tests/misc/fixture.h", "")
        self.write("tests/unit/isa/misc/fixture.c", "")
        report = check_tree(self.root)
        self.assertGreaterEqual(sum(v.code == "TEST_OWNERSHIP" for v in report.violations), 3)

    def test_empty_production_root_is_unusable(self) -> None:
        report = check_tree(self.root)
        self.assertFalse(report.ok)
        self.assertIn("ROOT", {violation.code for violation in report.violations})

    def test_large_handwritten_file_is_rejected(self) -> None:
        self.clean_source(body="\n".join("static void f%d(void) {}" % i for i in range(801)))
        self.assertIn("SIZE", self.codes_from_report())

    def test_oversized_test_source_is_rejected(self) -> None:
        self.clean_source()
        body = CONTRACT + "\n" + "\n".join("int test_line_%d(void) { return 0; }" % i for i in range(801))
        self.write("tests/support/fixture.c", body)
        self.assertIn("SIZE", self.codes_from_report())

    def test_cli_is_clear_and_uses_explicit_root(self) -> None:
        self.clean_source()
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            status = main(["--root", str(self.root)])
        self.assertEqual(status, 0)
        self.assertIn("source boundaries: PASS", stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
