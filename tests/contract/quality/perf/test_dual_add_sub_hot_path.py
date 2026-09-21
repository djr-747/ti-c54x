"""Contract tests for the observer-off dual-memory ADD/SUB perf lane.

Module contract: owns deterministic checks for the benchmark's fixed roster,
endpoint lifecycle, semantic hash, advisory comparator, and common-path ABI
guards. It does not claim TI machine timing or mutate benchmark history.
"""

from __future__ import annotations

import copy
import json
import re
import subprocess
import unittest
from pathlib import Path

from tools.perf import dual_add_sub_capture, dual_add_sub_compare


ROOT = Path(__file__).resolve().parents[4]
BENCH_SOURCE = ROOT / "bench" / "ti_c54x_dual_add_sub_bench.c"
CAPTURE_SOURCE = ROOT / "tools" / "perf" / "dual_add_sub_capture.py"
COMPARE_SOURCE = ROOT / "tools" / "perf" / "dual_add_sub_compare.py"


def _source(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _result(name: str, elapsed_ns: int = 80_000_000) -> dict[str, object]:
    spec = dual_add_sub_capture.EXPECTED_SCENARIOS[name]
    return {
        "scenario": name,
        "program_words": list(spec["program_words"]),
        "operation": spec["operation"],
        "destination": spec["destination"],
        "x_ar": 2,
        "y_ar": 3,
        "x_address_word": 0x100,
        "y_address_word": 0x200,
        "x_data_word": 0x1234,
        "y_data_word": 0x2,
        "initial_a_raw40": 1,
        "initial_b_raw40": 2,
        "initial_sxm": 0,
        "initial_ovm": 0,
        "initial_c": 0,
        "retired_count": dual_add_sub_capture.RETIRED,
        "attempted_count": dual_add_sub_capture.RETIRED,
        "final_pc_word": dual_add_sub_capture.RETIRED,
        "program_reads": dual_add_sub_capture.RETIRED,
        "endpoint_request_count": dual_add_sub_capture.RETIRED * 2,
        "endpoint_read_count": dual_add_sub_capture.RETIRED * 2,
        "endpoint_write_count": 0,
        "first_address_word": 0x100,
        "second_address_word": 0x200,
        "first_transaction_id": 1,
        "last_transaction_id": dual_add_sub_capture.RETIRED * 2,
        "request_order_valid": True,
        "address_sequence_valid": True,
        "fresh_transaction_ids": True,
        "pc_progression_valid": True,
        "state_valid": True,
        "final_a_raw40": spec["expected_a_raw40"],
        "final_b_raw40": spec["expected_b_raw40"],
        "final_c": 0,
        "final_ova": 0,
        "final_ovb": 0,
        "elapsed_ns": elapsed_ns,
        "elapsed_ticks": elapsed_ns,
        "min_elapsed_ns": elapsed_ns - 100_000,
        "max_elapsed_ns": elapsed_ns + 100_000,
        "ns_per_instruction": elapsed_ns / dual_add_sub_capture.RETIRED,
        "instructions_per_second": dual_add_sub_capture.RETIRED * 1_000_000_000.0 / elapsed_ns,
    }


def _payload() -> dict[str, object]:
    results = [_result(name) for name in dual_add_sub_capture.ROSTER]
    return {
        "schema_version": dual_add_sub_capture.SCHEMA_VERSION,
        "cohort": dual_add_sub_capture.COHORT,
        "clock_source": dual_add_sub_capture.CLOCK_SOURCE,
        "observer_off": True,
        "roster": list(dual_add_sub_capture.ROSTER),
        "semantic_hash": dual_add_sub_capture.semantic_hash(results),
        "samples": dual_add_sub_capture.SAMPLES,
        "warmup_windows": dual_add_sub_capture.WARMUPS,
        "window_count": dual_add_sub_capture.WINDOWS,
        "retirements_per_window": dual_add_sub_capture.RETIREMENTS_PER_WINDOW,
        "results": results,
    }


def _record(payload: dict[str, object], profile: str = "portable-o2-no-lto") -> dict[str, object]:
    record = copy.deepcopy(payload)
    record.update(
        captured_utc="2026-09-09T00:00:00+00:00",
        git_sha="a" * 40,
        vcs_state="clean",
        compiler="cc (fixture) 1.0",
        platform="fixture-platform",
        platform_machine="fixture-machine",
        cpu="fixture-cpu",
        build_flags="-Iinclude -Isrc -DTI_C54X_ENABLE_OBSERVERS=0 -O2 -DNDEBUG",
        profile=profile,
        benchmark_source_sha256="b" * 64,
        binary_sha256="c" * 64,
    )
    return record


class DualAddSubHotPathTests(unittest.TestCase):
    def test_files_and_observer_off_public_cpu_shape(self) -> None:
        self.assertTrue(BENCH_SOURCE.is_file())
        self.assertTrue(CAPTURE_SOURCE.is_file())
        self.assertTrue(COMPARE_SOURCE.is_file())
        bench = _source(BENCH_SOURCE)
        self.assertIn("TI_C54X_ENABLE_OBSERVERS=0", bench)
        self.assertIn("ti_c54x_cpu_run", bench)
        self.assertIn("DUAL_ADD_SUB_SAMPLES = 7", bench)
        self.assertIn("DUAL_ADD_SUB_WARMUPS = 16", bench)
        self.assertIn("DUAL_ADD_SUB_WINDOWS = 16", bench)
        self.assertIn("DUAL_ADD_SUB_RETIREMENTS_PER_WINDOW = 20000", bench)
        for word in ("0xA001", "0xA101", "0xA201", "0xA301"):
            self.assertIn(word, bench)
        self.assertNotIn("getenv(", bench)
        self.assertNotRegex(bench, r"\bti_c54x_(?:op|trusted_op)_[A-Za-z0-9_]+\s*\(")
        self.assertLess(len(bench.splitlines()), 500)

    def test_compile_guard(self) -> None:
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

    def test_payload_schema_hash_and_state_reject_drift(self) -> None:
        payload = _payload()
        self.assertEqual(dual_add_sub_capture.validate_payload(payload), payload)
        for mutation in (
            lambda value: value.update(observer_off=False),
            lambda value: value.update(roster=["add-dual-mod0-a"]),
            lambda value: value["results"][0].update(program_words=[0xA000]),
            lambda value: value["results"][1].update(endpoint_read_count=0),
            lambda value: value["results"][2].update(final_c=1),
            lambda value: value["results"][3].update(state_valid=False),
        ):
            candidate = copy.deepcopy(payload)
            mutation(candidate)
            with self.assertRaises(ValueError):
                dual_add_sub_capture.validate_payload(candidate)
        encoded = json.dumps(payload)
        self.assertEqual(
            dual_add_sub_capture.parse_output(encoded)["cohort"],
            dual_add_sub_capture.COHORT,
        )
        with self.assertRaises(ValueError):
            dual_add_sub_capture.parse_output(encoded + "\nnoise\n")

    def test_comparator_requires_strict_cohort_identity_without_gate(self) -> None:
        baseline = _record(_payload())
        current = copy.deepcopy(baseline)
        current["results"][0]["elapsed_ns"] += 50_000
        current["results"][0]["elapsed_ticks"] = current["results"][0]["elapsed_ns"]
        current["results"][0]["ns_per_instruction"] = (
            current["results"][0]["elapsed_ns"] / current["results"][0]["retired_count"]
        )
        current["results"][0]["instructions_per_second"] = (
            current["results"][0]["retired_count"]
            * 1_000_000_000.0
            / current["results"][0]["elapsed_ns"]
        )
        result = dual_add_sub_compare.compare_records(baseline, current)
        self.assertEqual(result["status"], "COMPARABLE")
        self.assertEqual(result["timing_claim"], "host observation only; no TI machine-cycle claim")
        for field, value in (
            ("cohort", "other"),
            ("profile", "portable-o3-lto"),
            ("semantic_hash", "0" * 16),
            ("cpu", "other-cpu"),
            ("compiler", "other-cc"),
            ("build_flags", "-O3"),
        ):
            drifted = copy.deepcopy(current)
            drifted[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                dual_add_sub_compare.compare_records(baseline, drifted)
        drifted = copy.deepcopy(current)
        drifted["benchmark_source_sha256"] = "e" * 64
        with self.assertRaises(ValueError):
            dual_add_sub_compare.compare_records(baseline, drifted)

    def test_common_trusted_path_and_abi_guards_remain_structural(self) -> None:
        run = _source(ROOT / "src/cpu/execution/run.c")
        trusted = _source(ROOT / "src/isa/decode/trusted_execution_packet.c")
        root = _source(ROOT / "src/sequencer/instruction/memory_instruction_transaction.c")
        self.assertNotRegex(run, r"case\s+TI_C54X_OPCODE_ID_")
        self.assertNotRegex(run, r"switch\s*\([^)]*(?:opcode|instruction|id)")
        self.assertNotRegex(run, r"\bti_c54x_op_[A-Za-z0-9_]+\s*\(")
        for source in (run, trusted, root):
            self.assertIsNone(
                re.search(
                    r"\b(?:malloc|calloc|realloc|free|getenv|fopen|fclose|"
                    r"fprintf|printf|puts|syslog)\s*\(",
                    source,
                )
            )
        self.assertIn("child_kind", root)
        self.assertIn("memory_instruction_pair_dispatch_advance", root)
        self.assertIn("prepare_fast", trusted)

        probe = r"""
#include <stdint.h>
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/opcode_decode.h"
#include "isa/decode/trusted_execution_packet.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/dual_word16_read_transaction.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/dual_address_resolution.h"
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24U, "address");
#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 48U, "memory");
_Static_assert(sizeof(ti_c54x_op_effect) == 72U, "effect");
_Static_assert(sizeof(ti_c54x_dual_word16_read_transaction) == 80U, "dual");
_Static_assert(sizeof(ti_c54x_memory_instruction_transaction) == 480U, "root");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 40U, "memory");
_Static_assert(sizeof(ti_c54x_op_effect) == 52U, "effect");
_Static_assert(sizeof(ti_c54x_dual_word16_read_transaction) == 72U, "dual");
_Static_assert(sizeof(ti_c54x_memory_instruction_transaction) == 436U, "root");
#else
#error "unsupported pointer width"
#endif
int main(void) { return 0; }
"""
        result = subprocess.run(
            ["cc", "-std=c11", "-Iinclude", "-Isrc", "-x", "c", "-fsyntax-only", "-"],
            cwd=ROOT,
            input=probe,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_no_absolute_timing_threshold_or_ti_cycle_claim(self) -> None:
        capture = _source(CAPTURE_SOURCE)
        compare = _source(COMPARE_SOURCE)
        self.assertNotRegex(capture, r"\b(?:threshold|baseline|ns.?target)\b")
        self.assertIn("no TI machine-cycle claim", compare)


if __name__ == "__main__":
    unittest.main()
