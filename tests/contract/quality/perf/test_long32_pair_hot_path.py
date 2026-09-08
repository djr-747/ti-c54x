"""Structural and executable quality checks for the private W47A seam.

Module contract: owns deterministic proofs that the private LONG32 pair
carrier/reducer remains fixed-size, allocation-free, bounded to two WORD16
requests, and isolated from public/opcode/CPU/effect-carrier surfaces.  It
does not claim bus atomicity, TI timing, or opcode coverage.
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
PAIR_DIR = ROOT / "src" / "sequencer" / "instruction"
PAIR_HEADER = PAIR_DIR / "long32_pair_transaction.h"
PAIR_SOURCE = PAIR_DIR / "long32_pair_transaction.c"
PAIR_TOKEN = re.compile(r"(?i)long32[ _-]*pair")


def _code_without_comments_or_literals(text: str) -> str:
    """Return enough C-like text for conservative structural checks."""

    without_comments = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', "", without_comments)


class Long32PairHotPathTests(unittest.TestCase):
    """Exercise source shape and a native/ILP32 ABI probe."""

    @staticmethod
    def _read(path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def _pair_text(self) -> tuple[str, str]:
        self.assertTrue(PAIR_HEADER.is_file(), PAIR_HEADER)
        self.assertTrue(PAIR_SOURCE.is_file(), PAIR_SOURCE)
        return self._read(PAIR_HEADER), self._read(PAIR_SOURCE)

    def _pair_files(self) -> tuple[Path, ...]:
        files = tuple(sorted(PAIR_DIR.glob("long32_pair_transaction.*")))
        self.assertTrue(files, "no W47A pair modules found")
        return (*files, Path(__file__))

    @staticmethod
    def _function_body(code: str, name: str) -> str:
        """Extract one brace-balanced C function body from comment-free code."""

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

    def test_private_modules_have_contracts_and_stay_small(self) -> None:
        header, _source = self._pair_text()
        for path in self._pair_files():
            text = self._read(path)
            self.assertIn("Module contract", text, path)
            self.assertIn("Owns:", text, path)
            self.assertIn("Does not own:", text, path)
            self.assertLess(len(text.splitlines()), 500, path)

    def test_carrier_is_one_embedded_word16_reducer_without_dynamic_state(self) -> None:
        _header, _source = self._pair_text()
        code = _code_without_comments_or_literals(
            "\n".join(self._read(path) for path in self._pair_files()[:-1])
        )
        embedded = re.findall(
            r"\bti_c54x_bus_word16_transaction\s+[A-Za-z_]\w*\s*;", code
        )
        self.assertEqual(
            embedded, ["ti_c54x_bus_word16_transaction word16_transaction;"]
        )
        self.assertNotRegex(code, r"\[[^\]\n]*\]")
        self.assertNotRegex(
            code,
            r"\b(?:malloc|calloc|realloc|free|aligned_alloc|getenv)\s*\(",
        )
        self.assertNotRegex(
            code,
            r"\b(?:fopen|fclose|fread|fwrite|fprintf|printf|puts|putchar|perror|syslog)\s*\(",
        )
        self.assertNotRegex(code, r"\(\s*\*\s*[A-Za-z_]\w*")
        self.assertNotRegex(code, r"\b(?:callback|logging|diagnostic)\b")

    def test_pair_reuses_child_reducer_and_never_replans(self) -> None:
        header, source = self._pair_text()
        combined = header + "\n" + source
        for field in (
            "operand",
            "access",
            "first_value_msw",
            "second_value_lsw",
            "first_transaction_id",
            "second_transaction_id",
        ):
            self.assertRegex(combined, rf"\b{field}\b")
        for function in (
            "prepare",
            "begin",
            "poll",
            "cancel",
            "complete",
            "reset",
        ):
            self.assertIn(f"ti_c54x_bus_word16_transaction_{function}", source)
        self.assertNotRegex(
            source,
            r"(?i)(?:resolve_memory_address|replan|address[_ ]*planner)",
        )

        source_code = _code_without_comments_or_literals(source)
        builder = "ti_c54x_long32_pair_build_word16_request"
        prepare_body = self._function_body(
            source_code, "ti_c54x_long32_pair_transaction_prepare"
        )
        poll_body = self._function_body(
            source_code, "ti_c54x_long32_pair_transaction_poll"
        )
        self.assertEqual(prepare_body.count(builder), 1)
        self.assertEqual(poll_body.count(builder), 1)
        self.assertEqual(source_code.count(builder), 2)

        validate_source = _code_without_comments_or_literals(
            self._read(PAIR_DIR / "long32_pair_transaction_validate.c")
        )
        child_match_body = self._function_body(
            validate_source, "ti_c54x_long32_pair_child_matches_slot"
        )
        self.assertNotIn(builder, child_match_body)
        self.assertIn("first_word", child_match_body)
        self.assertIn("second_word", child_match_body)

    def test_w47a_does_not_cross_public_opcode_cpu_or_effect_boundaries(self) -> None:
        paths = list((ROOT / "include" / "ti_c54x").rglob("*.h"))
        paths.extend(
            [
                ROOT / "src" / "isa" / "catalog" / "opcode_catalog.def",
                ROOT / "src" / "isa" / "ops" / "opcode_dispatch.c",
                ROOT / "src" / "isa" / "ops" / "memory" / "memory_effect.h",
                ROOT / "src" / "isa" / "ops" / "memory" / "memory_effect.c",
            ]
        )
        paths.extend((ROOT / "src" / "cpu").rglob("*.c"))
        paths.extend((ROOT / "src" / "cpu").rglob("*.h"))
        for path in paths:
            self.assertIsNone(PAIR_TOKEN.search(self._read(path)), path)

    @staticmethod
    def _probe_source() -> str:
        return r'''
#include <stdint.h>
#include <stdio.h>

#include "sequencer/instruction/long32_pair_transaction.h"

static int check_read_pair(void)
{
    ti_c54x_long32_pair_request request = {0};
    ti_c54x_long32_pair_transaction transaction = {0};
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_completion_result completion;
    ti_c54x_bus_word16_observation observation = {0};

    request.operand = ti_c54x_data_operand_long32(UINT16_C(0x2201));
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    request.first_value_msw = UINT16_C(0);
    request.second_value_lsw = UINT16_C(0);
    request.first_transaction_id = UINT64_C(41);
    request.second_transaction_id = UINT64_C(42);
    if (!ti_c54x_long32_pair_request_is_valid(&request) ||
        ti_c54x_long32_pair_transaction_prepare(&transaction, &request) != 0) {
        return 1;
    }
    request.operand.first_word = UINT16_C(0x0101);
    request.operand.second_word = UINT16_C(0x0100);
    request.first_transaction_id = UINT64_C(99);
    request.second_transaction_id = UINT64_C(100);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    if (issue.status != 0 || issue.transaction_id != UINT64_C(41) ||
        issue.request.access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        issue.request.address_word != UINT16_C(0x2201) ||
        transaction.word16_transaction.transaction_id != UINT64_C(41)) {
        return 2;
    }
    observation.transaction_id = UINT64_C(41);
    observation.kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation.read_value_word = UINT16_C(0xCAFE);
    observation.bus_status = TI_C54X_BUS_OK;
    if (ti_c54x_long32_pair_transaction_poll(&transaction, &observation) != 0) {
        return 3;
    }
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    if (issue.status != 0 || issue.transaction_id != UINT64_C(42) ||
        issue.request.address_word != UINT16_C(0x2200) ||
        transaction.word16_transaction.transaction_id != UINT64_C(42)) {
        return 4;
    }
    observation.transaction_id = UINT64_C(42);
    observation.read_value_word = UINT16_C(0xBEEF);
    if (ti_c54x_long32_pair_transaction_poll(&transaction, &observation) != 0) {
        return 5;
    }
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    if (completion.status != 0 || completion.first_msw != UINT16_C(0xCAFE) ||
        completion.second_lsw != UINT16_C(0xBEEF)) {
        return 6;
    }
    return 0;
}

int main(void)
{
    printf("request=%zu transaction=%zu pointer=%zu uint64=%zu\n",
           sizeof(ti_c54x_long32_pair_request),
           sizeof(ti_c54x_long32_pair_transaction), sizeof(void *),
           sizeof(uint64_t));
    return check_read_pair();
}
'''

    def _compile_and_run_probe(self, extra_flags: tuple[str, ...] = ()) -> str:
        sources = [
            PAIR_SOURCE,
            PAIR_DIR / "long32_pair_transaction_validate.c",
            ROOT / "src" / "bus" / "transaction" / "word16_transaction.c",
            ROOT / "src" / "bus" / "transaction" / "word16_transaction_validate.c",
            ROOT / "src" / "bus" / "request" / "word16_request.c",
            ROOT / "src" / "address" / "data_operand.c",
        ]
        with tempfile.TemporaryDirectory() as directory:
            directory_path = Path(directory)
            probe = directory_path / "long32_pair_probe.c"
            binary = directory_path / "long32_pair_probe"
            probe.write_text(self._probe_source(), encoding="utf-8")
            result = subprocess.run(
                [
                    "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    *extra_flags, "-Iinclude", "-Isrc", str(probe),
                    *(str(path) for path in sources), "-o", str(binary),
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

    def test_native_probe_freezes_request_and_transaction_sizes(self) -> None:
        if shutil.which("cc") is None:
            self.skipTest("native C compiler unavailable")
        output = self._compile_and_run_probe()
        match = re.search(
            r"request=(\d+) transaction=(\d+) pointer=(\d+) uint64=(\d+)", output
        )
        self.assertIsNotNone(match, output)
        request_size, transaction_size, pointer_size, uint64_size = (
            int(value) for value in match.groups()
        )
        self.assertEqual(pointer_size, ctypes.sizeof(ctypes.c_void_p), output)
        self.assertEqual(uint64_size, ctypes.sizeof(ctypes.c_uint64), output)
        if pointer_size == 8 and uint64_size == 8:
            expected = (32, 88)
        elif pointer_size == 4 and uint64_size == 8:
            expected = (32, 80)
        else:
            self.skipTest(f"unsupported native ABI: pointer={pointer_size}, uint64={uint64_size}")
        self.assertEqual((request_size, transaction_size), expected, output)

    def test_supported_m32_probe_freezes_ilp32_sizes(self) -> None:
        if shutil.which("cc") is None:
            self.skipTest("native C compiler unavailable")
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "m32_probe.c"
            binary = Path(directory) / "m32_probe"
            source.write_text("#include <stdint.h>\nint main(void) { return sizeof(uint64_t) != 8; }\n", encoding="utf-8")
            supported = subprocess.run(
                ["cc", "-m32", "-std=c11", str(source), "-o", str(binary)],
                cwd=ROOT, capture_output=True, text=True, check=False,
            )
        if supported.returncode != 0:
            self.skipTest("compiler does not support a complete -m32 probe")
        output = self._compile_and_run_probe(("-m32",))
        match = re.search(r"request=(\d+) transaction=(\d+) pointer=(\d+) uint64=(\d+)", output)
        self.assertIsNotNone(match, output)
        request_size, transaction_size, pointer_size, uint64_size = (
            int(value) for value in match.groups()
        )
        self.assertEqual((pointer_size, uint64_size), (4, 8), output)
        self.assertEqual((request_size, transaction_size), (32, 80), output)


if __name__ == "__main__":
    unittest.main()
