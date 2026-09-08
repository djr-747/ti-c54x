#!/usr/bin/env python3
"""Capture the dedicated observer-off ADD/SUB syntax-9 benchmark."""

from __future__ import annotations

import argparse
import datetime as _datetime
import hashlib
import json
import math
import platform
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

# Module contract
# Owns: strict output validation and cold capture metadata for the paired
# ADD/SUB register-only benchmark. It does not own ISA semantics, generic
# benchmark history, host timing thresholds, or CPU implementation.

SCHEMA_VERSION = 1
COHORT = "ti-c54x-add-sub-acc-shift5-observer-off-v1"
CLOCK_SOURCE = "CLOCK_MONOTONIC"
ROSTER = ("add-acc-shift5", "sub-acc-shift5")
SAMPLES = 7
WARMUPS = 16
WINDOWS = 16
RETIREMENTS_PER_WINDOW = 50_000
RETIRED = WINDOWS * RETIREMENTS_PER_WINDOW

EXPECTED_SCENARIOS: dict[str, dict[str, Any]] = {
    "add-acc-shift5": {
        "program_words": (0xF500,),
        "expected_b": 0x1234C350,
        "expected_c": 0,
    },
    "sub-acc-shift5": {
        "program_words": (0xF520,),
        "expected_b": 0x12333CB0,
        "expected_c": 1,
    },
}

_PAYLOAD_KEYS = {
    "schema_version",
    "cohort",
    "clock_source",
    "observer_off",
    "roster",
    "semantic_hash",
    "samples",
    "warmup_windows",
    "window_count",
    "retirements_per_window",
    "results",
}
_RESULT_KEYS = {
    "scenario",
    "program_words",
    "initial_a_raw40",
    "initial_b_raw40",
    "initial_sxm",
    "initial_ovm",
    "retired_count",
    "attempted_count",
    "final_pc_word",
    "program_reads",
    "final_a_raw40",
    "final_b_raw40",
    "final_c",
    "final_ova",
    "final_ovb",
    "state_valid",
    "pc_progression_valid",
    "elapsed_ns",
    "elapsed_ticks",
    "min_elapsed_ns",
    "max_elapsed_ns",
    "ns_per_instruction",
    "instructions_per_second",
}
_RECORD_KEYS = _PAYLOAD_KEYS | {
    "captured_utc",
    "git_sha",
    "vcs_state",
    "compiler",
    "platform",
    "platform_machine",
    "cpu",
    "build_flags",
    "profile",
    "benchmark_source_sha256",
    "binary_sha256",
}


def _fnv_bytes(value: int, data: bytes) -> int:
    for byte in data:
        value ^= byte
        value = (value * 1099511628211) & ((1 << 64) - 1)
    return value


def semantic_hash(results: list[dict[str, Any]]) -> str:
    value = 14695981039346656037
    for result in results:
        value = _fnv_bytes(value, result["scenario"].encode("ascii") + b"\0")
        word = result["program_words"][0]
        descriptor = bytes(
            [
                (word >> 8) & 0xFF,
                word & 0xFF,
                1,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                result["final_c"],
            ]
        )
        value = _fnv_bytes(value, descriptor)
    return f"{value:016x}"


def _positive_int(value: Any, field: str) -> None:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise ValueError(f"{field} must be a positive integer")


def _nonnegative_int(value: Any, field: str) -> None:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(f"{field} must be a non-negative integer")


def _validate_result(result: Any) -> None:
    if not isinstance(result, dict) or set(result) != _RESULT_KEYS:
        raise ValueError("ADD/SUB result has an unexpected schema")
    name = result.get("scenario")
    if not isinstance(name, str) or name not in EXPECTED_SCENARIOS:
        raise ValueError("ADD/SUB result has an unexpected scenario")
    spec = EXPECTED_SCENARIOS[name]
    if result.get("program_words") != list(spec["program_words"]):
        raise ValueError("ADD/SUB program-word roster drifted")
    if (
        result.get("initial_a_raw40") != 1
        or result.get("initial_b_raw40") != 0x12340000
        or result.get("initial_sxm") != 0
        or result.get("initial_ovm") != 0
    ):
        raise ValueError("ADD/SUB initial-state roster drifted")
    for field in (
        "retired_count",
        "attempted_count",
        "final_pc_word",
        "program_reads",
        "elapsed_ns",
        "elapsed_ticks",
        "min_elapsed_ns",
        "max_elapsed_ns",
    ):
        _positive_int(result.get(field), field)
    for field in (
        "initial_a_raw40",
        "initial_b_raw40",
        "final_a_raw40",
        "final_b_raw40",
        "final_c",
        "final_ova",
        "final_ovb",
    ):
        _nonnegative_int(result.get(field), field)
    if (
        result["retired_count"] != RETIRED
        or result["attempted_count"] != RETIRED
        or result["final_pc_word"] != RETIREMENTS_PER_WINDOW
        or result["program_reads"] != RETIRED
        or result["final_a_raw40"] != 1
        or result["final_b_raw40"] != spec["expected_b"]
        or result["final_c"] != spec["expected_c"]
        or result["final_ova"] != 0
        or result["final_ovb"] != 0
    ):
        raise ValueError("ADD/SUB architectural state/count invariant failed")
    if result["state_valid"] is not True or result["pc_progression_valid"] is not True:
        raise ValueError("ADD/SUB semantic self-check failed")
    if not (result["min_elapsed_ns"] <= result["elapsed_ns"] <= result["max_elapsed_ns"]):
        raise ValueError("ADD/SUB timing spread is invalid")
    if result["elapsed_ticks"] != result["elapsed_ns"]:
        raise ValueError("ADD/SUB clock arithmetic drifted")
    for field in ("ns_per_instruction", "instructions_per_second"):
        value = result.get(field)
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError(f"{field} must be numeric")
        if not math.isfinite(value) or value <= 0.0:
            raise ValueError(f"{field} must be finite and positive")
    expected_ns = result["elapsed_ns"] / RETIRED
    expected_ips = RETIRED * 1_000_000_000.0 / result["elapsed_ns"]
    if abs(result["ns_per_instruction"] - expected_ns) > max(0.01, expected_ns * 1e-6):
        raise ValueError("ADD/SUB ns-per-instruction arithmetic failed")
    if abs(result["instructions_per_second"] - expected_ips) > max(0.01, expected_ips * 1e-6):
        raise ValueError("ADD/SUB throughput arithmetic failed")


def validate_payload(payload: Any) -> dict[str, Any]:
    if not isinstance(payload, dict) or set(payload) != _PAYLOAD_KEYS:
        raise ValueError("ADD/SUB benchmark output has an unexpected schema")
    if payload["schema_version"] != SCHEMA_VERSION or payload["cohort"] != COHORT:
        raise ValueError("ADD/SUB benchmark schema/cohort mismatch")
    if payload["clock_source"] != CLOCK_SOURCE or payload["observer_off"] is not True:
        raise ValueError("ADD/SUB benchmark must use CLOCK_MONOTONIC with observers off")
    if payload["roster"] != list(ROSTER):
        raise ValueError("ADD/SUB benchmark roster mismatch")
    for field, expected in (
        ("samples", SAMPLES),
        ("warmup_windows", WARMUPS),
        ("window_count", WINDOWS),
        ("retirements_per_window", RETIREMENTS_PER_WINDOW),
    ):
        if payload[field] != expected:
            raise ValueError(f"ADD/SUB benchmark policy changed: {field}")
    results = payload["results"]
    if not isinstance(results, list) or [item.get("scenario") for item in results] != list(ROSTER):
        raise ValueError("ADD/SUB benchmark must emit the exact ordered roster")
    for result in results:
        _validate_result(result)
    if payload["semantic_hash"] != semantic_hash(results):
        raise ValueError("ADD/SUB semantic hash mismatch")
    if not isinstance(payload["semantic_hash"], str) or len(payload["semantic_hash"]) != 16:
        raise ValueError("ADD/SUB semantic hash must be 16 hexadecimal characters")
    try:
        int(payload["semantic_hash"], 16)
    except (TypeError, ValueError) as exc:
        raise ValueError("ADD/SUB semantic hash must be hexadecimal") from exc
    return payload


def parse_output(text: str) -> dict[str, Any]:
    if not text.strip():
        raise ValueError("ADD/SUB benchmark emitted no JSON")
    try:
        payload = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ValueError(f"ADD/SUB benchmark output is not one JSON object: {exc}") from exc
    return validate_payload(payload)


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def _git_metadata(root: Path) -> tuple[str, str]:
    try:
        sha = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True, stderr=subprocess.DEVNULL
        ).strip()
        dirty = bool(
            subprocess.check_output(
                ["git", "status", "--porcelain"], cwd=root, text=True, stderr=subprocess.DEVNULL
            )
        )
        return sha, "dirty" if dirty else "clean"
    except (OSError, subprocess.CalledProcessError):
        return "unknown", "dirty"


def _cpu_name() -> str:
    try:
        for line in Path("/proc/cpuinfo").read_text(encoding="utf-8").splitlines():
            if line.lower().startswith(("model name", "hardware")):
                return line.split(":", 1)[-1].strip()
    except OSError:
        pass
    return platform.processor() or "unknown"


def make_record(
    payload: dict[str, Any], root: Path, compiler: str, build_flags: str,
    profile: str, binary: Path,
) -> dict[str, Any]:
    validate_payload(payload)
    sha, vcs_state = _git_metadata(root)
    words = shlex.split(compiler)
    compiler_path = shutil.which(words[0]) if words else None
    compiler_version = "unknown"
    if compiler_path is not None:
        try:
            compiler_version = subprocess.check_output(
                words + ["--version"], text=True, stderr=subprocess.STDOUT
            ).splitlines()[0]
        except (OSError, subprocess.CalledProcessError, IndexError):
            pass
    record = dict(payload)
    record.update(
        captured_utc=_datetime.datetime.now(_datetime.timezone.utc).isoformat(),
        git_sha=sha,
        vcs_state=vcs_state,
        compiler=compiler_version,
        platform=platform.platform(),
        platform_machine=platform.machine(),
        cpu=_cpu_name(),
        build_flags=build_flags or "unknown",
        profile=profile,
        benchmark_source_sha256=_sha256(root / "bench/ti_c54x_add_sub_bench.c"),
        binary_sha256=_sha256(binary),
    )
    validate_record(record)
    return record


def validate_record(record: Any) -> dict[str, Any]:
    if not isinstance(record, dict) or set(record) != _RECORD_KEYS:
        raise ValueError("ADD/SUB capture record has an unexpected schema")
    validate_payload({key: record[key] for key in _PAYLOAD_KEYS})
    if record["vcs_state"] not in {"clean", "dirty"}:
        raise ValueError("ADD/SUB capture VCS state is invalid")
    for field in ("profile", "compiler", "platform", "platform_machine", "cpu", "build_flags"):
        value = record.get(field)
        if not isinstance(value, str) or not value.strip() or value.lower() == "unknown":
            raise ValueError(f"ADD/SUB capture metadata missing: {field}")
    sha = record.get("git_sha")
    if not isinstance(sha, str) or len(sha) < 40 or any(char not in "0123456789abcdef" for char in sha):
        raise ValueError("ADD/SUB capture metadata invalid: git_sha")
    for field in ("benchmark_source_sha256", "binary_sha256"):
        value = record.get(field)
        if not isinstance(value, str) or len(value) != 64:
            raise ValueError(f"ADD/SUB capture hash invalid: {field}")
        try:
            int(value, 16)
        except ValueError as exc:
            raise ValueError(f"ADD/SUB capture hash invalid: {field}") from exc
    if not isinstance(record["captured_utc"], str) or not record["captured_utc"].strip():
        raise ValueError("ADD/SUB capture timestamp missing")
    return record


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", type=Path, default=Path("build-bench/ti_c54x_add_sub_bench"))
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--allow-dirty", action="store_true")
    parser.add_argument("--compiler", default="cc")
    parser.add_argument("--build-flags", default="")
    parser.add_argument("--profile", default="portable-o2-no-lto")
    args = parser.parse_args(argv)
    try:
        completed = subprocess.run(
            [str(args.benchmark)], check=True, capture_output=True, text=True
        )
        payload = parse_output(completed.stdout)
        record = make_record(
            payload, args.root.resolve(), args.compiler, args.build_flags,
            args.profile, args.benchmark,
        )
        if record["vcs_state"] != "clean" and not args.allow_dirty:
            raise ValueError("clean VCS state required; use --allow-dirty for ad-hoc output")
        print(json.dumps(record, sort_keys=True, separators=(",", ":")))
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"ADD/SUB perf capture failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
