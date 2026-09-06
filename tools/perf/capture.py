#!/usr/bin/env python3
"""Capture and validate the hosted performance baseline."""

# Module contract
# Owns: hosted capture, strict JSONL validation, metadata, and append-only
# clean-baseline history. Does not own core timing, instrumentation, or knobs.

from __future__ import annotations

import argparse
import datetime as _datetime
import hashlib
import json
import math
import os
import platform
import re
import shutil
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
FIXED_SCENARIOS = {"nop", "ld-dp", "ld-asm", "ld-arp"}
SCENARIOS = FIXED_SCENARIOS | {"mixed-current"}
SAMPLES = 7
WARMUPS = 2
WINDOWS = 16
RETIREMENTS_PER_WINDOW = 50000
FIXED_SEMANTIC_HASHES = {
    "nop": "856a941d29b43c43",
    "ld-dp": "72a5067b3a02489f",
    "ld-asm": "731a51f47ad7e422",
    "ld-arp": "5d08d163159be334",
}
REQUIRED_RESULT_FIELDS = {
    "schema_version", "clock_source", "scenario", "retired_count",
    "elapsed_ns", "elapsed_ticks", "min_elapsed_ns", "max_elapsed_ns",
    "samples", "warmup_windows", "window_count", "retirements_per_window",
    "semantic_hash", "scenario_instruction_count", "catalog_instruction_count", "ns_per_instruction",
    "instructions_per_second",
}
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
GIT_SHA_PATTERN = re.compile(r"[0-9a-f]{40,64}")


def validate_result(value: Any) -> None:
    if not isinstance(value, dict):
        raise ValueError("benchmark result must be an object")
    missing = REQUIRED_RESULT_FIELDS.difference(value)
    if missing:
        raise ValueError(f"benchmark result missing fields: {sorted(missing)}")
    if value["schema_version"] != SCHEMA_VERSION or value["clock_source"] != "CLOCK_MONOTONIC":
        raise ValueError("unsupported benchmark schema or clock")
    if not isinstance(value["scenario"], str) or not value["scenario"]:
        raise ValueError("scenario must be a non-empty string")
    integer_fields = ("retired_count", "elapsed_ns", "elapsed_ticks", "min_elapsed_ns", "max_elapsed_ns", "samples", "warmup_windows", "window_count", "retirements_per_window", "scenario_instruction_count", "catalog_instruction_count")
    for field in integer_fields:
        if isinstance(value[field], bool) or not isinstance(value[field], int) or value[field] <= 0:
            raise ValueError(f"{field} must be a positive integer")
    for field in ("ns_per_instruction", "instructions_per_second"):
        if isinstance(value[field], bool) or not isinstance(value[field], (int, float)) or not math.isfinite(value[field]) or value[field] <= 0:
            raise ValueError(f"{field} must be positive and finite")
    if value["min_elapsed_ns"] > value["elapsed_ns"] or value["elapsed_ns"] > value["max_elapsed_ns"] or value["elapsed_ticks"] != value["elapsed_ns"]:
        raise ValueError("elapsed timing invariants failed")
    if not isinstance(value["semantic_hash"], str) or len(value["semantic_hash"]) != 16:
        raise ValueError("semantic hash must be 16 hexadecimal characters")
    try:
        int(value["semantic_hash"], 16)
    except ValueError as exc:
        raise ValueError("semantic hash must be hexadecimal") from exc
    if value["scenario"] in FIXED_SEMANTIC_HASHES and value["semantic_hash"] != FIXED_SEMANTIC_HASHES[value["scenario"]]:
        raise ValueError("fixed scenario semantic hash changed")
    if (value["samples"], value["warmup_windows"], value["window_count"], value["retirements_per_window"]) != (SAMPLES, WARMUPS, WINDOWS, RETIREMENTS_PER_WINDOW):
        raise ValueError("benchmark policy values changed")
    if value["retired_count"] != value["window_count"] * value["retirements_per_window"]:
        raise ValueError("sample/count invariants failed")
    expected_ns = value["elapsed_ns"] / value["retired_count"]
    expected_ips = value["retired_count"] * 1_000_000_000.0 / value["elapsed_ns"]
    if abs(value["ns_per_instruction"] - expected_ns) > 0.01 or abs(value["instructions_per_second"] - expected_ips) > max(0.01, expected_ips * 0.00001):
        raise ValueError("derived metric arithmetic failed")


def validate_results(results: list[dict[str, Any]], catalog_count: int | None = None) -> None:
    for result in results:
        validate_result(result)
    names = [result["scenario"] for result in results]
    if set(names) != SCENARIOS or len(names) != len(set(names)):
        raise ValueError("benchmark must emit the exact unique scenario roster")
    counts = {result["catalog_instruction_count"] for result in results}
    if len(counts) != 1 or (catalog_count is not None and counts != {catalog_count}):
        raise ValueError("catalog instruction count must be consistent")
    for result in results:
        if result["scenario"] in FIXED_SCENARIOS and result["scenario_instruction_count"] != 1:
            raise ValueError("fixed scenarios must contain one instruction")
        if result["scenario"] == "mixed-current" and result["scenario_instruction_count"] != result["catalog_instruction_count"]:
            raise ValueError("mixed-current instruction count mismatch")


def parse_benchmark_output(text: str) -> list[dict[str, Any]]:
    results = []
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ValueError(f"line {line_number} is not JSON: {exc}") from exc
        validate_result(value)
        results.append(value)
    validate_results(results)
    return results


def _git_metadata(root: Path) -> tuple[str, str]:
    try:
        sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
        dirty = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=root, text=True, stderr=subprocess.DEVNULL))
        return sha, "dirty" if dirty else "clean"
    except (OSError, subprocess.CalledProcessError):
        return "unknown", "unknown"


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def _catalog_count(root: Path) -> int:
    text = (root / "src/isa/catalog/opcode_catalog.def").read_text(encoding="utf-8")
    return len(re.findall(r"^TI_C54X_OPCODE_(?:EXACT|MASKED)\(", text, re.MULTILINE))


def _cpu_name() -> str:
    try:
        for line in Path("/proc/cpuinfo").read_text(encoding="utf-8").splitlines():
            if line.lower().startswith(("model name", "hardware")):
                return line.split(":", 1)[-1].strip()
    except OSError:
        pass
    return platform.processor() or "unknown"


def make_record(results: list[dict[str, Any]], root: Path, compiler: str = "cc", build_flags: str = "", binary: Path | None = None) -> dict[str, Any]:
    sha, vcs_state = _git_metadata(root)
    catalog_count = _catalog_count(root)
    validate_results(results, catalog_count)
    semantic_hash = hashlib.sha256(json.dumps(sorted((result["scenario"], result["semantic_hash"], "fixed" if result["scenario"] in FIXED_SCENARIOS else "evolving") for result in results), separators=(",", ":")).encode()).hexdigest()
    compiler_words = shlex.split(compiler)
    compiler_path = shutil.which(compiler_words[0]) if compiler_words else None
    compiler_version = "unknown"
    if compiler_path is not None:
        try:
            compiler_version = subprocess.check_output(compiler_words + ["--version"], text=True, stderr=subprocess.STDOUT).splitlines()[0]
        except (OSError, subprocess.CalledProcessError, IndexError):
            pass
    return {
        "schema_version": SCHEMA_VERSION,
        "clock_source": "CLOCK_MONOTONIC",
        "captured_utc": _datetime.datetime.now(_datetime.timezone.utc).isoformat(),
        "git_sha": sha,
        "vcs_state": vcs_state,
        "compiler": compiler_version,
        "platform": platform.platform(),
        "platform_machine": platform.machine(),
        "cpu": _cpu_name(),
        "build_flags": build_flags or "unknown",
        "catalog_instruction_count": _catalog_count(root),
        "scenario_semantic_hash": semantic_hash,
        "benchmark_source_sha256": _sha256(root / "bench/ti_c54x_bench.c"),
        "binary_sha256": _sha256(binary) if binary is not None else "unknown",
        "results": results,
    }


def validate_persisted_record(record: Any) -> None:
    if not isinstance(record, dict) or not isinstance(record.get("results"), list):
        raise ValueError("malformed benchmark history record")
    if record.get("schema_version") != SCHEMA_VERSION or record.get("clock_source") != "CLOCK_MONOTONIC":
        raise ValueError("invalid benchmark history schema or clock")
    catalog_count = record.get("catalog_instruction_count")
    if isinstance(catalog_count, bool) or not isinstance(catalog_count, int) or catalog_count <= 0:
        raise ValueError("invalid benchmark history catalog count")
    validate_results(record["results"], catalog_count)
    if record.get("vcs_state") != "clean":
        raise ValueError("benchmark history requires a clean VCS state")
    for field in ("compiler", "platform", "platform_machine", "cpu", "build_flags"):
        value = record.get(field)
        if not isinstance(value, str) or not value.strip() or value.lower() == "unknown":
            raise ValueError(f"benchmark history requires exact metadata: {field}")
    git_sha = record.get("git_sha")
    if not isinstance(git_sha, str) or GIT_SHA_PATTERN.fullmatch(git_sha) is None:
        raise ValueError("benchmark history requires an exact Git commit")
    for field in ("scenario_semantic_hash", "benchmark_source_sha256", "binary_sha256"):
        value = record.get(field)
        if not isinstance(value, str) or SHA256_PATTERN.fullmatch(value) is None:
            raise ValueError(f"benchmark history requires an exact hash: {field}")


def append_history(path: Path, record: dict[str, Any]) -> None:
    validate_persisted_record(record)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as stream:
        stream.write(json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", type=Path, default=Path("build-bench/ti_c54x_bench"))
    parser.add_argument("--history", type=Path)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--allow-dirty", action="store_true")
    parser.add_argument("--compiler", default="cc")
    parser.add_argument("--build-flags", default="")
    args = parser.parse_args(argv)
    try:
        completed = subprocess.run([str(args.benchmark)], check=True, capture_output=True, text=True)
        results = parse_benchmark_output(completed.stdout)
        record = make_record(results, args.root, args.compiler, args.build_flags, args.benchmark)
        if record["vcs_state"] != "clean" and not args.allow_dirty:
            raise ValueError("clean VCS state required; use --allow-dirty only for ad-hoc output")
        if args.history is not None and args.allow_dirty:
            raise ValueError("--allow-dirty cannot append baseline history")
        if args.history is not None:
            append_history(args.history, record)
        print(json.dumps(record, sort_keys=True, separators=(",", ":")))
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"perf capture failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
