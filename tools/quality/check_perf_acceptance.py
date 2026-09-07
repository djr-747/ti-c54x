#!/usr/bin/env python3
"""Validate the P2-W31P audit and stable hosted capture sets.

Module contract: owns deterministic schema/profile/stability/threshold checks
for the hosted release-performance contract. It does not run benchmarks,
mutate history, inspect production runtime state, claim TI timing, read
environment configuration, or impose a cross-host CI throughput threshold.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from pathlib import Path
from typing import Any

try:
    from tools.perf.capture import FIXED_SEMANTIC_HASHES, validate_persisted_record
except ModuleNotFoundError:
    # Keep direct execution usable from this sibling tools/quality directory,
    # matching the standalone tools/perf scripts' import fallback.
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "perf"))
    from capture import FIXED_SEMANTIC_HASHES, validate_persisted_record

PROFILE_O2 = "portable-o2-no-lto"
PROFILE_O3_LTO = "portable-o3-lto"
PROFILES = (PROFILE_O2, PROFILE_O3_LTO)
AUDIT_SCENARIOS = tuple(FIXED_SEMANTIC_HASHES) + ("mixed-current",)
_GIT_SHA_PATTERN = re.compile(r"[0-9a-f]{40,64}")
_SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
_SEMANTIC_HASH_PATTERN = re.compile(r"[0-9a-f]{16}")


def _object(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    return value


def _text(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{label} must be a non-empty string")
    return value


def _hash(value: Any, label: str, pattern: re.Pattern[str]) -> str:
    if not isinstance(value, str) or pattern.fullmatch(value) is None:
        raise ValueError(f"{label} has an invalid hash shape")
    return value


def _positive_integer(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise ValueError(f"{label} must be a positive integer")
    return value


def load_json_object(path: Path) -> dict[str, Any]:
    try:
        return _object(json.loads(path.read_text(encoding="utf-8")), str(path))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot load {path}: {exc}") from exc


def validate_audit(audit: Any) -> dict[str, Any]:
    value = _object(audit, "audit")
    if value.get("schema_version") != 1:
        raise ValueError("audit schema_version must be 1")
    _text(value.get("audit_id"), "audit_id")
    _object(value.get("module_contract"), "module_contract")
    source = _object(value.get("source"), "source")
    _hash(source.get("git_sha"), "source git_sha", _GIT_SHA_PATTERN)
    _text(source.get("production_source_tree"), "source production_source_tree")
    _positive_integer(source.get("catalog_instruction_count"),
                      "source catalog_instruction_count")
    _hash(source.get("benchmark_source_sha256"),
          "source benchmark_source_sha256", _SHA256_PATTERN)
    _hash(source.get("scenario_semantic_hash"),
          "source scenario_semantic_hash", _SHA256_PATTERN)
    host = _object(value.get("host"), "host")
    for field in ("compiler", "platform", "platform_machine", "cpu", "affinity"):
        _text(host.get(field), f"host {field}")
    policy = _object(value.get("acceptance_policy"), "acceptance_policy")
    if policy.get("target_ns_per_instruction_exclusive") != 60.0:
        raise ValueError("acceptance target must be exactly 60 ns exclusive")
    if policy.get("target_scenarios") != list(FIXED_SEMANTIC_HASHES):
        raise ValueError("acceptance target scenarios changed")
    if policy.get("profiles") != list(PROFILES):
        raise ValueError("acceptance profiles changed")
    if policy.get("capture_set_size") != 3:
        raise ValueError("capture set size must be exactly three")
    if policy.get("same_binary_and_scenario_hashes_required") is not True:
        raise ValueError("stable sets must require matching hashes")
    if policy.get("max_within_capture_range_over_median") != 0.15:
        raise ValueError("stability ratio must be exactly 0.15")

    profiles = value.get("profiles")
    if not isinstance(profiles, list) or len(profiles) < 3:
        raise ValueError("baseline audit must retain its three trigger captures")
    seen_kinds: set[str] = set()
    seen_profile_ids: set[str] = set()
    for profile in profiles:
        item = _object(profile, "audit profile")
        profile_id = _text(item.get("id"), "audit profile id")
        if profile_id in seen_profile_ids:
            raise ValueError("audit profile identities must be unique")
        seen_profile_ids.add(profile_id)
        _text(item.get("captured_utc"), "audit profile captured_utc")
        _text(item.get("source_vcs_state"), "audit profile source_vcs_state")
        _text(item.get("command"), "audit profile command")
        flags = _text(item.get("build_flags"), "audit profile build_flags")
        _hash(item.get("binary_sha256"), "audit profile binary_sha256",
              _SHA256_PATTERN)
        results = item.get("results")
        kind = _profile_for_flags(flags)
        seen_kinds.add(kind)
        if not isinstance(results, list):
            raise ValueError("audit profile results must be a list")
        scenarios: list[str] = []
        by_name: dict[str, dict[str, Any]] = {}
        for result_value in results:
            result = _object(result_value, "audit profile result")
            scenario = _text(result.get("scenario"), "audit scenario")
            if scenario in by_name:
                raise ValueError("audit profile scenario roster contains duplicates")
            scenarios.append(scenario)
            by_name[scenario] = result
        if scenarios != list(AUDIT_SCENARIOS):
            raise ValueError("audit profile scenario roster changed")
        for scenario, result in by_name.items():
            _hash(result.get("semantic_hash"), f"audit {scenario} semantic_hash",
                  _SEMANTIC_HASH_PATTERN)
            retired_count = _positive_integer(
                result.get("retired_count"), f"audit {scenario} retired_count")
            for field in ("min_elapsed_ns", "median_elapsed_ns", "max_elapsed_ns"):
                metric = result.get(field)
                if isinstance(metric, bool) or not isinstance(metric, int) or metric <= 0:
                    raise ValueError(f"audit {scenario} {field} is invalid")
            ns_per_instruction = result.get("ns_per_instruction")
            if (isinstance(ns_per_instruction, bool) or
                    not isinstance(ns_per_instruction, (int, float)) or
                    not math.isfinite(ns_per_instruction) or ns_per_instruction <= 0):
                raise ValueError(f"audit {scenario} ns_per_instruction is invalid")
            if not (result["min_elapsed_ns"] <= result["median_elapsed_ns"] <=
                    result["max_elapsed_ns"]):
                raise ValueError(f"audit {scenario} elapsed range is invalid")
            expected_ns_per_instruction = result["median_elapsed_ns"] / retired_count
            if abs(ns_per_instruction - expected_ns_per_instruction) > 0.0005:
                raise ValueError(f"audit {scenario} ns_per_instruction is incoherent")
            if scenario in FIXED_SEMANTIC_HASHES and result.get(
                    "semantic_hash") != FIXED_SEMANTIC_HASHES[scenario]:
                raise ValueError(f"audit {scenario} semantic hash changed")
    if seen_kinds != set(PROFILES):
        raise ValueError("audit must contain both frozen build profiles")
    return policy


def _profile_for_flags(flags: str) -> str:
    tokens = flags.split()
    if "-O2" in tokens and "-O3" not in tokens and "-flto" not in tokens:
        return PROFILE_O2
    if "-O3" in tokens and "-flto" in tokens and "-O2" not in tokens:
        return PROFILE_O3_LTO
    raise ValueError("capture flags do not identify a frozen profile")


def evaluate_capture_set(
    captures: list[dict[str, Any]],
    expected_profile: str,
    policy: dict[str, Any],
) -> dict[str, Any]:
    if expected_profile not in PROFILES:
        raise ValueError("unknown performance profile")
    if len(captures) != policy["capture_set_size"]:
        raise ValueError("capture set must contain exactly three records")
    for capture in captures:
        validate_persisted_record(capture)
        if _profile_for_flags(capture["build_flags"]) != expected_profile:
            raise ValueError("capture build profile mismatch")

    cohort_fields = ("binary_sha256", "scenario_semantic_hash", "compiler",
                     "platform", "platform_machine", "cpu", "build_flags",
                     "clock_source")
    for field in cohort_fields:
        if len({capture[field] for capture in captures}) != 1:
            raise ValueError(f"capture set cohort differs: {field}")
    scenario_hashes = [
        tuple((result["scenario"], result["semantic_hash"])
              for result in capture["results"])
        for capture in captures
    ]
    if len(set(scenario_hashes)) != 1:
        raise ValueError("capture set scenario hashes differ")

    target = policy["target_ns_per_instruction_exclusive"]
    max_ratio = policy["max_within_capture_range_over_median"]
    target_scenarios = policy["target_scenarios"]
    failures: list[str] = []
    for capture_index, capture in enumerate(captures, 1):
        by_name = {result["scenario"]: result for result in capture["results"]}
        for scenario in target_scenarios:
            result = by_name[scenario]
            ratio = ((result["max_elapsed_ns"] - result["min_elapsed_ns"]) /
                     result["elapsed_ns"])
            if result["ns_per_instruction"] >= target:
                failures.append(
                    f"capture {capture_index} {scenario} is not below {target} ns")
            if ratio > max_ratio:
                failures.append(
                    f"capture {capture_index} {scenario} stability ratio {ratio:.6f} exceeds {max_ratio}")
    return {
        "status": "ACCEPTED" if not failures else "INCONCLUSIVE",
        "profile": expected_profile,
        "capture_count": len(captures),
        "failures": failures,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path,
                        default=Path("bench/audits/p2-w31p-baseline.json"))
    parser.add_argument("--profile", choices=PROFILES, required=True)
    parser.add_argument("--capture", type=Path, action="append", required=True)
    args = parser.parse_args(argv)
    try:
        policy = validate_audit(load_json_object(args.audit))
        captures = [load_json_object(path) for path in args.capture]
        result = evaluate_capture_set(captures, args.profile, policy)
        print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        return 0 if result["status"] == "ACCEPTED" else 1
    except ValueError as exc:
        print(f"performance acceptance: ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
