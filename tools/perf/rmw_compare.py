#!/usr/bin/env python3
"""Compare two like-for-like observer-off atomic-logical captures."""

# Module contract
# Owns: same-cohort W46B capture validation and advisory per-scenario deltas.
# It does not own: benchmark execution, performance thresholds, history
# mutation, or TI machine-cycle/timing claims.

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

try:
    from tools.perf.rmw_capture import validate_record
except ModuleNotFoundError:
    from rmw_capture import validate_record


def _spread_percent(result: dict[str, Any]) -> float:
    median = result["elapsed_ns"]
    if median <= 0:
        raise ValueError("RMW elapsed time must be positive")
    return (result["max_elapsed_ns"] - result["min_elapsed_ns"]) * 100.0 / median


def compare_records(baseline: dict[str, Any], current: dict[str, Any]) -> dict[str, Any]:
    """Require exact semantic/cohort identity and return deltas without a gate."""

    validate_record(baseline)
    validate_record(current)
    if baseline["vcs_state"] != "clean":
        raise ValueError("RMW baseline must have clean VCS state")
    for field in ("cohort", "profile", "observer_off", "roster", "semantic_hash"):
        if baseline[field] != current[field]:
            raise ValueError(f"RMW capture drift in {field}")
    if baseline["benchmark_source_sha256"] != current["benchmark_source_sha256"]:
        raise ValueError("RMW benchmark source drifted")
    for field in ("platform", "platform_machine", "cpu", "compiler", "build_flags"):
        if baseline[field] != current[field]:
            raise ValueError(f"RMW capture cohort drift in {field}")
    before = {result["scenario"]: result for result in baseline["results"]}
    after = {result["scenario"]: result for result in current["results"]}
    if list(before) != list(after):
        raise ValueError("RMW capture scenario roster drifted")
    scenarios: dict[str, dict[str, Any]] = {}
    for name in before:
        old = before[name]
        new = after[name]
        if old["program_words"] != new["program_words"] or old["arf"] != new["arf"] or old["mode"] != new["mode"]:
            raise ValueError(f"RMW capture semantic drift in {name}")
        old_ns = old["ns_per_instruction"]
        new_ns = new["ns_per_instruction"]
        scenarios[name] = {
            "status": "COMPARABLE",
            "baseline_ns_per_instruction": old_ns,
            "current_ns_per_instruction": new_ns,
            "percent_change": (new_ns - old_ns) * 100.0 / old_ns,
            "baseline_spread_percent": _spread_percent(old),
            "current_spread_percent": _spread_percent(new),
        }
    return {
        "status": "COMPARABLE",
        "same_cohort": True,
        "cohort": baseline["cohort"],
        "profile": baseline["profile"],
        "observer_off": True,
        "semantic_hash": baseline["semantic_hash"],
        "scenarios": scenarios,
        "timing_claim": "host observation only; no TI machine-cycle claim",
    }


def _load(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"malformed RMW capture JSON: {path}") from exc
    if not isinstance(value, dict):
        raise ValueError("RMW capture must be a JSON object")
    return value


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--current-file", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        baseline = _load(args.baseline)
        current = json.load(sys.stdin) if str(args.current_file) == "-" else _load(args.current_file)
        if not isinstance(current, dict):
            raise ValueError("RMW current capture must be a JSON object")
        print(json.dumps(compare_records(baseline, current), sort_keys=True, separators=(",", ":")))
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"RMW perf compare failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
