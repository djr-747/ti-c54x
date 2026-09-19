"""Contract checks for the observer-off ADD/SUB syntax-9 benchmark."""

from __future__ import annotations

import copy
import json
import re
import subprocess
import unittest
from pathlib import Path

from tools.perf import add_sub_capture


ROOT = Path(__file__).resolve().parents[4]
BENCH_SOURCE = ROOT / "bench" / "ti_c54x_add_sub_bench.c"
CAPTURE_SOURCE = ROOT / "tools" / "perf" / "add_sub_capture.py"


def _source(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _result(name: str, elapsed_ns: int = 80_000_000) -> dict[str, object]:
    spec = add_sub_capture.EXPECTED_SCENARIOS[name]
    return {
        "scenario": name,
        "program_words": list(spec["program_words"]),
        "initial_a_raw40": 1,
        "initial_b_raw40": 0x12340000,
        "initial_sxm": 0,
        "initial_ovm": 0,
        "retired_count": add_sub_capture.RETIRED,
        "attempted_count": add_sub_capture.RETIRED,
        "final_pc_word": add_sub_capture.RETIREMENTS_PER_WINDOW,
        "program_reads": add_sub_capture.RETIRED,
        "final_a_raw40": 1,
        "final_b_raw40": spec["expected_b"],
        "final_c": spec["expected_c"],
        "final_ova": 0,
        "final_ovb": 0,
        "state_valid": True,
        "pc_progression_valid": True,
        "elapsed_ns": elapsed_ns,
        "elapsed_ticks": elapsed_ns,
        "min_elapsed_ns": elapsed_ns - 100_000,
        "max_elapsed_ns": elapsed_ns + 100_000,
        "ns_per_instruction": elapsed_ns / add_sub_capture.RETIRED,
        "instructions_per_second": add_sub_capture.RETIRED * 1_000_000_000.0 / elapsed_ns,
    }


def _payload() -> dict[str, object]:
    results = [_result(name) for name in add_sub_capture.ROSTER]
    return {
        "schema_version": add_sub_capture.SCHEMA_VERSION,
        "cohort": add_sub_capture.COHORT,
        "clock_source": add_sub_capture.CLOCK_SOURCE,
        "observer_off": True,
        "roster": list(add_sub_capture.ROSTER),
        "semantic_hash": add_sub_capture.semantic_hash(results),
        "samples": add_sub_capture.SAMPLES,
        "warmup_windows": add_sub_capture.WARMUPS,
        "window_count": add_sub_capture.WINDOWS,
        "retirements_per_window": add_sub_capture.RETIREMENTS_PER_WINDOW,
        "results": results,
    }


class AddSubAccShift5HotPathTests(unittest.TestCase):
    def test_build_wiring_and_policy_are_local(self) -> None:
        make_text = _source(ROOT / "Makefile")
        cmake_text = _source(ROOT / "CMakeLists.txt")
        self.assertIn("bench-add-sub:", make_text)
        self.assertIn("bench-add-sub-smoke:", make_text)
        self.assertIn("ti_c54x_add_sub_bench", make_text)
        self.assertIn("tools/perf/add_sub_capture.py", make_text)
        self.assertIn("ti_c54x_add_sub_bench", cmake_text)
        self.assertIn("bench_add_sub_smoke", cmake_text)
        self.assertTrue(BENCH_SOURCE.is_file())
        self.assertTrue(CAPTURE_SOURCE.is_file())
        self.assertIn("TI_C54X_ENABLE_OBSERVERS=0", _source(BENCH_SOURCE))
        target = make_text.split("bench-add-sub:", 1)[1].split(
            "bench-add-sub-smoke:", 1
        )[0]
        self.assertNotIn("history", target)
        self.assertNotIn("compare.py", target)

    def test_benchmark_compile_guard(self) -> None:
        common = [
            "cc",
            "-std=c11",
            "-Iinclude",
            "-Isrc",
            "-fsyntax-only",
            str(BENCH_SOURCE),
        ]
        for extra in (["-DTI_C54X_ENABLE_OBSERVERS=1"], []):
            result = subprocess.run(
                [*common, *extra], cwd=ROOT, capture_output=True, text=True
            )
            self.assertNotEqual(result.returncode, 0)
        result = subprocess.run(
            [*common, "-DTI_C54X_ENABLE_OBSERVERS=0"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_schema_and_semantic_roster_reject_drift(self) -> None:
        payload = _payload()
        self.assertEqual(add_sub_capture.validate_payload(payload), payload)
        for mutation in (
            lambda value: value.update(observer_off=False),
            lambda value: value.update(roster=["add-acc-shift5"]),
            lambda value: value["results"][0].update(program_reads=1),
            lambda value: value["results"][1].update(final_c=0),
            lambda value: value["results"][0].update(final_pc_word=0),
            lambda value: value["results"][0].update(state_valid=False),
        ):
            candidate = copy.deepcopy(payload)
            mutation(candidate)
            with self.assertRaises(ValueError):
                add_sub_capture.validate_payload(candidate)
        encoded = json.dumps(payload)
        self.assertEqual(
            add_sub_capture.parse_output(encoded)["cohort"],
            add_sub_capture.COHORT,
        )
        with self.assertRaises(ValueError):
            add_sub_capture.parse_output(encoded + "\nnoise\n")

    def test_public_trusted_path_has_no_opcode_branch_or_host_effects(self) -> None:
        run = _source(ROOT / "src/cpu/execution/run.c")
        trusted = _source(ROOT / "src/isa/decode/trusted_execution_packet.c")
        arithmetic = _source(ROOT / "src/isa/ops/arithmetic/op_arithmetic.c")
        forbidden = re.compile(
            r"\b(?:malloc|calloc|realloc|free|aligned_alloc|getenv|fopen|fclose|"
            r"fread|fwrite|fprintf|printf|puts|putchar|perror|syslog)\s*\("
        )
        self.assertNotRegex(run, r"case\s+TI_C54X_OPCODE_ID_")
        self.assertNotRegex(run, r"switch\s*\([^)]*(?:opcode|instruction|id)")
        self.assertNotRegex(run, r"\bti_c54x_op_[A-Za-z0-9_]+\s*\(")
        for source in (run, trusted, arithmetic):
            self.assertIsNone(forbidden.search(source))
        self.assertIn("prepare_fast", trusted)
        self.assertIn("avoiding a zeroed", trusted)

    def test_trusted_and_effect_carriers_retain_frozen_sizes(self) -> None:
        source = r"""
#include <stdint.h>
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/opcode_decode.h"
#include "isa/decode/trusted_execution_packet.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
_Static_assert(sizeof(ti_c54x_opcode_metadata) == 24U, "metadata");
_Static_assert(sizeof(ti_c54x_decoded_instruction) == 24U, "decoded");
_Static_assert(sizeof(ti_c54x_op_input_snapshot) == 80U, "input");
#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_trusted_execution_packet) == 104U, "packet");
_Static_assert(sizeof(ti_c54x_op_effect) == 56U, "effect");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_trusted_execution_packet) == 80U, "packet");
_Static_assert(sizeof(ti_c54x_op_effect) == 44U, "effect");
#else
#error "unsupported pointer width"
#endif
int main(void) { return 0; }
"""
        result = subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Iinclude",
                "-Isrc",
                "-x",
                "c",
                "-fsyntax-only",
                "-",
            ],
            cwd=ROOT,
            input=source,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_fixed_benchmark_dimensions_and_no_host_threshold(self) -> None:
        bench = _source(BENCH_SOURCE)
        capture = _source(CAPTURE_SOURCE)
        self.assertIn("ADD_SUB_SAMPLES = 7", bench)
        self.assertIn("ADD_SUB_WARMUPS = 16", bench)
        self.assertIn("ADD_SUB_WINDOWS = 16", bench)
        self.assertIn("ADD_SUB_RETIREMENTS_PER_WINDOW = 50000", bench)
        self.assertNotRegex(capture, r"\b(?:threshold|baseline|compare)\b")


if __name__ == "__main__":
    unittest.main()
