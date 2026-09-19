"""Contract checks for the W47B2C observer-off DLD/DST benchmark lane.

Module contract: owns deterministic proofs for the private benchmark schema,
local build wiring, trusted-packet dispatch boundaries, and fixed LONG32
carrier/root ABI sizes.  It does not claim TI machine timing, freeze a host
threshold, or validate opcode semantics beyond the executable's invariants.
"""

from __future__ import annotations

import copy
import ctypes
import json
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tools.perf import dld_dst_capture


ROOT = Path(__file__).resolve().parents[4]
BENCH_SOURCE = ROOT / "bench" / "ti_c54x_dld_dst_bench.c"
CAPTURE_SOURCE = ROOT / "tools" / "perf" / "dld_dst_capture.py"


def _source(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _result(name: str, elapsed_ns: int = 40_000_000) -> dict[str, object]:
    spec = dld_dst_capture.EXPECTED_SCENARIOS[name]
    retired = dld_dst_capture.WINDOWS * dld_dst_capture.RETIREMENTS_PER_WINDOW
    return {
        "scenario": name,
        "program_words": list(spec["program_words"]),
        "arf": 0,
        "mode": 13,
        "operation": spec["operation"],
        "long_offset_word": 0,
        "data_address_word": 256,
        "initial_accumulator_raw40": 0x12345678,
        "retired_count": retired,
        "attempted_count": retired,
        "final_pc_word": retired * 2,
        "program_reads": retired * 2,
        "endpoint_request_count": retired * 2,
        "endpoint_read_count": retired * 2 if spec["reads"] else 0,
        "endpoint_write_count": retired * 2 if spec["writes"] else 0,
        "first_address_word": 256,
        "second_address_word": 257,
        "first_transaction_id": 1,
        "last_transaction_id": retired * 2,
        "request_order_valid": True,
        "address_sequence_valid": True,
        "fresh_transaction_ids": True,
        "pc_progression_valid": True,
        "final_accumulator_raw40": 0x12345678,
        "source_unchanged": True,
        "final_write_msw": spec["final_write_msw"],
        "final_write_lsw": spec["final_write_lsw"],
        "elapsed_ns": elapsed_ns,
        "elapsed_ticks": elapsed_ns,
        "min_elapsed_ns": elapsed_ns - 100_000,
        "max_elapsed_ns": elapsed_ns + 100_000,
        "ns_per_instruction": elapsed_ns / retired,
        "instructions_per_second": retired * 1_000_000_000.0 / elapsed_ns,
    }


def _payload() -> dict[str, object]:
    results = [_result(name) for name in dld_dst_capture.ROSTER]
    return {
        "schema_version": dld_dst_capture.SCHEMA_VERSION,
        "cohort": dld_dst_capture.COHORT,
        "clock_source": dld_dst_capture.CLOCK_SOURCE,
        "observer_off": True,
        "roster": list(dld_dst_capture.ROSTER),
        "semantic_hash": dld_dst_capture.semantic_hash(results),
        "samples": dld_dst_capture.SAMPLES,
        "warmup_windows": dld_dst_capture.WARMUPS,
        "window_count": dld_dst_capture.WINDOWS,
        "retirements_per_window": dld_dst_capture.RETIREMENTS_PER_WINDOW,
        "results": results,
    }


class DldDstLong32HotPathTests(unittest.TestCase):
    def test_build_wiring_is_local_and_observer_off(self) -> None:
        make_text = _source(ROOT / "Makefile")
        cmake_text = _source(ROOT / "CMakeLists.txt")
        self.assertIn("bench-dld-dst:", make_text)
        self.assertIn("bench-dld-dst-smoke:", make_text)
        self.assertIn("ti_c54x_dld_dst_bench", make_text)
        self.assertIn("tools/perf/dld_dst_capture.py", make_text)
        self.assertIn("ti_c54x_dld_dst_bench", cmake_text)
        self.assertIn("bench_dld_dst_smoke", cmake_text)
        self.assertIn("TI_C54X_ENABLE_OBSERVERS=0", _source(BENCH_SOURCE))
        self.assertTrue(CAPTURE_SOURCE.is_file())

        dld_goal = make_text.split("bench-dld-dst:", 1)[1].split(
            "bench-dld-dst-smoke:", 1
        )[0]
        self.assertNotIn("history", dld_goal)
        self.assertNotIn("compare.py", dld_goal)

    def test_benchmark_compile_guard_rejects_observers_on_and_undefined(self) -> None:
        common = [
            "cc",
            "-std=c11",
            "-Iinclude",
            "-Isrc",
            "-fsyntax-only",
            str(BENCH_SOURCE),
        ]
        on = subprocess.run(
            [*common, "-DTI_C54X_ENABLE_OBSERVERS=1"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        undefined = subprocess.run(common, cwd=ROOT, capture_output=True, text=True)
        off = subprocess.run(
            [*common, "-DTI_C54X_ENABLE_OBSERVERS=0"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(on.returncode, 0)
        self.assertNotEqual(undefined.returncode, 0)
        self.assertEqual(off.returncode, 0, off.stderr)

    def test_schema_and_semantic_roster_reject_drift(self) -> None:
        payload = _payload()
        self.assertEqual(dld_dst_capture.validate_payload(payload), payload)
        for mutation in (
            lambda value: value.update(observer_off=False),
            lambda value: value.update(roster=["dld-long32-mod13"]),
            lambda value: value["results"][0].update(program_reads=1),
            lambda value: value["results"][1].update(endpoint_write_count=0),
            lambda value: value["results"][0].update(pc_progression_valid=False),
            lambda value: value["results"][0].update(final_pc_word=0),
        ):
            candidate = copy.deepcopy(payload)
            mutation(candidate)
            with self.assertRaises(ValueError):
                dld_dst_capture.validate_payload(candidate)
        self.assertEqual(
            dld_dst_capture.parse_output(json.dumps(payload))["cohort"],
            dld_dst_capture.COHORT,
        )
        with self.assertRaises(ValueError):
            dld_dst_capture.parse_output(json.dumps(payload) + "\nnoise\n")

    def test_trusted_packet_extracts_once_and_run_has_no_opcode_switch(self) -> None:
        packet = _source(ROOT / "src/isa/decode/trusted_execution_packet.c")
        run = _source(ROOT / "src/cpu/execution/run.c")
        self.assertEqual(packet.count("ti_c54x_operand_layout_extract_all_trusted("), 1)
        self.assertIn("ti_c54x_trusted_execution_packet_prepare_fast", packet)
        self.assertNotRegex(run, r"case\s+TI_C54X_OPCODE_ID_")
        self.assertNotRegex(run, r"switch\s*\([^)]*(?:opcode|instruction|id)")
        self.assertNotRegex(run, r"\bti_c54x_op_[A-Za-z0-9_]+\s*\(")
        self.assertIn("ti_c54x_trusted_op_dld_long32_mod13", _source(ROOT / "src/isa/ops/load/op_dld_long32.c"))
        self.assertIn("ti_c54x_trusted_op_dst_long32_mod13", _source(ROOT / "src/isa/ops/store/op_dst_long32.c"))

    def test_dld_dst_root_hot_path_has_no_host_side_effects(self) -> None:
        paths = (
            ROOT / "src/isa/ops/load/op_dld_long32.c",
            ROOT / "src/isa/ops/store/op_dst_long32.c",
            ROOT / "src/isa/ops/memory/memory_effect.c",
            ROOT / "src/sequencer/instruction/memory_instruction_transaction.c",
            ROOT / "src/sequencer/instruction/memory_instruction_long32.c",
            ROOT / "src/cpu/execution/run.c",
        )
        forbidden = re.compile(
            r"\b(?:malloc|calloc|realloc|free|aligned_alloc|getenv|fopen|fclose|"
            r"fread|fwrite|fprintf|printf|puts|putchar|perror|syslog)\s*\("
        )
        for path in paths:
            self.assertIsNone(forbidden.search(_source(path)), str(path))

    @staticmethod
    def _abi_probe(extra_flags: tuple[str, ...] = ()) -> tuple[int, int, int, int]:
        probe = """
#include <stdio.h>
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
int main(void) {
    printf("%zu %zu %zu %zu\\n", sizeof(void *), sizeof(uint64_t),
           sizeof(ti_c54x_op_memory_effect),
           sizeof(ti_c54x_memory_instruction_transaction));
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "abi.c"
            binary = Path(directory) / "abi"
            source.write_text(probe, encoding="utf-8")
            result = subprocess.run(
                ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", *extra_flags,
                 "-Iinclude", "-Isrc", str(source), "-o", str(binary)],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                raise unittest.SkipTest("compiler ABI probe unavailable")
            output = subprocess.check_output([str(binary)], cwd=ROOT, text=True)
        return tuple(int(value) for value in output.split())  # type: ignore[return-value]

    def test_fixed_effect_and_root_sizes_native(self) -> None:
        if shutil.which("cc") is None:
            self.skipTest("native C compiler unavailable")
        pointer, uint64, effect, root = self._abi_probe()
        self.assertEqual(pointer, ctypes.sizeof(ctypes.c_void_p))
        self.assertEqual(uint64, ctypes.sizeof(ctypes.c_uint64))
        if pointer == 8 and uint64 == 8:
            self.assertEqual((effect, root), (48, 456))
        elif pointer == 4 and uint64 == 8:
            self.assertEqual((effect, root), (40, 424))
        else:
            self.skipTest(f"unsupported native ABI: pointer={pointer}, uint64={uint64}")

    def test_fixed_effect_and_root_sizes_ilp32_when_supported(self) -> None:
        if shutil.which("cc") is None:
            self.skipTest("native C compiler unavailable")
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "m32.c"
            binary = Path(directory) / "m32"
            source.write_text("#include <stdint.h>\nint main(void){return sizeof(uint64_t)!=8;}\n", encoding="utf-8")
            supported = subprocess.run(
                ["cc", "-m32", "-std=c11", str(source), "-o", str(binary)],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
        if supported.returncode != 0:
            self.skipTest("compiler does not support a complete -m32 probe")
        pointer, uint64, effect, root = self._abi_probe(("-m32",))
        self.assertEqual((pointer, uint64, effect, root), (4, 8, 40, 424))


if __name__ == "__main__":
    unittest.main()
