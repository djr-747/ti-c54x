"""Contract tests for benchmark capture, schema validation, and history."""

# Module contract
#
# Owns: deterministic hosted-tool contract proofs.
# Does not own: CPU semantics, benchmark timing, or core build behavior.

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.perf.capture import append_history, make_record, parse_benchmark_output
from tools.perf.compare import compare, load_history, validate_record


class PerfToolsTests(unittest.TestCase):
    @staticmethod
    def _results(catalog_count: int = 6) -> list[dict[str, object]]:
        names = ["nop", "ld-dp", "ld-asm", "ld-arp", "mixed-current"]
        return [
            {
                "schema_version": 1,
                "clock_source": "CLOCK_MONOTONIC",
                "scenario": name,
                "retired_count": 800000,
                "elapsed_ns": 8000000,
                "elapsed_ticks": 8000000,
                "min_elapsed_ns": 7000000,
                "max_elapsed_ns": 9000000,
                "samples": 7,
                "warmup_windows": 16,
                "window_count": 16,
                "retirements_per_window": 50000,
                "semantic_hash": {
                    "nop": "856a941d29b43c43",
                    "ld-dp": "72a5067b3a02489f",
                    "ld-asm": "731a51f47ad7e422",
                    "ld-arp": "5d08d163159be334",
                    "mixed-current": "f59db4e45ad10a06",
                }[name],
                "scenario_instruction_count": 4 if name == "mixed-current" else 1,
                "catalog_instruction_count": catalog_count,
                "ns_per_instruction": 10.0,
                "instructions_per_second": 100000000.0,
            }
            for name in names
        ]

    @staticmethod
    def _with_coverage(results: list[dict[str, object]]) -> list[dict[str, object]]:
        for result in results:
            result["catalog_coverage_mode"] = (
                "reset-runnable-subset" if result["scenario"] == "mixed-current" else "fixed"
            )
            result["catalog_covered_instruction_count"] = (
                4 if result["scenario"] == "mixed-current" else 1
            )
        return results

    @classmethod
    def _new_results(cls) -> list[dict[str, object]]:
        return cls._with_coverage(cls._results())

    @classmethod
    def _legacy_results(cls) -> list[dict[str, object]]:
        return cls._results(catalog_count=4)

    @staticmethod
    def _make_record(results: list[dict[str, object]]) -> dict[str, object]:
        with patch("tools.perf.capture._catalog_count", return_value=6):
            return make_record(results, Path("."))

    def test_parse_and_validate_jsonl(self) -> None:
        text = "\n".join(json.dumps(item) for item in self._new_results())
        self.assertEqual(len(parse_benchmark_output(text)), 5)

    def test_parse_rejects_missing_field(self) -> None:
        with self.assertRaises(ValueError):
            parse_benchmark_output('{"schema_version":1,"scenario":"nop"}')

    def test_parse_rejects_roster_and_nonfinite_metric(self) -> None:
        mutations = (
            ("missing", lambda values: values.pop()),
            ("duplicate", lambda values: values[-1].update(scenario="nop")),
            ("boolean", lambda values: values[0].update(elapsed_ns=True)),
            ("nonfinite", lambda values: values[0].update(ns_per_instruction=float("nan"))),
        )
        for name, mutate in mutations:
            values = self._new_results()
            mutate(values)
            with self.subTest(case=name), self.assertRaises(ValueError):
                parse_benchmark_output("\n".join(json.dumps(item) for item in values))

    def test_parse_rejects_order_policy_count_and_hash(self) -> None:
        for field, value in (("min_elapsed_ns", 9000001), ("max_elapsed_ns", 7999999),
                             ("samples", 8), ("warmup_windows", 2),
                             ("window_count", 15), ("retirements_per_window", 1),
                             ("retired_count", 1), ("elapsed_ticks", 1),
                             ("ns_per_instruction", 1.0),
                             ("semantic_hash", "0000000000000000"),
                             ("scenario_instruction_count", 2)):
            values = self._new_results()
            values[0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                parse_benchmark_output("\n".join(json.dumps(item) for item in values))
        values = self._new_results()
        values[-1]["scenario_instruction_count"] = 3
        with self.assertRaises(ValueError):
            parse_benchmark_output("\n".join(json.dumps(item) for item in values))
        values = self._new_results()
        values[1]["catalog_instruction_count"] = 5
        with self.assertRaises(ValueError):
            parse_benchmark_output("\n".join(json.dumps(item) for item in values))

    def test_direct_make_record_rejects_malformed_results(self) -> None:
        values = self._new_results()
        values[0].pop("scenario_instruction_count")
        with self.assertRaises(ValueError):
            self._make_record(values)

    def test_validate_record_and_history_failures(self) -> None:
        record = self._make_record(self._new_results())
        for field in ("platform", "clock_source"):
            invalid = dict(record)
            invalid.pop(field)
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_record(invalid)
        invalid = dict(record)
        invalid["results"] = [dict(record["results"][0], catalog_instruction_count=99)] + record["results"][1:]
        with self.assertRaises(ValueError):
            validate_record(invalid)
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.jsonl"
            self.assertEqual(load_history(missing), [])
            empty = Path(directory) / "empty.jsonl"
            empty.write_text("\n", encoding="utf-8")
            self.assertEqual(load_history(empty), [])
            malformed = Path(directory) / "bad.jsonl"
            malformed.write_text("{bad\n", encoding="utf-8")
            with self.assertRaises(ValueError):
                load_history(malformed)
        values = self._new_results()
        values[0]["ns_per_instruction"] = float("nan")
        with self.assertRaises(ValueError):
            parse_benchmark_output("\n".join(json.dumps(item) for item in values))

    def test_history_append_is_jsonl(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "history.jsonl"
            record = self._make_record(self._new_results())
            record.update(
                vcs_state="clean",
                git_sha="a" * 40,
                compiler="test-cc 1.0",
                build_flags="-O2",
                binary_sha256="b" * 64,
            )
            append_history(path, record)
            append_history(path, record)
            self.assertEqual(len(path.read_text(encoding="utf-8").splitlines()), 2)
            self.assertEqual(json.loads(path.read_text().splitlines()[0]), record)

    def test_compare_only_reports_like_for_like(self) -> None:
        baseline = {"schema_version": 1, "catalog_instruction_count": 6, "cpu": "a", "platform": "p", "platform_machine": "x", "compiler": "c", "build_flags": "f", "clock_source": "CLOCK_MONOTONIC", "results": self._new_results()}
        current = {"schema_version": 1, "catalog_instruction_count": 6, "cpu": "a", "platform": "p", "platform_machine": "x", "compiler": "c", "build_flags": "f", "clock_source": "CLOCK_MONOTONIC", "results": self._new_results()}
        result = compare(baseline, current)["scenarios"]["nop"]
        self.assertTrue(result["like_for_like"])
        self.assertEqual(result["baseline_spread_percent"], 25.0)
        self.assertEqual(result["current_spread_percent"], 25.0)

    def test_compare_cohort_and_mixed_evolution(self) -> None:
        baseline = {"schema_version": 1, "catalog_instruction_count": 6, "cpu": "a", "platform": "p", "platform_machine": "x", "compiler": "c", "build_flags": "f", "clock_source": "CLOCK_MONOTONIC", "results": self._new_results()}
        current = json.loads(json.dumps(baseline))
        for item in current["results"]:
            item["catalog_instruction_count"] = 7
        current["catalog_instruction_count"] = 7
        self.assertEqual(compare(baseline, current)["scenarios"]["mixed-current"]["status"], "evolving")
        self.assertEqual(compare(baseline, current)["scenarios"]["nop"]["status"], "comparable")
        current["cpu"] = "other"
        self.assertEqual(compare(baseline, current)["scenarios"]["nop"]["status"], "different-cohort")

    def test_legacy_full_catalog_history_remains_loadable(self) -> None:
        legacy = {
            "schema_version": 1,
            "catalog_instruction_count": 4,
            "cpu": "a", "platform": "p", "platform_machine": "x",
            "compiler": "c", "build_flags": "f", "clock_source": "CLOCK_MONOTONIC",
            "vcs_state": "clean", "git_sha": "a" * 40,
            "scenario_semantic_hash": "b" * 64,
            "benchmark_source_sha256": "c" * 64, "binary_sha256": "d" * 64,
            "results": self._legacy_results(),
        }
        validate_record(legacy)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "history.jsonl"
            path.write_text(json.dumps(legacy) + "\n", encoding="utf-8")
            self.assertEqual(load_history(path), [legacy])

    def test_coverage_metadata_is_paired_and_strict(self) -> None:
        for mutate in (
            lambda values: values[0].pop("catalog_coverage_mode"),
            lambda values: values[-1].pop("catalog_covered_instruction_count"),
            lambda values: values[-1].update(catalog_coverage_mode="fixed"),
            lambda values: values[-1].update(catalog_covered_instruction_count=6),
            lambda values: values[-1].update(catalog_covered_instruction_count=7),
        ):
            values = self._new_results()
            mutate(values)
            with self.assertRaises(ValueError):
                parse_benchmark_output("\n".join(json.dumps(item) for item in values))

    def test_mixed_coverage_change_is_evolving_fixed_remains_comparable(self) -> None:
        baseline = {"schema_version": 1, "catalog_instruction_count": 6, "cpu": "a", "platform": "p", "platform_machine": "x", "compiler": "c", "build_flags": "f", "clock_source": "CLOCK_MONOTONIC", "results": self._new_results()}
        current = json.loads(json.dumps(baseline))
        current["results"][-1]["catalog_coverage_mode"] = "full-catalog"
        current["results"][-1]["catalog_covered_instruction_count"] = 6
        current["results"][-1]["scenario_instruction_count"] = 6
        self.assertEqual(compare(baseline, current)["scenarios"]["mixed-current"]["status"], "evolving")
        self.assertEqual(compare(baseline, current)["scenarios"]["nop"]["status"], "comparable")


if __name__ == "__main__":
    unittest.main()
