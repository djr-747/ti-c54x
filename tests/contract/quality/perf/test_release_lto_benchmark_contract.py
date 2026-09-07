#!/usr/bin/env python3
"""Contract tests for the P2-W31P hosted release-performance gate.

Module contract: owns deterministic proofs for the performance-acceptance
checker, its frozen audit inputs, and the Make benchmark profile wiring.  It
does not run benchmarks, mutate hosted history, or claim TI machine timing.
"""

from __future__ import annotations

import contextlib
import copy
import io
import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

from tools.quality import check_perf_acceptance
from tools.quality.check_perf_acceptance import (
    PROFILE_O2,
    PROFILE_O3_LTO,
    evaluate_capture_set,
    load_json_object,
    validate_audit,
)
from tools.perf.capture import FIXED_SEMANTIC_HASHES


ROOT = Path(__file__).resolve().parents[4]
AUDIT_PATH = ROOT / "bench" / "audits" / "p2-w31p-baseline.json"
CHECKER_PATH = ROOT / "tools" / "quality" / "check_perf_acceptance.py"
MAKEFILE_PATH = ROOT / "Makefile"
MIXED_SEMANTIC_HASH = "f59db4e45ad10a06"
CATALOG_COUNT = 24
RETIRED_COUNT = 800_000
BASE_FLAGS = "-Iinclude -Isrc -DTI_C54X_ENABLE_OBSERVERS=0"
COMMON_FLAGS = (
    "-DNDEBUG", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
    "-Wsign-conversion", "-Wshadow", "-Wundef", "-Wdouble-promotion", "-Werror",
)


class ReleaseLtoBenchmarkContractTests(unittest.TestCase):
    """Exercise acceptance policy without invoking the long-running bench."""

    @staticmethod
    def _profile_flags(profile: str) -> str:
        optimization = "-O2" if profile == PROFILE_O2 else "-O3 -flto"
        return f"{BASE_FLAGS} {optimization} {' '.join(COMMON_FLAGS)}"

    @classmethod
    def _result(
        cls,
        scenario: str,
        *,
        ns_per_instruction: float = 50.0,
        spread_ratio: float = 0.05,
    ) -> dict[str, Any]:
        elapsed_ns = int(ns_per_instruction * RETIRED_COUNT)
        half_span = int(elapsed_ns * spread_ratio / 2.0)
        result: dict[str, Any] = {
            "schema_version": 1,
            "clock_source": "CLOCK_MONOTONIC",
            "scenario": scenario,
            "retired_count": RETIRED_COUNT,
            "elapsed_ns": elapsed_ns,
            "elapsed_ticks": elapsed_ns,
            "min_elapsed_ns": elapsed_ns - half_span,
            "max_elapsed_ns": elapsed_ns + half_span,
            "samples": 7,
            "warmup_windows": 16,
            "window_count": 16,
            "retirements_per_window": 50_000,
            "semantic_hash": FIXED_SEMANTIC_HASHES.get(scenario, MIXED_SEMANTIC_HASH),
            "scenario_instruction_count": 1 if scenario != "mixed-current" else CATALOG_COUNT,
            "catalog_instruction_count": CATALOG_COUNT,
            "ns_per_instruction": elapsed_ns / RETIRED_COUNT,
            "instructions_per_second": RETIRED_COUNT * 1_000_000_000.0 / elapsed_ns,
            "catalog_coverage_mode": "fixed" if scenario != "mixed-current" else "full-catalog",
            "catalog_covered_instruction_count": 1 if scenario != "mixed-current" else CATALOG_COUNT,
        }
        return result

    @classmethod
    def _capture(cls, profile: str, *, ns_per_instruction: float = 50.0,
                 spread_ratio: float = 0.05) -> dict[str, Any]:
        scenarios = tuple(FIXED_SEMANTIC_HASHES) + ("mixed-current",)
        return {
            "schema_version": 1,
            "clock_source": "CLOCK_MONOTONIC",
            "compiler": "cc (fixture) 1.0",
            "platform": "fixture-platform",
            "platform_machine": "fixture-machine",
            "cpu": "fixture-cpu",
            "build_flags": cls._profile_flags(profile),
            "vcs_state": "clean",
            "git_sha": "a" * 40,
            "catalog_instruction_count": CATALOG_COUNT,
            "scenario_semantic_hash": "b" * 64,
            "benchmark_source_sha256": "c" * 64,
            "binary_sha256": "d" * 64,
            "results": [
                cls._result(
                    scenario,
                    ns_per_instruction=ns_per_instruction,
                    spread_ratio=spread_ratio,
                )
                for scenario in scenarios
            ],
        }

    @staticmethod
    def _profile_tokens(make_text: str, variable: str) -> tuple[str, ...]:
        lines = make_text.splitlines()
        start = next(
            (index for index, line in enumerate(lines)
             if re.match(rf"^{re.escape(variable)}\s*:=", line)),
            None,
        )
        if start is None:
            raise AssertionError(f"missing Make variable {variable}")
        body = re.split(r"\s*:=\s*", lines[start], maxsplit=1)[1]
        index = start
        while body.rstrip().endswith("\\") and index + 1 < len(lines):
            body = body.rstrip()[:-1] + " " + lines[index + 1].strip()
            index += 1
        return tuple(body.split())

    def test_make_profiles_are_frozen_and_reachable(self) -> None:
        make_text = MAKEFILE_PATH.read_text(encoding="utf-8")
        diagnostic = self._profile_tokens(make_text, "BENCH_DIAGNOSTIC_CFLAGS")
        release = self._profile_tokens(make_text, "BENCH_RELEASE_CFLAGS")
        self.assertEqual(diagnostic, ("-O2", *COMMON_FLAGS))
        self.assertEqual(release, ("-O3", "-flto", *COMMON_FLAGS))
        self.assertNotIn("-flto", diagnostic)
        self.assertEqual(
            re.search(r"(?m)^BENCH_CFLAGS\s*\?=\s*(\S+)$", make_text).group(1),
            "$(BENCH_DIAGNOSTIC_CFLAGS)",
        )
        self.assertIn("BENCH_CPPFLAGS := -Iinclude -Isrc -DTI_C54X_ENABLE_OBSERVERS=0", make_text)
        self.assertIn("$(PYTHON) tools/perf/capture.py --benchmark $(BENCH_EXE)", make_text)
        self.assertIn('--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"', make_text)
        self.assertRegex(
            make_text,
            r"(?ms)^bench-release:\n\s*\$\(MAKE\)\s+-B\s+"
            r"BENCH_BUILD_DIR=build-bench-release\s+\\\s*"
            r"BENCH_CFLAGS='\$\(BENCH_RELEASE_CFLAGS\)' bench",
        )

    def test_trusted_hot_path_has_one_preparation_plan_and_generic_dispatch(self) -> None:
        run_text = (ROOT / "src/cpu/execution/run.c").read_text(encoding="utf-8")
        packet_text = (ROOT / "src/isa/decode/trusted_execution_packet.c").read_text(
            encoding="utf-8"
        )
        layout_text = (ROOT / "src/isa/decode/operand_layout.c").read_text(
            encoding="utf-8"
        )
        self.assertEqual(
            len(re.findall(r"\bti_c54x_trusted_opcode_binding_for_first_word\s*\(", run_text)),
            1,
        )
        self.assertEqual(
            len(re.findall(r"\bti_c54x_trusted_execution_packet_prepare_fast\s*\(", run_text)),
            1,
        )
        self.assertEqual(
            len(re.findall(r"\bti_c54x_program_pc_bridge_plan_trusted\s*\(", run_text)),
            1,
        )
        self.assertEqual(
            len(re.findall(r"\bti_c54x_operand_layout_extract_all_trusted\s*\(", packet_text)),
            1,
        )
        fast_constructor = packet_text.split(
            "ti_c54x_trusted_execution_packet_prepare_fast", maxsplit=1
        )[1]
        trusted_extractor = layout_text.split(
            "ti_c54x_operand_layout_extract_all_trusted", maxsplit=1
        )[1]
        self.assertNotIn("ti_c54x_trusted_opcode_binding_is_canonical", fast_constructor)
        self.assertNotIn("ti_c54x_trusted_opcode_binding_for_id", fast_constructor)
        self.assertNotIn("ti_c54x_operand_layout_plan_for_id", fast_constructor)
        self.assertIn("binding->operand_plan", packet_text)
        self.assertNotIn("ti_c54x_operand_layout_descriptor_for_id", trusted_extractor)
        self.assertNotIn("ti_c54x_operand_layout_plan_for_id", trusted_extractor)
        self.assertNotIn("ti_c54x_operand_layout_set_validate_descriptor", trusted_extractor)
        self.assertNotIn("ti_c54x_operand_layout_set_resolve(", packet_text)
        self.assertNotIn("ti_c54x_operand_layout_set_extract_operand(", packet_text)
        self.assertNotRegex(run_text, r"switch\s*\([^)]*(?:opcode|binding|id)")
        self.assertNotRegex(run_text, r"case\s+TI_C54X_OPCODE_ID_")
        handler_calls = re.findall(r"\bti_c54x_op_[A-Za-z0-9_]+\s*\(", run_text)
        self.assertEqual(handler_calls, [])

    def test_frozen_audit_validates_and_retains_both_profiles(self) -> None:
        audit = load_json_object(AUDIT_PATH)
        policy = validate_audit(audit)
        self.assertEqual(audit["audit_id"], "P2-W31P-BASELINE-2026-09-07")
        self.assertEqual(policy["target_ns_per_instruction_exclusive"], 60.0)
        self.assertEqual(policy["target_scenarios"], list(FIXED_SEMANTIC_HASHES))
        self.assertEqual(policy["profiles"], [PROFILE_O2, PROFILE_O3_LTO])
        self.assertEqual(policy["capture_set_size"], 3)
        self.assertEqual(policy["max_within_capture_range_over_median"], 0.15)
        profiles = audit["profiles"]
        self.assertEqual(
            [profile["id"] for profile in profiles],
            ["portable-o2-no-lto-a", "portable-o2-no-lto-b", "portable-o3-lto-a"],
        )
        self.assertEqual(
            [check_perf_acceptance._profile_for_flags(profile["build_flags"]) for profile in profiles],
            [PROFILE_O2, PROFILE_O2, PROFILE_O3_LTO],
        )
        for profile in profiles:
            self.assertGreaterEqual(len(profile["results"]), 5)
            self.assertTrue(profile["binary_sha256"])
            self.assertTrue(profile["command"])

    def test_direct_checker_cli_help_is_available(self) -> None:
        completed = subprocess.run(
            [sys.executable, str(CHECKER_PATH), "--help"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("Validate the P2-W31P audit", completed.stdout)

    def test_audit_rejects_schema_policy_roster_profile_and_metric_drift(self) -> None:
        audit = load_json_object(AUDIT_PATH)
        mutations = {
            "schema": lambda value: value.update(schema_version=2),
            "target": lambda value: value["acceptance_policy"].update(
                target_ns_per_instruction_exclusive=60.1
            ),
            "scenario": lambda value: value["acceptance_policy"].update(
                target_scenarios=["nop"]
            ),
            "profile": lambda value: value["profiles"][0].update(build_flags="-O2 -flto"),
            "roster": lambda value: value["profiles"][0]["results"].pop(),
            "duplicate-scenario": lambda value: value["profiles"][0]["results"].append(
                copy.deepcopy(value["profiles"][0]["results"][0])
            ),
            "hash": lambda value: value["profiles"][0]["results"][0].update(
                semantic_hash="0000000000000000"
            ),
            "source-hash": lambda value: value["source"].update(
                benchmark_source_sha256="not-a-sha256"
            ),
            "retired-count": lambda value: value["profiles"][0]["results"][0].update(
                retired_count=0
            ),
            "ns-per-instruction": lambda value: value["profiles"][0]["results"][0].update(
                ns_per_instruction=999.0
            ),
            "metric": lambda value: value["profiles"][0]["results"][0].update(
                median_elapsed_ns=0
            ),
        }
        for name, mutate in mutations.items():
            candidate = copy.deepcopy(audit)
            mutate(candidate)
            with self.subTest(case=name), self.assertRaises(ValueError):
                validate_audit(candidate)

    def test_three_capture_sets_accept_for_each_profile(self) -> None:
        for profile in (PROFILE_O2, PROFILE_O3_LTO):
            captures = [self._capture(profile) for _ in range(3)]
            with self.subTest(profile=profile):
                self.assertEqual(
                    evaluate_capture_set(captures, profile, validate_audit(load_json_object(AUDIT_PATH))),
                    {
                        "status": "ACCEPTED",
                        "profile": profile,
                        "capture_count": 3,
                        "failures": [],
                    },
                )

    def test_inconclusive_set_retains_each_failed_capture_reason(self) -> None:
        policy = validate_audit(load_json_object(AUDIT_PATH))
        threshold = [self._capture(PROFILE_O2) for _ in range(3)]
        threshold[1] = self._capture(PROFILE_O2, ns_per_instruction=60.0)
        result = evaluate_capture_set(threshold, PROFILE_O2, policy)
        self.assertEqual(result["status"], "INCONCLUSIVE")
        self.assertEqual(result["capture_count"], 3)
        self.assertTrue(any("capture 2 nop is not below 60.0 ns" in item for item in result["failures"]))

        unstable = [self._capture(PROFILE_O3_LTO) for _ in range(3)]
        unstable[2] = self._capture(PROFILE_O3_LTO, spread_ratio=0.20)
        result = evaluate_capture_set(unstable, PROFILE_O3_LTO, policy)
        self.assertEqual(result["status"], "INCONCLUSIVE")
        self.assertTrue(any("capture 3 nop stability ratio" in item for item in result["failures"]))

    def test_mixed_current_is_validated_but_not_an_acceptance_target(self) -> None:
        policy = validate_audit(load_json_object(AUDIT_PATH))
        captures = [self._capture(PROFILE_O2) for _ in range(3)]
        for capture in captures:
            capture["results"][-1] = self._result(
                "mixed-current", ns_per_instruction=500.0, spread_ratio=1.0
            )
        result = evaluate_capture_set(captures, PROFILE_O2, policy)
        self.assertEqual(result["status"], "ACCEPTED")
        self.assertEqual(result["failures"], [])

    def test_malformed_capture_sets_are_rejected_before_acceptance(self) -> None:
        policy = validate_audit(load_json_object(AUDIT_PATH))
        valid = [self._capture(PROFILE_O2) for _ in range(3)]
        mutations = {
            "short-set": lambda values: values.pop(),
            "profile": lambda values: values[0].update(build_flags=self._profile_flags(PROFILE_O3_LTO)),
            "binary-cohort": lambda values: values[1].update(binary_sha256="e" * 64),
            "scenario-cohort": lambda values: values[1].update(scenario_semantic_hash="f" * 64),
            "compiler-cohort": lambda values: values[1].update(compiler="other compiler"),
            "platform-cohort": lambda values: values[1].update(platform="other platform"),
            "machine-cohort": lambda values: values[1].update(platform_machine="other machine"),
            "cpu-cohort": lambda values: values[1].update(cpu="other cpu"),
            "clock-cohort": lambda values: values[1].update(clock_source="CLOCK_PROCESS_CPUTIME_ID"),
            "scenario-hashes": lambda values: values[1]["results"][-1].update(semantic_hash="0123456789abcdef"),
            "malformed-result": lambda values: values[1]["results"][0].update(elapsed_ns=True),
            "unknown-profile": lambda values: None,
            "non-record": lambda values: values.__setitem__(1, []),
        }
        for name, mutate in mutations.items():
            candidate = copy.deepcopy(valid)
            with self.subTest(case=name), self.assertRaises(ValueError):
                if name == "unknown-profile":
                    evaluate_capture_set(candidate, "unknown", policy)
                else:
                    mutate(candidate)
                    evaluate_capture_set(candidate, PROFILE_O2, policy)

    def test_cli_returns_distinct_acceptance_inconclusive_and_error_statuses(self) -> None:
        policy = validate_audit(load_json_object(AUDIT_PATH))
        del policy  # The CLI reloads the audit; this only proves fixture setup is valid.
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            audit_path = directory / "audit.json"
            audit_path.write_text(AUDIT_PATH.read_text(encoding="utf-8"), encoding="utf-8")
            accepted_paths = []
            for index in range(3):
                path = directory / f"accepted-{index}.json"
                path.write_text(json.dumps(self._capture(PROFILE_O3_LTO)), encoding="utf-8")
                accepted_paths.append(path)
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                status = check_perf_acceptance.main(
                    ["--audit", str(audit_path), "--profile", PROFILE_O3_LTO,
                     *sum((["--capture", str(path)] for path in accepted_paths), [])]
                )
            self.assertEqual(status, 0)
            self.assertIn('"status":"ACCEPTED"', stdout.getvalue())

            inconclusive_path = directory / "inconclusive.json"
            inconclusive_path.write_text(
                json.dumps(self._capture(PROFILE_O3_LTO, ns_per_instruction=60.0)),
                encoding="utf-8",
            )
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                status = check_perf_acceptance.main(
                    ["--audit", str(audit_path), "--profile", PROFILE_O3_LTO,
                     "--capture", str(inconclusive_path), "--capture", str(accepted_paths[1]),
                     "--capture", str(accepted_paths[2])]
                )
            self.assertEqual(status, 1)
            self.assertIn('"status":"INCONCLUSIVE"', stdout.getvalue())

            malformed_path = directory / "malformed.json"
            malformed_path.write_text("[]", encoding="utf-8")
            with contextlib.redirect_stderr(io.StringIO()):
                status = check_perf_acceptance.main(
                    ["--audit", str(audit_path), "--profile", PROFILE_O3_LTO,
                     "--capture", str(malformed_path), "--capture", str(accepted_paths[1]),
                     "--capture", str(accepted_paths[2])]
                )
            self.assertEqual(status, 2)


if __name__ == "__main__":
    unittest.main()
