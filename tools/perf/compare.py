#!/usr/bin/env python3
"""Advisory same-cohort comparison for hosted benchmark records."""

# Module contract
# Owns: append-only history selection and advisory like-for-like comparison.
# Does not own: thresholds, CI policy, benchmark execution, or record mutation.

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

try:
    from tools.perf.capture import SCHEMA_VERSION, validate_persisted_record, validate_results
except ModuleNotFoundError:
    from capture import SCHEMA_VERSION, validate_persisted_record, validate_results


def compare(baseline: dict[str, Any], current: dict[str, Any]) -> dict[str, Any]:
    validate_record(baseline)
    validate_record(current)
    cohort_fields = ("cpu", "platform", "platform_machine", "compiler", "build_flags", "clock_source")
    same_cohort = all(baseline.get(field) == current.get(field) for field in cohort_fields)
    before = {item["scenario"]: item for item in baseline["results"]}
    after = {item["scenario"]: item for item in current["results"]}
    scenarios = {}
    for name in sorted(set(before) & set(after)):
        if (before[name]["semantic_hash"] != after[name]["semantic_hash"] or
                (name == "mixed-current" and before[name]["catalog_instruction_count"] != after[name]["catalog_instruction_count"])):
            scenarios[name] = {"status": "evolving", "like_for_like": False}
        elif not same_cohort:
            scenarios[name] = {"status": "different-cohort", "like_for_like": False}
        else:
            base = before[name]["ns_per_instruction"]
            now = after[name]["ns_per_instruction"]
            if base <= 0:
                scenarios[name] = {"status": "invalid-baseline", "like_for_like": False}
                continue
            scenarios[name] = {
                "status": "comparable",
                "like_for_like": True,
                "percent_change": (now - base) * 100.0 / base,
                "baseline_spread_percent": _spread_percent(before[name]),
                "current_spread_percent": _spread_percent(after[name]),
            }
    return {"same_cohort": same_cohort, "cohort_fields": cohort_fields, "scenarios": scenarios}


def _spread_percent(result: dict[str, Any]) -> float:
    """Return within-capture spread as a percent of the median metric."""
    median = result["elapsed_ns"]
    if median <= 0:
        raise ValueError("median elapsed time must be positive")
    return (result["max_elapsed_ns"] - result["min_elapsed_ns"]) * 100.0 / median


def validate_record(record: dict[str, Any]) -> None:
    if not isinstance(record, dict) or not isinstance(record.get("results"), list):
        raise ValueError("malformed benchmark record")
    if record.get("schema_version") != SCHEMA_VERSION or record.get("clock_source") != "CLOCK_MONOTONIC":
        raise ValueError("invalid record schema or clock")
    catalog_count = record.get("catalog_instruction_count")
    if not isinstance(catalog_count, int) or isinstance(catalog_count, bool) or catalog_count <= 0:
        raise ValueError("invalid record catalog count")
    validate_results(record["results"], catalog_count)
    for field in ("cpu", "platform", "platform_machine", "compiler", "build_flags", "clock_source"):
        value = record.get(field)
        if not isinstance(value, str) or not value.strip() or value.lower() == "unknown":
            raise ValueError(f"missing cohort field: {field}")


def load_history(path: Path) -> list[dict[str, Any]]:
    records = []
    if not path.exists():
        return records
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            try:
                record = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError("malformed benchmark history JSON") from exc
            validate_persisted_record(record)
            records.append(record)
    return records


def _run() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("baseline", type=Path, nargs="?")
    parser.add_argument("current", type=Path, nargs="?")
    parser.add_argument("--history", type=Path)
    parser.add_argument("--current-file", type=Path)
    args = parser.parse_args()
    if args.history:
        history = load_history(args.history)
        using_latest_history = args.current_file is None
        if args.current_file and str(args.current_file) != "-":
            current = json.loads(args.current_file.read_text(encoding="utf-8"))
        elif args.current_file and str(args.current_file) == "-":
            current = json.load(sys.stdin)
        else:
            if not history:
                print(json.dumps({"status": "no-history", "same_cohort": False}))
                return 0
            current = history[-1]
        validate_record(current)
        if not history:
            print(json.dumps({"status": "no-history", "same_cohort": False}))
            return 0
        cohort = {field: current.get(field) for field in ("cpu", "platform", "platform_machine", "compiler", "build_flags", "clock_source")}
        candidate_records = history[:-1] if using_latest_history else history
        candidates = [record for record in candidate_records if all(record.get(k) == v for k, v in cohort.items())]
        if not candidates:
            print(json.dumps({"status": "no-matching-baseline", "same_cohort": False}))
            return 0
        baseline = candidates[-1]
    else:
        if args.baseline is None or args.current is None:
            raise ValueError("baseline and current records are required")
        baseline = json.loads(args.baseline.read_text(encoding="utf-8"))
        current = json.loads(args.current.read_text(encoding="utf-8"))
    print(json.dumps(compare(baseline, current), sort_keys=True))
    return 0


def main() -> int:
    try:
        return _run()
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"perf compare failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
