#!/usr/bin/env python3
"""Capture the dedicated observer-off DLD/DST LONG32 benchmark."""

# Module contract
# Owns: strict W47B2C benchmark-output validation, cold capture metadata, and
# JSON serialization for an ad-hoc observer-off DLD/DST cohort.  It does not
# own the generic benchmark schema/history, timing thresholds, or CPU semantics.

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

SCHEMA_VERSION = 1
COHORT = "ti-c54x-dld-dst-long32-observer-off-v1"
CLOCK_SOURCE = "CLOCK_MONOTONIC"
ROSTER = ("dld-long32-mod13", "dst-long32-mod13")
SAMPLES = 5
WARMUPS = 8
WINDOWS = 8
RETIREMENTS_PER_WINDOW = 5_000

EXPECTED_SCENARIOS: dict[str, dict[str, Any]] = {
    "dld-long32-mod13": {
        "program_words": (0x56E8, 0x0000),
        "operation": 0,
        "reads": True,
        "writes": False,
        "final_write_msw": 0,
        "final_write_lsw": 0,
    },
    "dst-long32-mod13": {
        "program_words": (0x4EE8, 0x0000),
        "operation": 1,
        "reads": False,
        "writes": True,
        "final_write_msw": 0x1234,
        "final_write_lsw": 0x5678,
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
    "arf",
    "mode",
    "operation",
    "long_offset_word",
    "data_address_word",
    "initial_accumulator_raw40",
    "retired_count",
    "attempted_count",
    "final_pc_word",
    "program_reads",
    "endpoint_request_count",
    "endpoint_read_count",
    "endpoint_write_count",
    "first_address_word",
    "second_address_word",
    "first_transaction_id",
    "last_transaction_id",
    "request_order_valid",
    "address_sequence_valid",
    "fresh_transaction_ids",
    "pc_progression_valid",
    "final_accumulator_raw40",
    "source_unchanged",
    "final_write_msw",
    "final_write_lsw",
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
    """Recompute the hash from the exact two-scenario semantic roster."""

    value = 14695981039346656037
    for result in results:
        value = _fnv_bytes(value, result["scenario"].encode("ascii") + b"\0")
        for word in result["program_words"]:
            value = _fnv_bytes(value, bytes(((word >> 8) & 0xFF, word & 0xFF)))
        descriptor = bytes(
            (
                0,
                0,
                1,
                0,
                1,
                0,
                1,
                0,
                0x12,
                0x34,
                0x56,
                0x78,
                result["operation"],
            )
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
        raise ValueError("DLD/DST result has an unexpected schema")
    name = result.get("scenario")
    if not isinstance(name, str) or name not in EXPECTED_SCENARIOS:
        raise ValueError("DLD/DST result has an unexpected scenario")
    spec = EXPECTED_SCENARIOS[name]
    if result.get("program_words") != list(spec["program_words"]):
        raise ValueError("DLD/DST program-word roster drifted")
    if (
        result.get("arf") != 0
        or result.get("mode") != 13
        or result.get("operation") != spec["operation"]
        or result.get("long_offset_word") != 0
        or result.get("data_address_word") != 256
        or result.get("initial_accumulator_raw40") != 0x12345678
    ):
        raise ValueError("DLD/DST addressing roster drifted")
    retired = WINDOWS * RETIREMENTS_PER_WINDOW
    for field in (
        "retired_count",
        "attempted_count",
        "final_pc_word",
        "program_reads",
        "endpoint_request_count",
        "first_address_word",
        "second_address_word",
        "first_transaction_id",
        "last_transaction_id",
        "elapsed_ns",
        "elapsed_ticks",
        "min_elapsed_ns",
        "max_elapsed_ns",
    ):
        _positive_int(result.get(field), field)
    for field in ("endpoint_read_count", "endpoint_write_count"):
        _nonnegative_int(result.get(field), field)
    for field in (
        "final_accumulator_raw40",
        "final_write_msw",
        "final_write_lsw",
    ):
        _nonnegative_int(result.get(field), field)
    if result["retired_count"] != retired or result["attempted_count"] != retired:
        raise ValueError("DLD/DST attempt/retirement policy changed")
    if result["final_pc_word"] != retired * 2:
        raise ValueError("DLD/DST PC progression failed")
    if result["program_reads"] != retired * 2 or result["endpoint_request_count"] != retired * 2:
        raise ValueError("DLD/DST two-word counts failed")
    expected_reads = retired * 2 if name == ROSTER[0] else 0
    expected_writes = retired * 2 if name == ROSTER[1] else 0
    if result["endpoint_read_count"] != expected_reads or result["endpoint_write_count"] != expected_writes:
        raise ValueError("DLD/DST endpoint counts failed")
    if result["first_address_word"] != 256 or result["second_address_word"] != 257:
        raise ValueError("DLD/DST address order failed")
    if result["last_transaction_id"] < result["first_transaction_id"]:
        raise ValueError("DLD/DST transaction IDs are not ordered")
    for field in (
        "request_order_valid",
        "address_sequence_valid",
        "fresh_transaction_ids",
        "pc_progression_valid",
        "source_unchanged",
    ):
        if result.get(field) is not True:
            raise ValueError(f"DLD/DST semantic self-check failed: {field}")
    if result["final_accumulator_raw40"] != 0x12345678:
        raise ValueError("DLD/DST final accumulator drifted")
    if result["final_write_msw"] != spec["final_write_msw"] or result["final_write_lsw"] != spec["final_write_lsw"]:
        raise ValueError("DLD/DST final write drifted")
    if not (result["min_elapsed_ns"] <= result["elapsed_ns"] <= result["max_elapsed_ns"]):
        raise ValueError("DLD/DST timing spread is invalid")
    if result["elapsed_ticks"] != result["elapsed_ns"]:
        raise ValueError("DLD/DST clock arithmetic drifted")
    for field in ("ns_per_instruction", "instructions_per_second"):
        value = result.get(field)
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError(f"{field} must be numeric")
        if not math.isfinite(value) or value <= 0.0:
            raise ValueError(f"{field} must be finite and positive")
    expected_ns = result["elapsed_ns"] / retired
    expected_ips = retired * 1_000_000_000.0 / result["elapsed_ns"]
    if abs(result["ns_per_instruction"] - expected_ns) > max(0.01, expected_ns * 1e-6):
        raise ValueError("DLD/DST ns-per-instruction arithmetic failed")
    if abs(result["instructions_per_second"] - expected_ips) > max(0.01, expected_ips * 1e-6):
        raise ValueError("DLD/DST throughput arithmetic failed")


def validate_payload(payload: Any) -> dict[str, Any]:
    """Validate one executable's JSON object and return it unchanged."""

    if not isinstance(payload, dict) or set(payload) != _PAYLOAD_KEYS:
        raise ValueError("DLD/DST benchmark output has an unexpected schema")
    if payload["schema_version"] != SCHEMA_VERSION or payload["cohort"] != COHORT:
        raise ValueError("DLD/DST benchmark schema/cohort mismatch")
    if payload["clock_source"] != CLOCK_SOURCE or payload["observer_off"] is not True:
        raise ValueError("DLD/DST benchmark must use CLOCK_MONOTONIC with observers off")
    if payload["roster"] != list(ROSTER):
        raise ValueError("DLD/DST benchmark roster mismatch")
    for field, expected in (
        ("samples", SAMPLES),
        ("warmup_windows", WARMUPS),
        ("window_count", WINDOWS),
        ("retirements_per_window", RETIREMENTS_PER_WINDOW),
    ):
        if payload[field] != expected:
            raise ValueError(f"DLD/DST benchmark policy changed: {field}")
    results = payload["results"]
    if not isinstance(results, list) or [item.get("scenario") for item in results] != list(ROSTER):
        raise ValueError("DLD/DST benchmark must emit the exact ordered roster")
    for result in results:
        _validate_result(result)
    if payload["semantic_hash"] != semantic_hash(results):
        raise ValueError("DLD/DST semantic hash mismatch")
    if not isinstance(payload["semantic_hash"], str) or len(payload["semantic_hash"]) != 16:
        raise ValueError("DLD/DST semantic hash must be 16 hexadecimal characters")
    try:
        int(payload["semantic_hash"], 16)
    except (TypeError, ValueError) as exc:
        raise ValueError("DLD/DST semantic hash must be hexadecimal") from exc
    return payload


def parse_output(text: str) -> dict[str, Any]:
    """Parse exactly one JSON object; diagnostics or extra JSON are rejected."""

    if not text.strip():
        raise ValueError("DLD/DST benchmark emitted no JSON")
    try:
        payload = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ValueError(f"DLD/DST benchmark output is not one JSON object: {exc}") from exc
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
        benchmark_source_sha256=_sha256(root / "bench/ti_c54x_dld_dst_bench.c"),
        binary_sha256=_sha256(binary),
    )
    validate_record(record)
    return record


def validate_record(record: Any) -> dict[str, Any]:
    if not isinstance(record, dict) or set(record) != _RECORD_KEYS:
        raise ValueError("DLD/DST capture record has an unexpected schema")
    validate_payload({key: record[key] for key in _PAYLOAD_KEYS})
    if record["vcs_state"] not in {"clean", "dirty"}:
        raise ValueError("DLD/DST capture VCS state is invalid")
    for field in ("profile", "compiler", "platform", "platform_machine", "cpu", "build_flags"):
        value = record.get(field)
        if not isinstance(value, str) or not value.strip() or value.lower() == "unknown":
            raise ValueError(f"DLD/DST capture metadata missing: {field}")
    value = record.get("git_sha")
    if not isinstance(value, str) or len(value) < 40 or any(char not in "0123456789abcdef" for char in value):
        raise ValueError("DLD/DST capture metadata invalid: git_sha")
    for field in ("benchmark_source_sha256", "binary_sha256"):
        value = record.get(field)
        if not isinstance(value, str) or len(value) != 64:
            raise ValueError(f"DLD/DST capture hash invalid: {field}")
        try:
            int(value, 16)
        except ValueError as exc:
            raise ValueError(f"DLD/DST capture hash invalid: {field}") from exc
    if not isinstance(record["captured_utc"], str) or not record["captured_utc"].strip():
        raise ValueError("DLD/DST capture timestamp missing")
    return record


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", type=Path, default=Path("build-bench/ti_c54x_dld_dst_bench"))
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
        print(f"DLD/DST perf capture failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
