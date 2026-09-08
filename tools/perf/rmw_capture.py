#!/usr/bin/env python3
"""Capture the dedicated observer-off WORD16 atomic-logical benchmark."""

# Module contract
# Owns: strict W46B benchmark-output validation, cold capture metadata, and
# JSON serialization for an ad-hoc or frozen observer-off RMW cohort.  It does
# not own: the legacy benchmark schema/history, timing thresholds, or CPU
# semantics.

from __future__ import annotations

import argparse
import datetime as _datetime
import hashlib
import json
import math
import os
import platform
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
COHORT = "ti-c54x-rmw-observer-off-v1"
CLOCK_SOURCE = "CLOCK_MONOTONIC"
ROSTER = ("andm-mod0", "orm-mod0", "xorm-mod0")
SAMPLES = 7
WARMUPS = 16
WINDOWS = 16
RETIREMENTS_PER_WINDOW = 20_000

EXPECTED_SCENARIOS: dict[str, dict[str, Any]] = {
    "andm-mod0": {"program_words": (0x6880, 0x0F3C), "arf": 0, "mode": 0, "operation": 0},
    "orm-mod0": {"program_words": (0x6980, 0x0F3C), "arf": 0, "mode": 0, "operation": 1},
    "xorm-mod0": {"program_words": (0x6A80, 0x0F3C), "arf": 0, "mode": 0, "operation": 2},
}

_PAYLOAD_KEYS = {
    "schema_version", "cohort", "clock_source", "observer_off", "roster",
    "semantic_hash", "samples", "warmup_windows", "window_count",
    "retirements_per_window", "results",
}
_RESULT_KEYS = {
    "scenario", "program_words", "arf", "mode", "operation",
    "data_address_word", "initial_data_word", "retired_count",
    "elapsed_ns", "elapsed_ticks", "min_elapsed_ns", "max_elapsed_ns",
    "endpoint_read_count", "endpoint_write_count", "same_address",
    "fresh_transaction_ids", "phase_order_valid", "transform_valid",
    "ns_per_instruction",
    "instructions_per_second",
}
_RECORD_KEYS = _PAYLOAD_KEYS | {
    "captured_utc", "git_sha", "vcs_state", "compiler", "platform",
    "platform_machine", "cpu", "build_flags", "profile",
    "benchmark_source_sha256", "binary_sha256",
}


def _fnv_bytes(value: int, data: bytes) -> int:
    for byte in data:
        value ^= byte
        value = (value * 1099511628211) & ((1 << 64) - 1)
    return value


def semantic_hash(results: list[dict[str, Any]]) -> str:
    """Recompute the 16-hex semantic hash from the exact opcode roster."""

    value = 14695981039346656037
    for result in results:
        value = _fnv_bytes(value, result["scenario"].encode("ascii") + b"\0")
        for word in result["program_words"]:
            value = _fnv_bytes(value, bytes(((word >> 8) & 0xFF, word & 0xFF)))
        value = _fnv_bytes(value, bytes((result["arf"], result["mode"])))
        value = _fnv_bytes(value, result["data_address_word"].to_bytes(4, "big"))
        value = _fnv_bytes(value, result["initial_data_word"].to_bytes(2, "big"))
        value = _fnv_bytes(value, bytes((result["operation"], 2, 1, 2)))
    return f"{value:016x}"


def _positive_int(value: Any, field: str) -> None:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise ValueError(f"{field} must be a positive integer")


def _validate_result(result: Any) -> None:
    if not isinstance(result, dict) or set(result) != _RESULT_KEYS:
        raise ValueError("RMW result has an unexpected schema")
    name = result.get("scenario")
    if not isinstance(name, str) or name not in EXPECTED_SCENARIOS:
        raise ValueError("RMW result has an unexpected scenario")
    spec = EXPECTED_SCENARIOS[name]
    if result.get("program_words") != list(spec["program_words"]):
        raise ValueError("RMW program-word roster drifted")
    if (result.get("arf") != spec["arf"] or result.get("mode") != spec["mode"] or
            result.get("operation") != spec["operation"] or
            result.get("data_address_word") != 0 or
            result.get("initial_data_word") != 0xA55A):
        raise ValueError("RMW addressing roster drifted")
    for field in (
        "retired_count", "elapsed_ns", "elapsed_ticks", "min_elapsed_ns",
        "max_elapsed_ns", "endpoint_read_count", "endpoint_write_count",
    ):
        _positive_int(result.get(field), field)
    retired = result["retired_count"]
    if retired != WINDOWS * RETIREMENTS_PER_WINDOW:
        raise ValueError("RMW retirement policy changed")
    if result["endpoint_read_count"] != retired or result["endpoint_write_count"] != retired:
        raise ValueError("RMW endpoint counts do not match retirements")
    if result["elapsed_ticks"] != result["elapsed_ns"]:
        raise ValueError("RMW elapsed tick source drifted")
    if not (result["min_elapsed_ns"] <= result["elapsed_ns"] <= result["max_elapsed_ns"]):
        raise ValueError("RMW timing spread is invalid")
    for field in ("same_address", "fresh_transaction_ids", "phase_order_valid", "transform_valid"):
        if result.get(field) is not True:
            raise ValueError(f"RMW semantic self-check failed: {field}")
    for field in ("ns_per_instruction", "instructions_per_second"):
        value = result.get(field)
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError(f"{field} must be numeric")
        if not math.isfinite(value) or value <= 0.0:
            raise ValueError(f"{field} must be finite and positive")
    expected_ns = result["elapsed_ns"] / retired
    expected_ips = retired * 1_000_000_000.0 / result["elapsed_ns"]
    if abs(result["ns_per_instruction"] - expected_ns) > max(0.01, expected_ns * 1e-6):
        raise ValueError("RMW ns-per-instruction arithmetic failed")
    if abs(result["instructions_per_second"] - expected_ips) > max(0.01, expected_ips * 1e-6):
        raise ValueError("RMW throughput arithmetic failed")


def validate_payload(payload: Any) -> dict[str, Any]:
    """Validate one executable's JSON object and return it unchanged."""

    if not isinstance(payload, dict) or set(payload) != _PAYLOAD_KEYS:
        raise ValueError("RMW benchmark output has an unexpected schema")
    if payload["schema_version"] != SCHEMA_VERSION or payload["cohort"] != COHORT:
        raise ValueError("RMW benchmark schema/cohort mismatch")
    if payload["clock_source"] != CLOCK_SOURCE or payload["observer_off"] is not True:
        raise ValueError("RMW benchmark must use CLOCK_MONOTONIC with observers off")
    if payload["roster"] != list(ROSTER):
        raise ValueError("RMW benchmark roster mismatch")
    for field, expected in (
        ("samples", SAMPLES), ("warmup_windows", WARMUPS),
        ("window_count", WINDOWS), ("retirements_per_window", RETIREMENTS_PER_WINDOW),
    ):
        if payload[field] != expected:
            raise ValueError(f"RMW benchmark policy changed: {field}")
    results = payload["results"]
    if not isinstance(results, list) or [item.get("scenario") for item in results] != list(ROSTER):
        raise ValueError("RMW benchmark must emit the exact ordered roster")
    for result in results:
        _validate_result(result)
    if payload["semantic_hash"] != semantic_hash(results):
        raise ValueError("RMW semantic hash mismatch")
    if not isinstance(payload["semantic_hash"], str) or len(payload["semantic_hash"]) != 16:
        raise ValueError("RMW semantic hash must be 16 hexadecimal characters")
    try:
        int(payload["semantic_hash"], 16)
    except (TypeError, ValueError) as exc:
        raise ValueError("RMW semantic hash must be hexadecimal") from exc
    return payload


def parse_output(text: str) -> dict[str, Any]:
    """Parse exactly one JSON object; diagnostics or extra JSON are rejected."""

    if not text.strip():
        raise ValueError("RMW benchmark emitted no JSON")
    try:
        payload = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ValueError(f"RMW benchmark output is not one JSON object: {exc}") from exc
    return validate_payload(payload)


def validate_record(record: Any) -> dict[str, Any]:
    if not isinstance(record, dict) or set(record) != _RECORD_KEYS:
        raise ValueError("RMW capture record has an unexpected schema")
    validate_payload({key: record[key] for key in _PAYLOAD_KEYS})
    if record["vcs_state"] not in {"clean", "dirty"}:
        raise ValueError("RMW capture VCS state is invalid")
    for field in ("profile", "compiler", "platform", "platform_machine", "cpu", "build_flags"):
        value = record.get(field)
        if not isinstance(value, str) or not value.strip() or value.lower() == "unknown":
            raise ValueError(f"RMW capture metadata missing: {field}")
    for field in ("git_sha",):
        value = record.get(field)
        if not isinstance(value, str) or len(value) < 40 or any(char not in "0123456789abcdef" for char in value):
            raise ValueError(f"RMW capture metadata invalid: {field}")
    for field in ("benchmark_source_sha256", "binary_sha256"):
        value = record.get(field)
        if not isinstance(value, str) or len(value) != 64:
            raise ValueError(f"RMW capture hash invalid: {field}")
        try:
            int(value, 16)
        except ValueError as exc:
            raise ValueError(f"RMW capture hash invalid: {field}") from exc
    if not isinstance(record["captured_utc"], str) or not record["captured_utc"].strip():
        raise ValueError("RMW capture timestamp missing")
    if record["observer_off"] is not True:
        raise ValueError("RMW capture observer state changed")
    return record


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def _git_metadata(root: Path) -> tuple[str, str]:
    try:
        sha = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
        dirty = bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=root, text=True,
            stderr=subprocess.DEVNULL,
        ))
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
                words + ["--version"], text=True, stderr=subprocess.STDOUT,
            ).splitlines()[0]
        except (OSError, subprocess.CalledProcessError, IndexError):
            pass
    record = dict(payload)
    record.update({
        "captured_utc": _datetime.datetime.now(_datetime.timezone.utc).isoformat(),
        "git_sha": sha,
        "vcs_state": vcs_state,
        "compiler": compiler_version,
        "platform": platform.platform(),
        "platform_machine": platform.machine(),
        "cpu": _cpu_name(),
        "build_flags": build_flags or "unknown",
        "profile": profile,
        "benchmark_source_sha256": _sha256(root / "bench/ti_c54x_rmw_bench.c"),
        "binary_sha256": _sha256(binary),
    })
    validate_record(record)
    return record


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", type=Path, default=Path("build-bench/ti_c54x_rmw_bench"))
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--allow-dirty", action="store_true")
    parser.add_argument("--compiler", default="cc")
    parser.add_argument("--build-flags", default="")
    parser.add_argument("--profile", default="portable-o2-no-lto")
    args = parser.parse_args(argv)
    try:
        completed = subprocess.run(
            [str(args.benchmark)], check=True, capture_output=True, text=True,
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
        print(f"RMW perf capture failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
