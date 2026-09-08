"""Contract tests for the dedicated W46B observer-off RMW benchmark lane.

Module contract: owns deterministic proofs for the private benchmark's
schema, roster, semantic-hash, comparator, and build wiring.  It does not
claim TI machine timing, mutate benchmark history, or validate production
opcode semantics.
"""

from __future__ import annotations

import copy
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from tools.perf import rmw_capture, rmw_compare


ROOT = Path(__file__).resolve().parents[4]
BENCH_SOURCE = ROOT / "bench" / "ti_c54x_rmw_bench.c"
AUDIT_PATH = ROOT / "bench" / "audits" / "p2-w46b-rmw-baseline.json"


class AtomicLogicalRmwBenchmarkTests(unittest.TestCase):
    """Exercise the executable benchmark's cold validation policy."""

    @classmethod
    def _result(cls, scenario: str, elapsed_ns: int = 8_000_000) -> dict[str, object]:
        spec = rmw_capture.EXPECTED_SCENARIOS[scenario]
        retired = rmw_capture.WINDOWS * rmw_capture.RETIREMENTS_PER_WINDOW
        return {
            "scenario": scenario,
            "program_words": list(spec["program_words"]),
            "arf": spec["arf"],
            "mode": spec["mode"],
            "operation": spec["operation"],
            "data_address_word": 0,
            "initial_data_word": 0xA55A,
            "retired_count": retired,
            "elapsed_ns": elapsed_ns,
            "elapsed_ticks": elapsed_ns,
            "min_elapsed_ns": elapsed_ns - 100_000,
            "max_elapsed_ns": elapsed_ns + 100_000,
            "endpoint_read_count": retired,
            "endpoint_write_count": retired,
            "same_address": True,
            "fresh_transaction_ids": True,
            "phase_order_valid": True,
            "transform_valid": True,
            "ns_per_instruction": elapsed_ns / retired,
            "instructions_per_second": retired * 1_000_000_000.0 / elapsed_ns,
        }

    @classmethod
    def _payload(cls) -> dict[str, object]:
        results = [cls._result(name) for name in rmw_capture.ROSTER]
        payload: dict[str, object] = {
            "schema_version": rmw_capture.SCHEMA_VERSION,
            "cohort": rmw_capture.COHORT,
            "clock_source": "CLOCK_MONOTONIC",
            "observer_off": True,
            "roster": list(rmw_capture.ROSTER),
            "semantic_hash": rmw_capture.semantic_hash(results),
            "samples": rmw_capture.SAMPLES,
            "warmup_windows": rmw_capture.WARMUPS,
            "window_count": rmw_capture.WINDOWS,
            "retirements_per_window": rmw_capture.RETIREMENTS_PER_WINDOW,
            "results": results,
        }
        return payload

    @staticmethod
    def _record(payload: dict[str, object], profile: str = "portable-o2-no-lto") -> dict[str, object]:
        record = copy.deepcopy(payload)
        record.update(
            captured_utc="2026-09-08T00:00:00+00:00",
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

    def test_build_wiring_and_cold_benchmark_isolated(self) -> None:
        make_text = (ROOT / "Makefile").read_text(encoding="utf-8")
        cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("bench-rmw:", make_text)
        self.assertIn("bench-rmw-smoke:", make_text)
        self.assertIn("bench-rmw-compare:", make_text)
        self.assertIn("ti_c54x_rmw_bench", make_text)
        self.assertIn("tools/perf/rmw_capture.py", make_text)
        self.assertIn("tools/perf/rmw_compare.py", make_text)
        self.assertIn("ti_c54x_rmw_bench", cmake_text)
        self.assertIn("bench_rmw_smoke", cmake_text)
        self.assertIn("TI_C54X_BUILD_BENCHMARKS", cmake_text)
        self.assertTrue(BENCH_SOURCE.is_file())
        self.assertNotIn("ti_c54x_bench.c", BENCH_SOURCE.read_text(encoding="utf-8"))
        old_bench = (ROOT / "bench/ti_c54x_bench.c").read_text(encoding="utf-8")
        old_capture = (ROOT / "tools/perf/capture.py").read_text(encoding="utf-8")
        old_compare = (ROOT / "tools/perf/compare.py").read_text(encoding="utf-8")
        old_history = (ROOT / "bench/history/hosted-v1.jsonl").read_text(encoding="utf-8")
        self.assertIn('"nop"', old_bench)
        self.assertIn('"mixed-current"', old_bench)
        self.assertIn("FIXED_SCENARIOS", old_capture)
        self.assertIn("hosted benchmark", old_compare)
        self.assertTrue(old_history.strip())
        self.assertNotIn("hosted-v1.jsonl", make_text.split("bench-rmw:", 1)[1].split("bench-rmw-smoke:", 1)[0])
        self.assertNotIn("tools/perf/capture.py", make_text.split("bench-rmw:", 1)[1].split("bench-rmw-smoke:", 1)[0])

    def test_benchmark_rejects_observer_on_or_undefined_compile(self) -> None:
        source = BENCH_SOURCE.read_text(encoding="utf-8")
        self.assertIn("TI_C54X_ENABLE_OBSERVERS=0", source)
        self.assertIn("requires TI_C54X_ENABLE_OBSERVERS=0", source)
        common = ["cc", "-std=c11", "-Iinclude", "-Isrc", "-fsyntax-only", str(BENCH_SOURCE)]
        observer_on = subprocess.run(
            [*common, "-DTI_C54X_ENABLE_OBSERVERS=1"],
            cwd=ROOT, capture_output=True, text=True,
        )
        observer_off = subprocess.run(
            [*common, "-DTI_C54X_ENABLE_OBSERVERS=0"],
            cwd=ROOT, capture_output=True, text=True,
        )
        observer_undefined = subprocess.run(
            common, cwd=ROOT, capture_output=True, text=True,
        )
        self.assertNotEqual(observer_on.returncode, 0)
        self.assertNotEqual(observer_undefined.returncode, 0)
        self.assertEqual(observer_off.returncode, 0, observer_off.stderr)

    def test_endpoint_rejects_reused_transaction_id_executable(self) -> None:
        harness = f'''
#define main ti_c54x_rmw_bench_main
#include "{BENCH_SOURCE}"
#undef main
int main(void) {{
    static const rmw_scenario scenario = {{"andm-mod0", {{0x6880, 0x0F3C}}, RMW_AND}};
    rmw_bus_context bus = {{0}};
    ti_c54x_bus_word16_observation observation = {{0}};
    const ti_c54x_bus_word16_request read = {{
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, 0U, 0U}};
    const ti_c54x_bus_word16_request write = {{
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, 0U, 0x0F3C}};
    rmw_context_reset(&bus, &scenario);
    rmw_data_advance(&bus, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, 7U,
                     &read, &observation);
    rmw_data_advance(&bus, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, 7U,
                     &write, &observation);
    return bus.fresh_ids ? 1 : 0;
}}
'''
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "reused_id.c"
            binary = Path(directory) / "reused_id"
            source.write_text(harness, encoding="utf-8")
            result = subprocess.run(
                [
                    "cc", "-std=c11", "-O0", "-Iinclude", "-Isrc",
                    "-DTI_C54X_ENABLE_OBSERVERS=0", "-ffunction-sections",
                    "-fdata-sections", str(source), "-Wl,--gc-sections",
                    "-o", str(binary),
                ],
                cwd=ROOT, capture_output=True, text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            executed = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(executed.returncode, 0, executed.stderr)

    def test_exact_roster_and_semantic_hash(self) -> None:
        payload = self._payload()
        self.assertEqual(rmw_capture.validate_payload(payload), payload)
        self.assertEqual(payload["roster"], ["andm-mod0", "orm-mod0", "xorm-mod0"])
        self.assertEqual(len(payload["semantic_hash"]), 16)
        mutated = copy.deepcopy(payload)
        mutated["results"][1]["program_words"][1] ^= 1
        with self.assertRaises(ValueError):
            rmw_capture.validate_payload(mutated)

    def test_parser_rejects_json_noise_roster_and_metrics(self) -> None:
        payload = self._payload()
        self.assertEqual(rmw_capture.parse_output(json.dumps(payload))["cohort"], rmw_capture.COHORT)
        for mutation in (
            lambda value: value.update(roster=["andm-mod0"]),
            lambda value: value.update(observer_off=False),
            lambda value: value["results"][0].update(endpoint_read_count=0),
            lambda value: value["results"][0].update(ns_per_instruction=0.0),
            lambda value: value["results"][0].update(max_elapsed_ns=1),
        ):
            candidate = copy.deepcopy(payload)
            mutation(candidate)
            with self.assertRaises(ValueError):
                rmw_capture.validate_payload(candidate)
        with self.assertRaises(ValueError):
            rmw_capture.parse_output(json.dumps(payload) + "\nnot-json\n")

    def test_comparator_requires_cohort_profile_hash_and_roster(self) -> None:
        baseline = self._record(self._payload())
        current = copy.deepcopy(baseline)
        current["results"][0]["elapsed_ns"] += 50_000
        current["results"][0]["elapsed_ticks"] = current["results"][0]["elapsed_ns"]
        current["results"][0]["ns_per_instruction"] = current["results"][0]["elapsed_ns"] / current["results"][0]["retired_count"]
        current["results"][0]["instructions_per_second"] = current["results"][0]["retired_count"] * 1_000_000_000.0 / current["results"][0]["elapsed_ns"]
        result = rmw_compare.compare_records(baseline, current)
        self.assertEqual(result["status"], "COMPARABLE")
        self.assertIn("percent_change", result["scenarios"]["andm-mod0"])
        current["binary_sha256"] = "d" * 64
        self.assertEqual(rmw_compare.compare_records(baseline, current)["status"], "COMPARABLE")
        for field, value in (
            ("cohort", "other"), ("profile", "portable-o3-lto"),
            ("semantic_hash", "0" * 16), ("cpu", "other-cpu"),
            ("compiler", "other-cc"), ("build_flags", "-O3"),
        ):
            drifted = copy.deepcopy(current)
            if field == "semantic_hash":
                drifted[field] = value
            else:
                drifted[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                rmw_compare.compare_records(baseline, drifted)
        drifted = copy.deepcopy(current)
        drifted["benchmark_source_sha256"] = "e" * 64
        with self.assertRaises(ValueError):
            rmw_compare.compare_records(baseline, drifted)
        dirty_baseline = copy.deepcopy(baseline)
        dirty_baseline["vcs_state"] = "dirty"
        with self.assertRaises(ValueError):
            rmw_compare.compare_records(dirty_baseline, current)
        drifted = copy.deepcopy(current)
        drifted["roster"] = list(reversed(drifted["roster"]))
        with self.assertRaises(ValueError):
            rmw_compare.compare_records(baseline, drifted)

    def test_baseline_is_clean_if_present(self) -> None:
        if not AUDIT_PATH.exists():
            self.skipTest("no honest local W46B baseline captured yet")
        record = json.loads(AUDIT_PATH.read_text(encoding="utf-8"))
        rmw_capture.validate_record(record)
        self.assertEqual(record["vcs_state"], "clean")
        self.assertEqual(record["cohort"], rmw_capture.COHORT)


if __name__ == "__main__":
    unittest.main()
