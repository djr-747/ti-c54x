"""Structural contracts for the W50A dual-WORD16 read foundation.

Module contract: owns evidence-bearing shape checks for the private W50A
dual-read carrier, address resolver, and root dispatch seam.  It freezes the
existing value-carrier ABI, proves deterministic X-then-Y issue state, and
rejects host-side work on the new hot modules.  W50A deliberately does not
claim a nanosecond instruction result: the observer-off executable benchmark
belongs to the W50B opcode activation.
"""

from __future__ import annotations

import ctypes
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
ADDRESS_SPEC_HEADER = ROOT / "src/isa/ops/memory/memory_effect.h"
EFFECT_HEADER = ADDRESS_SPEC_HEADER
MEMORY_EFFECT_SOURCE = ROOT / "src/isa/ops/memory/memory_effect.c"
OP_EFFECT_HEADER = ROOT / "src/isa/ops/op_effect.h"
DUAL_READ_DIR = ROOT / "src/sequencer/instruction"
DUAL_READ_HEADER = DUAL_READ_DIR / "dual_word16_read_transaction.h"
DUAL_READ_SOURCE = DUAL_READ_DIR / "dual_word16_read_transaction.c"
DUAL_ADDRESS_DIR = ROOT / "src/sequencer/memory"
DUAL_ADDRESS_HEADER = DUAL_ADDRESS_DIR / "dual_address_resolution.h"
DUAL_ADDRESS_SOURCE = DUAL_ADDRESS_DIR / "dual_address_resolution.c"
DUAL_ROOT_HEADER = DUAL_READ_DIR / "memory_instruction_dual.h"
DUAL_ROOT_SOURCE = DUAL_READ_DIR / "memory_instruction_dual.c"
DUAL_CANDIDATE_HEADER = (
    DUAL_READ_DIR / "memory_instruction_dual_candidate_validate.h"
)
DUAL_CANDIDATE_SOURCE = (
    DUAL_READ_DIR / "memory_instruction_dual_candidate_validate.c"
)
DUAL_COMMIT_HEADER = DUAL_READ_DIR / "memory_instruction_dual_commit.h"
DUAL_COMMIT_SOURCE = DUAL_READ_DIR / "memory_instruction_dual_commit.c"
PAIR_DISPATCH_HEADER = DUAL_READ_DIR / "memory_instruction_pair_dispatch.h"
PAIR_DISPATCH_SOURCE = DUAL_READ_DIR / "memory_instruction_pair_dispatch.c"
ROOT_HEADER = DUAL_READ_DIR / "memory_instruction_transaction.h"
ROOT_SOURCE = DUAL_READ_DIR / "memory_instruction_transaction.c"
CPU_RUN_SOURCE = ROOT / "src/cpu/execution/run.c"

FORBIDDEN_CALL = re.compile(
    r"\b(?:malloc|calloc|realloc|free|aligned_alloc|getenv|"
    r"fopen|fclose|fread|fwrite|fprintf|printf|puts|putchar|perror|syslog)\s*\("
)


def _code_without_comments_or_literals(text: str) -> str:
    """Remove comments and literals for conservative C-shape checks."""

    without_comments = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', "", without_comments)


def _function_body(code: str, name: str) -> str:
    """Extract a brace-balanced function body from comment-free C text."""

    start = code.index(name)
    brace = code.index("{", start)
    depth = 0
    for index in range(brace, len(code)):
        if code[index] == "{":
            depth += 1
        elif code[index] == "}":
            depth -= 1
            if depth == 0:
                return code[brace + 1:index]
    raise AssertionError(f"unbalanced body for {name}")


class DualWord16HotPathTests(unittest.TestCase):
    """Exercise W50A's fixed carrier and generic dispatch budget."""

    @staticmethod
    def _read(path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def _require_dual_modules(self) -> tuple[Path, ...]:
        modules = (
            ADDRESS_SPEC_HEADER,
            MEMORY_EFFECT_SOURCE,
            DUAL_READ_HEADER,
            DUAL_READ_SOURCE,
            DUAL_ADDRESS_HEADER,
            DUAL_ADDRESS_SOURCE,
            DUAL_ROOT_HEADER,
            DUAL_ROOT_SOURCE,
            DUAL_CANDIDATE_HEADER,
            DUAL_CANDIDATE_SOURCE,
            DUAL_COMMIT_HEADER,
            DUAL_COMMIT_SOURCE,
            PAIR_DISPATCH_HEADER,
            PAIR_DISPATCH_SOURCE,
        )
        for path in modules:
            self.assertTrue(path.is_file(), f"planned W50A module is missing: {path}")
        return modules

    def test_dual_modules_have_contracts_and_remain_bounded(self) -> None:
        for path in self._require_dual_modules():
            text = self._read(path)
            self.assertIn("Module contract", text, path)
            self.assertIn("Owns:", text, path)
            self.assertIn("Does not own:", text, path)
            # ADDRESS_SPEC_HEADER carries the central address-spec ABI
            # union (one arm per address kind, static-assert-pinned
            # 24-byte spec). It grows ~24 lines per new address kind by
            # design and cannot split across headers without breaking
            # that ABI locality; the reviewed P3-S64 no-split decision
            # tracks the repository <800 rule for it instead of the
            # 500 module bound.
            bound = 800 if path == ADDRESS_SPEC_HEADER else 500
            self.assertLess(len(text.splitlines()), bound, path)

    def test_dual_reducer_is_one_value_owned_child_without_host_side_effects(self) -> None:
        header = self._read(DUAL_READ_HEADER)
        source = self._read(DUAL_READ_SOURCE)
        code = _code_without_comments_or_literals(header + "\n" + source)

        # The pair is a fixed value carrier: two copied requests, one reused
        # WORD16 child, and scalar staged values.  It owns no pointer field.
        self.assertEqual(
            re.findall(r"\bti_c54x_bus_word16_transaction\s+[A-Za-z_]\w*\s*;", code),
            ["ti_c54x_bus_word16_transaction word16_transaction;"],
        )
        transaction_body = re.search(
            r"typedef\s+struct\s+ti_c54x_dual_word16_read_transaction\s*\{(.*?)\}\s*"
            r"ti_c54x_dual_word16_read_transaction\s*;",
            code,
            flags=re.S,
        )
        self.assertIsNotNone(transaction_body)
        self.assertNotIn("*", transaction_body.group(1))
        self.assertNotRegex(code, r"\[[^\]\n]*\]")
        self.assertIsNone(FORBIDDEN_CALL.search(code))
        self.assertNotRegex(code, r"(?i)\b(?:observer|diagnostic|logging)\b")
        self.assertNotIn("ti_c54x_bus_word16_endpoint", code)

    def test_dual_reducer_has_deterministic_x_then_y_issue_shape(self) -> None:
        header = _code_without_comments_or_literals(self._read(DUAL_READ_HEADER))
        source = _code_without_comments_or_literals(self._read(DUAL_READ_SOURCE))
        combined = header + "\n" + source

        for state, value in (
            ("TI_C54X_DUAL_WORD16_READ_STATE_X", "1"),
            ("TI_C54X_DUAL_WORD16_READ_STATE_Y", "2"),
            ("TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY", "3"),
        ):
            self.assertRegex(
                header,
                rf"\b{state}\s*=\s*{value}\b",
            )
        self.assertIn("request.x_request", combined)
        self.assertIn("request.y_request", combined)
        self.assertEqual(
            source.count("ti_c54x_bus_word16_transaction_begin("),
            1,
        )
        self.assertRegex(
            source,
            r"state\s*=\s*TI_C54X_DUAL_WORD16_READ_STATE_Y",
        )
        self.assertRegex(
            source,
            r"ti_c54x_bus_word16_transaction_prepare\s*\([\s\S]*?"
            r"request\.y_request",
        )
        poll_body = _function_body(
            source, "ti_c54x_dual_word16_read_transaction_poll"
        )
        self.assertNotIn("ti_c54x_dual_word16_read_transaction_begin(", poll_body)
        self.assertNotIn("ti_c54x_sequencer_resolve_memory_address", source)

    def test_address_and_root_modules_stay_value_owned_and_side_effect_free(self) -> None:
        modules = self._require_dual_modules()
        for path in modules:
            code = _code_without_comments_or_literals(self._read(path))
            self.assertIsNone(FORBIDDEN_CALL.search(code), path)
            self.assertNotRegex(code, r"(?i)\b(?:observer|diagnostic|logging)\b")
        address_header = self._read(DUAL_ADDRESS_HEADER)
        self.assertNotRegex(address_header, r"typedef\s+struct[^;]*\*\s*\w+")
        self.assertIn("x", address_header)
        self.assertIn("y", address_header)

    def test_root_uses_generic_child_dispatch_without_replanning_after_prepared(self) -> None:
        self._require_dual_modules()
        root_header = self._read(ROOT_HEADER)
        root_source = _code_without_comments_or_literals(self._read(ROOT_SOURCE))
        dual_root_source = _code_without_comments_or_literals(self._read(DUAL_ROOT_SOURCE))

        self.assertIn("TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ", root_header)
        self.assertIn("TI_C54X_OP_EFFECT_MEMORY", root_source)
        self.assertIn("child_kind", root_source)
        self.assertIn("memory_instruction_pair_dispatch", root_source)
        self.assertIn("memory_instruction_dual", dual_root_source)
        pair_dispatch_source = _code_without_comments_or_literals(
            self._read(PAIR_DISPATCH_SOURCE)
        )
        self.assertIn(
            "TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ",
            pair_dispatch_source,
        )
        self.assertIn("memory_instruction_dual", pair_dispatch_source)
        self.assertNotIn("TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16", pair_dispatch_source)

        advance_start = root_source.index(
            "ti_c54x_memory_instruction_transaction_advance"
        )
        advance_source = root_source[advance_start:]
        self.assertNotRegex(
            advance_source,
            r"(?i)(?:resolve_memory_address|replan|redecode|decode|operand_layout|for_id)",
        )
        self.assertNotRegex(root_source, r"case\s+TI_C54X_OPCODE_ID_")
        run_source = self._read(CPU_RUN_SOURCE)
        self.assertNotRegex(run_source, r"case\s+TI_C54X_OPCODE_ID_")
        self.assertNotRegex(run_source, r"switch\s*\([^)]*(?:opcode|instruction|id)")

    @staticmethod
    def _abi_probe_source() -> str:
        # Address/effect/op-effect and root values are deliberately frozen.
        return r'''
#include <stdint.h>
#include <stdio.h>

#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "sequencer/instruction/dual_word16_read_transaction.h"
#include "sequencer/instruction/memory_instruction_transaction.h"

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24U, "address spec ABI");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 48U, "memory effect ABI");
_Static_assert(sizeof(ti_c54x_op_effect) == 72U, "op effect ABI");
_Static_assert(sizeof(ti_c54x_dual_word16_read_transaction) == 80U, "dual reducer ABI");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24U, "address spec ABI");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 40U, "memory effect ABI");
_Static_assert(sizeof(ti_c54x_op_effect) == 52U, "op effect ABI");
_Static_assert(sizeof(ti_c54x_dual_word16_read_transaction) == 72U, "dual reducer ABI");
#else
#error "unsupported pointer ABI"
#endif

int main(void)
{
    printf("pointer=%zu uint64=%zu address=%zu effect=%zu op_effect=%zu dual=%zu root=%zu\n",
           sizeof(void *), sizeof(uint64_t),
           sizeof(ti_c54x_op_memory_address_spec),
           sizeof(ti_c54x_op_memory_effect), sizeof(ti_c54x_op_effect),
           sizeof(ti_c54x_dual_word16_read_transaction),
           sizeof(ti_c54x_memory_instruction_transaction));
    return 0;
}
'''

    def _compile_and_run_abi_probe(self, extra_flags: tuple[str, ...] = ()) -> str:
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("native C compiler unavailable")
        with tempfile.TemporaryDirectory() as directory:
            directory_path = Path(directory)
            source = directory_path / "dual_abi_probe.c"
            binary = directory_path / "dual_abi_probe"
            source.write_text(self._abi_probe_source(), encoding="utf-8")
            result = subprocess.run(
                [
                    compiler,
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    *extra_flags,
                    "-Iinclude",
                    "-Isrc",
                    str(source),
                    "-o",
                    str(binary),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            executed = subprocess.run(
                [str(binary)], cwd=ROOT, capture_output=True, text=True, check=False
            )
            self.assertEqual(executed.returncode, 0, executed.stderr or executed.stdout)
            return executed.stdout

    def test_native_abi_probe_freezes_carriers(self) -> None:
        output = self._compile_and_run_abi_probe()
        match = re.search(
            r"pointer=(\d+) uint64=(\d+) address=(\d+) effect=(\d+) "
            r"op_effect=(\d+) dual=(\d+) root=(\d+)",
            output,
        )
        self.assertIsNotNone(match, output)
        pointer, uint64, address, effect, op_effect, dual, root = (
            int(value) for value in match.groups()
        )
        self.assertEqual(pointer, ctypes.sizeof(ctypes.c_void_p), output)
        self.assertEqual(uint64, ctypes.sizeof(ctypes.c_uint64), output)
        self.assertEqual(
            (address, effect, op_effect, dual, root), (24, 48, 72, 80, 576), output
        )

    def test_supported_m32_abi_probe_freezes_carriers(self) -> None:
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("native C compiler unavailable")
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "m32_probe.c"
            binary = Path(directory) / "m32_probe"
            source.write_text(
                "#include <stdint.h>\nint main(void){return sizeof(uint64_t)!=8;}\n",
                encoding="utf-8",
            )
            supported = subprocess.run(
                [compiler, "-m32", "-std=c11", str(source), "-o", str(binary)],
                cwd=ROOT,
                capture_output=True,
                text=True,
                check=False,
            )
        if supported.returncode != 0:
            self.skipTest("compiler does not support a complete -m32 probe")
        output = self._compile_and_run_abi_probe(("-m32",))
        match = re.search(
            r"pointer=(\d+) uint64=(\d+) address=(\d+) effect=(\d+) "
            r"op_effect=(\d+) dual=(\d+) root=(\d+)",
            output,
        )
        self.assertIsNotNone(match, output)
        pointer, uint64, address, effect, op_effect, dual, root = (
            int(value) for value in match.groups()
        )
        self.assertEqual((pointer, uint64), (4, 8), output)
        self.assertEqual(
            (address, effect, op_effect, dual, root), (24, 40, 52, 72, 516), output
        )


if __name__ == "__main__":
    unittest.main()
